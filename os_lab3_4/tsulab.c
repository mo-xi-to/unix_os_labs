#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/time.h>
#include <linux/ktime.h>

#define PROCFS_NAME "tsulab"
#define BUF_SIZE 128

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nikolaeva Anastasia 932302");
MODULE_DESCRIPTION("OS lab 3-4");

static struct proc_dir_entry *our_proc_file = NULL;

static int calculate_cinderella_minutes(void)
{
    struct timespec64 now;
    struct tm tm_val;
    int minutes_passed_today;
    int minutes_in_day = 24 * 60;
    
    ktime_get_real_ts64(&now);
    
    time64_to_tm(now.tv_sec, 0, &tm_val);

    minutes_passed_today = (tm_val.tm_hour * 60) + tm_val.tm_min;

    return minutes_in_day - minutes_passed_today;
}

static ssize_t procfile_read(struct file *file_pointer, char __user *buffer,
                             size_t buffer_length, loff_t *offset)
{
    char s[BUF_SIZE];
    int len;
    int minutes_left;
    ssize_t ret;

    if (*offset > 0)
        return 0;

    minutes_left = calculate_cinderella_minutes();

    len = snprintf(s, BUF_SIZE, "Cinderella has %d minutes left until midnight.\n", minutes_left);

    if (copy_to_user(buffer, s, len)) {
        return -EFAULT;
    }

    *offset += len;
    ret = len;
    
    return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops proc_file_fops = {
    .proc_read = procfile_read,
};
#else
static const struct file_operations proc_file_fops = {
    .read = procfile_read,
};
#endif

static int __init tsulab_init(void)
{
    pr_info("Welcome to the Tomsk State University\n");

    our_proc_file = proc_create(PROCFS_NAME, 0644, NULL, &proc_file_fops);
    
    if (NULL == our_proc_file) {
        pr_alert("Error: Could not initialize /proc/%s\n", PROCFS_NAME);
        return -ENOMEM;
    }

    pr_info("/proc/%s created\n", PROCFS_NAME);
    return 0;
}

static void __exit tsulab_exit(void)
{
    proc_remove(our_proc_file);
    pr_info("/proc/%s removed\n", PROCFS_NAME);
    pr_info("Tomsk State University forever!\n");
}

module_init(tsulab_init);
module_exit(tsulab_exit);