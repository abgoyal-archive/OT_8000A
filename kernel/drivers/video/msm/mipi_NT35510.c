/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_NT35510.h"

/*[PLATFORM]-Add-BEGIN by TCTNB.WQF, 2013/1/30, Add LCD second source support*/
#ifdef CONFIG_MACH_MSM8X25_SCRIBE5
#include <linux/gpio.h>
#define LCD_ID_PIN 119
#endif
/*[PLATFORM]-Add-END by TCTNB.WQF*/


static struct msm_panel_common_pdata *mipi_nt35510_pdata;
static struct dsi_buf nt35510_tx_buf;
static struct dsi_buf nt35510_rx_buf;

static int mipi_nt35510_bl_ctrl;

#define NT35510_SLEEP_OFF_DELAY 150
#define NT35510_DISPLAY_ON_DELAY 150

/*[PLATFORM]-Mod-BEGIN by TCTNB.WQF, 2012/9/5, Modify the display on seq*/
/* common setting */
static char display_off[2] = {0x28, 0x00};
static char enter_sleep[2] = {0x10, 0x00};
#ifdef CONFIG_MACH_MSM8X25_SCRIBE5
#else
static char exit_sleep[2] = {0x11, 0x00};
static char display_on[2] = {0x29, 0x00};
static char write_ram[2] = {0x2c, 0x00}; /* write ram */
#endif
/*[PLATFORM]-Mod-END by TCTNB.WQF*/

static struct dsi_cmd_desc nt35510_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 50, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 50, sizeof(enter_sleep), enter_sleep}
};

/*[PLATFORM]-Mod-BEGIN by TCTNB.WQF, 2012/8/21, Modify the display on seq*/
#ifdef CONFIG_MACH_MSM8X25_SCRIBE5
static char page1_cmd_0xF0[6]={0xF0,0x55,0xAA,0x52,0x08,0x01};	//Page 1
static char page1_cmd_0xB0[4]={0xB0,0x0D,0x0D,0x0D};	//#AVDD Set AVDD 5.2V
static char page1_cmd_0xB6[4]={0xB6,0x44,0x44,0x44};	//#AVDD ratio
static char page1_cmd_0xB1[4]={0xB1,0x0D,0x0D,0x0D};	//#AVEE  -5.2V
static char page1_cmd_0xB7[4]={0xB7,0x34,0x34,0x34};	//#AVEE ratio
static char page1_cmd_0xB2[4]={0xB2,0x00,0x00,0x00};	//#VCL  -2.5V
static char page1_cmd_0xB8[4]={0xB8,0x34,0x34,0x34};	//#VCL ratio
static char page1_cmd_0xBF[2]={0xBF,0x01};	//Set VGH voltage
static char page1_cmd_0xB3[4]={0xB3,0x0F,0x0F,0x0F};	//Set VGH 15v
static char page1_cmd_0xB5[4]={0xB5,0x08,0x08,0x08};	//Set VGL -10v
static char page1_cmd_0xC2[2]={0xC2,0x03};	//?
static char page1_cmd_0xB9[4]={0xB9,0x2E,0x2E,0x2E};	//#VGH ratio
static char page1_cmd_0xBA[4]={0xBA,0x2E,0x2E,0x2E};	//#VGL_REG -10V
static char page1_cmd_0xBC[4]={0xBC,0x00,0x78,0x00};	//#VGMP/VGSP 4.5V/0V
static char page1_cmd_0xBD[4]={0xBD,0x00,0x78,0x00};	//#VGMN/VGSN -4.5V/0V
static char page1_cmd_0xBE[3]={0xBE,0x00,0x64};	//#VCOM, 0x64
static char page1_cmd_0xD1[53]={0xD1,                                //R+
        0x00,0x04,0x00,0x08,0x00,0x14,0x00,0x23,0x00,0x3C,         // V0/V1/V3/V5/V7
        0x00,0x78,0x00,0xA0,0x00,0xE6,0x01,0x18,0x01,0x5C,         // V1/V15/V23/V31/V47
        0x01,0x8C,0x01,0xD2,0x02,0x05,0x02,0x06,0x02,0x35,        // V63/V95/V127/V128/V160
        0x02,0x64,0x02,0x7E,0x02,0x9A,0x02,0xAA,0x02,0xC0,         // V192/V208/V224/V232/V240
        0x02,0xCD,0x02,0xDF,0x02,0xEA,0x03,0x01,0x03,0x48,         // V244/V248/V250/V252/V254
        0x03,0xB6};        //V255
static char page1_cmd_0xD2[53]={0xD2,                                //G+
        0x00,0x04,0x00,0x08,0x00,0x14,0x00,0x23,0x00,0x3C,         // V0/V1/V3/V5/V7
        0x00,0x78,0x00,0xA0,0x00,0xE6,0x01,0x18,0x01,0x5C,         // V1/V15/V23/V31/V47
        0x01,0x8C,0x01,0xD2,0x02,0x05,0x02,0x06,0x02,0x35,        // V63/V95/V127/V128/V160
        0x02,0x64,0x02,0x7E,0x02,0x9A,0x02,0xAA,0x02,0xC0,         // V192/V208/V224/V232/V240
        0x02,0xCD,0x02,0xDF,0x02,0xEA,0x03,0x01,0x03,0x48,         // V244/V248/V250/V252/V254
        0x03,0xB6};        //V255
static char page1_cmd_0xD3[53]={0xD3,                                //B+
        0x00,0x04,0x00,0x08,0x00,0x14,0x00,0x23,0x00,0x3C,         // V0/V1/V3/V5/V7
        0x00,0x78,0x00,0xA0,0x00,0xE6,0x01,0x18,0x01,0x5C,         // V1/V15/V23/V31/V47
        0x01,0x8C,0x01,0xD2,0x02,0x05,0x02,0x06,0x02,0x35,        // V63/V95/V127/V128/V160
        0x02,0x64,0x02,0x7E,0x02,0x9A,0x02,0xAA,0x02,0xC0,         // V192/V208/V224/V232/V240
        0x02,0xCD,0x02,0xDF,0x02,0xEA,0x03,0x01,0x03,0x48,         // V244/V248/V250/V252/V254
        0x03,0xB6};        //V255
