/*
 *drivers/input/touchscreen/ft5x06_ex_fun.c
 *
 *FocalTech ft5x0x expand function for debug.
 *
 *Copyright (c) 2010  Focal tech Ltd.
 *
 *This software is licensed under the terms of the GNU General Public
 *License version 2, as published by the Free Software Foundation, and
 *may be copied, distributed, and modified under those terms.
 *
 *This program is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *GNU General Public License for more details.
 *
 *Note:the error code of EIO is the general error in this file.
 */


#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/earlysuspend.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>

#include <linux/io.h>
#include <linux/platform_device.h>
#include <mach/gpio.h>
#include <linux/mount.h>
#include <linux/netdevice.h>
#include <linux/input/ft5x06_ts.h>

struct Upgrade_Info {
	u16 delay_aa;		/*delay of write FT_UPGRADE_AA */
	u16 delay_55;		/*delay of write FT_UPGRADE_55 */
	u8 upgrade_id_1;	/*upgrade id 1 */
	u8 upgrade_id_2;	/*upgrade id 2 */
	u16 delay_readid;	/*delay of read id */
};


int fts_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf,
			  u32 dw_lenth);


/*[PLATFORM]-Mod-BEGIN by TCTNB.WPL, 2013/02/20, refer to bug399259*/
#ifdef CONFIG_MACH_MSM8X25_SCRIBE5
static unsigned char CTPM_FW_MUTTO[] = {
#include "ft5x06_firmware_upgrade_mutto.h"
};

static unsigned char CTPM_FW_JUNDA[] = {
#include "ft5x06_firmware_upgrade_junda.h"
};

static unsigned char* CTPM_FW;
extern int TP_VENDOR;

#else

static unsigned char CTPM_FW[] = {
#include "ft5x06_firmware_upgrade.h"
};
#endif
/*[PLATFORM]-Mod-END by TCTNB.WPL*/


static struct mutex g_device_mutex;

int ft5x0x_write_reg(struct i2c_client *client, u8 regaddr, u8 regvalue)
{
	unsigned char buf[2] = {0};
	buf[0] = regaddr;
	buf[1] = regvalue;

	return ft5x06_i2c_write(client, buf, sizeof(buf));
}


int ft5x0x_read_reg(struct i2c_client *client, u8 regaddr, u8 *regvalue)
{
	return ft5x06_i2c_read(client, &regaddr, 1, regvalue, 1);
}


int fts_ctpm_auto_clb(struct i2c_client *client)
{
	unsigned char uc_temp = 0x00;
	unsigned char i = 0;

	/*start auto CLB */
	msleep(200);

	ft5x0x_write_reg(client, 0, FTS_FACTORYMODE_VALUE);
	/*make sure already enter factory mode */
	msleep(100);
	/*write command to start calibration */
	ft5x0x_write_reg(client, 2, 0x4);
	msleep(300);
	for (i = 0; i < 100; i++) {
		ft5x0x_read_reg(client, 0, &uc_temp);
		/*return to normal mode, calibration finish */
		if (0x0 == ((uc_temp & 0x70) >> 4))
			break;
	}

	msleep(200);
	/*calibration OK */
	msleep(300);
	ft5x0x_write_reg(client, 0, FTS_FACTORYMODE_VALUE);	/*goto factory mode for store */
	msleep(100);	/*make sure already enter factory mode */
	ft5x0x_write_reg(client, 2, 0x5);	/*store CLB result */
	msleep(300);
	ft5x0x_write_reg(client, 0, FTS_WORKMODE_VALUE);	/*return to normal mode */
	msleep(300);

	/*store CLB result OK */
	return 0;
}

/*
upgrade with *.i file
*/
int fts_ctpm_fw_upgrade_with_i_file(struct i2c_client *client)
{
	u8 *pbt_buf = NULL;
	int i_ret;

/*[PLATFORM]-Mod-BEGIN by TCTNB.WPL, 2013/02/20, refer to bug399259*/
#ifdef CONFIG_MACH_MSM8X25_SCRIBE5
	int fw_len;

	if(TP_VENDOR == IS_MUTTO_TP){
		CTPM_FW = CTPM_FW_MUTTO;
		fw_len = sizeof(CTPM_FW_MUTTO);
	}else if(TP_VENDOR == IS_JUNDA_TP){
		CTPM_FW = CTPM_FW_JUNDA;
		fw_len = sizeof(CTPM_FW_JUNDA);
	}else{
		printk("\n\n--by wangpl--, <%s>, UNKOWN TP, return err\n\n", __func__);
		return -EIO;
	}
#else
	int fw_len = sizeof(CTPM_FW);
#endif
/*[PLATFORM]-Mod-END by TCTNB.WPL*/

	/*judge the fw that will be upgraded
	* if illegal, then stop upgrade and return.
	*/
	if (fw_len < 8 || fw_len > 32 * 1024) {
		dev_err(&client->dev, "%s:FW length error\n", __func__);
		return -EIO;
	}

	if ((CTPM_FW[fw_len - 8] ^ CTPM_FW[fw_len - 6]) == 0xFF
		&& (CTPM_FW[fw_len - 7] ^ CTPM_FW[fw_len - 5]) == 0xFF
		&& (CTPM_FW[fw_len - 3] ^ CTPM_FW[fw_len - 4]) == 0xFF) {
		/*FW upgrade */
		pbt_buf = CTPM_FW;
		/*call the upgrade function */

/*[PLATFORM]-Mod-BEGIN by TCTNB.WPL, 2013/02/20, refer to bug399259*/
#ifdef CONFIG_MACH_MSM8X25_SCRIBE5
	if(TP_VENDOR == IS_MUTTO_TP){
		i_ret = fts_ctpm_fw_upgrade(client, pbt_buf, sizeof(CTPM_FW_MUTTO));
	}else if(TP_VENDOR == IS_JUNDA_TP){
		i_ret = fts_ctpm_fw_upgrade(client, pbt_buf, sizeof(CTPM_FW_JUNDA));
	}else{
		printk("\n\n--by wangpl--, <%s>, UNKOWN TP, return err\n\n", __func__);
		return -EIO;
	}
#else
		i_ret = fts_ctpm_fw_upgrade(client, pbt_buf, sizeof(CTPM_FW));
#endif
/*[PLATFORM]-Mod-END by TCTNB.WPL*/

		if (i_ret != 0)
			dev_err(&client->dev, "%s:upgrade failed. err.\n",
					__func__);
#ifdef AUTO_CLB
		else
			fts_ctpm_auto_clb(client);	/*start auto CLB */
#endif
	} else {
		dev_err(&client->dev, "%s:FW format error\n", __func__);
		return -EBADFD;
	}

	return i_ret;
}

u8 fts_ctpm_get_i_file_ver(void)
{
	u16 ui_sz;

/*[PLATFORM]-Mod-BEGIN by TCTNB.WPL, 2013/02/20, refer to bug399259*/
#ifdef CONFIG_MACH_MSM8X25_SCRIBE5
	if(TP_VENDOR == IS_MUTTO_TP){
		CTPM_FW = CTPM_FW_MUTTO;
		ui_sz = sizeof(CTPM_FW_JUNDA);
	}else if(TP_VENDOR == IS_JUNDA_TP){
		CTPM_FW = CTPM_FW_JUNDA;
		ui_sz = sizeof(CTPM_FW_JUNDA);
	}else{
		printk("\n\n--by wangpl--, <%s>, UNKOWN TP, return err\n\n", __func__);
		return -EIO;
	}
#else
	ui_sz = sizeof(CTPM_FW);
#endif
/*[PLATFORM]-Mod-END by TCTNB.WPL*/

	if (ui_sz > 2)
		return CTPM_FW[ui_sz - 2];

	return 0x00;	/*default value */
}

