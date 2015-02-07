/*
NOTE:
The modification is appended to initialization of image sensor. 
After sensor initialization, use the function
bool otp_update_wb(unsigned char golden_rg, unsigned char golden_bg),
then the calibration of AWB will be applied. 
After finishing the OTP written, we will provide you the golden_rg and golden_bg settings.
*/
/******************************************************************************/
/*                                                            Date:12/10/2012 */
/*                                PRESENTATION                                */
/*                                                                            */
/*       Copyright 2012 TCL Communication Technology Holdings Limited.        */
/*                                                                            */
/* This material is company confidential, cannot be reproduced in any form    */
/* without the written permission of TCL Communication Technology Holdings    */
/* Limited.                                                                   */
/*                                                                            */
/* -------------------------------------------------------------------------- */
/*  Author :  Jin.Hu                                                     */
/*  Email  :  Jin.Hu@tcl-mobile.com                                      */
/*  Role   :                                                                  */
/*  Reference documents :                                                     */
/* -------------------------------------------------------------------------- */
/*  Comments :                                                                */
/*  File     :                                                                */
/*  Labels   :                                                                */
/* -------------------------------------------------------------------------- */
/* ========================================================================== */
/*     Modifications on Features list / Changes Request / Problems Report     */
/* -------------------------------------------------------------------------- */
/*    date   |        author        |         Key          |     comment      */
/* ----------|----------------------|----------------------|----------------- */
/* 12/10/2012|Jin.Hu	            |PR-371680             |OTP deal  for     */
/*           |                      |                      |preview blue      */
/* ----------|----------------------|----------------------|----------------- */
/******************************************************************************/
#include <linux/i2c.h>
#include <linux/delay.h>
#include "msm_sensor.h"

#if 0 //[移植Truly的方案，保留这里是为了方便后期可能出现的调试]
//#define SUPPORT_FLOATING

#define OTP_DATA_ADDR         0x3D00
#define OTP_LOAD_ADDR         0x3D21

#define OTP_WB_GROUP_ADDR     0x3D05
#define OTP_WB_GROUP_SIZE     9

#define GAIN_RH_ADDR          0x5186
#define GAIN_RL_ADDR          0x5187
#define GAIN_GH_ADDR          0x5188
#define GAIN_GL_ADDR          0x5189
#define GAIN_BH_ADDR          0x518A
#define GAIN_BL_ADDR          0x518B

#define GAIN_DEFAULT_VALUE    0x0400 // 1x gain

#define OTP_MID               0x06//0x02

static int my_global_otp_addr;
u8 R_flag, G_flag, B_flag;


static struct msm_sensor_ctrl_t *otp_local_s_ctrl = NULL;
#if 0
#define OTP_LOCAL_I2C_ADDR	(otp_local_s_ctrl->sensor_i2c_addr>> 1)
#else
#define OTP_LOCAL_I2C_ADDR	(otp_local_s_ctrl->sensor_i2c_client->client->addr >> 1)
#endif
#define OTP_LOCAL_I2C_ADAPTER	(otp_local_s_ctrl->sensor_i2c_client->client->adapter)


// R/G and B/G of current camera module
unsigned short rg_ratio = 0;
unsigned short bg_ratio = 0;

static int ov5647_i2c_rxdata(unsigned short saddr,
		unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
		{
			.addr  = saddr,
			.flags = 0,
			.len   = 2,
			.buf   = rxdata,
		},
		{
			.addr  = saddr,
			.flags = I2C_M_RD,
			.len   = 1,
			.buf   = rxdata,
		},
	};
	if (i2c_transfer(OTP_LOCAL_I2C_ADAPTER, msgs, 2) < 0) {
		CDBG("ov5647_i2c_rxdata faild 0x%x\n", saddr);
		return -EIO;
	}
	return 0;
}

static int32_t ov5647_i2c_txdata(unsigned short saddr,
		unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
			.addr = saddr,
			.flags = 0,
			.len = length,
			.buf = txdata,
		},
	};
	if (i2c_transfer(OTP_LOCAL_I2C_ADAPTER, msg, 1) < 0) {
		CDBG("ov5647_i2c_txdata faild 0x%x\n", saddr);
		return -EIO;
	}

	return 0;
}

int32_t ov5647_i2c_read(unsigned short raddr,
		unsigned short *rdata)
{
	int32_t rc = 0;
	unsigned char buf[2];

	if (!rdata)
		return -EIO;
	CDBG("%s:saddr:0x%x raddr:0x%x data:0x%x",__func__, OTP_LOCAL_I2C_ADDR, raddr, *rdata);
	memset(buf, 0, sizeof(buf));
	buf[0] = (raddr & 0xFF00) >> 8;
	buf[1] = (raddr & 0x00FF);
	rc = ov5647_i2c_rxdata(OTP_LOCAL_I2C_ADDR, buf, 1);
	if (rc < 0) {
		//CDBG("ov5647_i2c_read 0x%x failed!\n", raddr);
		printk(KERN_EMERG "--------ov5647_i2c_read 0x%x failed!----------\n", raddr);
		return rc;
	}
	*rdata = buf[0];
	//CDBG("ov5647_i2c_read 0x%x val = 0x%x!\n", raddr, *rdata);
	//printk(KERN_EMERG "--------ov5647_i2c_read SUCCEED!---------\n");
	
	return rc;
}

