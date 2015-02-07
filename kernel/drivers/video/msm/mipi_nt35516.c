/******************************************************************************/
/*                                                               Date:10/2012 */
/*                             PRESENTATION                                   */
/*                                                                            */
/*      Copyright 2012 TCL Communication Technology Holdings Limited.         */
/*                                                                            */
/* This material is company confidential, cannot be reproduced in any form    */
/* without the written permission of TCL Communication Technology Holdings    */
/* Limited.                                                                   */
/*                                                                            */
/* -------------------------------------------------------------------------- */
/* Author:  Alvin                                                             */
/* E-Mail:  Weifeng.Li@tcl-mobile.com                                         */
/* Role  :  NT35516                                                           */
/* Reference documents :  NT35516_SPEC_v0.3.pdf                               */
/* -------------------------------------------------------------------------- */
/* Comments: This is the driver for NT35516(qHD)                              */
/*                                                                            */
/* File    : kernel/drivers/video/msm/mipi_nt35516.c                          */
/* Labels  :                                                                  */
/* -------------------------------------------------------------------------- */
/* ========================================================================== */
/* Modifications on Features list / Changes Request / Problems Report         */
/* -------------------------------------------------------------------------- */
/* date    | author         | key                | comment (what, where, why) */
/* --------|----------------|--------------------|--------------------------- */
/* 15/10/12| Alvin          |                    | Create this file           */
/*---------|----------------|--------------------|--------------------------- */

/******************************************************************************/

#include <linux/leds.h>
#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_nt35516.h"
#include "mdp4.h"


static struct mipi_dsi_panel_platform_data *mipi_nt35516_pdata;

static struct dsi_buf nt35516_tx_buf;
static struct dsi_buf nt35516_rx_buf;
static int mipi_nt35516_lcd_init(void);

static int wled_trigger_initialized;


/* Display On in Cmd Mode */
static char cmd_on_0xFF[6]={0xFF,0xAA,0x55,0x25,0x01,0x01};
static char cmd_on_0xF2[36]={0xF2,0x00,0x00,0x4A,0x0A,0xA8,
    			0x00,0x00,0x00,0x00,0x00,
    			0x00,0x00,0x00,0x00,0x00,
    			0x00,0x00,0x0B,0x00,0x00,
    			0x00,0x00,0x00,0x00,0x00,
    			0x00,0x00,0x00,0x40,0x01,
    			0x51,0x00,0x01,0x00,0x01,
    			};
static char cmd_on_0xF3[8]={0xF3,0x02,0x03,0x07,0x45,0x88,0xD1,0x0D};
static char cmd_on_0xF0_0[6]={0xF0,0x55,0xAA,0x52,0x08,0x00};//#LV2 Page 0 enable
static char cmd_on_0xB1_0[4]={0xB1,0xCC, 0x00, 0x00}; //#Display control
static char cmd_on_0xB6_0[2]={0xB6,0x01};
static char cmd_on_0xB7_0[3]={0xB7,0x72, 0x72};
static char cmd_on_0xB8_0[5]={0xB8,0x01,0x01,0x01,0x01};
static char cmd_on_0xBB[2]={0xBB,0x53};
static char cmd_on_0xBC_0[4]={0xBC,0x04,0x04,0x04};
static char cmd_on_0xBD_0[6]={0xBD,0x01,0x93,0x10, 0x20, 0x01};
static char cmd_on_0xC9[7]={0xC9,0x61,0x06,0x0D,0x17,0x17,0x00};
static char cmd_on_0xF0_1[6]={0xF0,0x55,0xAA,0x52,0x08,0x01};//LV2 Page 1 enable
static char cmd_on_0xB0[4]={0xB0,0x0C,0x0C,0x0C};//AVDD Set AVDD
static char cmd_on_0xB1_1[4]={0xB1,0x0C,0x0C,0x0C};//#AVEE
static char cmd_on_0xB2[4]={0xB2,0x02,0x02,0x02};//#VCL 
static char cmd_on_0xB3[4]={0xB3,0x10,0x10,0x10};//#VGH
static char cmd_on_0xB4[4]={0xB4,0x06,0x06,0x06};//#VGLX
static char cmd_on_0xB6_1[4]={0xB6,0x54,0x54,0x54};
static char cmd_on_0xB7_1[4]={0xB7,0x24, 0x24, 0x24};
static char cmd_on_0xB8_1[4]={0xB8,0x30, 0x30, 0x30};//#AVEE
static char cmd_on_0xB9[4]={0xB9,0x34, 0x34, 0x34};
static char cmd_on_0xBA[4]={0xBA,0x24, 0x24, 0x24};
static char cmd_on_0xBC_1[4]={0xBC,0x00, 0x98, 0x00};
static char cmd_on_0xBD_1[4]={0xBD,0x00, 0x98, 0x00};
static char cmd_on_0xBE[2]={0xBE,0x57};
static char cmd_on_0xC2[2]={0xC2,0x00};
static char cmd_on_0xD0[5]={0xD0,0x0F,0x0F,0x10,0x10};
static char cmd_on_0xD1[17]={0xD1,0x00,0x23,0x00,0x24,0x00,
				0x31,0x00,0x52,0x00,0x72,
				0x00,0xAE,0x00,0xDE,0x01,
				0x24};
