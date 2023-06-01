#include <linux/sched.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <uapi/linux/fs.h>
#include <linux/file.h>

#include "../../fs/hmdfs/hmdfs_device_view.h"
#include "../../fs/hmdfs/inode.h"
#include "../../fs/hmdfs/hmdfs.h"
int write_content_and_inode(const char *path, const char *delpath)
{
    struct file *delfile = NULL; 
    loff_t wpos = 0;
    char writepath[128];
    struct hmdfs_inode_info *delinfo = NULL;
    struct inode *lower_inode = NULL;
    memset(writepath, 0, sizeof(writepath));
    strcpy(writepath, path);
    delfile = filp_open(delpath, O_RDWR | O_CREAT, 0);
    if (IS_ERR(delfile))
    {
        printk(KERN_ERR "open dir %s error.\n", path);
        return -1;
    }
    delinfo = hmdfs_i(delfile->f_inode);
    lower_inode = delinfo->lower_inode;
    if(!(lower_inode->i_flags & S_DEDUP)){
    kernel_write(delfile, writepath, sizeof(writepath), &wpos);
    printk(KERN_INFO "i_flags %u", lower_inode->i_flags);
    lower_inode->i_flags |= S_DEDUP;
    printk(KERN_INFO "i_flags %u", lower_inode->i_flags);
    write_inode_now(lower_inode,1);
    }
    filp_close(delfile, NULL);
    return 0;
}

static int dedup_path(const char *path)
{

    char buf_path[128];
    char buf_duppath[128];
    char buf_islocal[2];
    // 通过文件路径获取到vfs的file结构体
    struct file *file = NULL;
    loff_t pos = 0;
    file = filp_open(path, O_RDONLY | O_CREAT, 0);
    if (IS_ERR(file))
    {
        printk(KERN_ERR "open dir %s error.\n", path);
        return -1;
    }

    // read delpath.txt
    while (kernel_read(file, buf_path, sizeof(buf_path), &pos) > 0)
    {
        kernel_read(file, buf_duppath, sizeof(buf_duppath), &pos);
        kernel_read(file, buf_islocal, sizeof(buf_islocal), &pos);

        printk(KERN_INFO "path:%s ", buf_path);
        printk(KERN_INFO "delpath:%s ", buf_duppath);
        printk(KERN_INFO "islocal:%s ", buf_islocal);
        write_content_and_inode(buf_path, buf_duppath);
        memset(buf_path, 0, sizeof(buf_path));
        memset(buf_duppath, 0, sizeof(buf_duppath));
        memset(buf_islocal, 0, sizeof(buf_islocal));
    }
    printk(KERN_INFO "write successfully");
    filp_close(file, NULL);
    return 0;
}

static int __init dedup_init(void)
{
    dedup_path("/data/delpath");
    return 0;
}

static void __exit dedup_exit(void)
{
    printk(KERN_INFO "dedup stopped.\n");
}

module_init(dedup_init);
module_exit(dedup_exit);

MODULE_LICENSE("GPL");