int32_t ov5647_i2c_write_b_sensor(unsigned short waddr, uint8_t bdata)
{
	int32_t rc = -EFAULT;
	unsigned char buf[3];
	memset(buf, 0, sizeof(buf));
	buf[0] = (waddr & 0xFF00) >> 8;
	buf[1] = (waddr & 0x00FF);
	buf[2] = bdata;
	rc = ov5647_i2c_txdata(OTP_LOCAL_I2C_ADDR, buf, 3);
	if (rc < 0) {
#if 0
        CDBG("i2c_write_b failed, addr = 0x%x, val = 0x%x!\n",
				waddr, bdata);
#endif        
		printk(KERN_EMERG "------------i2c_write_b failed, addr = 0x%x, val = 0x%x!---------\n",
				waddr, bdata);
	}
	//printk(KERN_EMERG "------------i2c_write_b SUCCEED---------\n");
    
	return rc;
}



// Enable OTP read function
static void otp_read_enable(void)
{
	ov5647_i2c_write_b_sensor(OTP_LOAD_ADDR, 0x01);
	mdelay(10); // sleep > 10ms
}

// Disable OTP read function
static void otp_read_disable(void)
{
	ov5647_i2c_write_b_sensor(OTP_LOAD_ADDR, 0x00);
}

static void otp_read(unsigned short otp_addr, unsigned short* otp_data)
{
	otp_read_enable();
	ov5647_i2c_read(otp_addr,otp_data);
	otp_read_disable();
}

/*******************************************************************************
* Function    :  otp_clear
* Description :  Clear OTP buffer 
* Parameters  :  none
* Return      :  none
*******************************************************************************/	
static void otp_clear(void)
{
	// After read/write operation, the OTP buffer should be cleared to avoid accident write
	unsigned char i;
	for (i=0; i<32; i++) 
	{
		ov5647_i2c_write_b_sensor(OTP_DATA_ADDR+i, 0x00);
	}
}

/*******************************************************************************
* Function    :  otp_check_wb_group
* Description :  Check OTP Space Availability
* Parameters  :  [in] index : index of otp group (0, 1, 2)
* Return      :  0, group index is empty
                 1, group index has invalid data
                 2, group index has valid data
                -1, group index error
*******************************************************************************/	
static signed char otp_check_wb_group(unsigned char index)
{   
	unsigned short otp_addr = OTP_WB_GROUP_ADDR + index * OTP_WB_GROUP_SIZE;
	unsigned short  flag;

    if (index > 2)
	{
		printk("\n\nOTP input wb group index %d error\n", index);
		return -1;
	}
		
	otp_read(otp_addr, &flag);
	otp_clear();

	// Check all bytes of a group. If all bytes are '0', then the group is empty. 
	// Check from group 1 to group 2, then group 3.
	if (!flag)
	{
		printk("wb group %d is empty\n", index);
		return 0;
	}
	else if ((!(flag&0x80)) && (flag&0x7f))
	{
		printk("wb group %d has valid data\n", index);
		return 2;
	}
	else
	{
		printk("wb group %d has invalid data\n", index);
		return 1;
	}
}

/*******************************************************************************
* Function    :  otp_read_wb_group
* Description :  Read group value and store it in OTP Struct 
* Parameters  :  [in] index : index of otp group (0, 1, 2)
* Return      :  group index (0, 1, 2)
                 -1, error
*******************************************************************************/	
static signed char otp_read_wb_group(signed char index)
{
	unsigned short otp_addr;
	unsigned short  mid;

	if (index == -1)
	{
        printk("index == -1\n");
		// Check first OTP with valid data
		for (index=0; index<3; index++)
		{
			if (otp_check_wb_group(index) == 2)
			{
				printk("read wb from group %d", index);
				break;
			}
		}

		if (index > 2)
		{
			printk("no group has valid data\n");
			return -1;
		}
	}
	else
	{
        printk("index != -1\n");
        printk("read wb from group %d", index);
		if (otp_check_wb_group(index) != 2)
		{
			printk("read wb from group %d failed\n", index);
			return -1;
		}
	}
	otp_addr = OTP_WB_GROUP_ADDR + index * OTP_WB_GROUP_SIZE;
	my_global_otp_addr = otp_addr;
	otp_read(otp_addr, &mid);
	if ((mid&0x7f) != OTP_MID)
	{
        
        printk(KERN_EMERG "!!!!!!(mid&0x7f) != OTP_MID (mid&0x7f)=[0x%x], mid=[0x%x]\n", (mid&0x7f), mid);
		return -1;
	}
	otp_read(otp_addr+2, &rg_ratio);
	otp_read(otp_addr+3, &bg_ratio);
    otp_clear();

    
    printk(KERN_EMERG "~~~~~~~MID_addr = [0x%x], MID = [0x%x]\n", otp_addr  , mid&0x7f);
    printk(KERN_EMERG "~~~~~~~RG_addr  = [0x%x], RG  = [0x%x]\n", otp_addr+2, rg_ratio);
    printk(KERN_EMERG "~~~~~~~BG_addr  = [0x%x], BG  = [0x%x]\n", otp_addr+3, bg_ratio);

	return index;
}