/*
*get upgrade information depend on the ic type
*/
static void fts_get_upgrade_info(struct Upgrade_Info *upgrade_info)
{
	switch (DEVICE_IC_TYPE) {
	case IC_FT5X06:
		upgrade_info->delay_55 = FT5X06_UPGRADE_55_DELAY;
		upgrade_info->delay_aa = FT5X06_UPGRADE_AA_DELAY;
		upgrade_info->upgrade_id_1 = FT5X06_UPGRADE_ID_1;
		upgrade_info->upgrade_id_2 = FT5X06_UPGRADE_ID_2;
		upgrade_info->delay_readid = FT5X06_UPGRADE_READID_DELAY;
		break;
	case IC_FT5606:
		upgrade_info->delay_55 = FT5606_UPGRADE_55_DELAY;
		upgrade_info->delay_aa = FT5606_UPGRADE_AA_DELAY;
		upgrade_info->upgrade_id_1 = FT5606_UPGRADE_ID_1;
		upgrade_info->upgrade_id_2 = FT5606_UPGRADE_ID_2;
		upgrade_info->delay_readid = FT5606_UPGRADE_READID_DELAY;
		break;
	case IC_FT5316:
		upgrade_info->delay_55 = FT5316_UPGRADE_55_DELAY;
		upgrade_info->delay_aa = FT5316_UPGRADE_AA_DELAY;
		upgrade_info->upgrade_id_1 = FT5316_UPGRADE_ID_1;
		upgrade_info->upgrade_id_2 = FT5316_UPGRADE_ID_2;
		upgrade_info->delay_readid = FT5316_UPGRADE_READID_DELAY;
		break;
	default:
		break;
	}
}




/*update project setting
*only update these settings for COB project, or for some special case
*/
int fts_ctpm_update_project_setting(struct i2c_client *client)
{
	u8 uc_i2c_addr;	/*I2C slave address (7 bit address)*/
	u8 uc_io_voltage;	/*IO Voltage 0---3.3v;	1----1.8v*/
	u8 uc_panel_factory_id;	/*TP panel factory ID*/
	u8 buf[FTS_SETTING_BUF_LEN];
	u8 reg_val[2] = {0};
	u8 auc_i2c_write_buf[10] = {0};
	u8 packet_buf[FTS_SETTING_BUF_LEN + 6];
	u32 i = 0;
	int i_ret;

	uc_i2c_addr = client->addr;
	uc_io_voltage = 0x0;
	uc_panel_factory_id = 0x5a;


	/*Step 1:Reset  CTPM
	*write 0xaa to register 0xfc
	*/
	ft5x0x_write_reg(client, 0xfc, 0xaa);
	msleep(50);

	/*write 0x55 to register 0xfc */
	ft5x0x_write_reg(client, 0xfc, 0x55);
	msleep(30);

	/*********Step 2:Enter upgrade mode *****/
	auc_i2c_write_buf[0] = 0x55;
	auc_i2c_write_buf[1] = 0xaa;
	do {
		i++;
		i_ret = ft5x06_i2c_write(client, auc_i2c_write_buf, 2);
		msleep(5);
	} while (i_ret <= 0 && i < 5);


	/*********Step 3:check READ-ID***********************/
	auc_i2c_write_buf[0] = 0x90;
	auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =
			0x00;

	ft5x06_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);

	if (reg_val[0] == 0x79 && reg_val[1] == 0x3)
		dev_dbg(&client->dev, "[FTS] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",
			 reg_val[0], reg_val[1]);
	else
		return -EIO;

	auc_i2c_write_buf[0] = 0xcd;
	ft5x06_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 1);
	dev_dbg(&client->dev, "bootloader version = 0x%x\n", reg_val[0]);

	/*--------- read current project setting  ---------- */
	/*set read start address */
	buf[0] = 0x3;
	buf[1] = 0x0;
	buf[2] = 0x78;
	buf[3] = 0x0;

	ft5x06_i2c_read(client, buf, 4, buf, FTS_SETTING_BUF_LEN);
	dev_dbg(&client->dev, "[FTS] old setting: uc_i2c_addr = 0x%x,\
			uc_io_voltage = %d, uc_panel_factory_id = 0x%x\n",
			buf[0], buf[2], buf[4]);

	 /*--------- Step 4:erase project setting --------------*/
	auc_i2c_write_buf[0] = 0x63;
	ft5x06_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(100);

	/*----------  Set new settings ---------------*/
	buf[0] = uc_i2c_addr;
	buf[1] = ~uc_i2c_addr;
	buf[2] = uc_io_voltage;
	buf[3] = ~uc_io_voltage;
	buf[4] = uc_panel_factory_id;
	buf[5] = ~uc_panel_factory_id;
	packet_buf[0] = 0xbf;
	packet_buf[1] = 0x00;
	packet_buf[2] = 0x78;
	packet_buf[3] = 0x0;
	packet_buf[4] = 0;
	packet_buf[5] = FTS_SETTING_BUF_LEN;

	for (i = 0; i < FTS_SETTING_BUF_LEN; i++)
		packet_buf[6 + i] = buf[i];

	ft5x06_i2c_write(client, packet_buf, FTS_SETTING_BUF_LEN + 6);
	msleep(100);

	/********* reset the new FW***********************/
	auc_i2c_write_buf[0] = 0x07;
	ft5x06_i2c_write(client, auc_i2c_write_buf, 1);

	msleep(200);
	return 0;
}

int fts_ctpm_auto_upgrade(struct i2c_client *client)
{
	u8 uc_host_fm_ver = FT5x0x_REG_FW_VER;
	u8 uc_tp_fm_ver;
	int i_ret;

	ft5x0x_read_reg(client, FT5x0x_REG_FW_VER, &uc_tp_fm_ver);
	uc_host_fm_ver = fts_ctpm_get_i_file_ver();

	if (/*the firmware in touch panel maybe corrupted */
		uc_tp_fm_ver == FT5x0x_REG_FW_VER ||
		/*the firmware in host flash is new, need upgrade */
	     uc_tp_fm_ver < uc_host_fm_ver
	    ) {
		msleep(100);
		dev_dbg(&client->dev, "[FTS] uc_tp_fm_ver = 0x%x, uc_host_fm_ver = 0x%x\n",
				uc_tp_fm_ver, uc_host_fm_ver);
		i_ret = fts_ctpm_fw_upgrade_with_i_file(client);
		if (i_ret == 0)	{
			msleep(300);
			uc_host_fm_ver = fts_ctpm_get_i_file_ver();
			dev_dbg(&client->dev, "[FTS] upgrade to new version 0x%x\n",
					uc_host_fm_ver);
		} else {
			pr_err("[FTS] upgrade failed ret=%d.\n", i_ret);
			return -EIO;
		}
	}

	return 0;
}


