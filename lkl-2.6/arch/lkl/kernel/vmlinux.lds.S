#include <asm/vmlinux.lds.h>
#include <asm/target.h>
#include <asm/thread_info.h>

/*OUTPUT_FORMAT(TARGET_FORMAT)
OUTPUT_ARCH(TARGET_ARCH)*/
ENTRY(start_kernel)

#if (BITS_PER_LONG == 32) && defined(BIG_ENDIAN)
#define JIFFIES_OFFSET 4
#else
#define JIFFIES_OFFSET 0
#endif


SECTIONS
{
#ifdef __ELF__	
        /* Read-only sections, merged into text segment: */
        . = + SIZEOF_HEADERS;
        .interp : { *(.interp) }
        .hash          : { *(.hash)		}
        .gnu.hash      : { *(.gnu.hash)	}
        .dynsym        : { *(.dynsym)		}
        .dynstr        : { *(.dynstr)		}
        .rel.text      : { *(.rel.text)		}
        .rela.text     : { *(.rela.text) 	}
        .rel.data      : { *(.rel.data)		}
        .rela.data     : { *(.rela.data) 	}
        .rel.rodata    : { *(.rel.rodata) 	}
        .rela.rodata   : { *(.rela.rodata) 	}
        .rel.got       : { *(.rel.got)		}
        .rela.got      : { *(.rela.got)		}
        .rel.ctors     : { *(.rel.ctors)	}
        .rela.ctors    : { *(.rela.ctors)	}
        .rel.dtors     : { *(.rel.dtors)	}
        .rela.dtors    : { *(.rela.dtors)	}
        .rel.bss       : { *(.rel.bss)		}
        .rela.bss      : { *(.rela.bss)		}
        .rel.plt       : { *(.rel.plt)		}
        .rela.plt      : { *(.rela.plt)		}
        .plt : { *(.plt) }
#endif
	
        .text : {
		_text = .; 
                VMLINUX_SYMBOL(_stext) = .;
		TEXT_TEXT
                SCHED_TEXT
		LOCK_TEXT
		KPROBES_TEXT
                *(.fixup)
                VMLINUX_SYMBOL(_etext) = .;
                VMLINUX_SYMBOL(_sinittext) = .;
                *(.init.text)
                VMLINUX_SYMBOL(_einittext) = .;
	        *(.exit.text) 
		VMLINUX_SYMBOL(__start___bug_table) = .;	
		*(__bug_table)
		VMLINUX_SYMBOL(__stop___bug_table) = .;
	  	VMLINUX_SYMBOL(__start___ex_table) = . ; 
		 *(__ex_table)
	  	VMLINUX_SYMBOL(__stop___ex_table) = .;
	}
	
	. = ALIGN(4096);
	__init_begin = .;

	RODATA_SECTION : {
		VMLINUX_SYMBOL(__start_rodata) = .;

		*(RODATA_SECTION)
	
		*(__vermagic)
	
		/* Built-in firmware blobs */
		VMLINUX_SYMBOL(__start_builtin_fw) = .;
		*(.builtin_fw)
		VMLINUX_SYMBOL(__end_builtin_fw) = .;

		/* Kernel symbol table: Normal symbols */
		VMLINUX_SYMBOL(__start___ksymtab) = .;
		*(__ksymtab)
		VMLINUX_SYMBOL(__stop___ksymtab) = .;
	
		/* Kernel symbol table: GPL-only symbols */
		VMLINUX_SYMBOL(__start___ksymtab_gpl) = .;
		*(__ksymtab_gpl)
		VMLINUX_SYMBOL(__stop___ksymtab_gpl) = .;
	
		/* Kernel symbol table: Normal unused symbols */
		VMLINUX_SYMBOL(__start___ksymtab_unused) = .;
		*(__ksymtab_unused)
		VMLINUX_SYMBOL(__stop___ksymtab_unused) = .;
	
		/* Kernel symbol table: GPL-only unused symbols */
		VMLINUX_SYMBOL(__start___ksymtab_unused_gpl) = .;
		*(__ksymtab_unused_gpl)
		VMLINUX_SYMBOL(__stop___ksymtab_unused_gpl) = .;
	
		/* Kernel symbol table: GPL-future-only symbols */
		VMLINUX_SYMBOL(__start___ksymtab_gpl_future) = .;
		*(__ksymtab_gpl_future)
		VMLINUX_SYMBOL(__stop___ksymtab_gpl_future) = .;
	
		/* Kernel symbol table: Normal symbols */
		VMLINUX_SYMBOL(__start___kcrctab) = .;
		*(__kcrctab)
		VMLINUX_SYMBOL(__stop___kcrctab) = .;
	
		VMLINUX_SYMBOL(__start___kcrctab_gpl) = .;
		*(__kcrctab_gpl)
		VMLINUX_SYMBOL(__stop___kcrctab_gpl) = .;
	
		/* Kernel symbol table: Normal unused symbols */
		VMLINUX_SYMBOL(__start___kcrctab_unused) = .;
		*(__kcrctab_unused)
		VMLINUX_SYMBOL(__stop___kcrctab_unused) = .;
	
		/* Kernel symbol table: GPL-only unused symbols */
		VMLINUX_SYMBOL(__start___kcrctab_unused_gpl) = .;
		*(__kcrctab_unused_gpl)
		VMLINUX_SYMBOL(__stop___kcrctab_unused_gpl) = .;
	
		/* Kernel symbol table: GPL-future-only symbols */
		VMLINUX_SYMBOL(__start___kcrctab_gpl_future) = .;
		*(__kcrctab_gpl_future)
		VMLINUX_SYMBOL(__stop___kcrctab_gpl_future) = .;
	
		/* Kernel symbol table: strings */
		*(__ksymtab_strings)
	
		/* Built-in module parameters. */
		VMLINUX_SYMBOL(__start___param) = .;
		*(__param)
		VMLINUX_SYMBOL(__stop___param) = .;
        
	        VMLINUX_SYMBOL(__setup_start) = .;
                *(.init.setup) 
                VMLINUX_SYMBOL(__setup_end) = .;
        
	        VMLINUX_SYMBOL(__initcall_start) = .;
                INITCALLS
	        VMLINUX_SYMBOL(__initcall_end) = .;           

	        VMLINUX_SYMBOL(__con_initcall_start) = .;
                *(.con_initcall.init) 
                VMLINUX_SYMBOL(__con_initcall_end) = .;

		VMLINUX_SYMBOL(__security_initcall_start) = .;
		*(.security_initcall.init)
		VMLINUX_SYMBOL(__security_initcall_end) = .;
	
		VMLINUX_SYMBOL(__end_rodata) = .;
	}

        /* Read-write section, merged into data segment: */
        . = ALIGN(4096);
        .data    :
        {
		/* 
		 * This has to be here, otherwise ld puts jiffies in *ABS*.
		 */
		VMLINUX_SYMBOL(jiffies) = VMLINUX_SYMBOL(jiffies_64) + JIFFIES_OFFSET;

                DATA_DATA
                *(.init.data);	
                CONSTRUCTORS
		*(.data.cacheline_aligned) 
		*(.exit.data) 
        }

	VMLINUX_SYMBOL(_sextratext) = .;
        VMLINUX_SYMBOL(_eextratext) = .;	

        . = ALIGN(4096);
	__init_end = .;

        .bss : {
                VMLINUX_SYMBOL(__start_bss) = .;
		*(.sbss) *(.scommon)
                *(.dynbss)
                *(.bss)
                VMLINUX_SYMBOL(__stop_bss) = .;
        }

        VMLINUX_SYMBOL(_end) = . ;

        /* Sections to be discarded. */
        /DISCARD/ : {
                *(.exitcall.exit)
        }
}