#ifdef SUPPORT_FLOATING //Use this if support floating point values
/*******************************************************************************
* Function    :  otp_apply_wb
* Description :  Calcualte and apply R, G, B gain to module
* Parameters  :  [in] golden_rg : R/G of golden camera module
                 [in] golden_bg : B/G of golden camera module
* Return      :  1, success; 0, fail
*******************************************************************************/	
static bool otp_apply_wb(unsigned char golden_rg, unsigned char golden_bg)
{
	unsigned short gain_r = GAIN_DEFAULT_VALUE;
	unsigned short gain_g = GAIN_DEFAULT_VALUE;
	unsigned short gain_b = GAIN_DEFAULT_VALUE;

	double ratio_r, ratio_g, ratio_b;
	double cmp_rg, cmp_bg;

	if (!golden_rg || !golden_bg)
	{
		printk("golden_rg / golden_bg can not be zero\n");

        return 0;
	}

	// Calcualte R, G, B gain of current module from R/G, B/G of golden module
    // and R/G, B/G of current module
	cmp_rg = 1.0 * rg_ratio / golden_rg;
	cmp_bg = 1.0 * bg_ratio / golden_bg;

	if ((cmp_rg<1) && (cmp_bg<1))
	{
		// R/G < R/G golden, B/G < B/G golden
		ratio_g = 1;
		ratio_r = 1 / cmp_rg;
		ratio_b = 1 / cmp_bg;
	}
	else if (cmp_rg > cmp_bg)
	{
		// R/G >= R/G golden, B/G < B/G golden
		// R/G >= R/G golden, B/G >= B/G golden
		ratio_r = 1;
		ratio_g = cmp_rg;
		ratio_b = cmp_rg / cmp_bg;
	}
	else
	{
		// B/G >= B/G golden, R/G < R/G golden
		// B/G >= B/G golden, R/G >= R/G golden
		ratio_b = 1;
		ratio_g = cmp_bg;
		ratio_r = cmp_bg / cmp_rg;
	}

	// write sensor wb gain to registers
	// 0x0400 = 1x gain
	if (ratio_r != 1)
	{
		gain_r = (unsigned short)(GAIN_DEFAULT_VALUE * ratio_r);
		ov5647_i2c_write_b_sensor(GAIN_RH_ADDR, gain_r >> 8);
		ov5647_i2c_write_b_sensor(GAIN_RL_ADDR, gain_r & 0x00ff);
	}

	if (ratio_g != 1)
	{
		gain_g = (unsigned short)(GAIN_DEFAULT_VALUE * ratio_g);
		ov5647_i2c_write_b_sensor(GAIN_GH_ADDR, gain_g >> 8);
		ov5647_i2c_write_b_sensor(GAIN_GL_ADDR, gain_g & 0x00ff);
	}

	if (ratio_b != 1)
	{
		gain_b = (unsigned short)(GAIN_DEFAULT_VALUE * ratio_b);
		ov5647_i2c_write_b_sensor(GAIN_BH_ADDR, gain_b >> 8);
		ov5647_i2c_write_b_sensor(GAIN_BL_ADDR, gain_b & 0x00ff);
	}
zxvczxcvx
	printk("cmp_rg=%f, cmp_bg=%f\n", cmp_rg, cmp_bg);
	printk("ratio_r=%f, ratio_g=%f, ratio_b=%f\n", ratio_r, ratio_g, ratio_b);
	printk("gain_r=0x%x, gain_g=0x%x, gain_b=0x%x\n", gain_r, gain_g, gain_b);
zxvczxcvx
    return 1;
}

#else //Use this if not support floating point values