static char cmd_on_0xD2[17]={0xD2,0x01,0x54,0x01,0x9F,0x01,
	            0xC8,0x02,0x08,0x02,0x35,
                0x02,0x36,0x02,0x5E,0x02,
	            0x83};
static char cmd_on_0xD3[17]={0xD3,0x02,0x98,0x02,0xAF,0x02,
				0xC0,0x02,0xD1,0x02,0xD8,
                0x02,0xF3,0x02,0xF8,0x03,
	            0x00};
static char cmd_on_0xD4[5]={0xD4,0x03,0x1C,0x03,0x52};
static char cmd_on_0xD5[17]={0xD5,0x00,0x23,0x00,0x24,0x00,
                0x31,0x00,0x52,0x00,0x72,
                0x00,0xAE,0x00,0xDE,0x01,
	            0x24};
static char cmd_on_0xD6[17]={0xD6,0x01,0x54,0x01,0x9F,0x01,
				0xC8,0x02,0x08,0x02,0x35,
				0x02,0x36,0x02,0x5E,0x02,
				0x83};
static char cmd_on_0xD7[17]={0xD7,0x02,0x98,0x02,0xAF,0x02,
				0xC0,0x02,0xD1,0x02,0xD8,
				0x02,0xF3,0x02,0xF8,0x03,
				0x00};
static char cmd_on_0xD8[5]={0xD8,0x03,0x1C,0x03,0x52};
static char cmd_on_0xD9[17]={0xD9,0x00,0x23,0x00,0x24,0x00,
				0x31,0x00,0x52,0x00,0x72,
				0x00,0xAE,0x00,0xDE,0x01,
				0x24};
static char cmd_on_0xDD[17]={0xDD,0x01,0x54,0x01,0x9F,0x01,
				0xC8,0x02,0x08,0x02,0x35,
				0x02,0x36,0x02,0x5E,0x02,
				0x83};
static char cmd_on_0xDE[17]={0xDE,0x02,0x98,0x02,0xAF,0x02,
				0xC0,0x02,0xD1,0x02,0xD8,
				0x02,0xF3,0x02,0xF8,0x03,
				0x00};
static char cmd_on_0xDF[5]={0xDF,0x03,0x1C,0x03,0x52};
static char cmd_on_0xE0[17]={0xE0,0x00,0x23,0x00,0x24,0x00,
				0x31,0x00,0x52,0x00,0x72,
				0x00,0xAE,0x00,0xDE,0x01,
				0x24};
static char cmd_on_0xE1[17]={0xE1,0x01,0x54,0x01,0x9F,0x01,
				0xC8,0x02,0x08,0x02,0x35,
				0x02,0x36,0x02,0x5E,0x02,
				0x83};
static char cmd_on_0xE2[17]={0xE2,0x02,0x98,0x02,0xAF,0x02,
				0xC0,0x02,0xD1,0x02,0xD8,
				0x02,0xF3,0x02,0xF8,0x03,
				0x00};
static char cmd_on_0xE3[5]={0xE3,0x03,0x1C,0x03,0x52};

static char cmd_on_0xE4[17]={0xE4,0x00,0x23,0x00,0x24,0x00,
				0x31,0x00,0x52,0x00,0x72,
				0x00,0xAE,0x00,0xDE,0x01,
				0x24};
static char cmd_on_0xE5[17]={0xE5,0x01,0x54,0x01,0x9F,0x01,
				0xC8,0x02,0x08,0x02,0x35,
				0x02,0x36,0x02,0x5E,0x02,
				0x83};
static char cmd_on_0xE6[17]={0xE6,0x98,0x02,0xAF,0x02,
				0xC0,0x02,0xD1,0x02,0xD8,
				0x02,0xF3,0x02,0xF8,0x03,
				0x00};
static char cmd_on_0xE7[5]={0xE7,0x03,0x1C,0x03,0x52};
static char cmd_on_0xE8[17]={0xE8,0x00,0x23,0x00,0x24,0x00,
				0x31,0x00,0x52,0x00,0x72,
				0x00,0xAE,0x00,0xDE,0x01,
				0x24};
static char cmd_on_0xE9[17]={0xE9,0x01,0x54,0x01,0x9F,0x01,
				0xC8,0x02,0x08,0x02,0x35,
				0x02,0x36,0x02,0x5E,0x02,
				0x83};