static char page1_cmd_0xD4[53]={0xD4,                                //R-
        0x00,0x04,0x00,0x08,0x00,0x14,0x00,0x23,0x00,0x3C,         // V0/V1/V3/V5/V7
        0x00,0x78,0x00,0xA0,0x00,0xE6,0x01,0x18,0x01,0x5C,         // V1/V15/V23/V31/V47
        0x01,0x8C,0x01,0xD2,0x02,0x05,0x02,0x06,0x02,0x35,        // V63/V95/V127/V128/V160
        0x02,0x64,0x02,0x7E,0x02,0x9A,0x02,0xAA,0x02,0xC0,         // V192/V208/V224/V232/V240
        0x02,0xCD,0x02,0xDF,0x02,0xEA,0x03,0x01,0x03,0x48,         // V244/V248/V250/V252/V254
        0x03,0xB6};        //V255
static char page1_cmd_0xD5[53]={0xD5,                                //B-
        0x00,0x04,0x00,0x08,0x00,0x14,0x00,0x23,0x00,0x3C,         // V0/V1/V3/V5/V7
        0x00,0x78,0x00,0xA0,0x00,0xE6,0x01,0x18,0x01,0x5C,         // V1/V15/V23/V31/V47
        0x01,0x8C,0x01,0xD2,0x02,0x05,0x02,0x06,0x02,0x35,        // V63/V95/V127/V128/V160
        0x02,0x64,0x02,0x7E,0x02,0x9A,0x02,0xAA,0x02,0xC0,         // V192/V208/V224/V232/V240
        0x02,0xCD,0x02,0xDF,0x02,0xEA,0x03,0x01,0x03,0x48,         // V244/V248/V250/V252/V254
        0x03,0xB6};        //V255
static char page1_cmd_0xD6[53]={0xD6,                                //G-
        0x00,0x04,0x00,0x08,0x00,0x14,0x00,0x23,0x00,0x3C,         // V0/V1/V3/V5/V7
        0x00,0x78,0x00,0xA0,0x00,0xE6,0x01,0x18,0x01,0x5C,         // V1/V15/V23/V31/V47
        0x01,0x8C,0x01,0xD2,0x02,0x05,0x02,0x06,0x02,0x35,        // V63/V95/V127/V128/V160
        0x02,0x64,0x02,0x7E,0x02,0x9A,0x02,0xAA,0x02,0xC0,         // V192/V208/V224/V232/V240
        0x02,0xCD,0x02,0xDF,0x02,0xEA,0x03,0x01,0x03,0x48,         // V244/V248/V250/V252/V254
        0x03,0xB6};        //V255
static char page0_cmd_0xF0[6]={0xF0,0x55,0xAA,0x52,0x08,0x00};	//Page 0
static char page0_cmd_0xB1[3]={0xB1,0xFC,0x00};	//#Display control
static char page0_cmd_0xB5[2]={0xB5,0x50};	//#480x800
static char page0_cmd_0xB6[2]={0xB6,0x05};	//#Source hold time
static char page0_cmd_0xB7[3]={0xB7,0x70,0x70};	//Set Gate EQ
static char page0_cmd_0xB8[5]={0xB8,0x01,0x03,0x03,0x03}; //Set Source EQ
static char page0_cmd_0xBC[4]={0xBC,0x00,0x00,0x00}; //Column Control
static char page0_cmd_0xBD[6]={0xBD,0x01,0x84,0x1C,0x1C,0x00};	//Inversion Control
static char page0_cmd_0xC9[6]={0xC9,0xD0,0x02,0x50,0x50,0x50};	//#Timing control 4H w/ 4-delay
static char page0_cmd_0x35[2]={0x35,0x00};	//TE ON
static char page0_cmd_0x3A[2]={0x3A,0x77};	//16.7M color
static char page0_cmd_0x11[1]={0x11};	//Sleep Out
static char page0_cmd_0x29[1]={0x29};	//Display On
static char page0_cmd_0x2C[1]={0x2C};	//Start GRAM write
static char page0_cmd_0xB4[2]={0xB4,0x10};   //vivi color enable
static char page1_cmd_0xFF_1[5]={0xFF,0xAA,0x55,0xA5,0x80}; // page1
static char page1_cmd_0xF7[16]={0xF7,0x63,0x40,0x00,0x00,0x00,0x01,0xC4,0xA2,0x00,0x02,0x64,0x54,0x48,0x00,0xD0};
static char page1_cmd_0xFF_2[5]={0xFF,0xAA,0x55,0xA5,0x00};

static struct dsi_cmd_desc nt35510_cmd_display_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xF0), page1_cmd_0xF0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xB0), page1_cmd_0xB0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xB6), page1_cmd_0xB6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xB1), page1_cmd_0xB1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xB7), page1_cmd_0xB7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xB2), page1_cmd_0xB2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xB8), page1_cmd_0xB8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xBF), page1_cmd_0xBF},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xB3), page1_cmd_0xB3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xB5), page1_cmd_0xB5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xC2), page1_cmd_0xC2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xB9), page1_cmd_0xB9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xBA), page1_cmd_0xBA},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xBC), page1_cmd_0xBC},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xBD), page1_cmd_0xBD},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xBE), page1_cmd_0xBE},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xD1), page1_cmd_0xD1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xD2), page1_cmd_0xD2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xD3), page1_cmd_0xD3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xD4), page1_cmd_0xD4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xD5), page1_cmd_0xD5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xD6), page1_cmd_0xD6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0xF0), page0_cmd_0xF0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0xB1), page0_cmd_0xB1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0xB5), page0_cmd_0xB5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0xB6), page0_cmd_0xB6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0xB7), page0_cmd_0xB7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0xB8), page0_cmd_0xB8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0xBC), page0_cmd_0xBC},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0xBD), page0_cmd_0xBD},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0xC9), page0_cmd_0xC9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0x35), page0_cmd_0x35},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0x3A), page0_cmd_0x3A},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,sizeof(page0_cmd_0x11), page0_cmd_0x11},
	{DTYPE_DCS_WRITE, 1, 0, 0, 30,sizeof(page0_cmd_0x29), page0_cmd_0x29},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,sizeof(page0_cmd_0x2C), page0_cmd_0x2C},

};