#define OTP_MULTIPLE_FAC	10000
static bool otp_apply_wb(unsigned char golden_rg, unsigned char golden_bg)
{
	unsigned short gain_r = GAIN_DEFAULT_VALUE;
	unsigned short gain_g = GAIN_DEFAULT_VALUE;
	unsigned short gain_b = GAIN_DEFAULT_VALUE;

	unsigned short ratio_r, ratio_g, ratio_b;
	unsigned short cmp_rg, cmp_bg;

	u16 RH, RL, GH, GL, BH, BL;
    
	R_flag = 0;
	G_flag = 0;
	B_flag = 0;

	if (!golden_rg || !golden_bg)
	{
		printk("golden_rg / golden_bg can not be zero\n");
		return 0;
	}

	// Calcualte R, G, B gain of current module from R/G, B/G of golden module
    // and R/G, B/G of current module
	cmp_rg = OTP_MULTIPLE_FAC * rg_ratio / golden_rg;
	cmp_bg = OTP_MULTIPLE_FAC * bg_ratio / golden_bg;

	if ((cmp_rg < 1 * OTP_MULTIPLE_FAC) && (cmp_bg < 1 * OTP_MULTIPLE_FAC))
	{
		// R/G < R/G golden, B/G < B/G golden
		ratio_g = 1 * OTP_MULTIPLE_FAC;
		ratio_r = 1 * OTP_MULTIPLE_FAC * OTP_MULTIPLE_FAC / cmp_rg;
		ratio_b = 1 * OTP_MULTIPLE_FAC * OTP_MULTIPLE_FAC / cmp_bg;
	}
	else if (cmp_rg > cmp_bg)
	{
		// R/G >= R/G golden, B/G < B/G golden
		// R/G >= R/G golden, B/G >= B/G golden
		ratio_r = 1 * OTP_MULTIPLE_FAC;
		ratio_g = cmp_rg;
		ratio_b = OTP_MULTIPLE_FAC * cmp_rg / cmp_bg;
	}
	else
	{
		// B/G >= B/G golden, R/G < R/G golden
		// B/G >= B/G golden, R/G >= R/G golden
		ratio_b = 1 * OTP_MULTIPLE_FAC;
		ratio_g = cmp_bg;
		ratio_r = OTP_MULTIPLE_FAC * cmp_bg / cmp_rg;
	}

	// write sensor wb gain to registers
	// 0x0400 = 1x gain
	if (ratio_r != 1 * OTP_MULTIPLE_FAC)
	{
        printk(KERN_EMERG "-------XXA----------GAIN_R have writing\n");
        R_flag = 1;
		gain_r = GAIN_DEFAULT_VALUE * ratio_r / OTP_MULTIPLE_FAC;
		ov5647_i2c_write_b_sensor(GAIN_RH_ADDR, gain_r >> 8);
		ov5647_i2c_write_b_sensor(GAIN_RL_ADDR, gain_r & 0x00ff);
	}

	if (ratio_g != 1 * OTP_MULTIPLE_FAC)
	{
        printk(KERN_EMERG "-XXX----------------GAIN_G have writing\n");
        G_flag = 1;
		gain_g = GAIN_DEFAULT_VALUE * ratio_g / OTP_MULTIPLE_FAC;
		ov5647_i2c_write_b_sensor(GAIN_GH_ADDR, gain_g >> 8);
		ov5647_i2c_write_b_sensor(GAIN_GL_ADDR, gain_g & 0x00ff);
	}

	if (ratio_b != 1 * OTP_MULTIPLE_FAC)
	{
        printk(KERN_EMERG "-------XXY----------GAIN_B have writing\n");
        B_flag = 1;
		gain_b = GAIN_DEFAULT_VALUE * ratio_b / OTP_MULTIPLE_FAC;
		ov5647_i2c_write_b_sensor(GAIN_BH_ADDR, gain_b >> 8);
		ov5647_i2c_write_b_sensor(GAIN_BL_ADDR, gain_b & 0x00ff);
	}
#if 0
	printk("------------------#else\n");
	printk("-----cmp_rg  = [%d],   cmp_bg  =[%d]\n", cmp_rg, cmp_bg);
	printk("-----ratio_r = [%d],   ratio_g =[%d],   ratio_b=[%d]\n", ratio_r, ratio_g, ratio_b);
	printk("-----gain_r  = [0x%04x], gain_g  =[0x%04x], gain_b =[0x%04x]\n\n\n", gain_r, gain_g, gain_b);
#endif
//printk("---->gain_r  = [0x%x], gain_g  =[0x%x], gain_b =[0x%x]\n", gain_r/GAIN_DEFAULT_VALUE, gain_g/GAIN_DEFAULT_VALUE, gain_b/GAIN_DEFAULT_VALUE);
	if (R_flag == 1)
    {
	    printk(KERN_EMERG "[gainR:]RH_addr=[0x%04x],RH=[0x%02x], RL_addr=[0x%04x],RL=[0x%02x],\n",\
	    							GAIN_RH_ADDR, gain_r >> 8, GAIN_RL_ADDR, gain_r & 0x00ff);
    }
    if (G_flag == 1)
    {
		printk(KERN_EMERG "[gainG:]GH_addr=[0x%04x],GH=[0x%02x], GL_addr=[0x%04x],RL=[0x%02x],\n",\
		                            GAIN_GH_ADDR, gain_g >> 8, GAIN_GL_ADDR, gain_g & 0x00ff);
    }
    if (B_flag == 1)
    {
		printk(KERN_EMERG "[gainB:]BH_addr=[0x%04x],BH=[0x%02x], BL_addr=[0x%04x],BL=[0x%02x],\n",\
		                            GAIN_BH_ADDR, gain_b >> 8, GAIN_BL_ADDR, gain_b & 0x00ff);
	}

#if 1
	printk(KERN_EMERG "we read!!!!!!!\n");



	memset(&RH, 0, sizeof(RH));
	memset(&RL, 0, sizeof(RL));
	memset(&GH, 0, sizeof(GH));
	memset(&GL, 0, sizeof(GL));
	memset(&BH, 0, sizeof(BH));
	memset(&BL, 0, sizeof(BL));

	otp_read_enable();
	if (R_flag == 1)
    {
	    ov5647_i2c_read(GAIN_RH_ADDR, &RH);
	    ov5647_i2c_read(GAIN_RL_ADDR, &RL);
        printk(KERN_EMERG "[gainR:]RH_addr=[0x%04x],RH=[0x%04x], RL_addr=[0x%04x],RL=[0x%04x],\n",\
							GAIN_RH_ADDR, RH, GAIN_RL_ADDR, RL);
    }
    if (G_flag == 1)
    {
	    ov5647_i2c_read(GAIN_GH_ADDR, &GH);
	    ov5647_i2c_read(GAIN_GL_ADDR, &GL);
      	printk(KERN_EMERG "[gainG:]GH_addr=[0x%04x],GH=[0x%04x], GL_addr=[0x%04x],GL=[0x%04x],\n",\
	                            GAIN_GH_ADDR, GH, GAIN_GL_ADDR, GL);  
    }
    if (B_flag == 1)
    {
	    ov5647_i2c_read(GAIN_BH_ADDR, &BH);
	    ov5647_i2c_read(GAIN_BL_ADDR, &BL);
    	printk(KERN_EMERG "[gainB:]BH_addr=[0x%04x],BH=[0x%04x], BL_addr=[0x%04x],BL=[0x%04x],\n",\
                            GAIN_BH_ADDR, BH, GAIN_BL_ADDR, BL);
	}
	otp_read_disable();

#endif
	R_flag = 0;
	G_flag = 0;
	B_flag = 0;

    return 1;
}
#endif /* SUPPORT_FLOATING */

