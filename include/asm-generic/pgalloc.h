/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_GENERIC_PGALLOC_H
#define __ASM_GENERIC_PGALLOC_H

#ifdef CONFIG_MMU

#define GFP_PGTABLE_KERNEL  (GFP_KERNEL | __GFP_ZERO)
#define GFP_PGTABLE_USER    (GFP_PGTABLE_KERNEL | __GFP_ACCOUNT)

#include <linux/mmzone.h>  // for struct zone
#include <linux/gfp.h>


/**
 * __pte_alloc_one_kernel - allocate memory for a PTE-level kernel page table
 * @mm: the mm_struct of the current context
 *
 * This function is intended for architectures that need
 * anything beyond simple page allocation.
 *
 * Return: pointer to the allocated memory or %NULL on error
 */
static inline pte_t *__pte_alloc_one_kernel_noprof(struct mm_struct *mm)
{
    struct ptdesc *ptdesc = pagetable_alloc_noprof(GFP_PGTABLE_KERNEL &
            ~__GFP_HIGHMEM, 0);

    if (!ptdesc)
        return NULL;
    if (!pagetable_pte_ctor(mm, ptdesc)) {
        pagetable_free(ptdesc);
        return NULL;
    }

    return ptdesc_address(ptdesc);
}
#define __pte_alloc_one_kernel(...) alloc_hooks(__pte_alloc_one_kernel_noprof(__VA_ARGS__))

#ifndef __HAVE_ARCH_PTE_ALLOC_ONE_KERNEL
/**
 * pte_alloc_one_kernel - allocate memory for a PTE-level kernel page table
 * @mm: the mm_struct of the current context
 *
 * Return: pointer to the allocated memory or %NULL on error
 */
static inline pte_t *pte_alloc_one_kernel_noprof(struct mm_struct *mm)
{
    return __pte_alloc_one_kernel_noprof(mm);
}
#define pte_alloc_one_kernel(...)   alloc_hooks(pte_alloc_one_kernel_noprof(__VA_ARGS__))
#endif

/**
 * pte_free_kernel - free PTE-level kernel page table memory
 * @mm: the mm_struct of the current context
 * @pte: pointer to the memory containing the page table
 */
static inline void pte_free_kernel(struct mm_struct *mm, pte_t *pte)
{
    pagetable_dtor_free(virt_to_ptdesc(pte));
}

/**
 * __pte_alloc_one - allocate memory for a PTE-level user page table
 * @mm: the mm_struct of the current context
 * @gfp: GFP flags to use for the allocation
 *
 * Allocate memory for a page table and ptdesc and runs pagetable_pte_ctor().
 *
 * This function is intended for architectures that need
 * anything beyond simple page allocation or must have custom GFP flags.
 *
 * Return: `struct page` referencing the ptdesc or %NULL on error
 */
static inline pgtable_t __pte_alloc_one_noprof(struct mm_struct *mm, gfp_t gfp)
{
    struct ptdesc *ptdesc;

    ptdesc = pagetable_alloc_noprof(gfp, 0);
    if (!ptdesc)
        return NULL;
    if (!pagetable_pte_ctor(mm, ptdesc)) {
        pagetable_free(ptdesc);
        return NULL;
    }

    return ptdesc_page(ptdesc);
}
#define __pte_alloc_one(...)    alloc_hooks(__pte_alloc_one_noprof(__VA_ARGS__))

#ifndef __HAVE_ARCH_PTE_ALLOC_ONE
/**
 * pte_alloc_one - allocate a page for PTE-level user page table
 * @mm: the mm_struct of the current context
 *
 * Allocate memory for a page table and ptdesc and runs pagetable_pte_ctor().
 *
 * Return: `struct page` referencing the ptdesc or %NULL on error
 */
static inline pgtable_t pte_alloc_one_noprof(struct mm_struct *mm)
{
    return __pte_alloc_one_noprof(mm, GFP_PGTABLE_USER);
}
#define pte_alloc_one(...)  alloc_hooks(pte_alloc_one_noprof(__VA_ARGS__))
#endif

/*
 * Should really implement gc for free page table pages. This could be
 * done with a reference count in struct page.
 */

/**
 * pte_free - free PTE-level user page table memory
 * @mm: the mm_struct of the current context
 * @pte_page: the `struct page` referencing the ptdesc
 */
static inline void pte_free(struct mm_struct *mm, struct page *pte_page)
{
    struct ptdesc *ptdesc = page_ptdesc(pte_page);

    pagetable_dtor_free(ptdesc);
}

// Encode order into pud_t
static inline pud_t pud_set_pmd_order(pud_t pud, unsigned int order)
{
    pud_val(pud) &= ~PUD_PMD_ORDER_MASK;  // Clear previous
    pud_val(pud) |= ((unsigned long)order << PUD_PMD_ORDER_SHIFT);
    return pud;
}

// Decode order from pud_t
static inline unsigned int pud_get_pmd_order(pud_t pud)
{
    return (pud_val(pud) & PUD_PMD_ORDER_MASK) >> PUD_PMD_ORDER_SHIFT;
}

