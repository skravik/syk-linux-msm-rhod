/* arch/arm/mach-msm/smd_rpcrouter_device.c
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2007 QUALCOMM Incorporated
 * Author: San Mehat <san@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/byteorder.h>
#include <linux/platform_device.h>

#include <linux/msm_rpcrouter.h>
#include "smd_rpcrouter.h"

/* Next minor # available for a remote server */
static int next_minor = 1;

struct class *msm_rpcrouter_class;
dev_t msm_rpcrouter_devno;

static struct cdev rpcrouter_cdev;
static struct device *rpcrouter_device;

static int rpcrouter_open(struct inode *inode, struct file *filp)
{
	int rc;
	struct msm_rpc_endpoint *ept;

	rc = nonseekable_open(inode, filp);
	if (rc < 0)
		return rc;

	ept = msm_rpcrouter_create_local_endpoint(inode->i_rdev);
	if (!ept)
		return -ENOMEM;

	filp->private_data = ept;
printk(KERN_DEBUG "rpcrouter_open\n");
	return 0;
}

static int rpcrouter_release(struct inode *inode, struct file *filp)
{
	struct msm_rpc_endpoint *ept;
	ept = (struct msm_rpc_endpoint *) filp->private_data;

	return msm_rpcrouter_destroy_local_endpoint(ept);
}

static ssize_t rpcrouter_read(struct file *filp, char __user *buf,
			      size_t count, loff_t *ppos)
{
	struct msm_rpc_endpoint *ept;
	struct rr_fragment *frag, *next;
	int rc;

	ept = (struct msm_rpc_endpoint *) filp->private_data;

	rc = __msm_rpc_read(ept, &frag, count, -1);
	if (rc < 0)
		return rc;

	count = rc;

	while (frag != NULL) {		
		if (copy_to_user(buf, frag->data, frag->length)) {
			printk(KERN_ERR
			       "rpcrouter: could not copy all read data to user!\n");
			rc = -EFAULT;
		}
		buf += frag->length;
		next = frag->next;
		kfree(frag);
		frag = next;
	}

	return rc;
}

static ssize_t rpcrouter_write(struct file *filp, const char __user *buf,
				size_t count, loff_t *ppos)
{
	struct msm_rpc_endpoint	*ept;
	int rc = 0;
	void *k_buffer;

	ept = (struct msm_rpc_endpoint *) filp->private_data;

	if (count > RPCROUTER_MSGSIZE_MAX)
		return -EINVAL;

	k_buffer = kmalloc(count, GFP_KERNEL);
	if (!k_buffer)
		return -ENOMEM;

	if (copy_from_user(k_buffer, buf, count)) {
		rc = -EFAULT;
		goto write_out_free;
	}

	rc = msm_rpc_write(ept, k_buffer, count);
	if (rc < 0)
		goto write_out_free;

	rc = count;
write_out_free:
	kfree(k_buffer);
	return rc;
}

static unsigned int rpcrouter_poll(struct file *filp,
				   struct poll_table_struct *wait)
{
	struct msm_rpc_endpoint *ept;
	unsigned mask = 0;
	ept = (struct msm_rpc_endpoint *) filp->private_data;

	/* If there's data already in the read queue, return POLLIN.
	 * Else, wait for the requested amount of time, and check again.
	 */
	if (!list_empty(&ept->read_q))
		mask |= POLLIN;

	if (!mask) {
		poll_wait(filp, &ept->wait_q, wait);
		if (!list_empty(&ept->read_q))
			mask |= POLLIN;
	}

	return mask;
}

static long rpcrouter_ioctl(struct file *filp, unsigned int cmd,
			    unsigned long arg)
{
	struct msm_rpc_endpoint *ept;
	struct rpcrouter_ioctl_server_args server_args;
	int rc;
	uint32_t n;

printk(KERN_DEBUG "rpcrouter_ioctl: cmd=%u\n", cmd);

	ept = (struct msm_rpc_endpoint *) filp->private_data;
	switch (cmd) {

	case RPC_ROUTER_IOCTL_GET_VERSION:
		n = RPC_ROUTER_VERSION_V1;
		rc = put_user(n, (unsigned int *) arg);
		break;

	case RPC_ROUTER_IOCTL_GET_MTU:
		/* the pacmark word reduces the actual payload
		 * possible per message
		 */
		n = RPCROUTER_MSGSIZE_MAX - sizeof(uint32_t);
		rc = put_user(n, (unsigned int *) arg);
		break;

	case RPC_ROUTER_IOCTL_REGISTER_SERVER:
		rc = copy_from_user(&server_args, (void *) arg,
				    sizeof(server_args));
		if (rc < 0)
			break;
		msm_rpc_register_server(ept,
					server_args.prog,
					server_args.vers);
		break;

	case RPC_ROUTER_IOCTL_UNREGISTER_SERVER:
		rc = copy_from_user(&server_args, (void *) arg,
				    sizeof(server_args));
		if (rc < 0)
			break;

		msm_rpc_unregister_server(ept,
					  server_args.prog,
					  server_args.vers);
		break;

	default:
		rc = -EINVAL;
		break;
	}

	return rc;
}