/*******************************************************************************
* Function    :  otp_update_wb
* Description :  Update white balance settings from OTP
* Parameters  :  [in] golden_rg : R/G of golden camera module
                 [in] golden_bg : B/G of golden camera module
* Return      :  1, success; 0, fail
*******************************************************************************/	
bool otp_update_wb(struct msm_sensor_ctrl_t *otp_ov5647_s_ctrl, unsigned char golden_rg, unsigned char golden_bg) 
//bool otp_update_wb(struct msm_sensor_ctrl_t *otp_ov5647_s_ctrl, unsigned char golden_rg, unsigned char golden_bg) 
{
	printk("start wb update\n");
	otp_local_s_ctrl = otp_ov5647_s_ctrl;

//0x36
	printk("~~~~~~~~~I2C addr = [0x%02x]\n", (otp_local_s_ctrl->sensor_i2c_addr>> 1));
	printk("~~~~~~~~~I2C addr = [0x%02x]\n", (otp_local_s_ctrl->sensor_i2c_client->client->addr >> 1));
	if (otp_read_wb_group(-1) != -1)
	{
		if (otp_apply_wb(golden_rg, golden_bg) == 1)
		{
			printk("\n\n-----wb update finished--------\n\n");
#if 0
			otp_read(my_global_otp_addr+2, &rg_ratio);
			otp_read(my_global_otp_addr+3, &bg_ratio);
			printk(KERN_EMERG "After: rg_ratio=[0x%x]\n bg_ratio=[0x%x]\n", rg_ratio, bg_ratio);
#endif            
			return 1;
		}
	}
	printk("\n\n-----wb update failed--------\n\n");
    
	return 0;
}

#else //really begin from here
#define OTP_DEBUG_ON 0

//#define RG_RATIO_TYPICAL 0x59//0xFD//
//#define BG_RATIO_TYPICAL 0x5c//0xFd//

#define GAIN_RH_ADDR	0x5186
#define GAIN_RL_ADDR	0x5187
#define GAIN_GH_ADDR	0x5188
#define GAIN_GL_ADDR	0x5189
#define GAIN_BH_ADDR	0x518a
#define GAIN_BL_ADDR	0x518b

static struct msm_sensor_ctrl_t *otp_local_s_ctrl = NULL;
#define OTP_LOCAL_I2C_ADDR	(otp_local_s_ctrl->sensor_i2c_client->client->addr >> 1)
#define OTP_LOCAL_I2C_ADAPTER	(otp_local_s_ctrl->sensor_i2c_client->client->adapter)

struct otp_struct {
	u16  customer_id;
	u16  module_integrator_id;
	u16  lens_id;
	u16  rg_ratio;
	u16  bg_ratio;
	u16  user_data[3];
	u16  light_rg;
	u16  light_bg;
};

#if OTP_DEBUG_ON
int R_flag = 0;
int G_flag = 0;
int B_flag = 0;
u16 RH, RL, GH, GL, BH, BL;
#endif
static int ov5647_i2c_rxdata(unsigned short saddr,
		unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
		{
			.addr  = saddr,
			.flags = 0,
			.len   = 2,
			.buf   = rxdata,
		},
		{
			.addr  = saddr,
			.flags = I2C_M_RD,
			.len   = 1,
			.buf   = rxdata,
		},
	};
	if (i2c_transfer(OTP_LOCAL_I2C_ADAPTER, msgs, 2) < 0) {
		printk("=====> ov5647_i2c_rxdata faild 0x%x\n", saddr);
		return -EIO;
	}
	return 0;
}

static int32_t ov5647_i2c_txdata(unsigned short saddr,
		unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
			.addr = saddr,
			.flags = 0,
			.len = length,
			.buf = txdata,
		},
	};
	if (i2c_transfer(OTP_LOCAL_I2C_ADAPTER, msg, 1) < 0) {
		//CDBG("ov5647_i2c_txdata faild 0x%x\n", saddr);
		printk("=====> ov5647_i2c_txdata faild 0x%x\n", saddr);
		return -EIO;
	}

	return 0;
}

static u16 ov5647_i2c_read(u16 raddr)
//		unsigned short *rdata)
{
	int32_t rc = 0;
	unsigned char buf[2];
#if 0
	if (!rdata)
		return -EIO;
	CDBG("%s:saddr:0x%x raddr:0x%x data:0x%x",__func__, OTP_LOCAL_I2C_ADDR, raddr, *rdata);
#endif
	memset(buf, 0, sizeof(buf));
	buf[0] = (raddr & 0xFF00) >> 8;
	buf[1] = (raddr & 0x00FF);
	rc = ov5647_i2c_rxdata(OTP_LOCAL_I2C_ADDR, buf, 1);
	if (rc < 0) {
		//CDBG("ov5647_i2c_read 0x%x failed!\n", raddr);
		printk("--------ov5647_i2c_read 0x%x failed!----------\n", raddr);
		return rc;
	}
	//*rdata = buf[0];
	//CDBG("ov5647_i2c_read 0x%x val = 0x%x!\n", raddr, *rdata);
#if OTP_DEBUG_ON	
	printk(KERN_EMERG "--------ov5647_i2c_read SUCCEED!---------\n");
#endif	
	return buf[0];
}

