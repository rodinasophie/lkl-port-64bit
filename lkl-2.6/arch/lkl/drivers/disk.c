#include <linux/kernel.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>	/* invalidate_bdev */
#include <linux/bio.h>
#include <linux/interrupt.h>
#include <asm/irq_regs.h>

#include <asm/disk.h>

/*
 * The internal representation of our device.
 */
struct lkl_disk_dev {
	void *data;
        spinlock_t lock;                /* For mutual exclusion */
        struct request_queue *queue;    /* The device request queue */
        struct gendisk *gd;             /* The gendisk structure */
};

static void complete_request(struct lkl_disk_cs *cs)
{
	struct request *req=(struct request*)cs->linux_cookie;

	blk_end_request(req, cs->error, blk_rq_cur_bytes(req));

	kfree(cs);
}

static irqreturn_t lkl_disk_irq(int irq, void *dev_id)
{
	struct pt_regs *regs=get_irq_regs();

	complete_request(regs->irq_data);

	return IRQ_HANDLED;
}

static void lkl_disk_request(struct request_queue *q)
{
	struct request *rq;

	while (1) {
		struct lkl_disk_dev *dev;
		struct lkl_disk_cs cs;

		rq = blk_fetch_request(q);
		if (rq == NULL)
			break;

		if (!blk_fs_request(rq)) {
			printk (KERN_NOTICE "lkl_disk_request: Skip non-fs request\n");
			__blk_end_request_cur(rq, -EIO);
			continue;
		}

		dev = rq->rq_disk->private_data;

		do {
			cs.linux_cookie = rq;
			lkl_disk_do_rw(dev->data, blk_rq_pos(rq), blk_rq_cur_sectors(rq),
				       rq->buffer, rq_data_dir(rq), &cs);
			BUG_ON (cs.sync == 0); /* Async is broken.*/
		} while (__blk_end_request_cur(rq, cs.error ? -EIO : 0));
	}
}

static int lkl_disk_open(struct block_device *bdev, fmode_t mode)
{
	return 0;
}

/*
 * The device operations structure.
 */
static struct block_device_operations lkl_disk_ops = {
	.owner           = THIS_MODULE,
	.open 	         = lkl_disk_open,
};


static int major, failed_init, disks;

int _lkl_disk_del_disk(__kernel_dev_t devt)
{
	struct block_device *bdev;
	struct gendisk *gd;
	int ret = 0, partno;

	bdev = bdget(devt);
	if (!bdev)
		return -EINVAL;

	gd = get_gendisk(new_decode_dev(devt), &partno);
	if (!gd || gd->major != major) {
		ret = -EINVAL;
		goto out;
	}

	if (gd->queue)
		blk_cleanup_queue(gd->queue);

	del_gendisk(gd);

out:
	bdput(bdev);
	return ret;
}

__kernel_dev_t _lkl_disk_add_disk(void *data, int sectors)
{
	struct lkl_disk_dev *dev;

	if (failed_init)
		return 0;

        if (!(dev=kzalloc(sizeof(*dev), GFP_KERNEL)))
		return 0;

        dev->data=data;

	spin_lock_init(&dev->lock);

        if (!(dev->queue=blk_init_queue(lkl_disk_request, &dev->lock))) {
		kfree(dev);
		return 0;
	}

	blk_queue_logical_block_size(dev->queue, 512);

	dev->queue->queuedata = dev;

	if (!(dev->gd=alloc_disk(1))) {
		kfree(dev);
		/* FIXME: del queue? */
		return 0;
	}

	dev->gd->major = major;
	dev->gd->first_minor = disks;
	dev->gd->fops = &lkl_disk_ops;
	dev->gd->queue = dev->queue;
	dev->gd->private_data = dev;
	snprintf (dev->gd->disk_name, 32, "lkldisk%d", disks);
	set_capacity(dev->gd, sectors);

	add_disk(dev->gd);

	disks++;

	printk("lkldisk: attached %s @ dev=%d:%d\n",
	       dev->gd->disk_name,
	       dev->gd->major,
	       dev->gd->first_minor);

	return new_encode_dev(MKDEV(dev->gd->major, dev->gd->first_minor));
}

static int __init disk_init(void)
{
	int err;

	if ((err=request_irq(DISK_IRQ, lkl_disk_irq, 0, "lkldisk", NULL))) {
		failed_init=1;
		printk(KERN_ERR "lkldisk: unable to register irq %d: %d\n", DISK_IRQ, err);
		return err;
	}

	major = register_blkdev(0, "fd");
	if (major < 0) {
		failed_init=1;
		printk(KERN_ERR "fd: unable to register_blkdev: %d\n", major);
		free_irq(DISK_IRQ, NULL);
		return -EBUSY;
	}

	return 0;
}

late_initcall(disk_init);