static char cmd19_rotate[3] = {
	0xB1, 0xEC, 0x06,
};

/*[PLATFORM]-Add-BEGIN by TCTNB.WQF, 2013/1/30, Add LCD second source support*/
static char otm8018b_set_01[]={0x00,0x00};
static char otm8018b_set_02[]={0xff,0x80,0x09,0x01};
static char otm8018b_set_03[]={0x00,0x80};
static char otm8018b_set_04[]={0xff,0x80,0x09};
static char otm8018b_set_05[]={0x00,0x03};
static char otm8018b_set_06[]={0xff,0x01};
static char otm8018b_set_07[]={0x00,0xb4};
static char otm8018b_set_08[]={0xc0,0x10};
static char otm8018b_set_09[]={0x00,0x89};
static char otm8018b_set_11[]={0xc4,0x08};
static char otm8018b_set_12[]={0x00,0xa3};
static char otm8018b_set_13[]={0xc0,0x00};
static char otm8018b_set_14[]={0x00,0x82};
static char otm8018b_set_15[]={0xc5,0xa3};
static char otm8018b_set_16[]={0x00,0x90};
static char otm8018b_set_17[]={0xc5,0xd6,0x87};
static char otm8018b_set_18[]={0x00,0x00};
static char otm8018b_set_19[]={0xd8,0x74,0x72};
static char otm8018b_set_20[]={0x00,0x00};
static char otm8018b_set_21[]={0xd9,0x60};
static char otm8018b_set_22[]={0x00,0x00};
static char otm8018b_set_23[]={0xe1,0x09,0x0a,0x0e,0x0d,0x07,0x18,0x0d,0x0d,0x01,0x04,0x05,0x06,0x0e,0x25,0x22,0x05};
static char otm8018b_set_24[]={0x00,0x00};
static char otm8018b_set_25[]={0xe2,0x09,0x0a,0x0e,0x0d,0x07,0x18,0x0d,0x0d,0x01,0x04,0x05,0x06,0x0e,0x25,0x22,0x05};
static char otm8018b_set_26[]={0x00,0x81};
static char otm8018b_set_27[]={0xc1,0x77};
static char otm8018b_set_28[]={0x00,0xa1};
static char otm8018b_set_29[]={0xc1,0x08};
static char otm8018b_set_30[]={0x00,0x81};
static char otm8018b_set_31[]={0xc4,0x83};
static char otm8018b_set_32[]={0x00,0x92};
static char otm8018b_set_33[]={0xc5,0x01};
static char otm8018b_set_34[]={0x00,0xb1};
static char otm8018b_set_35[]={0xc5,0xa9};
static char otm8018b_set_36[]={0x00,0x80};
static char otm8018b_set_37[]={0xb3,0x45};
static char otm8018b_set_38[]={0x00,0x90};
static char otm8018b_set_39[]={0xb3,0x02};
static char otm8018b_set_40[]={0x00,0x80};
static char otm8018b_set_41[]={0xc0,0x00,0x58,0x00,0x14,0x16};
static char otm8018b_set_42[]={0x00,0xc1};
static char otm8018b_set_43[]={0xf5,0xc0};
static char otm8018b_set_44[]={0x00,0x90};
static char otm8018b_set_45[]={0xc0,0x00,0x44,0x00,0x00,0x00,0x03};
static char otm8018b_set_46[]={0x00,0xa6};
static char otm8018b_set_47[]={0xc1,0x00,0x00,0x00};
static char otm8018b_set_48[]={0x00,0x80};
static char otm8018b_set_49[]={0xce,0x86,0x03,0x00,0x85,0x03,0x00,0x84,0x03,0x00,0x83,0x03,0x00};
static char otm8018b_set_50[]={0x00,0xa0};
static char otm8018b_set_51[]={0xce,0x38,0x03,0x03,0x58,0x00,0x00,0x00,0x38,0x02,0x03,0x59,0x00,0x00,0x00};
static char otm8018b_set_52[]={0x00,0xb0};
static char otm8018b_set_53[]={0xce,0x38,0x01,0x03,0x5a,0x00,0x00,0x00,0x38,0x00,0x03,0x57,0x00,0x00,0x00};
static char otm8018b_set_54[]={0x00,0xc0};
static char otm8018b_set_55[]={0xce,0x30,0x00,0x03,0x5c,0x00,0x00,0x00,0x30,0x01,0x03,0x5d,0x00,0x00,0x00};
static char otm8018b_set_56[]={0x00,0xd0};
static char otm8018b_set_57[]={0xce,0x30,0x02,0x03,0x5e,0x00,0x00,0x00,0x30,0x03,0x03,0x5b,0x00,0x00,0x00};
static char otm8018b_set_58[]={0x00,0xc7};
static char otm8018b_set_59[]={0xcf,0x00};
static char otm8018b_set_60[]={0x00,0xc9};
static char otm8018b_set_61[]={0xcf,0x00};
static char otm8018b_set_62[]={0x00,0xc4};
static char otm8018b_set_63[]={0xcb,0x04,0x04,0x04,0x04,0x04,0x04};
static char otm8018b_set_64[]={0x00,0xd9};
static char otm8018b_set_65[]={0xcb,0x04,0x04,0x04,0x04,0x04,0x04};
static char otm8018b_set_66[]={0x00,0x84};
static char otm8018b_set_67[]={0xcc,0x0c,0x0a,0x10,0x0e,0x03,0x04};
static char otm8018b_set_68[]={0x00,0xa0};
static char otm8018b_set_69[]={0xcc,0x09,0x0f,0x0d,0x01,0x02};
static char otm8018b_set_70[]={0x00,0xb4};
static char otm8018b_set_71[]={0xcc,0x0d,0x0f,0x09,0x0b,0x02,0x01};
static char otm8018b_set_72[]={0x00,0xce};
static char otm8018b_set_73[]={0xcc,0x0e};
static char otm8018b_set_74[]={0x00,0xd0};
static char otm8018b_set_75[]={0xcc,0x10,0x0a,0x0c,0x04,0x03};
static char otm8018b_set_76[]={0x2b,0x00,0x00,0x03,0x1f};
/*[PLATFORM]-Add-END by TCTNB.WQF*/