static int32_t ov5647_i2c_write_b_sensor(u16 waddr, u8 bdata)
{
	int32_t rc = -EFAULT;
	unsigned char buf[3];
	memset(buf, 0, sizeof(buf));
	buf[0] = (waddr & 0xFF00) >> 8;
	buf[1] = (waddr & 0x00FF);
	buf[2] = bdata;
	rc = ov5647_i2c_txdata(OTP_LOCAL_I2C_ADDR, buf, 3);
	if (rc < 0) {
		printk("------------i2c_write_b failed, addr = 0x%x, val = 0x%x!---------\n",
				waddr, bdata);
	}
#if OTP_DEBUG_ON    
	printk(KERN_EMERG "------------i2c_write_b SUCCEED---------\n");
#endif    
	return rc;
}


// index: index of otp group. (0, 1, 2)
// return:0, group index is empty
//1, group index has invalid data
//2, group index has valid data
int check_otp(int index)
{
	//int temp, i;
	u16 temp, i;
	//int address;
	u16 address;
	// read otp into buffer
	//OV5647_write_i2c(0x3d21, 0x01);
	ov5647_i2c_write_b_sensor(0x3d21, 0x01);
	//Delay(1);
    mdelay(1);
	address = 0x3d05 + index*9;
#if 0    
	temp = OV5647_read_i2c(address);
#else
	temp = ov5647_i2c_read(address);
#endif
	// disable otp read
	//OV5647_write_i2c(0x3d21, 0x00);
	ov5647_i2c_write_b_sensor(0x3d21, 0x00);
	// clear otp buffer
	for (i=0;i<32;i++) {
		//OV5647_write_i2c(0x3d00 + i, 0x00);
		ov5647_i2c_write_b_sensor(0x3d00 + i, 0x00);
	}
	if (!temp) {
		return 0;
	}
	else if ((!(temp & 0x80)) && (temp&0x7f)) {
		return 2;
	}
	else {
		return 1;
	}
}

// index: index of otp group. (0, 1, 2)
// return: 0,
int read_otp(int index, struct otp_struct * otp_ptr)
{
	int i;
	//int address;
	u16 address;
	// read otp into buffer
	//OV5647_write_i2c(0x3d21, 0x01);
	ov5647_i2c_write_b_sensor(0x3d21, 0x01);
	//Delay(1);
    mdelay(1);
	address = 0x3d05 + index*9;
#if 0
	(*otp_ptr).customer_id = (OV5647_read_i2c(address) & 0x7f);
	(*otp_ptr).module_integrator_id = OV5647_read_i2c(address);
	(*otp_ptr).lens_id = OV5647_read_i2c(address + 1);
	(*otp_ptr).rg_ratio = OV5647_read_i2c(address + 2);
	(*otp_ptr).bg_ratio = OV5647_read_i2c(address + 3);
	(*otp_ptr).user_data[0] = OV5647_read_i2c(address + 4);
	(*otp_ptr).user_data[1] = OV5647_read_i2c(address + 5);
	(*otp_ptr).user_data[2] = OV5647_read_i2c(address + 6);
	(*otp_ptr).light_rg = OV5647_read_i2c(address + 7);
	(*otp_ptr).light_bg = OV5647_read_i2c(address + 8);
#else
	(*otp_ptr).customer_id = (ov5647_i2c_read(address) & 0x7f);
	(*otp_ptr).module_integrator_id = ov5647_i2c_read(address);
	(*otp_ptr).lens_id = ov5647_i2c_read(address + 1);
	(*otp_ptr).rg_ratio = ov5647_i2c_read(address + 2);
	(*otp_ptr).bg_ratio = ov5647_i2c_read(address + 3);
	(*otp_ptr).user_data[0] = ov5647_i2c_read(address + 4);
	(*otp_ptr).user_data[1] = ov5647_i2c_read(address + 5);
	(*otp_ptr).user_data[2] = ov5647_i2c_read(address + 6);
	(*otp_ptr).light_rg = ov5647_i2c_read(address + 7);
	(*otp_ptr).light_bg = ov5647_i2c_read(address + 8);
#endif
	// disable otp read
	//OV5647_write_i2c(0x3d21, 0x00);
	ov5647_i2c_write_b_sensor(0x3d21, 0x00);
	// clear otp buffer
	for (i=0;i<32;i++) {
		//OV5647_write_i2c(0x3d00 + i, 0x00);
		ov5647_i2c_write_b_sensor(0x3d00 + i, 0x00);
	}
	return 0;
}


