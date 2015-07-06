#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/hardirq.h>
#include <asm/irq_regs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/tick.h>

#include <asm/callbacks.h>

unsigned long irqs_enabled=0;

unsigned long __local_save_flags(void)
{
        return irqs_enabled;
}

void __local_irq_restore(unsigned long flags)
{
        irqs_enabled=flags;
}

void local_irq_enable(void)
{
        irqs_enabled=1;
}

void local_irq_disable(void)
{
        irqs_enabled=0;
}


unsigned int do_IRQ(int irq, struct pt_regs *regs)
{
	struct pt_regs *old_regs = set_irq_regs(regs);
	irq_enter();
	__do_IRQ(irq);
	irq_exit();
	set_irq_regs(old_regs);
	return 1;
}

struct irq_data {
	struct list_head list;
	struct pt_regs regs;
};

static struct irq_info {
        struct list_head data_list;
        int no_data_count;
	void *lock;
} irqs [NR_IRQS];

static void *sem;

void lkl_trigger_irq(int irq)
{
	BUG_ON(irq >= NR_IRQS);

	lkl_nops->sem_down(irqs[irq].lock);
        irqs[irq].no_data_count++;
	lkl_nops->sem_up(irqs[irq].lock);

	lkl_nops->sem_up(sem);
}

int lkl_trigger_irq_with_data(int irq, void *data)
{
	struct irq_data *id;

	BUG_ON(irq >= NR_IRQS);

	if (!(id=lkl_nops->mem_alloc(sizeof(*id))))
		return -ENOMEM;

	lkl_nops->sem_down(irqs[irq].lock);
	id->regs.irq_data=data;
	list_add_tail(&id->list, &irqs[irq].data_list);
	lkl_nops->sem_up(irqs[irq].lock);

	lkl_nops->sem_up(sem);

	return 0;
}


void lkl_purge_irq_queue(int irq)
{
	struct list_head *i, *aux;

	BUG_ON(irq >= NR_IRQS);

	lkl_nops->sem_down(irqs[irq].lock);
        irqs[irq].no_data_count=0;
	list_for_each_safe(i, aux, &irqs[irq].data_list) {
		struct irq_data *id=list_entry(i, struct irq_data, list);
		list_del(&id->list);
		lkl_nops->mem_free(id);
	}
	lkl_nops->sem_up(irqs[irq].lock);
}


static int dequeue_data(int irq, struct pt_regs *regs)
{
	struct list_head *i;
	struct irq_data *id=NULL;

	lkl_nops->sem_down(irqs[irq].lock);
	list_for_each(i, &irqs[irq].data_list) {
		id=list_entry(i, struct irq_data, list);
		list_del(&id->list);
		break;
	}
	lkl_nops->sem_up(irqs[irq].lock);

	if (!id)
		return -ENOENT;

	*regs=id->regs;
	lkl_nops->mem_free(id);

	return 0;
}

static int dequeue_nodata(int irq, struct pt_regs *regs)
{
	int count;


	lkl_nops->sem_down(irqs[irq].lock);
	count=irqs[irq].no_data_count;
	if (count > 0)
		irqs[irq].no_data_count--;
	lkl_nops->sem_up(irqs[irq].lock);

        if (count <= 0)
                return -ENOENT;

        return 0;
}

extern int linux_halted;

void run_irqs(void)
{
        local_irq_disable();

        do {
		struct pt_regs regs;
		int i;

		if (!linux_halted)
			lkl_nops->sem_down(sem);

                for(i=0; i<NR_IRQS; i++) {
                        while (dequeue_nodata(i, &regs) == 0)
                                do_IRQ(i, &regs);
                        while (dequeue_data(i, &regs) == 0) 
                                do_IRQ(i, &regs);
                }
        } while (!need_resched() && !linux_halted);

        local_irq_enable();
}


static struct rcu_head rcu_head;
static int rcu_done, rcu_start;
static void rcu_func(struct rcu_head *h)
{
       rcu_done=1;
}


/*
 * We run the IRQ handlers from here so that we don't have to
 * interrupt the current thread since the application might not be
 * able to do that.
 */
void cpu_idle(void)
{
	int loop=1;

	while (loop) {
		/*
		 * We need to exit, but before we can do so we need to run any
		 * pending jobs, like RCU calls, IRQs, or runable processes.
		 */
		if (linux_halted) {
			if (!rcu_start) {
				call_rcu(&rcu_head, rcu_func);
				rcu_start=1;
			}
			loop--;
		}
		tick_nohz_stop_sched_tick(1);
		run_irqs();
		if (need_resched() || !rcu_done) {
			/* 
			 * We have either runable processes or RCU calls to 
			 * process. Don't exit this turn. Maybe next time.
			 */ 
			if (linux_halted)
				loop++;
			tick_nohz_restart_sched_tick();
			preempt_enable_no_resched();
			schedule();
			preempt_disable();
		}

		
	}

}

void init_IRQ(void)
{
	int i;

	BUG_ON((sem=lkl_nops->sem_alloc(0)) == NULL);

	
	for(i=0; i<NR_IRQS; i++) {
		BUG_ON((irqs[i].lock=lkl_nops->sem_alloc(1)) == NULL);
		INIT_LIST_HEAD(&irqs[i].data_list);
		irqs[i].no_data_count=0;
		set_irq_chip_and_handler(i, &dummy_irq_chip, handle_simple_irq);
	}

	printk(KERN_INFO "lkl: IRQs initialized\n");
}

void free_IRQ(void)
{
	int i;

	for(i=0; i<NR_IRQS; i++)
		lkl_nops->sem_free(irqs[i].lock);
	lkl_nops->sem_free(sem);

	printk(KERN_INFO "lkl: IRQs freed\n");
}

int show_interrupts(struct seq_file *p, void *v)
{
        return 0;
}


void __init trap_init(void)
{
}
