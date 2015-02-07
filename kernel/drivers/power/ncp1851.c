/******************************************************************************/
/*                                                               Date:08/2012 */
/*                             PRESENTATION                                   */
/*                                                                            */
/*      Copyright 2012 TCL Communication Technology Holdings Limited.         */
/*                                                                            */
/* This material is company confidential, cannot be reproduced in any form    */
/* without the written permission of TCL Communication Technology Holdings    */
/* Limited.                                                                   */
/*                                                                            */
/* -------------------------------------------------------------------------- */
/* Author:  Yu Bin                                                            */
/* E-Mail:  bin.yu@tcl-mobile.com                                             */
/* Role  :  NCP1851                                                           */
/* Reference documents :  NCP1851 Datasheet Rev 1.0.pdf                       */
/* -------------------------------------------------------------------------- */
/* Comments: This is ncp1851 driver, this file is provide interface for       */
/*           msm_battery.c                                                    */
/* File    : kernel/drivers/power/ncp1851.c                                   */
/* Labels  :                                                                  */
/* -------------------------------------------------------------------------- */
/* ========================================================================== */
/* Modifications on Features list / Changes Request / Problems Report         */
/* -------------------------------------------------------------------------- */
/* date    | author         | key                | comment (what, where, why) */
/* --------|----------------|--------------------|--------------------------- */
/* 12/09/10| Yu Bin         |                    | Create this file           */
/*---------|----------------|--------------------|--------------------------- */
/* 12/09/20| Yu Bin         |                    | Add probed, set different  */
/*         |                |                    | current use usb_host or    */
/*         |                |                    | usb_wall, add battery full */
/*---------|----------------|--------------------|--------------------------- */
/* 12/09/28| Yu Bin         |                    | Add restart_charging       */
/*         |                |                    | interface, modified        */
/*         |                |                    | stop_charging intercade    */
/*---------|----------------|--------------------|--------------------------- */
/* 12/10/31| Yu Bin         |                    | Add DPP state also mean    */
/*         |                |                    | battery full               */
/*---------|----------------|--------------------|--------------------------- */
/* 12/11/14| Yu Bin         |                    | Add ncp1850 charing support*/
/*---------|----------------|--------------------|--------------------------- */
/* 12/12/06| Yu Bin         |                    | Disable ncp1850 interrupt  */
/*---------|----------------|--------------------|--------------------------- */
/******************************************************************************/

/*
In the feature, this file will contain three part:
1. Interface for msm_battery.c
    start_usb_charging, stop_charging ...... is the interface, msm_battery.c call them to control ncp1851.
2. Interface for APP
    ncp_dev_fops is the interface, APP can read/write ncp1851 registers through this interface.
3. Communication with AMSS
    use to control pm8029 charging state, not available now.
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/wait.h>

#if 1
#define NCP_MSG(fmt, arg...) printk(KERN_ERR "[NCP] %s: " fmt "[NCP]\n", __func__, ## arg)
#else
#define NCP_MSG(fmt, arg...) do {} while (0)
#endif

struct ncp_struct
{
        struct i2c_client *i2c_dev;
        struct miscdevice ncp_dev;
        unsigned int irq;
        bool irq_enabled;
        spinlock_t irq_enabled_lock;
};
static struct ncp_struct *ncp_global;

static bool is_ncp_probed = 0;

static int ncp_dev_open(struct inode *inode, struct file *filp)
{
        filp->private_data = ncp_global;
        return 0;
}

#define NCP1851ADDRESS 0x36 
#define IOCTL_I2C_IOCTL _IOWR('n', 88, int)

typedef struct
{
        u8 dir;
        u8 reg;
        u8 val;
}i2c;

/*======================================================================================
 *
 * Name:        static int ncp_i2c_write
 *
 * Author:      YuBin (bin.yu@tcl-mobile.com)
 * 
 * Date:        2012/09/11
 *
 * Description: Write one byte to ncp1851 register
 *
 * Inputs:      reg
 *              val
 *
 * Outputs:     -EIO
 *              0
 *
 * Notes:
 *
 *======================================================================================*/