int fts_ctpm_fw_upgrade(struct i2c_client *client, u8 *pbt_buf,
			  u32 dw_lenth)
{
	u8 reg_val[2] = {0};
	u32 i = 0;
	u32 packet_number;
	u32 j;
	u32 temp;
	u32 lenght;
	u8 packet_buf[FTS_PACKET_LENGTH + 6];
	u8 auc_i2c_write_buf[10];
	u8 bt_ecc;
	int i_ret;
	struct Upgrade_Info upgradeinfo;

	fts_get_upgrade_info(&upgradeinfo);

	/*********Step 1:Reset  CTPM *****/
	/*write 0xaa to register 0xfc */
	ft5x0x_write_reg(client, 0xfc, FT_UPGRADE_AA);
	msleep(upgradeinfo.delay_aa);

	/*write 0x55 to register 0xfc */
	ft5x0x_write_reg(client, 0xfc, FT_UPGRADE_55);

	msleep(upgradeinfo.delay_55);
	/*********Step 2:Enter upgrade mode *****/
	auc_i2c_write_buf[0] = FT_UPGRADE_55;
	auc_i2c_write_buf[1] = FT_UPGRADE_AA;
	do {
		i++;
		i_ret = ft5x06_i2c_write(client, auc_i2c_write_buf, 2);
		msleep(5);
	} while (i_ret <= 0 && i < 5);


	/*********Step 3:check READ-ID***********************/
	msleep(upgradeinfo.delay_readid);
	auc_i2c_write_buf[0] = 0x90;
	auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =
		0x00;
	ft5x06_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);


	if (reg_val[0] == upgradeinfo.upgrade_id_1
		&& reg_val[1] == upgradeinfo.upgrade_id_2) {
		dev_dbg(&client->dev, "[FTS] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",
			reg_val[0], reg_val[1]);
	} else {
		dev_dbg(&client->dev, "[FTS] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",
			reg_val[0], reg_val[1]);
		return -EIO;
	}

	auc_i2c_write_buf[0] = 0xcd;

	ft5x06_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 1);


	/*Step 4:erase app and panel paramenter area*/
	auc_i2c_write_buf[0] = 0x61;
	ft5x06_i2c_write(client, auc_i2c_write_buf, 1);	/*erase app area */
	msleep(FT_UPGRADE_EARSE_DELAY);
	/*erase panel parameter area */
	auc_i2c_write_buf[0] = 0x63;
	ft5x06_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(100);

	/*********Step 5:write firmware(FW) to ctpm flash*********/
	bt_ecc = 0;


	dw_lenth = dw_lenth - 8;
	packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
	packet_buf[0] = 0xbf;
	packet_buf[1] = 0x00;

	for (j = 0; j < packet_number; j++) {
		temp = j * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		lenght = FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (lenght >> 8);
		packet_buf[5] = (u8) lenght;

		for (i = 0; i < FTS_PACKET_LENGTH; i++) {
			packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}

		ft5x06_i2c_write(client, packet_buf, FTS_PACKET_LENGTH + 6);
		msleep(FTS_PACKET_LENGTH / 6 + 1);
	}

	if ((dw_lenth) % FTS_PACKET_LENGTH > 0) {
		temp = packet_number * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = (dw_lenth) % FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;

		for (i = 0; i < temp; i++) {
			packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}

		ft5x06_i2c_write(client, packet_buf, temp + 6);
		msleep(20);
	}


	/*send the last six byte */
	for (i = 0; i < 6; i++) {
		temp = 0x6ffa + i;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = 1;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;
		packet_buf[6] = pbt_buf[dw_lenth + i];
		bt_ecc ^= packet_buf[6];
		ft5x06_i2c_write(client, packet_buf, 7);
		msleep(20);
	}


	/*********Step 6: read out checksum***********************/
	/*send the opration head */
	auc_i2c_write_buf[0] = 0xcc;
	ft5x06_i2c_read(client, auc_i2c_write_buf, 1, reg_val, 1);
	if (reg_val[0] != bt_ecc) {
		dev_err(&client->dev, "[FTS]--ecc error! FW=%02x bt_ecc=%02x\n",
					reg_val[0],
					bt_ecc);
		return -EIO;
	}

	/*********Step 7: reset the new FW***********************/
	auc_i2c_write_buf[0] = 0x07;
	ft5x06_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(300);	/*make sure CTP startup normally */
	return 0;
}

/*sysfs debug*/

/*
*get firmware size

@firmware_name:firmware name
*note:the firmware default path is sdcard.
	if you want to change the dir, please modify by yourself.
*/
static int ft5x0x_GetFirmwareSize(char *firmware_name)
{
	struct file *pfile = NULL;
	struct inode *inode;
	unsigned long magic;
	off_t fsize = 0;
	char filepath[128];
	memset(filepath, 0, sizeof(filepath));

	sprintf(filepath, "/sdcard/%s", firmware_name);

	if (NULL == pfile)
		pfile = filp_open(filepath, O_RDONLY, 0);

	if (IS_ERR(pfile)) {
		pr_err("error occured while opening file %s.\n", filepath);
		return -EIO;
	}

	inode = pfile->f_dentry->d_inode;
	magic = inode->i_sb->s_magic;
	fsize = inode->i_size;
	filp_close(pfile, NULL);
	return fsize;
}



/*
*read firmware buf for .bin file.

@firmware_name: fireware name
@firmware_buf: data buf of fireware

note:the firmware default path is sdcard.
	if you want to change the dir, please modify by yourself.
*/
static int ft5x0x_ReadFirmware(char *firmware_name,
			       unsigned char *firmware_buf)
{
	struct file *pfile = NULL;
	struct inode *inode;
	unsigned long magic;
	off_t fsize;
	char filepath[128];
	loff_t pos;
	mm_segment_t old_fs;

	memset(filepath, 0, sizeof(filepath));
	sprintf(filepath, "/sdcard/%s", firmware_name);
	if (NULL == pfile)
		pfile = filp_open(filepath, O_RDONLY, 0);
	if (IS_ERR(pfile)) {
		pr_err("error occured while opening file %s.\n", filepath);
		return -EIO;
	}

	inode = pfile->f_dentry->d_inode;
	magic = inode->i_sb->s_magic;
	fsize = inode->i_size;
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	pos = 0;
	vfs_read(pfile, firmware_buf, fsize, &pos);
	filp_close(pfile, NULL);
	set_fs(old_fs);

	return 0;
}



/*
upgrade with *.bin file
*/

int fts_ctpm_fw_upgrade_with_app_file(struct i2c_client *client,
				       char *firmware_name)
{
	u8 *pbt_buf = NULL;
	int i_ret;
	int fwsize = ft5x0x_GetFirmwareSize(firmware_name);

	if (fwsize <= 0) {
		dev_err(&client->dev, "%s ERROR:Get firmware size failed\n",
					__func__);
		return -EIO;
	}

	if (fwsize < 8 || fwsize > 32 * 1024) {
		dev_dbg(&client->dev, "%s:FW length error\n", __func__);
		return -EIO;
	}

	/*=========FW upgrade========================*/
	pbt_buf = kmalloc(fwsize + 1, GFP_ATOMIC);

	if (ft5x0x_ReadFirmware(firmware_name, pbt_buf)) {
		dev_err(&client->dev, "%s() - ERROR: request_firmware failed\n",
					__func__);
		kfree(pbt_buf);
		return -EIO;
	}
	if ((pbt_buf[fwsize - 8] ^ pbt_buf[fwsize - 6]) == 0xFF
		&& (pbt_buf[fwsize - 7] ^ pbt_buf[fwsize - 5]) == 0xFF
		&& (pbt_buf[fwsize - 3] ^ pbt_buf[fwsize - 4]) == 0xFF) {
		/*call the upgrade function */
		i_ret = fts_ctpm_fw_upgrade(client, pbt_buf, fwsize);
		if (i_ret != 0)
			dev_dbg(&client->dev, "%s() - ERROR:[FTS] upgrade failed..\n",
						__func__);
		else {
#ifdef AUTO_CLB
			fts_ctpm_auto_clb(client);	/*start auto CLB*/
#endif
		 }
		kfree(pbt_buf);
	} else {
		dev_dbg(&client->dev, "%s:FW format error\n", __func__);
		kfree(pbt_buf);
		return -EIO;
	}

	return i_ret;
}

static ssize_t ft5x0x_tpfwver_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t num_read_chars = 0;
	u8 fwver = 0;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);

	mutex_lock(&g_device_mutex);

	if (ft5x0x_read_reg(client, FT5x0x_REG_FW_VER, &fwver) < 0)
		num_read_chars = snprintf(buf, PAGE_SIZE,
					"get tp fw version fail!\n");
	else
		num_read_chars = snprintf(buf, PAGE_SIZE, "%02X\n", fwver);

	mutex_unlock(&g_device_mutex);

	return num_read_chars;
}

static ssize_t ft5x0x_tpfwver_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	/*place holder for future use*/
	return -EPERM;
}



static ssize_t ft5x0x_tprwreg_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	/*place holder for future use*/
	return -EPERM;
}