#if CONFIG_PGTABLE_LEVELS > 2

#ifndef __HAVE_ARCH_PMD_ALLOC_ONE

static inline int get_largest_available_order(struct zonelist *zonelist, gfp_t gfp)
{
    struct zoneref *z;
    struct zone *zone;
    enum zone_type high_zoneidx = gfp_zone(gfp);

    static int MAX_ORDER = 10;

    for_each_zone_zonelist(zone, z, zonelist, high_zoneidx) {
        if (!zone_watermark_ok(zone, 0, 0, 0, 0)) // Optional: skip low zones
            continue;

        for (int order = MAX_ORDER - 1; order >= 0; order--) {
            if (zone->free_area[order].nr_free > 0)
                return order;
        }
    }

    return -1;  // no memory available
}


/**
 * pmd_alloc_one - allocate memory for a PMD-level page table
 * @mm: the mm_struct of the current context
 *
 * Allocate memory for a page table and ptdesc and runs pagetable_pmd_ctor().
 *
 * Allocations use %GFP_PGTABLE_USER in user context and
 * %GFP_PGTABLE_KERNEL in kernel context.
 *
 * Return: pointer to the allocated memory or %NULL on error
 */
static inline pmd_t *pmd_alloc_one_noprof(struct mm_struct *mm, unsigned long addr)
{
    struct ptdesc *ptdesc = NULL;
    gfp_t gfp = GFP_PGTABLE_USER;
    unsigned int order = 0;

    if (mm == &init_mm)
        gfp = GFP_PGTABLE_KERNEL;

#if defined(CONFIG_CONTIGUOUS_PAGETABLE_DEBUG)
    extern atomic_long_t pagetable_alloc_success[10];
    extern atomic_long_t pagetable_alloc_fail[10];
#endif

    extern int pagetable_alloc_mode;       // 0 = fixed, 1 = dynamic
    extern int pagetable_fixed_order;      // used in fixed mode only

    int nid = numa_node_id();  // current NUMA node
    struct zonelist *zonelist = node_zonelist(nid, gfp);
    int largest_order = get_largest_available_order(zonelist, gfp);
    // int largest_order = get_largest_available_order(mm->mmu_notifier_mm->zonelist, gfp);

    if (pagetable_alloc_mode == 0) {
        // Fixed order mode
        if (largest_order < pagetable_fixed_order) {
            // If the fixed order is larger than available, fallback to largest available
            order = largest_order;
        } else {
            order = pagetable_fixed_order;
        }

        ptdesc = pagetable_alloc_noprof(gfp, order);
#if defined(CONFIG_CONTIGUOUS_PAGETABLE_DEBUG)
        if (ptdesc)
            atomic_long_inc(&pagetable_alloc_success[order]);
        else
            atomic_long_inc(&pagetable_alloc_fail[order]);
#endif
	if (!ptdesc){
		ptdesc = pagetable_alloc_noprof(gfp, 0);
	}
    } else {
        // Dynamic fallback mode: try largest to smallest
        ptdesc = pagetable_alloc_noprof(gfp, largest_order);
#if defined(CONFIG_CONTIGUOUS_PAGETABLE_DEBUG)
        if (ptdesc)
            atomic_long_inc(&pagetable_alloc_success[largest_order]);
        else
            atomic_long_inc(&pagetable_alloc_fail[largest_order]);
#endif
        if (!ptdesc){
		ptdesc = pagetable_alloc_noprof(gfp, 0);
	}
    }

    if (!ptdesc)
        return NULL;
    if (!pagetable_pmd_ctor(mm, ptdesc)) {
        pagetable_free(ptdesc);
        return NULL;
    }
    
    pgd_t *pgd = pgd_offset(mm, addr);
    if (pgd_none(*pgd) || pgd_bad(*pgd))
	    return NULL;
    
    p4d_t *p4d = p4d_offset(pgd, addr);
    if (p4d_none(*p4d) || p4d_bad(*p4d))
	    return NULL;
    
    pud_t *pud = pud_offset(p4d, addr);
    if (pud_none(*pud) || pud_bad(*pud))
	    return NULL;
    
    // Store the order bits in the PUD entry
    *pud = pud_set_pmd_order(*pud, order);

    return ptdesc_address(ptdesc);
}

#define pmd_alloc_one(...)  alloc_hooks(pmd_alloc_one_noprof(__VA_ARGS__))
#endif

#ifndef __HAVE_ARCH_PMD_FREE
static inline void pmd_free(struct mm_struct *mm, pmd_t *pmd)
{
    struct ptdesc *ptdesc = virt_to_ptdesc(pmd);

    BUG_ON((unsigned long)pmd & (PAGE_SIZE-1));
    pagetable_dtor_free(ptdesc);
}
#endif

#endif /* CONFIG_PGTABLE_LEVELS > 2 */

#if CONFIG_PGTABLE_LEVELS > 3

