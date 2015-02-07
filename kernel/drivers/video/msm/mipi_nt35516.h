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
/* Comments: This is headfile of the driver for NT35516(qHD)                  */
/*                                                                            */
/* File    : kernel/drivers/video/msm/mipi_nt35516.h                          */
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

#ifndef __MIPI_NT35516_H__
#define __MIPI_NT35516_H__

#define NOVATEK_TWO_LANE

int mipi_nt35516_device_register(struct msm_panel_info *pinfo,
                u32 channel, u32 panel);

#endif  /* __MIPI_NT35516_H__ */