static ssize_t ft5x0x_tprwreg_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	ssize_t num_read_chars = 0;
	int retval;
	long unsigned int wmreg = 0;
	u8 regaddr = 0xff, regvalue = 0xff;
	u8 valbuf[5] = {0};

	memset(valbuf, 0, sizeof(valbuf));
	mutex_lock(&g_device_mutex);
	num_read_chars = count - 1;

	if (num_read_chars != 2) {
		if (num_read_chars != 4) {
			pr_info("please input 2 or 4 character\n");
			goto error_return;
		}
	}

	memcpy(valbuf, buf, num_read_chars);
	retval = strict_strtoul(valbuf, 16, &wmreg);

	if (0 != retval) {
		dev_err(&client->dev, "%s() - ERROR: Could not convert the "\
						"given input to a number." \
						"The given input was: \"%s\"\n",
						__func__, buf);
		goto error_return;
	}

	if (2 == num_read_chars) {
		/*read register*/
		regaddr = wmreg;
		if (ft5x0x_read_reg(client, regaddr, &regvalue) < 0)
			dev_err(&client->dev, "Could not read the register(0x%02x)\n",
						regaddr);
		else
			pr_info("the register(0x%02x) is 0x%02x\n",
					regaddr, regvalue);
	} else {
		regaddr = wmreg >> 8;
		regvalue = wmreg;
		if (ft5x0x_write_reg(client, regaddr, regvalue) < 0)
			dev_err(&client->dev, "Could not write the register(0x%02x)\n",
							regaddr);
		else
			dev_err(&client->dev, "Write 0x%02x into register(0x%02x) successful\n",
							regvalue, regaddr);
	}

error_return:
	mutex_unlock(&g_device_mutex);

	return count;
}

static ssize_t ft5x0x_fwupdate_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	/* place holder for future use */
	return -EPERM;
}

/*upgrade from *.i*/
static ssize_t ft5x0x_fwupdate_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct ft5x0x_ts_data *data = NULL;
	u8 uc_host_fm_ver;
	int i_ret;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);

	data = (struct ft5x0x_ts_data *)i2c_get_clientdata(client);

	mutex_lock(&g_device_mutex);

	disable_irq(client->irq);
	i_ret = fts_ctpm_fw_upgrade_with_i_file(client);
	if (i_ret == 0) {
		msleep(300);
		uc_host_fm_ver = fts_ctpm_get_i_file_ver();
		pr_info("%s [FTS] upgrade to new version 0x%x\n", __func__,
					 uc_host_fm_ver);
	} else
		dev_err(&client->dev, "%s ERROR:[FTS] upgrade failed.\n",
					__func__);

	enable_irq(client->irq);
	mutex_unlock(&g_device_mutex);

	return count;
}

static ssize_t ft5x0x_fwupgradeapp_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	/*place holder for future use*/
	return -EPERM;
}


/*upgrade from app.bin*/
static ssize_t ft5x0x_fwupgradeapp_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	char fwname[128];
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);

	memset(fwname, 0, sizeof(fwname));
	sprintf(fwname, "%s", buf);
	fwname[count - 1] = '\0';

	mutex_lock(&g_device_mutex);
	disable_irq(client->irq);

	fts_ctpm_fw_upgrade_with_app_file(client, fwname);

	enable_irq(client->irq);
	mutex_unlock(&g_device_mutex);

	return count;
}

/*[PLATFORM]-Del-BEGIN by TCTNB.WPL, 2013/01/10, this FW upgrade file is from FAE, remove this function for compile error, refer to bug386588*/
#if 0
static int ft5x0x_read_rawdata(struct i2c_client *client, u16 rawdata[][FTS_RX_MAX],
			u8 tx, u8 rx)
{
	u8 i = 0, j = 0, k = 0;
	int err = 0;
	u8 regvalue = 0x00;
	u8 regaddr = 0x00;
	u16 dataval = 0x0000;
	u8 writebuf[2] = {0};
	u8 read_buffer[FTS_RX_MAX * 2];
	/*scan*/
	err = ft5x0x_read_reg(client, FTS_DEVICE_MODE_REG, &regvalue);
	if (err < 0) {
		return err;
	} else {
		regvalue |= 0x80;
		err = ft5x0x_write_reg(client, FTS_DEVICE_MODE_REG, regvalue);
		if (err < 0) {
			return err;
		} else {
			for(i=0; i<20; i++)
			{
				msleep(8);
				err = ft5x0x_read_reg(client, FTS_DEVICE_MODE_REG, 
							&regvalue);
				if (err < 0) {
					return err;
				} else {
					if (0 == (regvalue >> 7))
						break;
				}
			}
		}
	}

	/*get rawdata*/
	dev_dbg(&client->dev, "%s() - Reading raw data...\n", __func__);
	for(i=0; i<tx; i++)
	{
		memset(read_buffer, 0x00, (FTS_RX_MAX * 2));
		writebuf[0] = FTS_RAW_READ_REG;
		writebuf[1] = i;
		err = ft5x06_i2c_write(client, writebuf, 2);
		if (err < 0) {
			return err;
		}
		/* Read the data for this row */
		regaddr = FTS_RAW_BEGIN_REG;
		err = ft5x06_i2c_read(client, &regaddr, 1, read_buffer, rx*2);
		if (err < 0) {
			return err;
		}
		k = 0;
		for (j = 0; j < rx*2; j += 2)
        	{
			dataval  = read_buffer[j];
			dataval  = (dataval << 8);
			dataval |= read_buffer[j+1];
			rawdata[i][k] = dataval;
			k++;
        	}
	}

	return 0;
}
/*raw data show*/
static ssize_t ft5x0x_rawdata_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t num_read_chars = 0;
	u16 before_rawdata[FTS_TX_MAX][FTS_RX_MAX];
	u8 rx = 0, tx = 0;
	u8 i = 0, j = 0;
	int err = 0;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);

	mutex_lock(&g_device_mutex);
	/*entry factory*/
	err = ft5x0x_write_reg(client, FTS_DEVICE_MODE_REG, FTS_FACTORYMODE_VALUE);
	if (err < 0) {
		num_read_chars = sprintf(buf,
			"%s:rawdata show error!\n", __func__);
		goto RAW_ERROR;
	}

	/*get rx and tx num*/
	err = ft5x0x_read_reg(client, FTS_TXNUM_REG, &tx);
	if (err < 0) {
		num_read_chars = sprintf(buf,
				"%s:get tx error!\n", __func__);
		goto RAW_ERROR;
	}
	err = ft5x0x_read_reg(client, FTS_RXNUM_REG, &rx);
	if (err < 0) {
		num_read_chars = sprintf(buf,
			"%s:get rx error!\n", __func__);
		goto RAW_ERROR;
	}
	num_read_chars += sprintf(&(buf[num_read_chars]), 
			"tp channel: %u * %u\n", tx, rx);

	/*get rawdata*/
	err = ft5x0x_read_rawdata(client, before_rawdata, tx, rx);
	if (err < 0) {
		num_read_chars = sprintf(buf,
				"%s:rawdata show error!\n", __func__);
		goto RAW_ERROR;
	} else {
		for (i=0; i<tx; i++) {
			for (j=0; j<rx; j++) {
				num_read_chars += sprintf(&(buf[num_read_chars]),
						"%u ", before_rawdata[i][j]);
			}
			buf[num_read_chars-1] = '\n';
		}
	}
RAW_ERROR:
	/*enter work mode*/
	err = ft5x0x_write_reg(client, FTS_DEVICE_MODE_REG, FTS_WORKMODE_VALUE);
	if (err < 0)
		dev_err(&client->dev,
			"%s:enter work error!\n", __func__);
	msleep(100);
	mutex_unlock(&g_device_mutex);
	return num_read_chars;
}
#endif
/*[PLATFORM]-Del-END by TCTNB.WPL*/

static ssize_t ft5x0x_rawdata_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	/*place holder for future use*/
	return -EPERM;
}