static inline pud_t *__pud_alloc_one_noprof(struct mm_struct *mm, unsigned long addr)
{
    gfp_t gfp = GFP_PGTABLE_USER;
    struct ptdesc *ptdesc;

    if (mm == &init_mm)
        gfp = GFP_PGTABLE_KERNEL;
    gfp &= ~__GFP_HIGHMEM;

    ptdesc = pagetable_alloc_noprof(gfp, 0);
    if (!ptdesc)
        return NULL;

    pagetable_pud_ctor(ptdesc);
    return ptdesc_address(ptdesc);
}
#define __pud_alloc_one(...)    alloc_hooks(__pud_alloc_one_noprof(__VA_ARGS__))

#ifndef __HAVE_ARCH_PUD_ALLOC_ONE
/**
 * pud_alloc_one - allocate memory for a PUD-level page table
 * @mm: the mm_struct of the current context
 *
 * Allocate memory for a page table using %GFP_PGTABLE_USER for user context
 * and %GFP_PGTABLE_KERNEL for kernel context.
 *
 * Return: pointer to the allocated memory or %NULL on error
 */
static inline pud_t *pud_alloc_one_noprof(struct mm_struct *mm, unsigned long addr)
{
    return __pud_alloc_one_noprof(mm, addr);
}
#define pud_alloc_one(...)  alloc_hooks(pud_alloc_one_noprof(__VA_ARGS__))
#endif

static inline void __pud_free(struct mm_struct *mm, pud_t *pud)
{
    struct ptdesc *ptdesc = virt_to_ptdesc(pud);

    BUG_ON((unsigned long)pud & (PAGE_SIZE-1));
    pagetable_dtor_free(ptdesc);
}

#ifndef __HAVE_ARCH_PUD_FREE
static inline void pud_free(struct mm_struct *mm, pud_t *pud)
{
    __pud_free(mm, pud);
}
#endif

#endif /* CONFIG_PGTABLE_LEVELS > 3 */

#if CONFIG_PGTABLE_LEVELS > 4

static inline p4d_t *__p4d_alloc_one_noprof(struct mm_struct *mm, unsigned long addr)
{
    gfp_t gfp = GFP_PGTABLE_USER;
    struct ptdesc *ptdesc;

    if (mm == &init_mm)
        gfp = GFP_PGTABLE_KERNEL;
    gfp &= ~__GFP_HIGHMEM;

    ptdesc = pagetable_alloc_noprof(gfp, 0);
    if (!ptdesc)
        return NULL;

    pagetable_p4d_ctor(ptdesc);
    return ptdesc_address(ptdesc);
}
#define __p4d_alloc_one(...)    alloc_hooks(__p4d_alloc_one_noprof(__VA_ARGS__))

#ifndef __HAVE_ARCH_P4D_ALLOC_ONE
static inline p4d_t *p4d_alloc_one_noprof(struct mm_struct *mm, unsigned long addr)
{
    return __p4d_alloc_one_noprof(mm, addr);
}
#define p4d_alloc_one(...)  alloc_hooks(p4d_alloc_one_noprof(__VA_ARGS__))
#endif

static inline void __p4d_free(struct mm_struct *mm, p4d_t *p4d)
{
    struct ptdesc *ptdesc = virt_to_ptdesc(p4d);

    BUG_ON((unsigned long)p4d & (PAGE_SIZE-1));
    pagetable_dtor_free(ptdesc);
}

#ifndef __HAVE_ARCH_P4D_FREE
static inline void p4d_free(struct mm_struct *mm, p4d_t *p4d)
{
    if (!mm_p4d_folded(mm))
        __p4d_free(mm, p4d);
}
#endif

#endif /* CONFIG_PGTABLE_LEVELS > 4 */

static inline pgd_t *__pgd_alloc_noprof(struct mm_struct *mm, unsigned int order)
{
    gfp_t gfp = GFP_PGTABLE_USER;
    struct ptdesc *ptdesc;

    if (mm == &init_mm)
        gfp = GFP_PGTABLE_KERNEL;
    gfp &= ~__GFP_HIGHMEM;

    ptdesc = pagetable_alloc_noprof(gfp, order);
    if (!ptdesc)
        return NULL;

    pagetable_pgd_ctor(ptdesc);
    return ptdesc_address(ptdesc);
}
#define __pgd_alloc(...)    alloc_hooks(__pgd_alloc_noprof(__VA_ARGS__))

static inline void __pgd_free(struct mm_struct *mm, pgd_t *pgd)
{
    struct ptdesc *ptdesc = virt_to_ptdesc(pgd);

    BUG_ON((unsigned long)pgd & (PAGE_SIZE-1));
    pagetable_dtor_free(ptdesc);
}

#ifndef __HAVE_ARCH_PGD_FREE
static inline void pgd_free(struct mm_struct *mm, pgd_t *pgd)
{
    __pgd_free(mm, pgd);
}
#endif

#endif /* CONFIG_MMU */

#endif /* __ASM_GENERIC_PGALLOC_H */
