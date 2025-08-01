// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/init.h>

#define PROC_ALLOC_MODE     "pagetable_alloc_mode"
#define PROC_FIXED_ORDER    "pagetable_fixed_order"

// --- Global tunables ---
int pagetable_alloc_mode = 1;     // 1 = dynamic fallback (default), 0 = fixed
int pagetable_fixed_order = 5;    // order 5 = 128KB

EXPORT_SYMBOL_GPL(pagetable_alloc_mode);
EXPORT_SYMBOL_GPL(pagetable_fixed_order);

// --- /proc read helpers ---
static int show_alloc_mode(struct seq_file *m, void *v)
{
    seq_printf(m, "%d\n", pagetable_alloc_mode);
    return 0;
}

static int show_fixed_order(struct seq_file *m, void *v)
{
    seq_printf(m, "%d\n", pagetable_fixed_order);
    return 0;
}

// --- /proc write helpers ---
static ssize_t write_alloc_mode(struct file *file, const char __user *buf,
                size_t count, loff_t *ppos)
{
    char buffer[16];
    int val;

    if (count >= sizeof(buffer))
        return -EINVAL;
    if (copy_from_user(buffer, buf, count))
        return -EFAULT;

    buffer[count] = '\0';
    if (kstrtoint(strim(buffer), 10, &val) != 0 || (val != 0 && val != 1))
        return -EINVAL;

    pagetable_alloc_mode = val;
    return count;
}

static ssize_t write_fixed_order(struct file *file, const char __user *buf,
                 size_t count, loff_t *ppos)
{
    char buffer[16];
    int val;

    if (count >= sizeof(buffer))
        return -EINVAL;
    if (copy_from_user(buffer, buf, count))
        return -EFAULT;

    buffer[count] = '\0';
    if (kstrtoint(strim(buffer), 10, &val) != 0 || val < 0 || val > 9)
        return -EINVAL;

    pagetable_fixed_order = val;
    return count;
}

// --- file ops ---
static int open_alloc_mode(struct inode *inode, struct file *file)
{
    return single_open(file, show_alloc_mode, NULL);
}

static int open_fixed_order(struct inode *inode, struct file *file)
{
    return single_open(file, show_fixed_order, NULL);
}

static const struct proc_ops proc_alloc_mode_ops = {
    .proc_open    = open_alloc_mode,
    .proc_read    = seq_read,
    .proc_write   = write_alloc_mode,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static const struct proc_ops proc_fixed_order_ops = {
    .proc_open    = open_fixed_order,
    .proc_read    = seq_read,
    .proc_write   = write_fixed_order,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

// --- init function ---
static int __init pagetable_proc_init(void)
{
    proc_create(PROC_ALLOC_MODE, 0644, NULL, &proc_alloc_mode_ops);
    proc_create(PROC_FIXED_ORDER, 0644, NULL, &proc_fixed_order_ops);
    pr_info("pagetable_config: /proc/%s and /proc/%s created\n",
        PROC_ALLOC_MODE, PROC_FIXED_ORDER);
    return 0;
}

late_initcall(pagetable_proc_init);