/*[PLATFORM]-Del-BEGIN by TCTNB.WPL, 2013/01/10, this FW upgrade file is from FAE, remove this function for compile error, refer to bug386588*/
#if 0
/*diff data show. default voltage level is 2.*/
static ssize_t ft5x0x_diffdata_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t num_read_chars = 0;
	u16 before_rawdata[FTS_TX_MAX][FTS_RX_MAX];
	u16 after_rawdata[FTS_TX_MAX][FTS_RX_MAX];
	u8 orig_vol = 0x00;
	u8 rx = 0, tx = 0;
	u8 i = 0, j = 0;
	int err = 0;
	u8 regvalue = 0x00;
	u16 tmpdata = 0x0000;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);

	mutex_lock(&g_device_mutex);
	/*entry factory*/
	err = ft5x0x_write_reg(client, FTS_DEVICE_MODE_REG, FTS_FACTORYMODE_VALUE);
	if (err < 0) {
		num_read_chars = sprintf(buf,
			"%s:rawdata show error!\n", __func__);
		goto RAW_ERROR;
	}

	/*get rx and tx num*/
	err = ft5x0x_read_reg(client, FTS_TXNUM_REG, &tx);
	if (err < 0) {
		num_read_chars = sprintf(buf,
				"%s:get tx error!\n", __func__);
		goto RAW_ERROR;
	}
	err = ft5x0x_read_reg(client, FTS_RXNUM_REG, &rx);
	if (err < 0) {
		num_read_chars = sprintf(buf,
			"%s:get rx error!\n", __func__);
		goto RAW_ERROR;
	}
	num_read_chars += sprintf(&(buf[num_read_chars]), 
			"tp channel: %u * %u\n", tx, rx);
	/*get rawdata*/
	err = ft5x0x_read_rawdata(client, before_rawdata, tx, rx);
	if (err < 0) {
		num_read_chars = sprintf(buf,
				"%s:diffdata show error!\n", __func__);
		goto RAW_ERROR;
	} 

	/*get original voltage and change it to get new frame rawdata*/
	err = ft5x0x_read_reg(client, FTS_VOLTAGE_REG, &regvalue);
	if (err < 0) {
		num_read_chars = sprintf(buf,
				"%s:get original voltage error!\n", __func__);
		goto RAW_ERROR;
	} else 
		orig_vol = regvalue;
	
	if (orig_vol <= 1)
    		regvalue = orig_vol + 2;
    	else {			
    		if(orig_vol >= 4)
    			regvalue = 1;
    		else
    			regvalue = orig_vol - 2;
    	}
    	if (regvalue > 7)
    		regvalue = 7;
    	if (regvalue <= 0)
    		regvalue = 0;
		
	num_read_chars += sprintf(&(buf[num_read_chars]), 
			"original voltage: %u changed voltage:%u\n",
			orig_vol, regvalue);
	err = ft5x0x_write_reg(client, FTS_VOLTAGE_REG, regvalue);
	if (err < 0) {
		num_read_chars = sprintf(buf,
				"%s:set original voltage error!\n", __func__);
		goto RAW_ERROR;
	}
	
	/*get rawdata*/
	for (i=0; i<3; i++)
		err = ft5x0x_read_rawdata(client, after_rawdata, tx, rx);
	if (err < 0) {
		num_read_chars = sprintf(buf,
				"%s:diffdata show error!\n", __func__);
		goto RETURN_ORIG_VOLTAGE;
	} else {
		for (i=0; i<tx; i++) {
			for (j=0; j<rx; j++) {
				if (after_rawdata[i][j] > before_rawdata[i][j])
					tmpdata = after_rawdata[i][j] - before_rawdata[i][j];
				else
					tmpdata = before_rawdata[i][j] - after_rawdata[i][j];
				num_read_chars += sprintf(&(buf[num_read_chars]),
						"%u ", tmpdata);
			}
			buf[num_read_chars-1] = '\n';
		}
	}
	
RETURN_ORIG_VOLTAGE:
	err = ft5x0x_write_reg(client, FTS_VOLTAGE_REG, orig_vol);
	if (err < 0)
		dev_err(&client->dev,
			"%s:return original voltage error!\n", __func__);
RAW_ERROR:
	/*enter work mode*/
	err = ft5x0x_write_reg(client, FTS_DEVICE_MODE_REG, FTS_WORKMODE_VALUE);
	if (err < 0)
		dev_err(&client->dev,
			"%s:enter work error!\n", __func__);
	msleep(100);
	mutex_unlock(&g_device_mutex);
	
	return num_read_chars;
}
#endif
/*[PLATFORM]-Del-END by TCTNB.WPL*/

static ssize_t ft5x0x_diffdata_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	/*place holder for future use*/
	return -EPERM;
}

/*open & short param*/
#define VERYSMALL_TX_RX	1
#define SMALL_TX_RX			2
#define NORMAL_TX_RX		0
#define RAWDATA_BEYOND_VALUE		10000
#define DIFFERDATA_ABS_OPEN		10
#define DIFFERDATA_ABS_ABNORMAL	100
#define RAWDATA_SMALL_VALUE		5500 /*cross short*/
static u16 g_min_rawdata = 6000;
static u16 g_max_rawdata = 9500;
static u16 g_min_diffdata = 50;
static u16 g_max_diffdata = 550;
static u8 g_voltage_level = 2;	/*default*/

/*[PLATFORM]-Del-BEGIN by TCTNB.WPL, 2013/01/10, this FW upgrade file is from FAE, remove this function for compile error, refer to bug386588*/
#if 0
static void OnAnalyseRXOpenShort(u8 tx, u8 rx,
			u16 diffdata[][FTS_RX_MAX], u16 rawdata[][FTS_RX_MAX],
			char *buf, ssize_t *pnum_read_chars)
{
	u16 i = 0, j = 0, k = 0;
	u16 abnormalRX[FTS_RX_MAX];
	int sumrx = 0, avgrx = 0;
	u16 sumabnormal = 0;
	u16 index = 0;
	int sumrawbeyond = 0;
	int sumsmallraw = 0;
	int sumbeyond = 0;
	int iadd = 1;
	int smallindex = 0, smallrawvalue = RAWDATA_BEYOND_VALUE;
	for (i = 0; i < rx; i++) {
		sumrx = 0;
		for (j=0; j<tx; j++)
		{
			sumrx += diffdata[j][i];
		}
		avgrx = sumrx/tx;
		if(avgrx <= DIFFERDATA_ABS_OPEN)
			abnormalRX[i] = VERYSMALL_TX_RX;
		else if(avgrx <= DIFFERDATA_ABS_ABNORMAL)
			abnormalRX[i] = SMALL_TX_RX;
		else
			abnormalRX[i] = NORMAL_TX_RX;
	}
	for (i = 0; i < rx;) {
		sumabnormal = 0;
		if (NORMAL_TX_RX == abnormalRX[i])
			i++;
		else {
			
			if(i == (rx-1))
			{
				index = 0;
			}
			else
				index = i+1;
			for (j = index; j < rx; j++) {
				if (abnormalRX[j] > 0)
					sumabnormal++;
				else
					break;
			}
			
			if (VERYSMALL_TX_RX == abnormalRX[i])
			{				
				if(1 == sumabnormal)
				{
					for (j = 0; j < tx; j++) {
						if(diffdata[j][i] < DIFFERDATA_ABS_OPEN)
							sumbeyond++;
					}
					if (sumbeyond > (tx*4/5)) {
						*pnum_read_chars += sprintf(&(buf[*pnum_read_chars]),
							"short circuit: Rx%d & GND\n", (i + 1));
						i += 2;
					}	
					else
						i++;
				}
				else if (sumabnormal > 1) {
					for (k=0; k<sumabnormal; k++) {
						if (abnormalRX[i] != VERYSMALL_TX_RX)
							break;
						for (j=0; j<tx; j++) {
    							if (rawdata[j][i] >= RAWDATA_BEYOND_VALUE)
    								sumbeyond++;
    						}
    						if (sumbeyond > (rx*4/5)) {
							*pnum_read_chars += sprintf(&(buf[*pnum_read_chars]),
								"Open circuit: RX%d\n", (i + 1));
    						}	
    						i++;
					}
				}
				else	
				{
					sumbeyond = 0;
					for(j=0; j<tx; j++)
					{
						if(rawdata[j][i] >= RAWDATA_BEYOND_VALUE)
							sumbeyond++;
						if (rawdata[j][i] <= RAWDATA_SMALL_VALUE) {
							sumsmallraw++;
							if (rawdata[j][i] < smallrawvalue) {
								smallindex = j;
								smallrawvalue = rawdata[j][i];
							}
						}
					}
					if (sumbeyond > (tx*4/5))
						*pnum_read_chars += sprintf(&(buf[*pnum_read_chars]),
								"Open circuit: RX%d\n", (i + 1));
					if (sumsmallraw > 0) {
						//ÅÐ¶Ïcross short
						sumbeyond = 0;
						for (j=0; j<rx; j++) {
							if(diffdata[smallindex][j] < DIFFERDATA_ABS_OPEN)
								sumbeyond++;
						}
						if (sumbeyond > (rx*4/5))
							*pnum_read_chars += sprintf(&(buf[*pnum_read_chars]),
								"cross short circuit: Rx%d & Tx%d\n", (i + 1),
								(smallindex+1));
					}
					i++;
				}					
			}
			else
			{
				for(j=i+1; j<(i+1+sumabnormal); j++) {
					
					for(k=0; k<tx; k++) {
						if (diffdata[k][j] > DIFFERDATA_ABS_OPEN)
							sumbeyond++;
						if (rawdata[k][j] > RAWDATA_BEYOND_VALUE)
							sumrawbeyond++;
					}
					if (sumbeyond > (tx*4/5))
						iadd++;
					else {
						if (sumrawbeyond > (tx*4/5))
							break;
						else
							iadd++;
					}
				}
				if (iadd > 1) {
					*pnum_read_chars += sprintf(&(buf[*pnum_read_chars]),
						"Short circuit:RX%d", (i+1));
					for(j=0; j < (iadd-1); j++)
						*pnum_read_chars += sprintf(&(buf[*pnum_read_chars]),
							 " & RX%d", (i + 2 + j));
					buf[*pnum_read_chars-1] ='\n';
				}
				i += iadd;
			}
		}			
	}

}