#else
static char cmd0[6] = {
	0xF0, 0x55, 0xAA, 0x52,
	0x08, 0x01,
};
static char cmd1[4] = {
	0xBC, 0x00, 0xA0, 0x00,
};
static char cmd2[4] = {
	0xBD, 0x00, 0xA0, 0x00,
};
static char cmd3[3] = {
	0xBE, 0x00, 0x79,
};
static char cmd4[53] = {
	0xD1, 0x00, 0x00, 0x00,
	0x14, 0x00, 0x32, 0x00,
	0x4F, 0x00, 0x65, 0x00,
	0x8B, 0x00, 0xA8, 0x00,
	0xD5, 0x00, 0xF7, 0x01,
	0x2B, 0x01, 0x54, 0x01,
	0x8E, 0x01, 0xBB, 0x01,
	0xBC, 0x01, 0xE3, 0x02,
	0x08, 0x02, 0x1C, 0x02,
	0x39, 0x02, 0x4F, 0x02,
	0x76, 0x02, 0xA3, 0x02,
	0xE3, 0x03, 0x12, 0x03,
	0x4C, 0x03, 0x66, 0x03,
	0x9A,
};
static char cmd5[53] = {
	0xD2, 0x00, 0x00, 0x00,
	0x14, 0x00, 0x32, 0x00,
	0x4F, 0x00, 0x65, 0x00,
	0x8B, 0x00, 0xA8, 0x00,
	0xD5, 0x00, 0xF7, 0x01,
	0x2B, 0x01, 0x54, 0x01,
	0x8E, 0x01, 0xBB, 0x01,
	0xBC, 0x01, 0xE3, 0x02,
	0x08, 0x02, 0x1C, 0x02,
	0x39, 0x02, 0x4F, 0x02,
	0x76, 0x02, 0xA3, 0x02,
	0xE3, 0x03, 0x12, 0x03,
	0x4C, 0x03, 0x66, 0x03,
	0x9A,
};
static char cmd6[53] = {
	0xD3, 0x00, 0x00, 0x00,
	0x14, 0x00, 0x32, 0x00,
	0x4F, 0x00, 0x65, 0x00,
	0x8B, 0x00, 0xA8, 0x00,
	0xD5, 0x00, 0xF7, 0x01,
	0x2B, 0x01, 0x54, 0x01,
	0x8E, 0x01, 0xBB, 0x01,
	0xBC, 0x01, 0xE3, 0x02,
	0x08, 0x02, 0x1C, 0x02,
	0x39, 0x02, 0x4F, 0x02,
	0x76, 0x02, 0xA3, 0x02,
	0xE3, 0x03, 0x12, 0x03,
	0x4C, 0x03, 0x66, 0x03,
	0x9A,
};
static char cmd7[53] = {
	0xD4, 0x00, 0x00, 0x00,
	0x14, 0x00, 0x32, 0x00,
	0x4F, 0x00, 0x65, 0x00,
	0x8B, 0x00, 0xA8, 0x00,
	0xD5, 0x00, 0xF7, 0x01,
	0x2B, 0x01, 0x54, 0x01,
	0x8E, 0x01, 0xBB, 0x01,
	0xBC, 0x01, 0xE3, 0x02,
	0x08, 0x02, 0x1C, 0x02,
	0x39, 0x02, 0x4F, 0x02,
	0x76, 0x02, 0xA3, 0x02,
	0xE3, 0x03, 0x12, 0x03,
	0x4C, 0x03, 0x66, 0x03,
	0x9A,
};
static char cmd8[53] = {
	0xD5, 0x00, 0x00, 0x00,
	0x14, 0x00, 0x32, 0x00,
	0x4F, 0x00, 0x65, 0x00,
	0x8B, 0x00, 0xA8, 0x00,
	0xD5, 0x00, 0xF7, 0x01,
	0x2B, 0x01, 0x54, 0x01,
	0x8E, 0x01, 0xBB, 0x01,
	0xBC, 0x01, 0xE3, 0x02,
	0x08, 0x02, 0x1C, 0x02,
	0x39, 0x02, 0x4F, 0x02,
	0x76, 0x02, 0xA3, 0x02,
	0xE3, 0x03, 0x12, 0x03,
	0x4C, 0x03, 0x66, 0x03,
	0x9A,
};
static char cmd9[53] = {
	0xD6, 0x00, 0x00, 0x00,
	0x14, 0x00, 0x32, 0x00,
	0x4F, 0x00, 0x65, 0x00,
	0x8B, 0x00, 0xA8, 0x00,
	0xD5, 0x00, 0xF7, 0x01,
	0x2B, 0x01, 0x54, 0x01,
	0x8E, 0x01, 0xBB, 0x01,
	0xBC, 0x01, 0xE3, 0x02,
	0x08, 0x02, 0x1C, 0x02,
	0x39, 0x02, 0x4F, 0x02,
	0x76, 0x02, 0xA3, 0x02,
	0xE3, 0x03, 0x12, 0x03,
	0x4C, 0x03, 0x66, 0x03,
	0x9A,
};
static char cmd10[4] = {
	0xB0, 0x0A, 0x0A, 0x0A,
};
static char cmd11[4] = {
	0xB1, 0x0A, 0x0A, 0x0A,
};
static char cmd12[4] = {
	0xBA, 0x24, 0x24, 0x24,
};
static char cmd13[4] = {
	0xB9, 0x24, 0x24, 0x24,
};
static char cmd14[4] = {
	0xB8, 0x24, 0x24, 0x24,
};
static char cmd15[6] = {
	0xF0, 0x55, 0xAA, 0x52,
	0x08, 0x00,
};
static char cmd16[2] = {
	0xB3, 0x00,
};
static char cmd17[2] = {
	0xB4, 0x10,
};
static char cmd18[2] = {
	0xB6, 0x02,
};
static char cmd19[3] = {
	0xB1, 0xEC, 0x00,
};
static char cmd19_rotate[3] = {
	0xB1, 0xEC, 0x06,
};
static char cmd20[4] = {
	0xBC, 0x05, 0x05, 0x05,
};
static char cmd21[3] = {
	0xB7, 0x20, 0x20,
};
static char cmd22[5] = {
	0xB8, 0x01, 0x03, 0x03,
	0x03,
};
static char cmd23[19] = {
	0xC8, 0x01, 0x00, 0x78,
	0x50, 0x78, 0x50, 0x78,
	0x50, 0x78, 0x50, 0xC8,
	0x3C, 0x3C, 0xC8, 0xC8,
	0x3C, 0x3C, 0xC8,
};
static char cmd24[6] = {
	0xBD, 0x01, 0x84, 0x07,
	0x31, 0x00,
};
static char cmd25[6] = {
	0xBE, 0x01, 0x84, 0x07,
	0x31, 0x00,
};
static char cmd26[6] = {
	0xBF, 0x01, 0x84, 0x07,
	0x31, 0x00,
};
static char cmd27[2] = {
	0x35, 0x00,
};
static char config_MADCTL[2] = {0x36, 0x00};
static struct dsi_cmd_desc nt35510_cmd_display_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd0), cmd0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd1), cmd1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd2), cmd2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd3), cmd3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd4), cmd4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd5), cmd5},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd6), cmd6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd7), cmd7},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd8), cmd8},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd9), cmd9},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd10), cmd10},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd11), cmd11},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd12), cmd12},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd13), cmd13},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd14), cmd14},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd15), cmd15},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd16), cmd16},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd17), cmd17},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd18), cmd18},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd19), cmd19},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd20), cmd20},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd21), cmd21},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd22), cmd22},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd23), cmd23},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd24), cmd24},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd25), cmd25},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd26), cmd26},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd27), cmd27},

	{DTYPE_DCS_WRITE, 1, 0, 0, 0,	sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,	sizeof(display_on), display_on},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(config_MADCTL), config_MADCTL},

	{DTYPE_DCS_WRITE, 1, 0, 0, 0,	sizeof(write_ram), write_ram},
};
#endif
/*[PLATFORM]-Mod-END by TCTNB.WQF*/

