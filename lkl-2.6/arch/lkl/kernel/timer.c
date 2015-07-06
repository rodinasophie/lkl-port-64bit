#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/jiffies.h>
#include <linux/sysdev.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#include <asm/callbacks.h>

void __udelay(unsigned long usecs)
{
}

void __ndelay(unsigned long nsecs)
{
}

void __devinit calibrate_delay(void)
{
}

static cycle_t clock_read(struct clocksource *cs)
{
        return lkl_nops->time();
}

static struct clocksource clocksource = {
	.name	= "lkl",
	.rating = 499,
	.read	= clock_read,
        .mult   = 1, 
	.mask	= CLOCKSOURCE_MASK(64),
	.shift	= 0,
	.flags  = CLOCK_SOURCE_IS_CONTINUOUS
};

static void clockevent_set_mode(enum clock_event_mode mode,
                                struct clock_event_device *evt)
{
	switch(mode) {
	default:
	case CLOCK_EVT_MODE_PERIODIC:
		BUG();
		break;
	case CLOCK_EVT_MODE_SHUTDOWN:
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_ONESHOT:
                break;
	}
}

static irqreturn_t timer_irq(int irq, void *dev_id)
{
        struct clock_event_device *dev=(struct clock_event_device*)dev_id;

        dev->event_handler(dev);

        return IRQ_HANDLED;
}
  
static int clockevent_next_event(unsigned long delta,
                                 struct clock_event_device *evt) 
{
	/* a few ns won't matter */
        if (delta <= LKL_TIMER_LAST_OP)
		delta=LKL_TIMER_LAST_OP+1;
                
	lkl_nops->timer(delta);

        return 0;
}

static struct clock_event_device clockevent = {
	.name		= "lkl",
	.features	= CLOCK_EVT_FEAT_ONESHOT,
	.set_mode	= clockevent_set_mode,
	.set_next_event = clockevent_next_event,
	.shift		= 0,
	.mult           = 1,
	.irq		= 0,
	.max_delta_ns   = 0xffffffff,
	.min_delta_ns   = 0,
	.cpumask = cpu_all_mask,
};


static struct irqaction irq0  = {
	.handler = timer_irq,
	.flags = IRQF_DISABLED | IRQF_NOBALANCING | IRQF_TIMER,
	.dev_id = &clockevent,
	.name = "timer"
};

void __init time_init(void)
{
	BUG_ON(!lkl_nops->timer || !lkl_nops->time);

	clockevent.cpumask = cpumask_of(0);
	setup_irq(TIMER_IRQ, &irq0);

	BUG_ON(clocksource_register(&clocksource) != 0);
        
        clockevents_register_device(&clockevent);

	lkl_nops->timer(LKL_TIMER_INIT);

        printk("lkl: timer initialized\n");
}