static  void OnAnalyseTXOpenShort(u8 tx, u8 rx,
			u16 diffdata[][FTS_RX_MAX],
			u16 rawdata[][FTS_RX_MAX],
			char *buf, ssize_t *pnum_read_chars)
{
	u16 i = 0, j = 0, k = 0;
	u16 abnormalTX[FTS_TX_MAX];
	int sumtx = 0, avgtx = 0;
	u16 sumabnormal = 0;
	u16 index = 0;
	int sumrawbeyond = 0;
	int sumbeyond = 0;
	int iadd = 1;

	for (i=0; i < tx; i++) {
		sumtx = 0;
		for (j=0; j<rx; j++)
			sumtx += diffdata[i][j];
		avgtx = sumtx/rx;
		if(avgtx <= DIFFERDATA_ABS_OPEN)
			abnormalTX[i] = VERYSMALL_TX_RX;
		else if(avgtx <= DIFFERDATA_ABS_ABNORMAL)
			abnormalTX[i] = SMALL_TX_RX;
		else
			abnormalTX[i] = NORMAL_TX_RX;
	}
	for (i=0; i<tx; ) {
		sumabnormal = 0;
		if (NORMAL_TX_RX == abnormalTX[i])
			i++;
		else
		{
			if (i==(tx-1))
				index = 0;
			else
				index = i+1;
			for (j=index; j<tx; j++) {
				if(abnormalTX[j] > 0)
					sumabnormal++;
				else
					break;
			}
			
			if (VERYSMALL_TX_RX == abnormalTX[i]) {				
				if(1 == sumabnormal) {
					for (j=0; j < rx; j++) {
						if (diffdata[index][j] > DIFFERDATA_ABS_OPEN)
							sumbeyond++;
					}
					if (sumbeyond > (rx*4/5)) {
						*pnum_read_chars += sprintf(&(buf[*pnum_read_chars]),
							 "short circuit: Tx%d & GND\n", (i + 2 + j));
						i += 2;
					}	
					else
						i++;
				}
				else if (sumabnormal > 1) {
					for (k=0; k<sumabnormal; k++) {
						for(j=0; j<rx; j++) {
    							if(rawdata[i][j] >= RAWDATA_BEYOND_VALUE)
    								sumbeyond++;
    						}
    						if (sumbeyond > (rx*4/5))
							*pnum_read_chars += sprintf(&(buf[*pnum_read_chars]),
							 	"Open circuit: Tx%d\n", (i + 1));
    						i++;
					}
				}
				else {
					for(j=0; j<rx; j++)
					{
						if(rawdata[i][j] >= RAWDATA_BEYOND_VALUE)
							sumbeyond++;
					}
					if(sumbeyond > (rx*4/5))
						*pnum_read_chars += sprintf(&(buf[*pnum_read_chars]),
							 	"Open circuit: Tx%d\n", (i + 1));
					i++;
				}					
			}
			else
			{
				for (j=i+1; j < (i+1+sumabnormal); j++) {
					for(k=0; k<rx; k++) {
						if(diffdata[j][k] > DIFFERDATA_ABS_OPEN)
							sumbeyond++;
						if(rawdata[j][k] > RAWDATA_BEYOND_VALUE)
							sumrawbeyond++;
					}
					if(sumbeyond > (rx*4/5))					
						break;
					else {
						if(sumrawbeyond > (rx*4/5))
							break;
						else
							iadd++;
					}
				}
				if (iadd > 1) {
					*pnum_read_chars += sprintf(&(buf[*pnum_read_chars]),
						"Short circuit:TX%d", (i+1));
					for(j=0; j < (iadd-1); j++)
						*pnum_read_chars += sprintf(&(buf[*pnum_read_chars]),
							 " & TX%d", (i + 2 + j));
					buf[*pnum_read_chars-1] = '\n';
				}
				i += iadd;
			}
		}			
	}
}
/*open short show*/
static ssize_t ft5x0x_openshort_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t num_read_chars = 0;
	u16 rawdata[FTS_TX_MAX][FTS_RX_MAX];
	u16 diffdata[FTS_TX_MAX][FTS_RX_MAX];
	u8 orig_vol = 0x00;
	u8 rx = 0, tx = 0;
	u8 i = 0, j = 0;
	int err = 0;
	u8 regvalue = 0x00;
	u8 bpass = 1;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);

	mutex_lock(&g_device_mutex);
	/*entry factory*/
	err = ft5x0x_write_reg(client, FTS_DEVICE_MODE_REG, FTS_FACTORYMODE_VALUE);
	if (err < 0) {
		num_read_chars = sprintf(buf,
			"%s:rawdata show error!\n", __func__);
		goto RAW_ERROR;
	}

	/*get rx and tx num*/
	err = ft5x0x_read_reg(client, FTS_TXNUM_REG, &tx);
	if (err < 0) {
		num_read_chars = sprintf(buf,
				"%s:get tx error!\n", __func__);
		goto RAW_ERROR;
	}
	err = ft5x0x_read_reg(client, FTS_RXNUM_REG, &rx);
	if (err < 0) {
		num_read_chars = sprintf(buf,
			"%s:get rx error!\n", __func__);
		goto RAW_ERROR;
	}
	num_read_chars += sprintf(&(buf[num_read_chars]), 
			"tp channel: %u * %u\n", tx, rx);
	/*get rawdata*/
	err = ft5x0x_read_rawdata(client, rawdata, tx, rx);
	if (err < 0) {
		num_read_chars = sprintf(buf,
				"%s:diffdata show error!\n", __func__);
		goto RAW_ERROR;
	} 

	/*get original voltage and change it to get new frame rawdata*/
	err = ft5x0x_read_reg(client, FTS_VOLTAGE_REG, &regvalue);
	if (err < 0) {
		num_read_chars = sprintf(buf,
				"%s:get original voltage error!\n", __func__);
		goto RAW_ERROR;
	} else 
		orig_vol = regvalue;

	if (g_voltage_level <= 2) {
		if (orig_vol <= 1)
	    		regvalue = orig_vol + g_voltage_level;
	    	else {			
	    		if(orig_vol >= 4)
	    			regvalue = 1;
	    		else
	    			regvalue = orig_vol - g_voltage_level;
	    	}
	} else if (3 == g_voltage_level) {
		if (orig_vol <= 2)
			regvalue = orig_vol + g_voltage_level;
		else
			regvalue = orig_vol - g_voltage_level;
	} else {
		if(orig_vol <= 3)
			regvalue = orig_vol + g_voltage_level;
		else
			regvalue = orig_vol - g_voltage_level;
	}
    	if (regvalue > 7)
    		regvalue = 7;
    	if (regvalue <= 0)
    		regvalue = 0;
		
	num_read_chars += sprintf(&(buf[num_read_chars]), 
			"original voltage: %u changed voltage:%u\n",
			orig_vol, regvalue);
	err = ft5x0x_write_reg(client, FTS_VOLTAGE_REG, regvalue);
	if (err < 0) {
		num_read_chars = sprintf(buf,
				"%s:set original voltage error!\n", __func__);
		goto RAW_ERROR;
	}
	
	/*get rawdata*/
	for (i=0; i<3; i++)
		err = ft5x0x_read_rawdata(client, diffdata, tx, rx);
	if (err < 0) {
		num_read_chars = sprintf(buf,
				"%s:diffdata show error!\n", __func__);
		goto RETURN_ORIG_VOLTAGE;
	} else {
		for (i=0; i<tx; i++) {
			for (j=0; j<rx; j++) {
				if (rawdata[i][j] > diffdata[i][j])
					diffdata[i][j] = rawdata[i][j] - diffdata[i][j];
				else
					diffdata[i][j] = diffdata[i][j] - rawdata[i][j];
				if (rawdata[i][j] > g_max_rawdata ||
					rawdata[i][j] < g_min_rawdata)
					bpass = 0;
				if (diffdata[i][j] > g_max_diffdata ||
					diffdata[i][j] < g_min_diffdata)
					bpass = 0;
			}
		}
	}

	if (0 == bpass) {
		OnAnalyseRXOpenShort(tx, rx, diffdata, rawdata, buf, &num_read_chars);
		OnAnalyseTXOpenShort(tx, rx, diffdata, rawdata, buf, &num_read_chars);
	} else
		num_read_chars = sprintf(buf,
				"Good. None open and short.\n");