// R_gain, sensor red gain of AWB, 0x400 =1
// G_gain, sensor green gain of AWB, 0x400 =1
// B_gain, sensor blue gain of AWB, 0x400 =1
// return 0;
int update_awb_gain(int R_gain, int G_gain, int B_gain)
{
#if OTP_DEBUG_ON    
    printk(KERN_EMERG "----------%s begin\n", __func__);
#endif
	if (R_gain>0x400) {
#if 0
		OV5647_write_i2c(0x5186, R_gain>>8);
		OV5647_write_i2c(0x5187, R_gain & 0x00ff);
#else
		ov5647_i2c_write_b_sensor(GAIN_RH_ADDR, R_gain>>8);
		ov5647_i2c_write_b_sensor(GAIN_RL_ADDR, R_gain & 0x00ff);
#if OTP_DEBUG_ON        
        R_flag = 1;
#endif
#endif
    }
	if (G_gain>0x400) {
#if 0        
		OV5647_write_i2c(0x5188, G_gain>>8);
		OV5647_write_i2c(0x5189, G_gain & 0x00ff);
#else
		ov5647_i2c_write_b_sensor(GAIN_GH_ADDR, G_gain>>8);
		ov5647_i2c_write_b_sensor(GAIN_GL_ADDR, G_gain & 0x00ff);
#if OTP_DEBUG_ON        
		G_flag = 1;
#endif
#endif
    }
	if (B_gain>0x400) {
#if 0
        OV5647_write_i2c(0x518a, B_gain>>8);
		OV5647_write_i2c(0x518b, B_gain & 0x00ff);
#else        
		ov5647_i2c_write_b_sensor(GAIN_BH_ADDR, B_gain>>8);
		ov5647_i2c_write_b_sensor(GAIN_BL_ADDR, B_gain & 0x00ff);
#if OTP_DEBUG_ON
        B_flag = 1;
#endif
#endif
    }
#if OTP_DEBUG_ON
	memset(&RH, 0, sizeof(RH));
    memset(&RL, 0, sizeof(RL));	
    memset(&GH, 0, sizeof(GH));
    memset(&GL, 0, sizeof(GL));	
    memset(&BH, 0, sizeof(BH));
    memset(&BL, 0, sizeof(BL));

    if (R_flag == 1)
    {
		printk(KERN_EMERG "[WR:gainR:]RH_addr=[0x%04x],RH=[0x%02x], RL_addr=[0x%04x],RL=[0x%02x],\n",\
                                GAIN_RH_ADDR, R_gain >> 8, GAIN_RL_ADDR, R_gain & 0x00ff);
        RH = ov5647_i2c_read(GAIN_RH_ADDR);
        RL = ov5647_i2c_read(GAIN_RL_ADDR);
        printk(KERN_EMERG "[RD:gainR:]RH_addr=[0x%04x],RH=[0x%04x], RL_addr=[0x%04x],RL=[0x%04x],\n",\
                            GAIN_RH_ADDR, RH, GAIN_RL_ADDR, RL);

    }
    if (G_flag == 1)
    {
        printk(KERN_EMERG "[WR:gainG:]GH_addr=[0x%04x],GH=[0x%02x], GL_addr=[0x%04x],RL=[0x%02x],\n",\
                                    GAIN_GH_ADDR, G_gain >> 8, GAIN_GL_ADDR, G_gain & 0x00ff);
        GH = ov5647_i2c_read(GAIN_GH_ADDR);
        GL = ov5647_i2c_read(GAIN_GL_ADDR);
        printk(KERN_EMERG "[RD:gainG:]GH_addr=[0x%04x],GH=[0x%04x], GL_addr=[0x%04x],GL=[0x%04x],\n",\
                                GAIN_GH_ADDR, GH, GAIN_GL_ADDR, GL);  
    }
    if (B_flag == 1)
    {
        printk(KERN_EMERG "WR:[gainB:]BH_addr=[0x%04x],BH=[0x%02x], BL_addr=[0x%04x],BL=[0x%02x],\n",\
                                    GAIN_BH_ADDR, B_gain >> 8, GAIN_BL_ADDR, B_gain & 0x00ff);
        BH = ov5647_i2c_read(GAIN_BH_ADDR);
        BL = ov5647_i2c_read(GAIN_BL_ADDR);
        printk(KERN_EMERG "[RD:gainB:]BH_addr=[0x%04x],BH=[0x%04x], BL_addr=[0x%04x],BL=[0x%04x],\n",\
                            GAIN_BH_ADDR, BH, GAIN_BL_ADDR, BL);
    }
    R_flag = 0;
    G_flag = 0;
    B_flag = 0;
    
    printk(KERN_EMERG "----------%s end\n", __func__);
#endif

	return 0;
}