static struct dsi_cmd_desc nt35510_cmd_display_on_cmds_rotate[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(cmd19_rotate), cmd19_rotate},
};

/*[PLATFORM]-Mod-BEGIN by TCTNB.WQF, 2012/9/5, Modify the display on seq*/
#ifdef CONFIG_MACH_MSM8X25_SCRIBE5
static struct dsi_cmd_desc nt35510_video_display_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xF0), page1_cmd_0xF0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xB0), page1_cmd_0xB0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xB6), page1_cmd_0xB6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xB1), page1_cmd_0xB1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xB7), page1_cmd_0xB7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xB2), page1_cmd_0xB2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xB8), page1_cmd_0xB8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xBF), page1_cmd_0xBF},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xB3), page1_cmd_0xB3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xB5), page1_cmd_0xB5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xC2), page1_cmd_0xC2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xB9), page1_cmd_0xB9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xBA), page1_cmd_0xBA},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xBC), page1_cmd_0xBC},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xBD), page1_cmd_0xBD},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xBE), page1_cmd_0xBE},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xD1), page1_cmd_0xD1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xD2), page1_cmd_0xD2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xD3), page1_cmd_0xD3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xD4), page1_cmd_0xD4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xD5), page1_cmd_0xD5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xD6), page1_cmd_0xD6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0xF0), page0_cmd_0xF0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0xB1), page0_cmd_0xB1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0xB5), page0_cmd_0xB5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0xB6), page0_cmd_0xB6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0xB7), page0_cmd_0xB7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0xB8), page0_cmd_0xB8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0xBC), page0_cmd_0xBC},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0xF0), page0_cmd_0xF0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10,sizeof(page0_cmd_0xBC), page0_cmd_0xBC},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0xBD), page0_cmd_0xBD},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0xC9), page0_cmd_0xC9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0x35), page0_cmd_0x35},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0x3A), page0_cmd_0x3A},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0xB4), page0_cmd_0xB4},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,sizeof(page0_cmd_0x11), page0_cmd_0x11},
	{DTYPE_DCS_WRITE, 1, 0, 0, 30,sizeof(page0_cmd_0x29), page0_cmd_0x29},
	//{DTYPE_DCS_WRITE, 1, 0, 0, 0,sizeof(page0_cmd_0x2C), page0_cmd_0x2C},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xF0), page1_cmd_0xF0},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xFF_1), page1_cmd_0xFF_1},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xF7), page1_cmd_0xF7},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page1_cmd_0xFF_2), page1_cmd_0xFF_2},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(page0_cmd_0xF0), page0_cmd_0xF0},

};