static char cmd_on_0xEA[17]={0xEA,0x02,0x98,0x02,0xAF,0x02,
				0xC0,0x02,0xD1,0x02,0xD8,
				0x02,0xF3,0x02,0xF8,0x03,
				0x00};
static char cmd_on_0xEB[5]={0xEB,0x03,0x1C,0x03,0x52};
static char cmd_on_0x3A[2]={0x3A,0x77};
static char cmd_on_0x36[2]={0x36,0x00};
static char cmd_on_0x35[2]={0x35,0x01}; // Alvin: TE on 0x00 -> 0x01
static char cmd_on_0x11[2]={0x11,0x00}; // Sleep-Out
static char cmd_on_0x29[2]={0x29,0x00}; // Display ON


/* Display On in Video Mode */
//TODO Not validated
#if 0
static char video_on_0x01[2]={0x01, 0x00}; // Sw reset
static char video_on_0x11[2]={0x11, 0x00}; // Exit sleep
static char video_on_0x29[2]={0x29, 0x00}; // Display On
static char video_on_0xAE[2]={0xae, 0x03}; // Set num of lanes
static char video_on_0x3A[2]={0x3A, 0x77}; // rgb888
static char video_on_0x53[2]={0x53, 0x24}; // led_pwm2
static char video_on_0x55[2]={0x55, 0x00}; // led_pwm3
#endif

/* Display Off */
static char display_off_0x28[2]={0x28, 0x00}; // Display Off
static char display_off_0x10[2]={0x10, 0x00}; // Sleep-In



static struct dsi_cmd_desc nt35516_video_on_cmds[] = {
#if 0
	{DTYPE_DCS_WRITE, 1, 0, 0, 50, sizeof(video_on_0x01), video_on_0x01},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(video_on_0x11), video_on_0x11},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(video_on_0x29), video_on_0x29},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10, sizeof(video_on_0xAE), video_on_0xAE},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10,	sizeof(video_on_0x3A), video_on_0x3A},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10,	sizeof(video_on_0x53), video_on_0x53},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10,	sizeof(video_on_0x55), video_on_0x55},
#endif
};

static struct dsi_cmd_desc nt35516_cmd_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xFF), cmd_on_0xFF},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xF2), cmd_on_0xF2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xF3), cmd_on_0xF3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xF0_0), cmd_on_0xF0_0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xB1_0), cmd_on_0xB1_0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xB6_0), cmd_on_0xB6_0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xB7_0), cmd_on_0xB7_0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xB8_0), cmd_on_0xB8_0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xBB), cmd_on_0xBB},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xBC_0), cmd_on_0xBC_0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xBD_0), cmd_on_0xBD_0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xC9), cmd_on_0xC9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xF0_1), cmd_on_0xF0_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xB0), cmd_on_0xB0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xB1_1), cmd_on_0xB1_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xB2), cmd_on_0xB2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xB3), cmd_on_0xB3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xB4), cmd_on_0xB4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xB6_1), cmd_on_0xB6_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xB7_1), cmd_on_0xB7_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xB8_1), cmd_on_0xB8_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xB9), cmd_on_0xB9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xBA), cmd_on_0xBA},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xBC_1), cmd_on_0xBC_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xBD_1), cmd_on_0xBD_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xBE), cmd_on_0xBE},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xC2), cmd_on_0xC2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xD0), cmd_on_0xD0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xD1), cmd_on_0xD1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xD2), cmd_on_0xD2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xD3), cmd_on_0xD3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xD4), cmd_on_0xD4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xD5), cmd_on_0xD5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xD6), cmd_on_0xD6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xD7), cmd_on_0xD7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xD8), cmd_on_0xD8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xD9), cmd_on_0xD9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xDD), cmd_on_0xDD},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xDE), cmd_on_0xDE},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xDF), cmd_on_0xDF},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xE0), cmd_on_0xE0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xE1), cmd_on_0xE1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xE2), cmd_on_0xE2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xE3), cmd_on_0xE3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xE4), cmd_on_0xE4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xE5), cmd_on_0xE5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xE6), cmd_on_0xE6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xE7), cmd_on_0xE7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xE8), cmd_on_0xE8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xE9), cmd_on_0xE9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xEA), cmd_on_0xEA},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0xEB), cmd_on_0xEB},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0x3A), cmd_on_0x3A},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0x36), cmd_on_0x36},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_on_0x35), cmd_on_0x35},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 120, sizeof(cmd_on_0x11), cmd_on_0x11},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 50, sizeof(cmd_on_0x29), cmd_on_0x29},
};

static struct dsi_cmd_desc nt35516_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_off_0x28), display_off_0x28},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(display_off_0x10), display_off_0x10}
};
#if 0
static char manufacture_id[2] = {0x04, 0x00}; /* DTYPE_DCS_READ */

