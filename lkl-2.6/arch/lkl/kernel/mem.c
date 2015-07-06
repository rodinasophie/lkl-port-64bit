#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/pgtable.h>
#include <asm/callbacks.h>
#include <linux/bootmem.h>
#include <linux/mm.h>
#include <linux/swap.h>

unsigned long phys_mem, phys_mem_size, memory_end;
unsigned long empty_zero_page;

static unsigned long _phys_mem;

#ifdef CONFIG_LKL_SLAB

#include <linux/rbtree.h>
#include <linux/module.h>

/*
 * We want user blocks clear of our metadata, so that we can easily catch
 * under/over flows.
 */
struct mem_block_meta {
	void *block;
	size_t size;
	struct rb_node rb_node;
};

static struct rb_root meta_tree = RB_ROOT;

static inline struct mem_block_meta* mem_block_find_meta(const void *b)
{
        struct rb_node *i = meta_tree.rb_node;
        struct mem_block_meta *mbm;

        while (i)
        {
                mbm = rb_entry(i, struct mem_block_meta, rb_node);
                if (b < mbm->block)
                        i = i->rb_left;
                else if (b > mbm->block)
                        i = i->rb_right;
                else if (b != mbm->block) 
                        return NULL;
                else
                        return mbm;
        }
        return NULL;
	
}

static inline void mem_block_insert_meta(struct mem_block_meta *mbm)
{
        struct rb_node **p=&meta_tree.rb_node, *parent=NULL;
        struct mem_block_meta *i;

        while (*p)
        {
                parent = *p;
                i = rb_entry(parent, struct mem_block_meta, rb_node);

                if (mbm->block < i->block)
                        p = &(*p)->rb_left;
                else if (mbm->block > i->block)
                        p = &(*p)->rb_right;
                else
                        BUG();
        }
        
        rb_link_node(&mbm->rb_node, parent, p);
        rb_insert_color(&mbm->rb_node, &meta_tree);
}


static inline struct mem_block_meta* __alloc_mem_block(size_t size)
{
	struct mem_block_meta *mbm=lkl_nops->mem_alloc(sizeof(*mbm));

	if (!mbm)
		return NULL;

	mbm->size=size;
	if (!(mbm->block=lkl_nops->mem_alloc(size))) {
		lkl_nops->mem_free(mbm);
		return NULL;
	}

	mem_block_insert_meta(mbm);

	return mbm;
}

static inline void* alloc_mem_block(size_t size)
{
	struct mem_block_meta *mbm=__alloc_mem_block(size);
	if (mbm)
		return mbm->block;
	return NULL;
}

static inline  void free_mem_block(const void *b)
{
	struct mem_block_meta *mbm=mem_block_find_meta(b);
	BUG_ON(mbm == NULL);
	rb_erase(&mbm->rb_node, &meta_tree);
	lkl_nops->mem_free(mbm);
	lkl_nops->mem_free((void*)b);
}

void *__kmalloc(size_t size, gfp_t gfp)
{
	return alloc_mem_block(size);
}
EXPORT_SYMBOL(__kmalloc);

void kfree(const void *block)
{
	if (!block)
		return;
	free_mem_block(block);
}

EXPORT_SYMBOL(kfree);

size_t ksize(const void *b)
{
	struct mem_block_meta *mbm=mem_block_find_meta(b);
	BUG_ON(mbm == NULL);
	return mbm->size;
}

struct kmem_cache {
	unsigned int size; /*, align; - no align */
	unsigned long flags;
	const char *name;
	void (*ctor) (void *);
};

struct kmem_cache *kmem_cache_create(const char *name, size_t size,
	size_t align, unsigned long flags, void (*ctor) (void*))
{
	struct kmem_cache *c;

	c = alloc_mem_block(sizeof(struct kmem_cache));

	if (c) {
		c->name = name;
		c->size = size;
		c->flags = flags;
		c->ctor = ctor;
	} else if (flags & SLAB_PANIC)
		panic("Cannot create slab cache %s\n", name);

	return c;
}
EXPORT_SYMBOL(kmem_cache_create);

void kmem_cache_destroy(struct kmem_cache *c)
{
	free_mem_block(c);
}
EXPORT_SYMBOL(kmem_cache_destroy);

void *kmem_cache_alloc(struct kmem_cache *c, gfp_t flags)
{
	void *b=alloc_mem_block(c->size);

	if (c->ctor)
		c->ctor(b);

	return b;
}
EXPORT_SYMBOL(kmem_cache_alloc);

static void __kmem_cache_free(void *b, int size)
{
	free_mem_block(b);
}

struct mem_block_rcu {
	struct rcu_head rh;
	void *block;
};

static void kmem_rcu_free(struct rcu_head *rc)
{
	struct mem_block_rcu *mbr = container_of(rc, struct mem_block_rcu, rh);

	free_mem_block(mbr->block);
	lkl_nops->mem_free(mbr);
}