RETURN_ORIG_VOLTAGE:
	err = ft5x0x_write_reg(client, FTS_VOLTAGE_REG, orig_vol);
	if (err < 0)
		dev_err(&client->dev,
			"%s:return original voltage error!\n", __func__);
RAW_ERROR:
	/*enter work mode*/
	err = ft5x0x_write_reg(client, FTS_DEVICE_MODE_REG, FTS_WORKMODE_VALUE);
	if (err < 0)
		dev_err(&client->dev,
			"%s:enter work error!\n", __func__);
	msleep(100);
	mutex_unlock(&g_device_mutex);
	
	return num_read_chars;
}
#endif
/*[PLATFORM]-Del-END by TCTNB.WPL*/

static ssize_t ft5x0x_openshort_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	/*place holder for future use*/
	return -EPERM;
}

static ssize_t ft5x0x_setrawrange_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t num_read_chars = 0;

	mutex_lock(&g_device_mutex);
	num_read_chars = sprintf(buf,
				"min rawdata:%d max rawdata:%d\n",
				g_min_rawdata, g_max_rawdata);
	
	mutex_unlock(&g_device_mutex);
	
	return num_read_chars;
}

static ssize_t ft5x0x_setrawrange_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	ssize_t num_read_chars = 0;
	int retval;
	long unsigned int wmreg = 0;
	u8 valbuf[12] = {0};

	memset(valbuf, 0, sizeof(valbuf));
	mutex_lock(&g_device_mutex);
	num_read_chars = count - 1;

	if (num_read_chars != 8) {
		dev_err(&client->dev, "%s:please input 1 character\n",
				__func__);
		goto error_return;
	}

	memcpy(valbuf, buf, num_read_chars);
	retval = strict_strtoul(valbuf, 16, &wmreg);

	if (0 != retval) {
		dev_err(&client->dev, "%s() - ERROR: Could not convert the "\
						"given input to a number." \
						"The given input was: \"%s\"\n",
						__func__, buf);
		goto error_return;
	}
	g_min_rawdata = wmreg >> 16;
	g_max_rawdata = wmreg;
error_return:
	mutex_unlock(&g_device_mutex);

	return count;
}
static ssize_t ft5x0x_setdiffrange_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t num_read_chars = 0;
	mutex_lock(&g_device_mutex);
	num_read_chars = sprintf(buf,
				"min diffdata:%d max diffdata:%d\n",
				g_min_diffdata, g_max_diffdata);
	mutex_unlock(&g_device_mutex);
	
	return num_read_chars;
}

static ssize_t ft5x0x_setdiffrange_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	ssize_t num_read_chars = 0;
	int retval;
	long unsigned int wmreg = 0;
	u8 valbuf[12] = {0};

	memset(valbuf, 0, sizeof(valbuf));
	mutex_lock(&g_device_mutex);
	num_read_chars = count - 1;

	if (num_read_chars != 8) {
		dev_err(&client->dev, "%s:please input 1 character\n",
				__func__);
		goto error_return;
	}

	memcpy(valbuf, buf, num_read_chars);
	retval = strict_strtoul(valbuf, 16, &wmreg);

	if (0 != retval) {
		dev_err(&client->dev, "%s() - ERROR: Could not convert the "\
						"given input to a number." \
						"The given input was: \"%s\"\n",
						__func__, buf);
		goto error_return;
	}
	g_min_diffdata = wmreg >> 16;
	g_max_diffdata = wmreg;
error_return:
	mutex_unlock(&g_device_mutex);

	return count;

}

static ssize_t ft5x0x_setvoltagelevel_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t num_read_chars = 0;
	mutex_lock(&g_device_mutex);
	num_read_chars = sprintf(buf, "voltage level:%d\n", g_voltage_level);
	mutex_unlock(&g_device_mutex);
	
	return num_read_chars;
}

static ssize_t ft5x0x_setvoltagelevel_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	ssize_t num_read_chars = 0;
	int retval;
	long unsigned int wmreg = 0;
	u8 valbuf[5] = {0};

	memset(valbuf, 0, sizeof(valbuf));
	mutex_lock(&g_device_mutex);
	num_read_chars = count - 1;

	if (num_read_chars != 1) {
		dev_err(&client->dev, "%s:please input 1 character\n",
				__func__);
		goto error_return;
	}

	memcpy(valbuf, buf, num_read_chars);
	retval = strict_strtoul(valbuf, 16, &wmreg);

	if (0 != retval) {
		dev_err(&client->dev, "%s() - ERROR: Could not convert the "\
						"given input to a number." \
						"The given input was: \"%s\"\n",
						__func__, buf);
		goto error_return;
	}
	g_voltage_level = wmreg;
	
error_return:
	mutex_unlock(&g_device_mutex);

	return count;
}