/*[PLATFORM]-Add-BEGIN by TCTNB.WQF, 2013/1/30, Add LCD second source support*/
static struct dsi_cmd_desc otm8018b_video_display_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_01), otm8018b_set_01},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_02), otm8018b_set_02},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_03), otm8018b_set_03},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_04), otm8018b_set_04},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_05), otm8018b_set_05},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_06), otm8018b_set_06},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_07), otm8018b_set_07},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_08), otm8018b_set_08},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_09), otm8018b_set_09},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_11), otm8018b_set_11},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_12), otm8018b_set_12},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_13), otm8018b_set_13},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_14), otm8018b_set_14},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_15), otm8018b_set_15},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_16), otm8018b_set_16},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_17), otm8018b_set_17},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_18), otm8018b_set_18},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_19), otm8018b_set_19},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_20), otm8018b_set_20},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_21), otm8018b_set_21},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_22), otm8018b_set_22},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_23), otm8018b_set_23},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_24), otm8018b_set_24},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_25), otm8018b_set_25},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_26), otm8018b_set_26},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_27), otm8018b_set_27},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_28), otm8018b_set_28},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_29), otm8018b_set_29},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_30), otm8018b_set_30},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_31), otm8018b_set_31},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_32), otm8018b_set_32},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_33), otm8018b_set_33},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_34), otm8018b_set_34},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_35), otm8018b_set_35},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_36), otm8018b_set_36},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_37), otm8018b_set_37},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_38), otm8018b_set_38},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_39), otm8018b_set_39},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_40), otm8018b_set_40},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_41), otm8018b_set_41},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_42), otm8018b_set_42},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_43), otm8018b_set_43},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_44), otm8018b_set_44},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_45), otm8018b_set_45},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_46), otm8018b_set_46},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_47), otm8018b_set_47},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_48), otm8018b_set_48},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_49), otm8018b_set_49},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_50), otm8018b_set_50},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_51), otm8018b_set_51},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_52), otm8018b_set_52},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_53), otm8018b_set_53},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_54), otm8018b_set_54},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_55), otm8018b_set_55},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_56), otm8018b_set_56},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_57), otm8018b_set_57},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_58), otm8018b_set_58},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_59), otm8018b_set_59},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_60), otm8018b_set_60},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_61), otm8018b_set_61},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_62), otm8018b_set_62},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_63), otm8018b_set_63},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_64), otm8018b_set_64},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_65), otm8018b_set_65},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_66), otm8018b_set_66},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_67), otm8018b_set_67},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_68), otm8018b_set_68},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_69), otm8018b_set_69},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_70), otm8018b_set_70},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_71), otm8018b_set_71},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_72), otm8018b_set_72},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_73), otm8018b_set_73},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_74), otm8018b_set_74},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_75), otm8018b_set_75},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8018b_set_76), otm8018b_set_76},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,sizeof(page0_cmd_0x11), page0_cmd_0x11},
	{DTYPE_DCS_WRITE, 1, 0, 0, 30,sizeof(page0_cmd_0x29), page0_cmd_0x29},
};
/*[PLATFORM]-Add-END by TCTNB.WQF*/

#else
static char video0[6] = {
	0xF0, 0x55, 0xAA, 0x52,
	0x08, 0x01,
};
static char video1[4] = {
	0xBC, 0x00, 0xA0, 0x00,
};
static char video2[4] = {
	0xBD, 0x00, 0xA0, 0x00,
};
static char video3[3] = {
	0xBE, 0x00, 0x79,
};
static char video4[53] = {
	0xD1, 0x00, 0x00, 0x00,
	0x14, 0x00, 0x32, 0x00,
	0x4F, 0x00, 0x65, 0x00,
	0x8B, 0x00, 0xA8, 0x00,
	0xD5, 0x00, 0xF7, 0x01,
	0x2B, 0x01, 0x54, 0x01,
	0x8E, 0x01, 0xBB, 0x01,
	0xBC, 0x01, 0xE3, 0x02,
	0x08, 0x02, 0x1C, 0x02,
	0x39, 0x02, 0x4F, 0x02,
	0x76, 0x02, 0xA3, 0x02,
	0xE3, 0x03, 0x12, 0x03,
	0x4C, 0x03, 0x66, 0x03,
	0x9A,
};
static char video5[53] = {
	0xD2, 0x00, 0x00, 0x00,
	0x14, 0x00, 0x32, 0x00,
	0x4F, 0x00, 0x65, 0x00,
	0x8B, 0x00, 0xA8, 0x00,
	0xD5, 0x00, 0xF7, 0x01,
	0x2B, 0x01, 0x54, 0x01,
	0x8E, 0x01, 0xBB, 0x01,
	0xBC, 0x01, 0xE3, 0x02,
	0x08, 0x02, 0x1C, 0x02,
	0x39, 0x02, 0x4F, 0x02,
	0x76, 0x02, 0xA3, 0x02,
	0xE3, 0x03, 0x12, 0x03,
	0x4C, 0x03, 0x66, 0x03,
	0x9A,
};
static char video6[53] = {
	0xD3, 0x00, 0x00, 0x00,
	0x14, 0x00, 0x32, 0x00,
	0x4F, 0x00, 0x65, 0x00,
	0x8B, 0x00, 0xA8, 0x00,
	0xD5, 0x00, 0xF7, 0x01,
	0x2B, 0x01, 0x54, 0x01,
	0x8E, 0x01, 0xBB, 0x01,
	0xBC, 0x01, 0xE3, 0x02,
	0x08, 0x02, 0x1C, 0x02,
	0x39, 0x02, 0x4F, 0x02,
	0x76, 0x02, 0xA3, 0x02,
	0xE3, 0x03, 0x12, 0x03,
	0x4C, 0x03, 0x66, 0x03,
	0x9A,
};
static char video7[53] = {
	0xD4, 0x00, 0x00, 0x00,
	0x14, 0x00, 0x32, 0x00,
	0x4F, 0x00, 0x65, 0x00,
	0x8B, 0x00, 0xA8, 0x00,
	0xD5, 0x00, 0xF7, 0x01,
	0x2B, 0x01, 0x54, 0x01,
	0x8E, 0x01, 0xBB, 0x01,
	0xBC, 0x01, 0xE3, 0x02,
	0x08, 0x02, 0x1C, 0x02,
	0x39, 0x02, 0x4F, 0x02,
	0x76, 0x02, 0xA3, 0x02,
	0xE3, 0x03, 0x12, 0x03,
	0x4C, 0x03, 0x66, 0x03,
	0x9A,
};
static char video8[53] = {
	0xD5, 0x00, 0x00, 0x00,
	0x14, 0x00, 0x32, 0x00,
	0x4F, 0x00, 0x65, 0x00,
	0x8B, 0x00, 0xA8, 0x00,
	0xD5, 0x00, 0xF7, 0x01,
	0x2B, 0x01, 0x54, 0x01,
	0x8E, 0x01, 0xBB, 0x01,
	0xBC, 0x01, 0xE3, 0x02,
	0x08, 0x02, 0x1C, 0x02,
	0x39, 0x02, 0x4F, 0x02,
	0x76, 0x02, 0xA3, 0x02,
	0xE3, 0x03, 0x12, 0x03,
	0x4C, 0x03, 0x66, 0x03,
	0x9A,
};
static char video9[53] = {
	0xD6, 0x00, 0x00, 0x00,
	0x14, 0x00, 0x32, 0x00,
	0x4F, 0x00, 0x65, 0x00,
	0x8B, 0x00, 0xA8, 0x00,
	0xD5, 0x00, 0xF7, 0x01,
	0x2B, 0x01, 0x54, 0x01,
	0x8E, 0x01, 0xBB, 0x01,
	0xBC, 0x01, 0xE3, 0x02,
	0x08, 0x02, 0x1C, 0x02,
	0x39, 0x02, 0x4F, 0x02,
	0x76, 0x02, 0xA3, 0x02,
	0xE3, 0x03, 0x12, 0x03,
	0x4C, 0x03, 0x66, 0x03,
	0x9A,
};
static char video10[4] = {
	0xB0, 0x0A, 0x0A, 0x0A,
};
static char video11[4] = {
	0xB1, 0x0A, 0x0A, 0x0A,
};
static char video12[4] = {
	0xBA, 0x24, 0x24, 0x24,
};
static char video13[4] = {
	0xB9, 0x24, 0x24, 0x24,
};
static char video14[4] = {
	0xB8, 0x24, 0x24, 0x24,
};
static char video15[6] = {
	0xF0, 0x55, 0xAA, 0x52,
	0x08, 0x00,
};
static char video16[2] = {
	0xB3, 0x00,
};
static char video17[2] = {
	0xB4, 0x10,
};
static char video18[2] = {
	0xB6, 0x02,
};
static char video19[3] = {
	0xB1, 0xFC, 0x00,
};
static char video20[4] = {
	0xBC, 0x05, 0x05, 0x05,
};
static char video21[3] = {
	0xB7, 0x20, 0x20,
};
static char video22[5] = {
	0xB8, 0x01, 0x03, 0x03,
	0x03,
};
static char video23[19] = {
	0xC8, 0x01, 0x00, 0x78,
	0x50, 0x78, 0x50, 0x78,
	0x50, 0x78, 0x50, 0xC8,
	0x3C, 0x3C, 0xC8, 0xC8,
	0x3C, 0x3C, 0xC8,
};
static char video24[6] = {
	0xBD, 0x01, 0x84, 0x07,
	0x31, 0x00,
};
static char video25[6] = {
	0xBE, 0x01, 0x84, 0x07,
	0x31, 0x00,
};
static char video26[6] = {
	0xBF, 0x01, 0x84, 0x07,
	0x31, 0x00,
};
static char video27[2] = {
	0x35, 0x00,
};