static int ncp_i2c_write(u8 reg, u8 *val)
{
        u8 tmp[2]={reg, *val};
        struct i2c_msg msg[] = {
                {
                        .addr  = NCP1851ADDRESS,
                        .flags = 0,
                        .len = 2,
                        .buf = tmp,
                },
        };

        if (i2c_transfer(ncp_global->i2c_dev->adapter, msg, 1) < 0) {
                printk(KERN_ERR "ERROR: ncp_i2c_write failed, reg=%x, val=%x .\n",reg,*val);
                return -EIO;
        }

        return 0;
}

/*======================================================================================
 *
 * Name:        static int ncp_i2c_read
 *
 * Author:      YuBin (bin.yu@tcl-mobile.com)
 * 
 * Date:        2012/09/11
 *
 * Description: Write one byte from ncp1851 register
 *
 * Inputs:      reg
 *              val
 *
 * Outputs:     -EIO
 *              0
 *
 * Notes:
 *
 *======================================================================================*/
static int ncp_i2c_read(u8 reg, u8 *val)
{
        struct i2c_msg msg[] = {
                {
                        .addr   = NCP1851ADDRESS,
                        .flags = 0,
                        .len   = 1,
                        .buf   = &reg,
                },
                {
                        .addr   = NCP1851ADDRESS,
                        .flags = I2C_M_RD,
                        .len   = 1,
                        .buf   = val,
                },
        };

	if (i2c_transfer(ncp_global->i2c_dev->adapter, msg, 2) < 0) {
                printk(KERN_ERR "ERROR: ncp_i2c_read failed, reg=%x, val=%x .\n",reg,*val);
                return -EIO;
        }

        return 0;
}

/*======================================================================================
 *
 * Name:        static long ncp_dev_ioctl
 *
 * Author:      YuBin (bin.yu@tcl-mobile.com)
 * 
 * Date:        2012/09/11
 *
 * Description: ncp1851 ioctl operation for android
 *
 * Inputs:      filp
 *              cmd
 *              arg
 *
 * Outputs:     -EIO
 *              -EFAULT
 *              0
 *
 * Notes:       It is called by android.
 *
 *======================================================================================*/
static long ncp_dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
        void __user *argp = (void __user *)arg;
        i2c rv;
        int ret;

        if (copy_from_user(&rv, argp, sizeof(rv)) != 0)
        {
                printk(KERN_ERR "ERROR: ncp copy_from_user failed.\n");
                return -EFAULT;
        }
	
        if(rv.dir == 0)
        {
                ret = ncp_i2c_write(rv.reg, &rv.val);
                if(ret)
                        return ret;
        }
        else if(rv.dir == 1)
        {
                ret = ncp_i2c_read(rv.reg, &rv.val);
                if(ret)
                        return ret;
        }

        if (copy_to_user((i2c *) argp, &rv, sizeof(rv)) != 0)
        {
                printk(KERN_ERR "ERROR: ncp copy_to_user failed.\n");
                return -EFAULT;
        }

        return 0;
}

static struct file_operations ncp_dev_fops = {
        .owner =    THIS_MODULE,
        .unlocked_ioctl =  ncp_dev_ioctl,
        .open =     ncp_dev_open,
};

/*======================================================================================
 *
 * Name:        static irqreturn_t ncp_irq_handler
 *
 * Author:      YuBin (bin.yu@tcl-mobile.com)
 * 
 * Date:        2012/09/11
 *
 * Description: ncp1851 irq handler
 *
 * Inputs:      irq
 *              dev_id
 *
 * Outputs:     IRQ_HANDLED
 *
 * Notes:       It is reserved for feature.
 *
 *======================================================================================*/
static irqreturn_t ncp_irq_handler(int irq, void *dev_id)
{
        struct ncp_struct *ncp = dev_id;

        disable_irq_nosync(ncp->irq);
        NCP_MSG("");
        //enable_irq(ncp->irq);

        return IRQ_HANDLED;
}

/*======================================================================================
 *
 * Name:        static int ncp_probe
 *
 * Author:      YuBin (bin.yu@tcl-mobile.com)
 * 
 * Date:        2012/09/11
 *
 * Description: ncp1851 probe function
 *
 * Inputs:      client
 *              devid
 *
 * Outputs:     0
 *              <0
 *
 * Notes:       It is called once when kernel boot up.
 *
 *======================================================================================*/