static struct dsi_cmd_desc nt35516_manufacture_id_cmd = {
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(manufacture_id), manufacture_id};

static uint32 mipi_nt35516_manufacture_id(struct msm_fb_data_type *mfd)
{
	struct dsi_buf *rp, *tp;
	struct dsi_cmd_desc *cmd;
	uint32 *lp;

	tp = &nt35516_tx_buf;
	rp = &nt35516_rx_buf;
	cmd = &nt35516_manufacture_id_cmd;
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 3);
	lp = (uint32 *)rp->data;
	pr_info("%s: manufacture_id=%x\n", __func__, *lp);
	return *lp;
}
#endif



static int mipi_nt35516_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;
	struct msm_panel_info *pinfo;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	pinfo = &mfd->panel_info;
	//if (pinfo->is_3d_panel)
	//	support_3d = TRUE;

	mipi  = &mfd->panel_info.mipi;

	if (mipi->mode == DSI_VIDEO_MODE) {
		mipi_dsi_cmds_tx(&nt35516_tx_buf, nt35516_video_on_cmds,
				ARRAY_SIZE(nt35516_video_on_cmds));
	} else {
		mipi_dsi_cmds_tx(&nt35516_tx_buf, nt35516_cmd_on_cmds,
				ARRAY_SIZE(nt35516_cmd_on_cmds));

		/* clean up ack_err_status */
		mipi_dsi_cmd_bta_sw_trigger();
		//mipi_nt35516_manufacture_id(mfd);

	}

	return 0;
}

static int mipi_nt35516_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(&nt35516_tx_buf, nt35516_display_off_cmds,
			ARRAY_SIZE(nt35516_display_off_cmds));

	return 0;
}

DEFINE_LED_TRIGGER(bkl_led_trigger);

static char led_pwm1[2] = {0x51, 0x0};	/* DTYPE_DCS_WRITE1 */
static struct dsi_cmd_desc backlight_cmd = {
	DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(led_pwm1), led_pwm1};

struct dcs_cmd_req cmdreq_bl;

static void mipi_nt35516_set_backlight(struct msm_fb_data_type *mfd)
{
    pr_debug("%s: bl_level: %d\n", __func__, mfd->bl_level);
	if ((mipi_nt35516_pdata->enable_wled_bl_ctrl)
	    && (wled_trigger_initialized)) {
		led_trigger_event(bkl_led_trigger, mfd->bl_level);
		return;
	}

	led_pwm1[1] = (unsigned char)mfd->bl_level;

	cmdreq_bl.cmds = &backlight_cmd;
	cmdreq_bl.cmds_cnt = 1;
	cmdreq_bl.flags = 0;
	cmdreq_bl.rlen = 0;
	cmdreq_bl.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq_bl);
}

static int __devinit mipi_nt35516_lcd_probe(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;
	struct platform_device *current_pdev;
	static struct mipi_dsi_phy_ctrl *phy_settings;
	static char dlane_swap;

	if (pdev->id == 0) {
		mipi_nt35516_pdata = pdev->dev.platform_data;

		if (mipi_nt35516_pdata
			&& mipi_nt35516_pdata->phy_ctrl_settings) {
			phy_settings = (mipi_nt35516_pdata->phy_ctrl_settings);
		}

		if (mipi_nt35516_pdata
			&& mipi_nt35516_pdata->dlane_swap) {
			dlane_swap = (mipi_nt35516_pdata->dlane_swap);
		}

		return 0;
	}

	current_pdev = msm_fb_add_device(pdev);

	if (current_pdev) {
		mfd = platform_get_drvdata(current_pdev);
		if (!mfd)
			return -ENODEV;
		if (mfd->key != MFD_KEY)
			return -EINVAL;

		mipi  = &mfd->panel_info.mipi;

		if (phy_settings != NULL)
			mipi->dsi_phy_db = phy_settings;

		if (dlane_swap)
			mipi->dlane_swap = dlane_swap;
	}
	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_nt35516_lcd_probe,
	.driver = {
		.name   = "mipi_nt35516",
	},
};

static struct msm_fb_panel_data nt35516_panel_data = {
	.on		= mipi_nt35516_lcd_on,
	.off		= mipi_nt35516_lcd_off,
	.set_backlight = mipi_nt35516_set_backlight,
};


static int ch_used[3];

int mipi_nt35516_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_nt35516_lcd_init();
	if (ret) {
		pr_err("mipi_nt35516_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_nt35516", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	nt35516_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &nt35516_panel_data,
		sizeof(nt35516_panel_data));
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int mipi_nt35516_lcd_init(void)
{

	led_trigger_register_simple("bkl_trigger", &bkl_led_trigger);
	pr_info("%s: SUCCESS (WLED TRIGGER)\n", __func__);
	wled_trigger_initialized = 1;

	mipi_dsi_buf_alloc(&nt35516_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&nt35516_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}
