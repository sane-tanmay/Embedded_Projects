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
* File Name    : can_api_demo.h
* Version      : 1.00
* Description  : Header file for can_api_demo.c
*******************************************************************************/
/*******************************************************************************
* History : DD.MM.YYYY  Version     Description
*         : 05.03.2012  1.0         First port for YRDKRX63N    
*******************************************************************************/

#ifndef _API_DEMO_H   /* Multiple inclusion prevention. */
#define _API_DEMO_H

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
#include <stdint.h>
#include "r_can_api.h"
#include "platform.h"
#include "config_r_can_rapi.h"


/******************************************************************************
Typedef definitions
******************************************************************************/
/* Application error codes */

enum app_err_enum 
{    
    APP_NO_ERR 			= 0x00,
    APP_ERR_CAN_PERIPH 	= 0x01,
    APP_ERR_CAN_INIT 	= 0x02,
    APP_TABLE_ERR 		= 0x04,
    APP_STATE_ERR 		= 0x08
};

/* CAN channel numbers */
                  
enum CAN_channel_num 
{	
    CH_0 = 0,
    CH_1 = 1,
    CH_2 = 2
};

/******************************************************************************
Macro definitions
******************************************************************************/
/* TEST CAN ID */
#define 	TX_ID_DEFAULT 		0x001
#define 	RX_ID_DEFAULT 		0x001
#define 	REMOTE_TEST_ID		0x050

/* Mailboxes used for demo. Keep mailboxes 4 apart if you want masks 
independent - not affecting neighboring mailboxes. */
#define 	CANBOX_TX		    0x01	//Mailbox #1
#define 	CANBOX_RX 		    0x04	//Mailbox #4

/* Added to show how to use multiple mailboxes. */
#define 	CANBOX_REMOTE_RX	0x08	//Mailbox #8
#define 	CANBOX_REMOTE_TX	0x0C	//Mailbox #12


/******************************************************************************
Global variables and functions exported
******************************************************************************/
/* Data */

extern can_frame_t		g_tx_dataframe;
extern can_frame_t		g_rx_dataframe;
extern can_frame_t		g_remote_frame;

#if (USE_CAN_POLL == 0)
extern uint32_t				CAN0_tx_sentdata_flag;
extern uint32_t				CAN0_tx_err_sentdata_flag;
extern uint32_t				CAN0_rx_newdata_flag;
extern uint32_t				CAN0_rx_test_newdata_flag;
#endif 

extern enum app_err_enum	app_err_nr;

/* Functions */
uint32_t reset_all_errors(void);
void can_api_demo(void);
void delay(uint32_t);

#endif //API_DEMO_H
/* eof */