static struct dsi_cmd_desc nt35510_video_display_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video0), video0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video1), video1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video2), video2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video3), video3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video4), video4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video5), video5},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video6), video6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video7), video7},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video8), video8},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video9), video9},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video10), video10},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video11), video11},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video12), video12},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video13), video13},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video14), video14},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video15), video15},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video16), video16},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video17), video17},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video18), video18},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video19), video19},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video20), video20},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video21), video21},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video22), video22},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video23), video23},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video24), video24},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video25), video25},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video26), video26},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video27), video27},
	{DTYPE_DCS_WRITE, 1, 0, 0, NT35510_SLEEP_OFF_DELAY, sizeof(exit_sleep),
			exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, NT35510_DISPLAY_ON_DELAY, sizeof(display_on),
			display_on},
};
#endif

static char config_video_MADCTL[2] = {0x36, 0xC0};
/*[PLATFORM]-Mod-END by TCTNB.WQF*/

static struct dsi_cmd_desc nt35510_video_display_on_cmds_rotate[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(config_video_MADCTL), config_video_MADCTL},
};

static int mipi_nt35510_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;
	static int rotate;
/*[PLATFORM]-Add-BEGIN by TCTNB.WQF, 2013/1/30, Add LCD second source support*/
#ifdef CONFIG_MACH_MSM8X25_SCRIBE5
	int lcd_id;
#endif
/*[PLATFORM]-Mod-END by TCTNB.WQF*/

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi  = &mfd->panel_info.mipi;

	if (!mfd->cont_splash_done) {
		mfd->cont_splash_done = 1;
		return 0;
	}

	if (mipi_nt35510_pdata && mipi_nt35510_pdata->rotate_panel)
		rotate = mipi_nt35510_pdata->rotate_panel();

	if (mipi->mode == DSI_VIDEO_MODE) {
/*[PLATFORM]-Mod-BEGIN by TCTNB.WQF, 2013/1/30, Add LCD second source support*/
#ifdef CONFIG_MACH_MSM8X25_SCRIBE5
		lcd_id = gpio_get_value_cansleep(LCD_ID_PIN);
		printk("wqf-----lcd_id=%d\n", lcd_id);
		if (lcd_id == 1) {
		    mipi_dsi_cmds_tx(&nt35510_tx_buf,
				otm8018b_video_display_on_cmds,
				ARRAY_SIZE(otm8018b_video_display_on_cmds));
		} else {
			mipi_dsi_cmds_tx(&nt35510_tx_buf,
				nt35510_video_display_on_cmds,
				ARRAY_SIZE(nt35510_video_display_on_cmds));

			if (rotate) {
				mipi_dsi_cmds_tx(&nt35510_tx_buf,
					nt35510_video_display_on_cmds_rotate,
					ARRAY_SIZE(nt35510_video_display_on_cmds_rotate));
		    }
		}
#endif
/*[PLATFORM]-Mod-END by TCTNB.WQF*/
	} else if (mipi->mode == DSI_CMD_MODE) {
		mipi_dsi_cmds_tx(&nt35510_tx_buf,
			nt35510_cmd_display_on_cmds,
			ARRAY_SIZE(nt35510_cmd_display_on_cmds));

		if (rotate) {
			mipi_dsi_cmds_tx(&nt35510_tx_buf,
				nt35510_cmd_display_on_cmds_rotate,
			ARRAY_SIZE(nt35510_cmd_display_on_cmds_rotate));
		}
	}

	return 0;
}

static int mipi_nt35510_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	pr_debug("mipi_nt35510_lcd_off E\n");

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(&nt35510_tx_buf, nt35510_display_off_cmds,
			ARRAY_SIZE(nt35510_display_off_cmds));

	pr_debug("mipi_nt35510_lcd_off X\n");
	return 0;
}

