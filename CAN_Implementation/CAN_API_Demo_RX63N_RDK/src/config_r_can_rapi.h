/*******************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only 
* intended for use with Renesas products. No other uses are authorized. This 
* software is owned by Renesas Electronics Corporation and is protected under
* all applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT
* LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE 
* AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED.
* TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS 
* ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE 
* FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR
* ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE
* BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software
* and to discontinue the availability of this software. By using this software,
* you agree to the additional terms and conditions found by accessing the 
* following link:
* http://www.renesas.com/disclaimer *
* Copyright (C) 2012 Renesas Electronics Corporation. All rights reserved.    
*******************************************************************************/
/*******************************************************************************
* File Name    : config_r_can_api.h
* Version      : 2.00
* Description  : Exports the Renesas CAN API
*******************************************************************************/
/*******************************************************************************
* History : DD.MM.YYYY Version  Description
*         : 22.3.2010  1.00     For RX62N with new CAN API.
*         : 15.4.2010           R_CAN_Control 'Enter Sleep' added.
*                               R_CAN_TxStopMsg added.
*         : 15.06.2011          Added typedefs and defines to support multiple 
*                               CAN channel selection in RX630
*         : 08.12.2011  2.00    Added support for extended ID frame mode.
*         : 06.03.2012          Settings for YRDKRX63N
*******************************************************************************/

#ifndef R_CAN_API_CFG_H
#define R_CAN_API_CFG_H

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
#include "platform.h" 

/******************************************************************************
Typedef definitions
******************************************************************************/
/******************************************************************************
Macro definitions
******************************************************************************/
/* Set to 0 to use the CAN interrupts. Set to 1 if you want to poll CAN 
mailboxes for messages sent and received. */
#define USE_CAN_POLL        0   //1 or 0

/* Level */
#define CAN0_INT_LVL		2
#define CAN1_INT_LVL        2
#define CAN2_INT_LVL        2

/* Not implemented in the latest code releases to not cause confusion with the 
existing example code. If you have many mailboxes configured and alot of CAN 
traffic, you may want to consider using this feature. Please see the MCU 
hardware manual. */
#define USE_CAN_API_SEARCH  1

/* Frame ID mode: Standard ID, Extended ID, or Mixed ID mode */
/* Must choose 1 and only 1 from: */
//#define FRAME_ID_MODE   STD_ID_MODE
//#define FRAME_ID_MODE   EXT_ID_MODE
#define FRAME_ID_MODE   MIXED_ID_MODE

/*** Board specific ports ******************************************************
Map the transceiver control pins here. Tranceivers vary in the control pins present
and these may need I/O ports assigned to them. This example has "Enable" and
"Standby" control pins on its tranceiver.
If the board has more than one CAN port, then expand these defines as necessary */

/* Configure CAN0 STBn pin. 
Output. High = not standby. */
//#define CAN0_TRX_STB_PORT     ?
//#define CAN0_TRX_STB_PIN      ?
//#define CAN0_TRX_STB_LVL      1 //High = Not standby.

/* Configure CAN0 EN pin. Output. High to enable CAN transceiver. */
//#define CAN0_TRX_ENABLE_PORT  ?
//#define CAN0_TRX_ENABLE_PIN   ?
//#define CAN0_TRX_ENABLE_LVL   1 //High = Enable.

/* Configure CAN1 STBn pin. 
Output. High = not standby. */
//#define CAN1_TRX_STB_PORT       G
//#define CAN1_TRX_STB_PIN        0
//#define CAN1_TRX_STB_LVL        1 //High = Not standby.

/* Configure CAN1 EN pin. Output. High to enable CAN transceiver. */
//#define CAN1_TRX_ENABLE_PORT    G
//#define CAN1_TRX_ENABLE_PIN     1
//#define CAN1_TRX_ENABLE_LVL     1 //High = Enable.

/* Configure CAN2 STBn pin. 
Output. High = not standby. */
//#define CAN2_TRX_STB_PORT     ?
//#define CAN2_TRX_STB_PIN      ?
//#define CAN2_TRX_STB_LVL      1 //High = Not standby.

/* Configure CAN2 EN pin. Output. High to enable CAN transceiver. */
//#define CAN2_TRX_ENABLE_PORT  ?
//#define CAN2_TRX_ENABLE_PIN   ?
//#define CAN2_TRX_ENABLE_LVL   1 //High = Enable.


/*** RX and TX ports ******************************************************
/* The RX630 and RX63N may have up to 3 CAN channels depending on package.*/
/* Check RX Group User Manual for port availability for your device.      */

    /* Configure the CAN0 RX and TX pins to be used. */
#define CAN0_RX_PORT    P33
    //#define CAN0_RX_PORT    PD2
#define CAN0_TX_PORT    P32
    //#define CAN0_TX_PORT    PD1

    /* Configure the CAN1 RX and TX pins to be used. */
    //#define CAN1_RX_PORT    P15
    //#define CAN1_RX_PORT   P55
    //#define CAN1_TX_PORT    P14
    //#define CAN1_TX_PORT   P54

    /* Configure the CAN2 RX and TX pins if used. */
    //#define CAN2_RX_PORT    P67
    //#define CAN2_TX_PORT    P66


    /*** Baudrate settings ********************************************************
        Calculation of baudrate:
        ********************************
        *    PCLK = 48 MHz = fcan.     *
        *    fcanclk = fcan/prescale   *
        ********************************
    
        Example 1) 
        Desired baudrate 500 kbps.
        Selecting prescale to 4.
        fcanclk = 48000000/4
        fcanclk = 12000000 Hz
        Bitrate = fcanclk/Tqtot
        or,
        Tqtot = fcanclk/bitrate
        Tqtot = 12000000/500000
        Tqtot = 120/5 = 24.
        Tqtot = TSEG1 + TSEG2 + SS
        Using TSEG1 = 15 Tq
          TSEG2 = 8 Tq
          SS = 1 Tq always
          Re-synchronization Control (SJW) should be 1-4 Tq (must be <=TSEG2). 
    */
    #define CAN_BRP     4
    #define CAN_TSEG1   15
    #define CAN_TSEG2   8
    #define CAN_SJW		2
    
    /* 
    Example 2) Selecting prescale to 8.
    Desired baudrate 500 kbps.
    fcanclk = 48000000/8
    fcanclk = 6000000 Hz
    Tqtot = fcanclk/bitrate
    Tqtot = 6000000/500000
    Tqtot = 60/5 = 12.
    Tqtot = TSEG1 + TSEG2 + SS
    Using     TSEG1 = 8 Tq
            TSEG2 = 3 Tq
            SS = 1 Tq always
            SJW should be 1-4 Tq (<=TSEG2). *
    #define CAN_BRP     8
    #define CAN_TSEG1   8
    #define CAN_TSEG2   3
    #define CAN_SJW     1    */


/* Max loops to poll a CAN register bit for expected value. If you are using 
polled mode, and If you wish to wait a certain time to check that a mailbox 
has recieved a frame, increase this value. This can be set to a very low value,
but do not set to zero or the mailbox may not be chacded at all! */
#define MAX_CANREG_POLLCYCLES   5

#endif    /* R_CAN_API_CFG_H */
/* eof */