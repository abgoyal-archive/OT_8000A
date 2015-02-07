
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
/* Comments: This is ncp1851 driver interface for msm_battery.c               */
/*                                                                            */
/* File    : kernel/drivers/power/ncp1851.h                                   */
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

/******************************************************************************/



u8 NCPRX(u8 reg);
void NCPTX(u8 reg, u8 val);

void start_usb_pc_charging(void);
void start_usb_wall_charging(void);
void stop_charging(void);
void restart_charging(void);
bool is_battery_full(void);
