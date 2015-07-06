#include <ddk/ntddk.h>
#include <asm/callbacks.h>
#include <asm/disk_portable.h>

#ifdef ASYNC
NTSTATUS lkl_disk_completion(DEVICE_OBJECT *dev, IRP *irp, void *arg)
{
	struct lkl_disk_cs *cs=(struct lkl_disk_cs*)arg;
	IO_STATUS_BLOCK *isb=irp->UserIosb;
	MDL *mdl;

	DbgPrint("%s:%d: %d\n", __FUNCTION__, __LINE__, isb->Status);

	if (isb->Status == STATUS_SUCCESS)
		cs->error=LKL_DISK_CS_SUCCESS;
	else
		cs->error=LKL_DISK_CS_ERROR;
	lkl_trigger_irq_with_data(LKL_DISK_IRQ, cs);

	ExFreePool(isb);

	while ((mdl = irp->MdlAddress)) {
		DbgPrint("%s:%d\n", __FUNCTION__, __LINE__);
		irp->MdlAddress = mdl->Next;
		MmUnlockPages(mdl);
		IoFreeMdl(mdl);
	}

	if (irp->Flags & IRP_INPUT_OPERATION) {
		DbgPrint("%s:%d\n", __FUNCTION__, __LINE__);
		IO_STACK_LOCATION *isl = IoGetCurrentIrpStackLocation(irp);
		RtlCopyMemory(irp->UserBuffer, irp->AssociatedIrp.SystemBuffer, isl->Parameters.Read.Length);
	}

	if (irp->Flags & IRP_DEALLOCATE_BUFFER) {
		DbgPrint("%s:%d\n", __FUNCTION__, __LINE__);
		ExFreePoolWithTag(irp->AssociatedIrp.SystemBuffer, '  oI');
	}


	DbgPrint("%s:%d\n", __FUNCTION__, __LINE__);
	IoFreeIrp(irp);
	DbgPrint("%s:%d\n", __FUNCTION__, __LINE__);
	return STATUS_MORE_PROCESSING_REQUIRED;
}


void lkl_disk_do_rw(void *_dev, unsigned long sector, unsigned long nsect,
	      char *buffer, int dir, struct lkl_disk_cs *cs)

{
	DEVICE_OBJECT *dev=(DEVICE_OBJECT*)_dev;
	IRP *irp;
	IO_STATUS_BLOCK *isb;
	LARGE_INTEGER offset = {
		.QuadPart = sector*512,
	};

	DbgPrint("%s:%d: dir=%d buffer=%p offset=%u nsect=%u\n", __FUNCTION__, __LINE__, dir, buffer, sector, nsect);

	if (!(isb=ExAllocatePool(NonPagedPool, sizeof(*isb)))) {
		cs->error=LKL_DISK_CS_ERROR;
		lkl_trigger_irq_with_data(LKL_DISK_IRQ, cs);
		return;
	}
		
	irp=IoBuildAsynchronousFsdRequest(dir?IRP_MJ_WRITE:IRP_MJ_READ,
					  dev, buffer, 0 /*nsect*512*/, &offset,
					  isb);
	if (!irp) {
		ExFreePool(isb);
		cs->error=LKL_DISK_CS_ERROR;
		lkl_trigger_irq_with_data(LKL_DISK_IRQ, cs);
		return;
	}

	IoSetCompletionRoutine(irp, lkl_disk_completion, cs, TRUE, TRUE, TRUE);

	IoCallDriver(dev, irp);

}
#else
void lkl_disk_do_rw(void *_dev, unsigned long sector, unsigned long nsect,
		    char *buffer, int dir, struct lkl_disk_cs *cs)
{
	DEVICE_OBJECT *dev=(DEVICE_OBJECT*)_dev;
	IRP *irp;
	IO_STATUS_BLOCK isb;
	KEVENT event;
	NTSTATUS status;
	LARGE_INTEGER offset = {
		.QuadPart = sector*512,
	};

	KeInitializeEvent(&event, NotificationEvent, FALSE);

	irp=IoBuildSynchronousFsdRequest(dir?IRP_MJ_WRITE:IRP_MJ_READ,
					  dev, buffer, nsect*512, &offset,
					  &event, &isb);

	cs->sync=1;

	if (!irp) {
		cs->error=LKL_DISK_CS_ERROR;
		return;
	}

	if (IoCallDriver(dev, irp) != STATUS_PENDING) {
		KeSetEvent(&event, 0, FALSE);
		isb.Status = status;
	}

	KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
	
	if (isb.Status == STATUS_SUCCESS)
		cs->error=LKL_DISK_CS_SUCCESS;
	else 
		cs->error=LKL_DISK_CS_ERROR;
}
#endif