static struct file_operations rpcrouter_server_fops = {
	.owner	 = THIS_MODULE,
	.open	 = rpcrouter_open,
	.release = rpcrouter_release,
	.read	 = rpcrouter_read,
	.write	 = rpcrouter_write,
	.poll    = rpcrouter_poll,
	.unlocked_ioctl	 = rpcrouter_ioctl,
};

static struct file_operations rpcrouter_router_fops = {
	.owner	 = THIS_MODULE,
	.open	 = rpcrouter_open,
	.release = rpcrouter_release,
	.read	 = rpcrouter_read,
	.write	 = rpcrouter_write,
	.poll    = rpcrouter_poll,
	.unlocked_ioctl = rpcrouter_ioctl,
};

int msm_rpcrouter_create_server_cdev(struct rr_server *server)
{
	int rc;

	if (next_minor == RPCROUTER_MAX_REMOTE_SERVERS) {
		printk(KERN_ERR
		       "rpcrouter: Minor numbers exhausted - Increase "
		       "RPCROUTER_MAX_REMOTE_SERVERS\n");
		return -ENOBUFS;
	}
	server->device_number =
		MKDEV(MAJOR(msm_rpcrouter_devno), next_minor++);

	server->device =
		device_create(msm_rpcrouter_class, rpcrouter_device,
			      server->device_number, NULL, "%.8x:%.8x",
			      server->prog, server->vers);
	if (IS_ERR(server->device)) {
		printk(KERN_ERR
		       "rpcrouter: Unable to create device (%ld)\n",
		       PTR_ERR(server->device));
		return PTR_ERR(server->device);;
	}

	cdev_init(&server->cdev, &rpcrouter_server_fops);
	server->cdev.owner = THIS_MODULE;

	rc = cdev_add(&server->cdev, server->device_number, 1);
	if (rc < 0) {
		printk(KERN_ERR
		       "rpcrouter: Unable to add chrdev (%d)\n", rc);
		device_destroy(msm_rpcrouter_class, server->device_number);
		return rc;
	}
printk(KERN_DEBUG "rpcrouter_create_server: %x:%x\n", server->prog, server->vers);
	return 0;
}

int msm_rpcrouter_create_server_pdev(struct rr_server *server)
{
	sprintf(server->pdev_name, "rs%.8x:%.8x",
		server->prog, server->vers);

	server->p_device.base.id = -1;
	server->p_device.base.name = server->pdev_name;

	server->p_device.prog = server->prog;
	server->p_device.vers = server->vers;

	platform_device_register(&server->p_device.base);
printk(KERN_DEBUG "rpcrouter_create_server: %s\n", server->pdev_name);
	return 0;
}

int msm_rpcrouter_init_devices(void)
{
	int rc;
	int major;

	/* Create the device nodes */
	msm_rpcrouter_class = class_create(THIS_MODULE, "oncrpc");
	if (IS_ERR(msm_rpcrouter_class)) {
		rc = -ENOMEM;
		printk(KERN_ERR
		       "rpcrouter: failed to create oncrpc class\n");
		goto fail;
	}

	rc = alloc_chrdev_region(&msm_rpcrouter_devno, 0,
				 RPCROUTER_MAX_REMOTE_SERVERS + 1,
				 "oncrpc");
	if (rc < 0) {
		printk(KERN_ERR
		       "rpcrouter: Failed to alloc chardev region (%d)\n", rc);
		goto fail_destroy_class;
	}

	major = MAJOR(msm_rpcrouter_devno);
	rpcrouter_device = device_create(msm_rpcrouter_class, NULL,
					 msm_rpcrouter_devno, NULL, "%.8x:%d",
					 0, 0);
	if (IS_ERR(rpcrouter_device)) {
		rc = -ENOMEM;
		goto fail_unregister_cdev_region;
	}

	cdev_init(&rpcrouter_cdev, &rpcrouter_router_fops);
	rpcrouter_cdev.owner = THIS_MODULE;

	rc = cdev_add(&rpcrouter_cdev, msm_rpcrouter_devno, 1);
	if (rc < 0)
		goto fail_destroy_device;

printk(KERN_DEBUG "RPC_dev init complete!\n");
	return 0;

fail_destroy_device:
	device_destroy(msm_rpcrouter_class, msm_rpcrouter_devno);
fail_unregister_cdev_region:
	unregister_chrdev_region(msm_rpcrouter_devno,
				 RPCROUTER_MAX_REMOTE_SERVERS + 1);
fail_destroy_class:
	class_destroy(msm_rpcrouter_class);
fail:
printk(KERN_ERR "RPC_dev init failed!\n");
	return rc;
}

void msm_rpcrouter_exit_devices(void)
{
	printk(KERN_DEBUG "RPC_dev exit\n");
	cdev_del(&rpcrouter_cdev);
	device_destroy(msm_rpcrouter_class, msm_rpcrouter_devno);
	unregister_chrdev_region(msm_rpcrouter_devno,
				 RPCROUTER_MAX_REMOTE_SERVERS + 1);
	class_destroy(msm_rpcrouter_class);
}