static int ncp_probe(struct i2c_client *client, const struct i2c_device_id * devid)
{
        int ret=0;
        struct ncp_struct *ncp;

        NCP_MSG("");

        if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
                printk(KERN_ERR "ERROR: ncp i2c check functionality failed.\n");
                ret=-ENODEV;
                goto exit_i2c_check_functionality;
        }

        ncp = kzalloc(sizeof(struct ncp_struct), GFP_KERNEL);
        if (ncp == NULL) {
                printk(KERN_ERR "ERROR: ncp kzalloc failed.\n");
                ret=-ENOMEM;
                goto exit_kzalloc;
        }

        ncp->i2c_dev=client;
        ncp->i2c_dev->addr=NCP1851ADDRESS;
        ncp->irq = client->irq;
        ncp->ncp_dev.minor = MISC_DYNAMIC_MINOR;
        ncp->ncp_dev.name = "ncp";
        ncp->ncp_dev.fops = &ncp_dev_fops;

        ret = misc_register(&ncp->ncp_dev);
        if (ret) {
                printk(KERN_ERR "ERROR: misc_register failed.\n.");
                goto exit_misc_register;
        }

        ret = request_irq(ncp->irq , ncp_irq_handler, IRQF_TRIGGER_RISING, "ncp_flag", ncp);
        if (ret) {
                printk(KERN_ERR "ERROR: request_irq failed.\n");
                goto exit_request_irq;
        }

        i2c_set_clientdata(client, ncp);
        ncp_global=ncp;

        is_ncp_probed = 1;
        return 0;

exit_request_irq:
        misc_deregister(&ncp->ncp_dev);
exit_misc_register:
        kfree(ncp);
exit_kzalloc:
exit_i2c_check_functionality:
        return ret;
}

/*======================================================================================
 *
 * Name:        static int ncp_remove
 *
 * Author:      YuBin (bin.yu@tcl-mobile.com)
 * 
 * Date:        2012/09/11
 *
 * Description: ncp1851 remove function
 *
 * Inputs:      client
 *
 * Outputs:     0
 *
 * Notes:       It won't be called as now on.
 *
 *======================================================================================*/
static int ncp_remove(struct i2c_client *client)
{
        struct ncp_struct *ncp;

        NCP_MSG("");

        ncp = i2c_get_clientdata(client);
        free_irq(client->irq,ncp);
        misc_deregister(&ncp->ncp_dev);
        kfree(ncp);
	
        return 0;
}

static const struct i2c_device_id ncp_id[] = {
        { "ncp", 0 },
        { }
};

static struct i2c_driver ncp_driver = {
        .probe	= ncp_probe,
        .remove	= ncp_remove,
        .id_table	= ncp_id,
        .driver	= {
                .name	= "ncp",
        },
};

/*======================================================================================
 *
 * Name:        static int __init ncp_init
 *
 * Author:      YuBin (bin.yu@tcl-mobile.com)
 * 
 * Date:        2012/09/11
 *
 * Description: ncp1851 module install
 *
 * Inputs:      void
 *
 * Outputs:     depand on i2c_add_driver()
 *
 * Notes:       It is called once when kernel boot up.
 *
 *======================================================================================*/
static int __init ncp_init(void)
{
        NCP_MSG("");
        return i2c_add_driver(&ncp_driver);
}

/*======================================================================================
 *
 * Name:        static void __exit ncp_exit
 *
 * Author:      YuBin (bin.yu@tcl-mobile.com)
 * 
 * Date:        2012/09/11
 *
 * Description: ncp1851 module uninstall
 *
 * Inputs:      void
 *
 * Outputs:     void
 *
 * Notes:       It won't be called as now on.
 *
 *======================================================================================*/
static void __exit ncp_exit(void)
{
        NCP_MSG("");
        i2c_del_driver(&ncp_driver);
}

module_init(ncp_init);
module_exit(ncp_exit);

/*======================================================================================
 *
 * Name:        u8 NCPRX
 *
 * Author:      YuBin (bin.yu@tcl-mobile.com)
 * 
 * Date:        2012/09/11
 *
 * Description: ncp1851 i2c read interface for msm_battery.c
 *
 * Inputs:      reg
 *
 * Outputs:     val
 *
 * Notes:
 *
 *======================================================================================*/