void kmem_cache_free(struct kmem_cache *c, void *b)
{
	if (unlikely(c->flags & SLAB_DESTROY_BY_RCU)) {
		struct mem_block_rcu *mbr=lkl_nops->mem_alloc(sizeof(*mbr));
		if (!mbr)
			printk(KERN_ERR "%s: mem_alloc failed!\n", __FUNCTION__);
		INIT_RCU_HEAD(&mbr->rh);
		mbr->block=b;
		call_rcu(&mbr->rh, kmem_rcu_free);
	} else {
		__kmem_cache_free(b, c->size);
	}
}
EXPORT_SYMBOL(kmem_cache_free);

unsigned int kmem_cache_size(struct kmem_cache *c)
{
	return c->size;
}
EXPORT_SYMBOL(kmem_cache_size);

const char *kmem_cache_name(struct kmem_cache *c)
{
	return c->name;
}
EXPORT_SYMBOL(kmem_cache_name);

int kmem_cache_shrink(struct kmem_cache *d)
{
	return 0;
}
EXPORT_SYMBOL(kmem_cache_shrink);

int kmem_ptr_validate(struct kmem_cache *a, const void *b)
{
	struct mem_block_meta *mbm=mem_block_find_meta(b);
	if (!mbm)
		return -EINVAL;
	return 0;
}

static unsigned int lkl_slab_ready __read_mostly;

int slab_is_available(void)
{
	return lkl_slab_ready;
}

void __init kmem_cache_init(void)
{
	lkl_slab_ready = 1;
}

#endif/* CONFIG_LKL_SLAB */

void show_mem(void)
{
}


void free_initmem(void)
{
}

void __init mem_init_0(void)
{
	int bootmap_size;

#if 0
	memory_end = _ramend; /* by now the stack is part of the init task */

	init_mm.start_code = (unsigned long) &_stext;
	init_mm.end_code = (unsigned long) &_etext;
	init_mm.end_data = (unsigned long) &_edata;
	init_mm.brk = (unsigned long) 0;
#endif

	phys_mem_size=lkl_nops->phys_mem_size;
        _phys_mem=phys_mem=(unsigned long)lkl_nops->mem_alloc(phys_mem_size);
        BUG_ON(!phys_mem);
	memory_end=phys_mem+phys_mem_size;

        if (PAGE_ALIGN(phys_mem) != phys_mem) {
		phys_mem_size-=PAGE_ALIGN(phys_mem)-phys_mem;
		phys_mem=PAGE_ALIGN(phys_mem);
		phys_mem_size=(phys_mem_size/PAGE_SIZE)*PAGE_SIZE;
	}


	/*
	 * Give all the memory to the bootmap allocator, tell it to put the
	 * boot mem_map at the start of memory.
	 */
	bootmap_size = init_bootmem(virt_to_pfn(PAGE_OFFSET), virt_to_pfn(PAGE_OFFSET+phys_mem_size));

	/*
	 * Free the usable memory, we have to make sure we do not free
	 * the bootmem bitmap so we then reserve it after freeing it :-)
	 */
	free_bootmem(phys_mem, phys_mem_size);
	reserve_bootmem(phys_mem, bootmap_size, BOOTMEM_DEFAULT);

	/*
	 * Get kmalloc into gear.
	 */
#if 0
	empty_bad_page_table = (unsigned long)alloc_bootmem_pages(PAGE_SIZE);
	empty_bad_page = (unsigned long)alloc_bootmem_pages(PAGE_SIZE);
#endif
	empty_zero_page = (unsigned long)alloc_bootmem_pages(PAGE_SIZE);
	memset((void *)empty_zero_page, 0, PAGE_SIZE);

	{
		unsigned long zones_size[MAX_NR_ZONES] = {0, };

		zones_size[ZONE_NORMAL] = (phys_mem_size) >> PAGE_SHIFT;
		free_area_init(zones_size);
	}

}

void __init mem_init(void)
{
	unsigned long tmp;
	int codek = 0, datak = 0, initk = 0;
	extern char _etext, _stext, __start_rodata, __stop_bss, __init_begin, __init_end;

	max_mapnr = num_physpages = (((unsigned long) high_memory) - PAGE_OFFSET) >> PAGE_SHIFT;

	/* this will put all memory onto the freelists */
	totalram_pages = free_all_bootmem();

	codek = (&_etext - &_stext) >> 10;
	datak = (&__stop_bss - &__start_rodata) >> 10;
	initk = (&__init_begin - &__init_end) >> 10;
	tmp = nr_free_pages() << PAGE_SHIFT;
	printk(KERN_INFO "Memory available: %luk/%luk RAM, (%dk kernel code, %dk data)\n",
	       tmp >> 10,
	       phys_mem_size >> 10,
	       codek,
	       datak
	       );

}

void mem_cleanup(void)
{
	lkl_nops->mem_free((void*)_phys_mem);
}
