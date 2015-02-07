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

static struct msm_panel_info pinfo;

static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
	/* DSI Bit Clock at 500 MHz, 2 lane, RGB888 */
	/* regulator */
	{0x03, 0x01, 0x01, 0x00},
	/* timing   */
/*[PLATFORM]-Mod-BEGIN by TCTNB.YJ, 2012/12/10, Modify the argument of the mipi video mode */	
	{0x8b, 0x33, 0x14, 0x00, 0x45, 0x4b, 0x19, 0x37,
	0x32, 0x03, 0x04},
/*[PLATFORM]-Mod-END by TCTNB.YJ*/
	/* phy ctrl */
	{0x7f, 0x00, 0x00, 0x00},
	/* strength */
	{0xbb, 0x02, 0x06, 0x00},
	/* pll control */
/*[PLATFORM]-Mod-BEGIN by TCTNB.YJ, 2012/12/10, Modify the argument of the mipi video mode */	
	{0x00, 0xed, 0x31, 0xd2, 0x00, 0x40, 0x37, 0x62,
/*[PLATFORM]-Mod-END by TCTNB.YJ*/
	0x01, 0x0f, 0x07,
	0x05, 0x14, 0x03, 0x0, 0x0, 0x0, 0x20, 0x0, 0x02, 0x0},
};

static int mipi_video_nt35510_wvga_pt_init(void)
{
	int ret;

	if (msm_fb_detect_client("mipi_video_nt35510_wvga"))
		return 0;

	pinfo.xres = 480;
	pinfo.yres = 800;
	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
/*[PLATFORM]-Mod-BEGIN by TCTNB.YJ, 2012/12/10, Modify the argument of the mipi video mode */	
	pinfo.lcdc.h_back_porch = 108;
	pinfo.lcdc.h_front_porch = 108;
	pinfo.lcdc.h_pulse_width = 8;
	pinfo.lcdc.v_back_porch = 10;
	pinfo.lcdc.v_front_porch = 10;
	pinfo.lcdc.v_pulse_width = 1;
/*[PLATFORM]-Mod-END by TCTNB.YJ*/
	pinfo.lcdc.border_clr = 0;	/* blk */
	pinfo.lcdc.underflow_clr = 0xff;	/* blue */
	/* number of dot_clk cycles HSYNC active edge is
	delayed from VSYNC active edge */
	pinfo.lcdc.hsync_skew = 0;
/*[PLATFORM]-Mod-BEGIN by TCTNB.YJ, 2012/12/10, Modify the argument of the mipi video mode */	
	pinfo.clk_rate = 499380000;
/*[PLATFORM]-Mod-END by TCTNB.YJ*/
	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;

	pinfo.mipi.mode = DSI_VIDEO_MODE;
	/* send HSA and HE following VS/VE packet */
	pinfo.mipi.pulse_mode_hsa_he = TRUE;
	pinfo.mipi.hfp_power_stop = TRUE; /* LP-11 during the HFP period */
	pinfo.mipi.hbp_power_stop = TRUE; /* LP-11 during the HBP period */
	pinfo.mipi.hsa_power_stop = TRUE; /* LP-11 during the HSA period */
	/* LP-11 or let Command Mode Engine send packets in
	HS or LP mode for the BLLP of the last line of a frame */
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	/* LP-11 or let Command Mode Engine send packets in
	HS or LP mode for packets sent during BLLP period */
	pinfo.mipi.bllp_power_stop = TRUE;

	pinfo.mipi.traffic_mode = DSI_BURST_MODE;
	pinfo.mipi.dst_format =  DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB; /* RGB */
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;

/*[PLATFORM]-Mod-BEGIN by TCTNB.YJ, 2012/12/10, Modify the argument of the mipi video mode */	
	pinfo.mipi.t_clk_post = 0x04;
	pinfo.mipi.t_clk_pre = 0x1f;
/*[PLATFORM]-Mod-END by TCTNB.YJ*/

	pinfo.mipi.stream = 0; /* dma_p */
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_NONE;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
/*[PLATFORM]-Mod-BEGIN by TCTNB.YJ, 2012/12/10, Modify the argument of the mipi video mode */	
	pinfo.mipi.frame_rate = 72; //67;//60; /* FIXME */ [PLATFORM]-MOd by TCTNB.LiuJie, 2012/11/10,FR-350422 ,[Google CTS]android.view:android.view.cts.DisplayRefreshRateTest Fail
/*[PLATFORM]-Mod-END by TCTNB.YJ*/

	pinfo.mipi.dsi_phy_db = &dsi_video_mode_phy_db;
	pinfo.mipi.dlane_swap = 0x01;
	/* append EOT at the end of data burst */
	pinfo.mipi.tx_eot_append = 0x01;

	ret = mipi_nt35510_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_WVGA_PT);

	if (ret)
		pr_err("%s: failed to register device!\n", __func__);

	return ret;
}

module_init(mipi_video_nt35510_wvga_pt_init);