u8 NCPRX(u8 reg)
{
        u8 val;
        if(ncp_i2c_read(reg, &val)){
                printk(KERN_ERR "ERROR: NCPRX failed, reg=%x, val=%x .\n",reg,val);
                return 0xff;
        }else{
                return val;
        }
}

/*======================================================================================
 *
 * Name:        void NCPTX
 *
 * Author:      YuBin (bin.yu@tcl-mobile.com)
 * 
 * Date:        2012/09/11
 *
 * Description: ncp1851 i2c write interface for msm_battery.c
 *
 * Inputs:      reg
 *              val
 *
 * Outputs:     void
 *
 * Notes:
 *
 *======================================================================================*/
void NCPTX(u8 reg, u8 val)
{
        if(ncp_i2c_write(reg, &val)){
                printk(KERN_ERR "ERROR: NCPTX failed, reg=%x, val=%x .\n",reg,val);
        }
}

/*======================================================================================
 *
 * Name:        void start_usb_pc_charging
 *
 * Author:      YuBin (bin.yu@tcl-mobile.com)
 * 
 * Date:        2012/09/20
 *
 * Description: ncp1851 charging control interface for msm_battery.c
 *
 * Inputs:      void
 *
 * Outputs:     void
 *
 * Notes:
 *
 *======================================================================================*/
void start_usb_pc_charging(void)
{
        u8 a = (NCPRX(0x02)&0x08)>>3;

        if(1==a)//1850
        {
                NCPTX(0x02,0xF8);
                NCPTX(0x10,0xA4);
        }
        else if(0==a)//1851
        {
                NCPTX(0x02,0xF0);
                NCPTX(0x10,0x60);
        }

        NCPTX(0x01,0x41);
        NCPTX(0x0E,0x2A);
        NCPTX(0x0F,0x21);
}

/*======================================================================================
 *
 * Name:        void start_usb_wall_charging
 *
 * Author:      YuBin (bin.yu@tcl-mobile.com)
 * 
 * Date:        2012/09/20
 *
 * Description: ncp1851 charging control interface for msm_battery.c
 *
 * Inputs:      void
 *
 * Outputs:     void
 *
 * Notes:
 *
 *======================================================================================*/
void start_usb_wall_charging(void)
{
        u8 a = (NCPRX(0x02)&0x08)>>3;

        if(1==a)//1850
        {
                NCPTX(0x02,0xF8);
                NCPTX(0x10,0xA4);
        }
        else if(0==a)//1851
        {
                NCPTX(0x02,0xF0);
                NCPTX(0x10,0x60);
        }

        NCPTX(0x01,0x41);
        NCPTX(0x0E,0x2A);
        NCPTX(0x0F,0x26);
}

/*======================================================================================
 *
 * Name:        void stop_charging
 *
 * Author:      YuBin (bin.yu@tcl-mobile.com)
 * 
 * Date:        2012/09/11
 *
 * Description: ncp1851 charging control interface for msm_battery.c
 *
 * Inputs:      void
 *
 * Outputs:     void
 *
 * Notes:
 *
 *======================================================================================*/
void stop_charging(void)
{
        NCPTX(0x01, 0x01);
}

/*======================================================================================
 *
 * Name:        void restart_charging
 *
 * Author:      YuBin (bin.yu@tcl-mobile.com)
 * 
 * Date:        2012/09/20
 *
 * Description: ncp1851 charging control interface for msm_battery.c
 *
 * Inputs:      void
 *
 * Outputs:     void
 *
 * Notes:
 *
 *======================================================================================*/
void restart_charging(void)
{
        NCPTX(0x01, 0x41);
}

/*======================================================================================
 *
 * Name:        bool is_battery_full
 *
 * Author:      YuBin (bin.yu@tcl-mobile.com)
 * 
 * Date:        2012/09/20
 *
 * Description: ncp1851 charging control interface for msm_battery.c
 *
 * Inputs:      void
 *
 * Outputs:     bool
 *
 * Notes:
 *
 *======================================================================================*/
bool is_battery_full(void)
{
       u8 a;

       if(!is_ncp_probed)
                return 0;

       a = NCPRX(0x00)>>4;
       if((a==6)||(a==7))
                return 1;
       else
                return 0;
}

MODULE_DESCRIPTION("ncp1851driver");
MODULE_AUTHOR("yubin");
MODULE_LICENSE("GPL");


