/**
 * This file contains the file operations used
 * inside the chrdevs.
 * Actually only read, release and ioctl are ops of interest.
 */
#ifndef HOP_FOPS_H
#define HOP_FOPS_H

#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/fs.h>

// FUNCTION PROTOTYPES


/**
 * hop_open - open a profiled thread device node instance
 */
int hop_open(struct inode *inode, struct file *file);

/**
 * hop_ctl_open - open the ctl device node instance
 */
int hop_ctl_open(struct inode *inode, struct file *file);

/**
 * hop_release
 */
int hop_release(struct inode *inode, struct file *file);

/**
 * hop_ctl_release
 */
int hop_ctl_release(struct inode *inode, struct file *file);


/**
 * hop_read - read HOP samples from the device
 * @count:  number of bytes to read; value must be (1) at least the size of
 *      one entry and (2) no more than the total buffer size
 *
 * This function reads as many HOP samples as possible
 *
 * Returns: Number of bytes read, or negative error code
 */
ssize_t hop_read(struct file *filp, char __user *buf, size_t count,
            loff_t *fpos);

/**
 * hop_ioctl() - perform an ioctl command
 *
 * This function is actually used to read a profiled thread statistics
 */
long hop_ioctl(struct file *file, unsigned int cmd, unsigned long arg);


/**
 * hop_ctl_ioctl() - perform an ioctl command
 *
 * Through this function is possible to manage the HOP module.
 */
long hop_ctl_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

#endif /* HOP_FOPS_H */