static ssize_t mipi_nt35510_wta_bl_ctrl(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t ret = strnlen(buf, PAGE_SIZE);
	int err;

	err =  kstrtoint(buf, 0, &mipi_nt35510_bl_ctrl);
	if (err)
		return ret;

	pr_info("%s: bl ctrl set to %d\n", __func__, mipi_nt35510_bl_ctrl);

	return ret;
}

static DEVICE_ATTR(bl_ctrl, S_IWUSR, NULL, mipi_nt35510_wta_bl_ctrl);

static struct attribute *mipi_nt35510_fs_attrs[] = {
	&dev_attr_bl_ctrl.attr,
	NULL,
};

static struct attribute_group mipi_nt35510_fs_attr_group = {
	.attrs = mipi_nt35510_fs_attrs,
};

static int mipi_nt35510_create_sysfs(struct platform_device *pdev)
{
	int rc;
	struct msm_fb_data_type *mfd = platform_get_drvdata(pdev);

	if (!mfd) {
		pr_err("%s: mfd not found\n", __func__);
		return -ENODEV;
	}
	if (!mfd->fbi) {
		pr_err("%s: mfd->fbi not found\n", __func__);
		return -ENODEV;
	}
	if (!mfd->fbi->dev) {
		pr_err("%s: mfd->fbi->dev not found\n", __func__);
		return -ENODEV;
	}
	rc = sysfs_create_group(&mfd->fbi->dev->kobj,
		&mipi_nt35510_fs_attr_group);
	if (rc) {
		pr_err("%s: sysfs group creation failed, rc=%d\n",
			__func__, rc);
		return rc;
	}

	return 0;
}

static int __devinit mipi_nt35510_lcd_probe(struct platform_device *pdev)
{
	struct platform_device *pthisdev = NULL;
	struct msm_fb_panel_data *pdata;
/*[PLATFORM]-Add-BEGIN by TCTNB.WQF, 2013/1/30, Add LCD second source support*/
#ifdef CONFIG_MACH_MSM8X25_SCRIBE5
	int ret;
#endif
/*[PLATFORM]-Add-END by TCTNB.WQF*/
	pr_debug("%s\n", __func__);

	if (pdev->id == 0) {
		mipi_nt35510_pdata = pdev->dev.platform_data;
		if (mipi_nt35510_pdata->bl_lock)
			spin_lock_init(&mipi_nt35510_pdata->bl_spinlock);
		return 0;
	}

	pdata = pdev->dev.platform_data;
	if (mipi_nt35510_pdata && mipi_nt35510_pdata->rotate_panel()
			&& pdata->panel_info.type == MIPI_CMD_PANEL)
		pdata->panel_info.lcd.refx100 = 6200;

	pthisdev = msm_fb_add_device(pdev);
	mipi_nt35510_create_sysfs(pthisdev);

/*[PLATFORM]-Add-BEGIN by TCTNB.WQF, 2013/1/30, Add LCD second source support*/
#ifdef CONFIG_MACH_MSM8X25_SCRIBE5
	ret = gpio_tlmm_config(GPIO_CFG(LCD_ID_PIN, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	if (ret) {
		pr_err("%s: lcd_select_gpio config error\n", __func__);
	}

	ret = gpio_request(LCD_ID_PIN, "lcd_id_pin");
	if (ret) {
		pr_err("%s: lcd_select_gpio request error\n", __func__);
	}

	ret = gpio_direction_input(LCD_ID_PIN);
	if (ret) {
		pr_err("%s: lcd_select_gpio set direction error\n", __func__);
	}
#endif
/*[PLATFORM]-Add-END by TCTNB.WQF*/

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_nt35510_lcd_probe,
	.driver = {
		.name   = "mipi_NT35510",
	},
};

static int old_bl_level;

static void mipi_nt35510_set_backlight(struct msm_fb_data_type *mfd)
{
	int bl_level;
	unsigned long flags;
	bl_level = mfd->bl_level;

	if (mipi_nt35510_pdata->bl_lock) {
		if (!mipi_nt35510_bl_ctrl) {
/*[PLATFORM]-Mod-BEGIN by TCTNB.WQF, 2012/8/21, Modify the bl rang ro 0~32*/
#ifdef CONFIG_MACH_MSM8X25_SCRIBE5
			/* Level received is of range 0 to bl_max,
			   We need to convert the levels to 0
			   to 32 */
			bl_level = (2 * bl_level * 32 + mfd->panel_info.bl_max)
					/(2 * mfd->panel_info.bl_max);
#else
			/* Level received is of range 1 to bl_max,
			   We need to convert the levels to 1
			   to 31 */
			bl_level = (2 * bl_level * 31 + mfd->panel_info.bl_max)
					/(2 * mfd->panel_info.bl_max);
#endif
/*[PLATFORM]-Mod-END by TCTNB.WQF*/
			if (bl_level == old_bl_level)
				return;

			if (bl_level == 0)
				mipi_nt35510_pdata->backlight(0, 1);

			if (old_bl_level == 0)
				mipi_nt35510_pdata->backlight(50, 1);

			spin_lock_irqsave(&mipi_nt35510_pdata->bl_spinlock,
						flags);
			mipi_nt35510_pdata->backlight(bl_level, 0);
			spin_unlock_irqrestore(&mipi_nt35510_pdata->bl_spinlock,
						flags);
			old_bl_level = bl_level;
		} else {
			mipi_nt35510_pdata->backlight(bl_level, 1);
		}
	} else {
		mipi_nt35510_pdata->backlight(bl_level, mipi_nt35510_bl_ctrl);
	}
}

static struct msm_fb_panel_data nt35510_panel_data = {
	.on	= mipi_nt35510_lcd_on,
	.off = mipi_nt35510_lcd_off,
	.set_backlight = mipi_nt35510_set_backlight,
};

static int ch_used[3];

static int mipi_nt35510_lcd_init(void)
{
	mipi_dsi_buf_alloc(&nt35510_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&nt35510_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}
int mipi_nt35510_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_nt35510_lcd_init();
	if (ret) {
		pr_err("mipi_nt35510_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_NT35510", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	nt35510_panel_data.panel_info = *pinfo;
	ret = platform_device_add_data(pdev, &nt35510_panel_data,
				sizeof(nt35510_panel_data));
	if (ret) {
		pr_debug("%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		pr_debug("%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}