// call this function after OV5647 initialization
// return value: 0 update success
//1, no OTP
//int update_otp()
int ov5647_v4l2_update_otp(struct msm_sensor_ctrl_t *ov5647_s_ctrl, u8 golden_rg, u8 golden_bg)
{
    struct otp_struct current_otp;
	int i;
	int otp_index;
	int temp;
	int R_gain, G_gain, B_gain, G_gain_R, G_gain_B;
	int rg, bg;

	current_otp.bg_ratio = 0;
	current_otp.customer_id = 0;
    current_otp.lens_id = 0;
    current_otp.light_bg = 0;
    current_otp.light_rg = 0;
    current_otp.module_integrator_id = 0;
    current_otp.rg_ratio = 0;

	otp_local_s_ctrl = ov5647_s_ctrl;
	// R/G and B/G of current camera module is read out from sensor OTP
	// check first OTP with valid data
	for(i=0;i<3;i++)
    {
		temp = check_otp(i);
		if (temp == 2)
        {
			otp_index = i;
			break;
		}
	}
	if (i==3)
    {
		// no valid wb OTP data
		return 1;
	}
	read_otp(otp_index, &current_otp);
	if(current_otp.light_rg==0)
    {
#if OTP_DEBUG_ON        
        printk(KERN_EMERG "-------------light_rg = 0\n");
#endif
		// no light source information in OTP
		rg = current_otp.rg_ratio;
	}
	else
    {
#if OTP_DEBUG_ON        
        printk(KERN_EMERG "-------------light_rg != 0\n");
#endif
		// light source information found in OTP
		rg = current_otp.rg_ratio * (current_otp.light_rg +128) / 256;
	}
	if(current_otp.light_bg==0)
    {
#if OTP_DEBUG_ON
        printk(KERN_EMERG "-------------light_bg = 0\n");
#endif
		// no light source information in OTP
		bg = current_otp.bg_ratio;
	}
	else
    {
#if OTP_DEBUG_ON        
        printk(KERN_EMERG "-------------light_bg != 0\n");
#endif
		// light source information found in OTP
		bg = current_otp.bg_ratio * (current_otp.light_bg +128) / 256;
	}

	//calculate G gain
	//0x400 = 1x gain
	if(bg < golden_bg)
    {
        if (rg < golden_rg)
        {
			// current_otp.bg_ratio < BG_RATIO_TYPICAL &&
			// current_otp.rg_ratio < RG_RATIO_TYPICAL
			G_gain = 0x400;
			B_gain = 0x400 * golden_bg / bg;
			R_gain = 0x400 * golden_rg / rg;
		}
		else
        {
			// current_otp.bg_ratio < BG_RATIO_TYPICAL &&
			// current_otp.rg_ratio >= RG_RATIO_TYPICAL
			R_gain = 0x400;
			G_gain = 0x400 * rg / golden_rg;
			B_gain = G_gain * golden_bg /bg;
		}
	}
	else
    {
		if (rg < golden_rg)
        {
			// current_otp.bg_ratio >= BG_RATIO_TYPICAL &&
			// current_otp.rg_ratio < RG_RATIO_TYPICAL
			B_gain = 0x400;
			G_gain = 0x400 * bg / golden_bg;
			R_gain = G_gain * golden_rg / rg;
		}
		else
        {
			// current_otp.bg_ratio >= BG_RATIO_TYPICAL &&
			// current_otp.rg_ratio >= RG_RATIO_TYPICAL
			G_gain_B = 0x400 * bg / golden_bg;
			G_gain_R = 0x400 * rg / golden_rg;
			if(G_gain_B > G_gain_R )
            {
				B_gain = 0x400;
				G_gain = G_gain_B;
				R_gain = G_gain * golden_rg /rg;
			}
			else
            {
				R_gain = 0x400;
				G_gain = G_gain_R;
				B_gain = G_gain * golden_bg / bg;
			}
		}
	}
#if OTP_DEBUG_ON
    printk(KERN_EMERG "\n****************************\n");
	printk(KERN_EMERG "customer_id=[0x%x]\n module_integrator_id=[0x%x]\n lens_id=[0x%x]\n \
        	rg_ratio=[0x%x]\n bg_ratio=[0x%x]\n \
        	light_rg=[0x%x]\n light_bg=[0x%x]\n", \
        	current_otp.customer_id, current_otp.module_integrator_id, current_otp.lens_id,\
        	current_otp.rg_ratio, current_otp.bg_ratio,\
        	current_otp.light_rg, current_otp.light_bg);
    
    printk("gain_r=0x%x, gain_g=0x%x, gain_b=0x%x\n", R_gain, G_gain, B_gain);
    printk(KERN_EMERG "\n****************************\n");
#endif    
	update_awb_gain(R_gain, G_gain, B_gain);

	return 0;
}

#if 0
// return value: 0 no otp
//> 0: otp r/g value
int get_otp_rg(void)
{
	struct otp_struct current_otp;
	int i;
	int otp_index;
	int temp;
	// check first OTP with valid data
	for(i=0;i<3;i++) {
		temp = check_otp(i);
		if (temp == 2) {
			otp_index = i;
			break;
		}
	}
	if (i==3) {
		// no valid wb OTP data
		return 0;
	}
	read_otp(otp_index, &current_otp);
	return current_otp.rg_ratio;
}
// return value: 0 no otp
//> 0: otp r/g value
int get_otp_bg(void)
{
	struct otp_struct current_otp;
	int i;
	int otp_index;
	int temp;
	// check first OTP with valid data
	for(i=0;i<3;i++) {
		temp = check_otp(i);
		if (temp == 2) {
			otp_index = i;
			break;
		}
	}
	if (i==3) {
		// no valid wb OTP data
		return 0;
	}
	read_otp(otp_index, &current_otp);
	return current_otp.bg_ratio;
}

float get_awb_ratio(int index)
{
	float ratio;
    
	if(index == 0)
	{
		ratio = 1;
	}
	else
	{
		ratio = (float) index / 128;
	}
	return ratio;
}

// return value: 1 ,index = 0
float get_light_ratio(int index)
{
	float ratio;
	if(index == 0)
	{
		ratio = 1;
	}
	else
	{
		ratio = ((float) index + 128) / 256;
	}
    	
	return ratio;
}
// return value: 0 ,index = 1
//> 0: otp light r/g value
int get_otp_light_rg(void)
{
	struct otp_struct current_otp;
	int i;
	int otp_index;
	int temp;

    // check first OTP with valid data
	for(i=0;i<3;i++) {
		temp = check_otp(i);
		if (temp == 2) {
			otp_index = i;
			break;
		}
	}
	if (i==3) {
		// no valid wb OTP data
		return 0;
	}
	read_otp(otp_index, &current_otp);
	return current_otp.light_rg;
}

// return value: 0 ,index = 1
//> 0: otp light r/g value
int get_otp_light_bg(void)
{
	struct otp_struct current_otp;
	int i;
	int otp_index;
	int temp;
    
	// check first OTP with valid data
	for(i=0;i<3;i++) {
        temp = check_otp(i);
		if (temp == 2) {
			otp_index = i;
			break;
		}
	}
	if (i==3) {
		// no valid wb OTP data
		return 0;
	}
	read_otp(otp_index, &current_otp);
	return current_otp.light_bg;
}

#endif
#endif