static ssize_t ft5x0x_checkautoclb_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	ssize_t num_read_chars = 0;
	u8 reg_val[2] = {0};
	u8 auc_i2c_write_buf[10];
	int i_ret = 0;
	int i = 0;
	struct Upgrade_Info upgradeinfo;
	
	mutex_lock(&g_device_mutex);

	fts_get_upgrade_info(&upgradeinfo);

	/*********Step 1:Reset  CTPM *****/
	/*write 0xaa to register 0xfc */
	ft5x0x_write_reg(client, 0xfc, FT_UPGRADE_AA);
	msleep(upgradeinfo.delay_aa);

	/*write 0x55 to register 0xfc */
	ft5x0x_write_reg(client, 0xfc, FT_UPGRADE_55);

	msleep(upgradeinfo.delay_55);
	/*********Step 2:Enter upgrade mode *****/
	auc_i2c_write_buf[0] = FT_UPGRADE_55;
	auc_i2c_write_buf[1] = FT_UPGRADE_AA;
	do {
		i++;
		i_ret = ft5x06_i2c_write(client, auc_i2c_write_buf, 2);
		msleep(5);
	} while (i_ret <= 0 && i < 5);


	/*********Step 3:check READ-ID***********************/
	msleep(upgradeinfo.delay_readid);
	auc_i2c_write_buf[0] = 0x90;
	auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =
		0x00;
	ft5x06_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 2);

	if (reg_val[0] == upgradeinfo.upgrade_id_1
		&& reg_val[1] == upgradeinfo.upgrade_id_2) {
		num_read_chars = sprintf(buf, "%s:[FTS]CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",
			__func__, reg_val[0], reg_val[1]);
		goto EXIT_CHECK;
	} else {
		num_read_chars = sprintf(buf, "%s:[FTS] CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",
			__func__, reg_val[0], reg_val[1]);
		goto EXIT_CHECK;
	}

	/*********Step 4:read clb area start addr:0x7C00***********************/
	auc_i2c_write_buf[0] = 0x03;
	auc_i2c_write_buf[1] = 0x00;//H
	auc_i2c_write_buf[2] = 0x7C;//M
	auc_i2c_write_buf[3] = 0x00;//L
	i_ret = ft5x06_i2c_read(client, auc_i2c_write_buf, 4, reg_val, 1);
	if (i_ret < 0) {
		num_read_chars = sprintf(buf, "check clb failed!\r\n");
		goto EXIT_CHECK;
	} else {
		if(0xFF == reg_val[0])
			num_read_chars = sprintf(buf, "None calibration.\r\n");
		else
			num_read_chars = sprintf(buf, "Calibrated!\r\n");
	}
	
	/**********Step 5:reset FW to return work mode**************/
	auc_i2c_write_buf[0] = 0x07;
	ft5x06_i2c_write(client, auc_i2c_write_buf, 1);
	msleep(200);
EXIT_CHECK:	
	mutex_unlock(&g_device_mutex);
	
	return num_read_chars;
}

static ssize_t ft5x0x_checkautoclb_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	/*place holder for future use*/
	return -EPERM;
}


/*sysfs */
/*get the fw version
*example:cat ftstpfwver
*/
static DEVICE_ATTR(ftstpfwver, S_IRUGO | S_IWUSR, ft5x0x_tpfwver_show,
			ft5x0x_tpfwver_store);

/*upgrade from *.i
*example: echo 1 > ftsfwupdate
*/
static DEVICE_ATTR(ftsfwupdate, S_IRUGO | S_IWUSR, ft5x0x_fwupdate_show,
			ft5x0x_fwupdate_store);

/*read and write register
*read example: echo 88 > ftstprwreg ---read register 0x88
*write example:echo 8807 > ftstprwreg ---write 0x07 into register 0x88
*
*note:the number of input must be 2 or 4.if it not enough,please fill in the 0.
*/
static DEVICE_ATTR(ftstprwreg, S_IRUGO | S_IWUSR, ft5x0x_tprwreg_show,
			ft5x0x_tprwreg_store);


/*upgrade from app.bin
*example:echo "*_app.bin" > ftsfwupgradeapp
*/
static DEVICE_ATTR(ftsfwupgradeapp, S_IRUGO | S_IWUSR, ft5x0x_fwupgradeapp_show,
			ft5x0x_fwupgradeapp_store);

/*[PLATFORM]-Del-BEGIN by TCTNB.WPL, 2013/01/10, this FW upgrade file is from FAE, remove this function for compile error, refer to bug386588*/
#if 0
/*show a frame rawdata
*example:cat ftsrawdatashow
*/
static DEVICE_ATTR(ftsrawdatashow, S_IRUGO | S_IWUSR, ft5x0x_rawdata_show,
			ft5x0x_rawdata_store);

/*show a frame diffdata
*example:cat ftsdiffdatashow
*/
static DEVICE_ATTR(ftsdiffdatashow, S_IRUGO | S_IWUSR, ft5x0x_diffdata_show,
			ft5x0x_diffdata_store);

/*if tp is bad, then show where is opened or shorted.
*example:cat ftsopenshortshow
*/
static DEVICE_ATTR(ftsopenshortshow, S_IRUGO | S_IWUSR, ft5x0x_openshort_show,
			ft5x0x_openshort_store);

#endif
/*[PLATFORM]-Del-END by TCTNB.WPL*/

static DEVICE_ATTR(ftsrawdatashow, S_IRUGO | S_IWUSR, NULL,
			ft5x0x_rawdata_store);


static DEVICE_ATTR(ftsdiffdatashow, S_IRUGO | S_IWUSR, NULL,
			ft5x0x_diffdata_store);


static DEVICE_ATTR(ftsopenshortshow, S_IRUGO | S_IWUSR, NULL,
			ft5x0x_openshort_store);

/*set range of rawdata
*example:echo 60009999 > ftssetrawrangeshow(range is 6000-9999)
*		cat ftssetrawrangeshow
*/
static DEVICE_ATTR(ftssetrawrangeshow, S_IRUGO | S_IWUSR, ft5x0x_setrawrange_show,
			ft5x0x_setrawrange_store);

/*set range of diffdata
*example:echo 00501000 > ftssetdiffrangeshow(range is 50-1000)
*		cat ftssetdiffrangeshow
*/
static DEVICE_ATTR(ftssetdiffrangeshow, S_IRUGO | S_IWUSR, ft5x0x_setdiffrange_show,
			ft5x0x_setdiffrange_store);

/*set range of diffdata
*example:echo 2 > ftssetvoltagelevelshow
*		cat ftssetvoltagelevelshow
*/
static DEVICE_ATTR(ftssetvoltagelevelshow, S_IRUGO | S_IWUSR,
			ft5x0x_setvoltagelevel_show,
			ft5x0x_setvoltagelevel_store);

/*set range of diffdata
*example:echo 2 > ftssetvoltagelevelshow
*		cat ftssetvoltagelevelshow
*/
static DEVICE_ATTR(ftscheckautoclbshow, S_IRUGO | S_IWUSR,
			ft5x0x_checkautoclb_show,
			ft5x0x_checkautoclb_store);

/*add your attr in here*/
static struct attribute *ft5x0x_attributes[] = {
	&dev_attr_ftstpfwver.attr,
	&dev_attr_ftsfwupdate.attr,
	&dev_attr_ftstprwreg.attr,
	&dev_attr_ftsfwupgradeapp.attr,
	&dev_attr_ftsrawdatashow.attr,
	&dev_attr_ftsdiffdatashow.attr,
	&dev_attr_ftsopenshortshow.attr,
	&dev_attr_ftssetrawrangeshow.attr,
	&dev_attr_ftssetdiffrangeshow.attr,
	&dev_attr_ftssetvoltagelevelshow.attr,
	&dev_attr_ftscheckautoclbshow.attr,
	NULL
};

static struct attribute_group ft5x0x_attribute_group = {
	.attrs = ft5x0x_attributes
};

/*create sysfs for debug*/
int ft5x0x_create_sysfs(struct i2c_client *client)
{
	int err;
	err = sysfs_create_group(&client->dev.kobj, &ft5x0x_attribute_group);
	if (0 != err) {
		dev_err(&client->dev,
					 "%s() - ERROR: sysfs_create_group() failed.\n",
					 __func__);
		sysfs_remove_group(&client->dev.kobj, &ft5x0x_attribute_group);
		return -EIO;
	} else {
		mutex_init(&g_device_mutex);
		pr_info("ft5x0x:%s() - sysfs_create_group() succeeded.\n",
				__func__);
	}
	return err;
}

void ft5x0x_release_mutex(void)
{
	mutex_destroy(&g_device_mutex);
}
