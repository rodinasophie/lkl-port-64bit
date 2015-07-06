#include <linux/sched.h>
#include <linux/kallsyms.h>
#include <linux/module.h>
#include <asm/ptrace.h>
#include <asm/callbacks.h>


/* these are stolen from arch/um/sysrq.c */
void show_trace(struct task_struct *task, unsigned long * stack)
{
	unsigned long addr;

	if (!stack) {
		stack = (unsigned long*) &stack;
		WARN_ON(1);
	}

	printk(KERN_INFO "Call Trace: \n");
	while (((long) stack & (THREAD_SIZE-1)) != 0) {
		addr = *stack;
		if (__kernel_text_address(addr)) {
			printk(KERN_INFO "%08lx:  [<%08lx>]",
			       (unsigned long) stack, addr);
			print_symbol(KERN_CONT " %s", addr);
			printk(KERN_CONT "\n");
		}
		stack++;
	}
	printk(KERN_INFO "\n");
}

void dump_stack(void)
{
	unsigned long stack;

	show_trace(current, &stack);
	if (lkl_nops->dump_stack)
		lkl_nops->dump_stack();
}
EXPORT_SYMBOL(dump_stack);

void show_regs(struct pt_regs *regs)
{
  printk(KERN_ALERT "dummy lkl show_regs() impl\n");
}

