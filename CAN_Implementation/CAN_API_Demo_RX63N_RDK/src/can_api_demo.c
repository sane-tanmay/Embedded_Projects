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
* File Name	   : can_api_demo.c
* Version      : 1.0
* Device(s)	   : RX63N
* Tool-Chain   : RX Standard Toolchain 1.0.0
* H/W Platform : YRDKRX63N
* Description  : Demonstration of CAN receive and transmit using the CAN API.
*                CAN Baudrate: 500 kbps.
*                
* Operation	   : The demo can be run in three ways:
*                1) Program two boards and connect them together over the CAN 
*                   bus.
*                2) With CANPORT_TEST_1_INT_LOOPBACK used in the R_CAN_PortSet
*                   API you can communicate internally, no external bus needed!
*                3) Use a CAN bus monitor, e.g. SysTec's low-cost monitor 
*                   3204000, to send and receive frames to/from the demo. 
*                   Remote frames can also be demonstrated if CAN interrupts 
*                   are enabled. See last paragraph below.
*
*                OPERATION:
*                The demo transmits and receives frames with CAN-ID 1 by 
*                default. The default demo CAN-ID value is set in can_api_demo.h  
*                by g_tx_id_default.
*                
*                The software starts up by immediately sending ten test frames. 
*                This has two purposes, to check the link and to demonstrate 
*                how messages are sent back-to-back as quickly as possible.
*                
*                Press SW1 to send one CAN frame. The frame will be received 
*                and displayed by the other RSK as long as that board's receive
*                ID (RxID) matches the sending boards transmit ID (TxID).
*                
*                Press SW2 to display the current demo TxID on the LCD. To inc-
*                rement the TxID hold SW2 down and press SW3. The actual send 
*                command is invoked by the Sw1Func function.
*
*                Press SW3 to display current demo RxID on the LCD. To change 
*                RxID hold SW3 down and press SW2.
*
*                By default, polled CAN is used. To use the CAN interrupts for 
*                faster processing, uncomment USE_CAN_POLL in file 
*                config_r_can_rapi.h.
*
*                REMOTE frames:
*                Besides demonstrating Tx and Rx of Standard CAN frames, the 
*                demo will also send remote frame responses for remote frame 
*                requests received by the mailbox at CAN-ID 50h (defined by
*                REMOTE_TEST_ID in can_api_demo.h).
*                Remote frames demo is only done ininterrupt mode:   
*                "#define USE_CAN_POLL = 0" set in the CAN API config file. 
*                Remote requests are not sent by this demo as it is, and so must
*                come from an outside source, e.g. the CAN monitor mentioned 
*                above. The external CAN source must be set to send remote frame
*                requests to CAN-ID 50h.
*
*******************************************************************************/
/*******************************************************************************
* History : DD.MM.YYYY Version  Description
*         : 05.03.2012   1.0    First release. Ported to YRDKRX63N from various
*                               prior sources.
*******************************************************************************/

/*******************************************************************************
Includes	<System Includes> , "Project Includes"
*******************************************************************************/
#include <machine.h>
#include <stdint.h>
#include <stdio.h>

#include "platform.h"
#include "config_r_can_rapi.h"
#include "can_api_demo.h"
#include "switches.h"


/*******************************************************************************
Macro definitions
*******************************************************************************/
#define LCD_DELAY               0x00400000
#define NR_STARTUP_TEST_FRAMES	10
#define MAX_CHANNELS 3  /* RX63x */

/* Pick only ONE demo testmode below by uncommenting the macro definition. */ 
#define DEMO_NORMAL               1
//#define DEMO_TEST_1_INT_LOOPBACK    1
//#define DEMO_TEST_0_EXT_LOOPBACK  1
//#define DEMO_TEST_LISTEN_ONLY     1


/******************************************************************************
Exported global variables (to be accessed by other files)
******************************************************************************/
/* Demo data */
can_frame_t		g_tx_dataframe;
can_frame_t		g_rx_dataframe;
can_frame_t		g_remote_frame;

/* the CAN peripheral channel used in this demo */
uint32_t g_can_channel;

#if TEST_FIFO
can_frame_t	    tx_fifo_dataframe;
uint8_t         tx_fifo_flag = 0;
#endif


/* Demo flags. */
#if (USE_CAN_POLL == 0)
uint32_t			CAN0_tx_sentdata_flag = 0;
uint32_t			CAN0_tx_remote_sentdata_flag = 0;
uint32_t			CAN0_rx_newdata_flag = 0;
uint32_t			CAN0_rx_test_newdata_flag = 0;
uint32_t			CAN0_rx_g_remote_frame_flag = 0;
#endif 

enum app_err_enum	app_err_nr;

/* Functions */
void lcd_flash(void);

/******************************************************************************
Private global variables and functions
******************************************************************************/

/* Errors. Peripheral and bus errors. Space for each channel. */
static uint32_t		error_bus_status[MAX_CHANNELS];
static uint32_t     error_bus_status_prev[MAX_CHANNELS];
static uint32_t		can_state[MAX_CHANNELS];
static uint32_t		nr_times_reached_busoff[MAX_CHANNELS];

/* TEST CAN ID */
uint32_t g_tx_id_default;
uint32_t g_rx_id_default;

/* Functions */
static uint32_t init_can_app(void);
static void check_can_errors(void);
static void handle_can_bus_state(uint8_t ch_nr);

#if (USE_CAN_POLL == 1)
static void can_poll_demo(void);
#else 
static void can_int_demo(void);
#endif 


/*****************************************************************************
* Function name:    can_api_demo
* Description  : 	Main Can API demo program
* Arguments    :    none
* Return value : 	none
*****************************************************************************/
void can_api_demo(void)
{
    uint32_t i;
    uint32_t api_status = R_CAN_OK;
    uint8_t     disp_buf[13] = {0}; /* Temporary storage for display strings. */   

    g_can_channel = CH_0; /* using CAN channel 0 for this demo */

    /* Set default mailbox IDs for the demo*/
    if (FRAME_ID_MODE == STD_ID_MODE)
    {
        g_tx_id_default = 0x001;
        g_rx_id_default = 0x001;    
    }
    else
    {
        g_tx_id_default = 0x000A0001;
        g_rx_id_default = 0x000A0001;    
    }

    /* Init CAN. */
    api_status = R_CAN_Create(g_can_channel);
    
    if (api_status != R_CAN_OK)
    {   /* An error at this stage is fatal to demo, so stop here. */
        //sprintf((char *) disp_buf, "API err:%02X  ", api_status);
        //lcd_display(LCD_LINE8, disp_buf); 
                       
        while(1)
        {
            nop();/* Wait here and leave error displayed. */
        } 
    } 
    
    /***************************************************************************
    * Pick ONE R_CAN_PortSet call below by uncommenting the matching macro  
    * in the Macro definitions section above.
    ***************************************************************************/  
    /* Normal CAN bus usage. */
    #if DEMO_NORMAL
    R_CAN_PortSet(g_can_channel, ENABLE);
    /* Test modes. With Internal loopback mode you only need one board! */
    #elif DEMO_TEST_1_INT_LOOPBACK
    R_CAN_PortSet(g_can_channel, CANPORT_TEST_1_INT_LOOPBACK);
    #elif DEMO_TEST_0_EXT_LOOPBACK
    R_CAN_PortSet(g_can_channel, CANPORT_TEST_0_EXT_LOOPBACK);
    #elif DEMO_TEST_LISTEN_ONLY
    R_CAN_PortSet(g_can_channel, CANPORT_TEST_LISTEN_ONLY);
    #endif

    /* Initialize CAN mailboxes. */
    api_status |= init_can_app();

    /* Is all OK after all CAN initialization? */
    if (api_status != R_CAN_OK)
    {
        api_status = R_CAN_OK;
        app_err_nr = APP_ERR_CAN_INIT;
    }

    /* Interrupt Enable flag is set by default. */

    /*****************************************************************************/
    /* This is how you send multiple messages back to back. Sending 10 messages. */
    /*****************************************************************************/
    if (FRAME_ID_MODE == STD_ID_MODE )
    {
        api_status |= R_CAN_TxSet(g_can_channel, CANBOX_TX, &g_tx_dataframe, DATA_FRAME);   
    }
    else
    {
        api_status |= R_CAN_TxSetXid(g_can_channel, CANBOX_TX, &g_tx_dataframe, DATA_FRAME);
    }

    #if (USE_CAN_POLL == 1)
    while (R_CAN_TxCheck(g_can_channel, CANBOX_TX))
    {
        /* Poll loop. A real application should provide for timeout. */
    }

    for (i = 0; i < NR_STARTUP_TEST_FRAMES; i++)
    {
        api_status |= R_CAN_Tx(g_can_channel, CANBOX_TX);

        while (R_CAN_TxCheck(g_can_channel, CANBOX_TX))
        {
            /* Poll loop. A real application should provide for timeout. */
        }
    }
    
    #else	/* Using CAN interrupts. */
    while (0 == CAN0_tx_sentdata_flag)
    {
        /* Poll loop. Flag set by hardware. A real application should provide for timeout. */
    }

    CAN0_tx_sentdata_flag = 0;

    for (i = 0; i < NR_STARTUP_TEST_FRAMES; i++)
    {
        api_status |= R_CAN_Tx(g_can_channel, CANBOX_TX);

        while (0 == CAN0_tx_sentdata_flag)
        {
            /* Poll loop. Flag set by hardware. A real application should provide for timeout. */
        }

        CAN0_tx_sentdata_flag = 0; /* Clear the flag for next loop iteration. */
    }
    #endif

    /*	M A I N	L O O P	* * * * * * * * * * * * * * * * * * * * * * * * * */	
    while(1)
    {
        /* User pressing switch(es)? */
        read_switches();

        check_can_errors();

        if (can_state[0] != R_CAN_STATUS_BUSOFF)
        {
            #if (USE_CAN_POLL == 1)
            can_poll_demo();
            #else
            can_int_demo();
            #endif 
        }
        else
            /* Bus Off. */
        {
            lcd_display(LCD_LINE7, "App in	");
            lcd_display(LCD_LINE8, "Bus Off ");

            /* handle_can_bus_state() will restart app. */
            lcd_flash();
        }

        /* Reset receive/transmit indication. */
        //LED6 = LED_OFF;
        //LED7 = LED_OFF;
    }
}/* End function main(). */


#if (USE_CAN_POLL == 1)
/*****************************************************************************
* Function name:    can_poll_demo
* Description  : 	POLLED CAN demo version
* Arguments    :    none
* Return value : 	none
*****************************************************************************/
static void can_poll_demo(void)
{
    uint32_t	api_status = R_CAN_OK;
    uint8_t     disp_buf[13] = {0}; /* Temporary storage for display strings. */    

    /*** TRANSMITTED any frames? */
    api_status = R_CAN_TxCheck(g_can_channel, CANBOX_TX);
/*
    if (R_CAN_OK == api_status)
    {
        LED6 = LED_ON;		
        lcd_display(LCD_LINE7, "TxChk OK");

        sprintf((char *)disp_buf, "%02X%02X%02X%02X",  
            g_tx_dataframe.data[0], 
            g_tx_dataframe.data[1],
            g_tx_dataframe.data[2], 
            g_tx_dataframe.data[3] );

        lcd_display(LCD_LINE8, disp_buf);                          
        lcd_flash();
    }*/

    /* Since we are always polling for transmits, api_status for R_CAN_TxCheck 
      will most often be other than R_CAN_OK, don't show that in this demo.*/

    /*** RECEIVED any frames? */
    api_status = R_CAN_RxPoll(g_can_channel, CANBOX_RX);

   /* if (R_CAN_OK == api_status)
    {
        LED5 = LED_ON;
        lcd_display(LCD_LINE7, "Rx Poll OK");        
        lcd_flash();

        // Read CAN data and show. 
        api_status = R_CAN_RxRead(g_can_channel, CANBOX_RX, &g_rx_dataframe);
        
        lcd_display(LCD_LINE7, "Rx Read: ");

        sprintf((char *)disp_buf, "%02X%02X%02X%02X",  
            g_rx_dataframe.data[0], 
            g_rx_dataframe.data[1],
            g_rx_dataframe.data[2], 
            g_rx_dataframe.data[3] );

        lcd_display(LCD_LINE8, disp_buf);               
        lcd_flash();

        

        LED5 = LED_OFF;		
    } */
	
}/* End function can_poll_demo() */

#else

/*****************************************************************************
* Function name:    can_int_demo
* Description  : 	INTERRUPT driven CAN demo version
* Arguments    :    none
* Return value : 	none
*****************************************************************************/
static void can_int_demo(void)
{
    uint32_t	api_status = R_CAN_OK;
    uint8_t     disp_buf[13] = {0}; /* Temporary storage for display strings. */ 

    /************************************************************************
    * Using CAN INTERRUPTS.													*
    * See also r_can_api.c for the ISR example.								*
    *************************************************************************/
    /* TRANSMITTED any frames? If flag can0_tx_sentdata_flag was set by the 
    CAN Tx ISR, the frame was sent successfully. */
    if (CAN0_tx_sentdata_flag)
    {
        CAN0_tx_sentdata_flag = 0; /* Clear the flag for next time. */
/*        LED6 = LED_ON;

        // Show CAN frame was sent. 
        lcd_display(LCD_LINE7, "Tx OK   ");

        sprintf((char *)disp_buf, "%c%c%c%c",  
            g_tx_dataframe.data[0], 
            g_tx_dataframe.data[1],
            g_tx_dataframe.data[2], 
            g_tx_dataframe.data[3] );

//        lcd_display(LCD_LINE8, disp_buf);
//        lcd_flash();
*/    }

    if (CAN0_tx_remote_sentdata_flag)
    {
        CAN0_tx_remote_sentdata_flag = 0;
       // lcd_display(LCD_LINE7, "TxRemote"); 
        //lcd_flash();
    }					

    /*** RECEIVED any frames? Only need to check if flag is set by CAN Rx ISR.
    Will only receive own frames in CAN port test modes 0 and 1. */
    if (CAN0_rx_newdata_flag)
    {
        CAN0_rx_newdata_flag = 0;
        LED4 = LED_ON;

        //lcd_display(LCD_LINE1, "Rx OK. Read:"); 

        /* Read CAN data. */
        api_status = R_CAN_RxRead(g_can_channel, CANBOX_RX, &g_rx_dataframe);

        /* Format read-data bytes into character string. */
        sprintf((char *)disp_buf, "%c%c%c",  
            g_rx_dataframe.data[0], 
            g_rx_dataframe.data[1],
            g_rx_dataframe.data[2] );

        /* Display the formatted string. */                    
        lcd_display(LCD_LINE2, disp_buf);

        /* Clear the displayed data after a pause. */         
//        lcd_flash();

        /* Display error, if any. */
        

        LED4 = LED_OFF;
    }

    if (CAN0_rx_test_newdata_flag)
    {
        CAN0_rx_test_newdata_flag = 0;
        LED4 = LED_ON;
        lcd_display(LCD_LINE6, "Rx Test"); 
        lcd_flash();
    }

    /* Set up remote reply if remote request came in. */
    if (1 == CAN0_rx_g_remote_frame_flag)
    {
        CAN0_rx_g_remote_frame_flag = 0;
        g_remote_frame.data[0]++;
        
        if (FRAME_ID_MODE == STD_ID_MODE )
        {
            R_CAN_TxSet(g_can_channel, CANBOX_REMOTE_TX, &g_remote_frame, DATA_FRAME);   
        }
        else
        {
            R_CAN_TxSetXid(g_can_channel, CANBOX_REMOTE_TX, &g_remote_frame, DATA_FRAME);             
        }    
    }

    LED4 = LED_OFF;

}/* End function can_int_demo(). */

#endif  /* USE_CAN_POLL == */


/*****************************************************************************
* Function name:    init_can_app
* Description  : 	Initialize CAN demo application
* Arguments    :    none
* Return value : 	none
*****************************************************************************/
static uint32_t init_can_app(void)
{	
    uint32_t	api_status = R_CAN_OK;
    uint32_t    i; /* Common loop index variable. */

    can_state[0] = R_CAN_STATUS_ERROR_ACTIVE;

    for (i = 0; i < MAX_CHANNELS; i++) /* Initialize status for all channels. */
    {
        error_bus_status[i] = R_CAN_STATUS_ERROR_ACTIVE;
        error_bus_status_prev[i] = R_CAN_STATUS_ERROR_ACTIVE;            
    }
    
    /* Configure mailboxes in Halt mode. */
    api_status |= R_CAN_Control(g_can_channel, HALT_CANMODE);

    /********	Init demo to recieve data	********/	
    /* Use API to set one CAN mailbox for demo receive. */
    /* Standard id. Choose value 0-0x07FF (2047). */
    if (FRAME_ID_MODE == STD_ID_MODE)
    { 
        api_status |= R_CAN_RxSet(g_can_channel, CANBOX_RX, g_rx_id_default, DATA_FRAME);
        
        /* Mask for receive box. Write to mask only in Halt mode. */
        /* 0x7FF = no mask. 0x7FD = mask bit 1, for example; If receive ID is set to 1, both
           ID 1 and 3 should be received. */
        R_CAN_RxSetMask( g_can_channel, CANBOX_RX, 0x7FF);           
    }
    else
    {
        api_status |= R_CAN_RxSetXid(g_can_channel, CANBOX_RX, g_rx_id_default, DATA_FRAME);
        
        /* Mask for receive box. Write to mask only in Halt mode. */
        /* 0x1FFFFFFF = no mask. 0x1FFFFFFD = mask bit 1, for example; If receive ID is set to 1, both
           ID 1 and 3 should be received. */
        R_CAN_RxSetMask( g_can_channel, CANBOX_RX, 0x1FFFFFFD);                  
    }

    /********	Init. demo Tx dataframe RAM structure	********/	
    /* Standard id. Choose value 0-0x07FF (2047). */
    g_tx_dataframe.id		=	g_tx_id_default;
    g_tx_dataframe.dlc		=	8;
    g_tx_dataframe.data[0]	=	0x00;
    g_tx_dataframe.data[1]	=	0x11;
    g_tx_dataframe.data[2]	=	0x22;
    g_tx_dataframe.data[3]	=	0x33;
    g_tx_dataframe.data[4]	=	0x44;
    g_tx_dataframe.data[5]	=	0x55;
    g_tx_dataframe.data[6]	=	0x66;
    g_tx_dataframe.data[7]	=	0x77;

    /* API to send will be set up in SW1Func() in file switches.c. */
    api_status |= R_CAN_Control(g_can_channel, OPERATE_CANMODE);

    /*************** Init. remote dataframe response **********************/
    g_remote_frame.id = REMOTE_TEST_ID;

    /* Length is specified by the remote request. */
    /* Stuff with some data.. */
    for (i = 0; i < 8; i++)
    {
        g_remote_frame.data[i] = i;
    }    
    
    /* Prepare mailbox for Tx. */    	
    if (FRAME_ID_MODE == STD_ID_MODE)
    {
        R_CAN_RxSet(g_can_channel, CANBOX_REMOTE_RX, REMOTE_TEST_ID, REMOTE_FRAME);
    }
    else
    {
        R_CAN_RxSetXid(g_can_channel, CANBOX_REMOTE_RX, REMOTE_TEST_ID, REMOTE_FRAME);        
    }
    /***********************************************************************/

    /* Set frame buffer id so LCD shows correct receive ID from start. */
    g_rx_dataframe.id = g_rx_id_default;

    return api_status;

} /* End function init_can_app(). */


/*****************************************************************************
* Function name:    check_can_errors
* Description  : 	Check for all possible errors, in app and peripheral. Add 
*				    checking for your app here.
* Arguments    :    none
* Return value : 	none
*****************************************************************************/
static void check_can_errors(void)
{
    uint8_t disp_buf[13] = {0}; /* Temporary storage for display strings. */

    /* Error passive or more? */
    handle_can_bus_state(g_can_channel);

    if (app_err_nr)
    {
        /* Show error to user */
        /* RESET ERRORs with SW1. */
        LED7 = LED_ON;

        lcd_display(LCD_LINE7,"App err");      
        sprintf((char *)disp_buf, "    %02X", app_err_nr);
        lcd_display(LCD_LINE8, disp_buf);        
        lcd_flash();

        LED7 = LED_OFF;
    }
    
}/* End function check_can_errors(). */


/*****************************************************************************
* Function name:    handle_can_bus_state
* Description  : 	Check CAN peripheral bus state.
* Arguments    :    Bus number, 0 or 1.
* Return value : 	none
*****************************************************************************/
static void handle_can_bus_state(uint8_t ch_nr)
{
    can_frame_t err_tx_dataframe;
    uint8_t disp_buf[13] = {0}; /* Temporary storage for display strings. */

    /* Has the status register reached error passive or more? */
    if (ch_nr < MAX_CHANNELS)
    {
        error_bus_status[ch_nr] = R_CAN_CheckErr(ch_nr);
    }
    else
    {
        return;
    }

    /* Tell user if CAN bus status changed.
    All Status bits are read only. */
    if (error_bus_status[ch_nr] != error_bus_status_prev[ch_nr])
    {	
        switch (error_bus_status[ch_nr])
        {
            /* Error Active. */
            case R_CAN_STATUS_ERROR_ACTIVE:

                /* Only report if there was a previous error. */
                if (error_bus_status_prev[ch_nr] > R_CAN_STATUS_ERROR_ACTIVE)
                {
                    if (CH_0 == ch_nr)
                    {
                        lcd_display(LCD_LINE6, "Bus0: OK");	 
                    }
                    else
                    {
                        lcd_display(LCD_LINE6, "Bus1: OK");                       
                    }
                    
                    delay(0x400000);    /* Allow user time to see display. */
                    lcd_display(LCD_LINE6, "            ");	 /* Now clear it. */                   
                }

                /* Restart if returned from Bus Off. */
                if (R_CAN_STATUS_BUSOFF == error_bus_status_prev[ch_nr])
                {
                    /* Restart CAN */
                    if (R_CAN_OK != R_CAN_Create(ch_nr))
                    {
                        app_err_nr |= APP_ERR_CAN_PERIPH;
                    }

                    /* Restart CAN demos even if only one channel failed. */
                    init_can_app();
                }
            break;	

            /* Error Passive. */
            case R_CAN_STATUS_ERROR_PASSIVE:
            /* Continue into Busoff case to display. */

            case R_CAN_STATUS_BUSOFF:
             /* Bus Off. */
             
            default:
                if (CH_0 == ch_nr)
                {
                   // sprintf((char *)disp_buf, "bus0: %02X", error_bus_status[ch_nr]);                 
                   // lcd_display(LCD_LINE6, disp_buf);
                }
                else if (CH_1 == ch_nr)
                {
                    //sprintf((char *)disp_buf, "bus1: %02X", error_bus_status[ch_nr]);                 
                   // lcd_display(LCD_LINE6, disp_buf);
                }
                else if (CH_2 == ch_nr)
                {
                   // sprintf((char *)disp_buf, "bus2: %02X", error_bus_status[ch_nr]);                 
                   // lcd_display(LCD_LINE6, disp_buf);
                }
                else
                {
                    /* No else. */
                }

                delay(0x400000);
                nr_times_reached_busoff[ch_nr]++;
            break;
        }
        
        error_bus_status_prev[ch_nr] = error_bus_status[ch_nr];

        /* Transmit CAN bus status change */
        err_tx_dataframe.id = 0x700 + ch_nr;
        err_tx_dataframe.dlc =	1;
        err_tx_dataframe.data[0] = error_bus_status[ch_nr];

        /* Send Error state on both channels. Maybe at least one is up. 
        Warning: If CAN1 and CAN1 are connected to eachother, they will try to
        send practically simultaneously. Let this be a lesson; sending the same 
        ID from two nodes onto the same bus at the same time is very hazardous
        as the arbitration cannot take place. Only use both lines below if 
        CAN1 and CAN1 are on different buses. */
        if (FRAME_ID_MODE == STD_ID_MODE )
        {
            R_CAN_TxSet(g_can_channel, CANBOX_TX, &err_tx_dataframe, DATA_FRAME);   
        }
        else
        {
            R_CAN_TxSetXid(g_can_channel, CANBOX_TX, &err_tx_dataframe, DATA_FRAME);
        } 

    }

}/* End function handle_can_bus_state() */

/*******************************************************************************
* Function name:    reset_all_errors
* Description  : 	Reset all types of errors, application and CAN peripeheral errors.
* Arguments    :    none
* Return value : 	CAN API code
*******************************************************************************/
uint32_t reset_all_errors(void)
{		
    uint32_t status = 0;

    /* Reset errors */
    app_err_nr = APP_NO_ERR;

    error_bus_status[0] = 0;
    error_bus_status[1] = 0;

    /* You can chooose to not reset error_bus_status_prev; if there was an error, 
    keep info to signal recovery */
    error_bus_status_prev[0] = 0; 
    error_bus_status_prev[1] = 0;

    nr_times_reached_busoff[0] = 0;
    nr_times_reached_busoff[1] = 0;

    /* Reset Error Judge Factor and Error Code registers */
    CAN0.EIFR.BYTE /*= CAN0.EIFR.BYTE*/ = 0;

    /* Reset CAN0 Error Code Store Register (ECSR). */
    CAN0.ECSR.BYTE = 0;

    /* Reset CAN0 Error Counters. */
    CAN0.RECR = 0;
    CAN0.TECR = 0;

    return status;
}/* End function reset_all_errors() */


/*******************************************************************************
* Function name:    delay
* Description  :    Demo delay
* Arguments    :    32-bit delay count value
* Return value : 	none
*******************************************************************************/
#pragma noinline(delay)
void delay(uint32_t n)
{
    uint32_t i;

    for(i = 0; i < n; i++)
    {
        nop();
    }

}/* End function delay() */


/*******************************************************************************
* Function name:    lcd_flash
* Description  :    Erases the frequently updated display area after a delay.
* Arguments    :    none
* Return value :    none
*******************************************************************************/
void lcd_flash(void)
{
    delay(LCD_DELAY); 
    lcd_display(LCD_LINE7, "            ");				 
    lcd_display(LCD_LINE8, "            "); 

} /* End function lcd_flash(). */


/*******************************************************************************
CAN INTERRRUPTS 
Interrupts are to be duplicated for each CAN channel used except for the Error 
interrupt which handles all channels in a group. 
Vectors are set according to the channel.
*******************************************************************************/
#if (USE_CAN_POLL == 0)
/*****************************************************************************
* Function name:    CAN0_TXM0_ISR
* Description  :    CAN0 Transmit interrupt. Check which mailbox transmitted 
*                   data and process it.	
* Arguments    :    N/A
* Return value :    N/A

*****************************************************************************/
#pragma interrupt CAN0_TXM0_ISR(vect=VECT_CAN0_TXM0, enable) 
void CAN0_TXM0_ISR(void)
{
    uint32_t api_status = R_CAN_OK;

    api_status = R_CAN_TxCheck(CH_0, CANBOX_TX);

    if (R_CAN_OK == api_status)
    {
        CAN0_tx_sentdata_flag = 1;
    }

    api_status = R_CAN_TxCheck(CH_0, CANBOX_REMOTE_TX);

    if (R_CAN_OK == api_status)
    {
        CAN0_tx_remote_sentdata_flag = 1;
    }

    /* Use mailbox search reg. Should be faster than above if a lot of mailboxes to check. 
    Not verified. */
}/* end CAN0_TXM0_ISR() */


/*****************************************************************************
* Function name:    CAN0_RXM0_ISR
* Description  :    CAN0 Receive interrupt.
*   				Check which mailbox received data and process it.
* Arguments    :    N/A
* Return value :    N/A
*****************************************************************************/
#pragma interrupt CAN0_RXM0_ISR(vect=VECT_CAN0_RXM0, enable)
void CAN0_RXM0_ISR(void)
{
    /* Use CAN API. */
    uint32_t api_status = R_CAN_OK;

    api_status = R_CAN_RxPoll(CH_0, CANBOX_RX);

    if (R_CAN_OK == api_status)
    {
        CAN0_rx_newdata_flag = 1;
    }

    api_status = R_CAN_RxPoll(CH_0, CANBOX_REMOTE_RX);

    if (R_CAN_OK == api_status)
    {
        /* REMOTE_FRAME FRAME REQUEST RECEIVED */
        /* Do not set BP on the next line to check for Remote frame. By the time you 
        continue, the recsucc flag will already have changed to be a trmsucc flag in 
        the CAN status reg. */

        /* Reset of the receive/transmit flag in the MCTL register will be done by 
        set_remote_reply_std_CAN0(). */

        /* Set flag to inform application. */
        CAN0_rx_g_remote_frame_flag = 1;

        g_remote_frame.dlc = (uint8_t)(CAN0.MB[CANBOX_REMOTE_RX].DLC);		

        /* Reset NEWDATA flag since we won't be reading the mailbox. */
        CAN0.MCTL[CANBOX_REMOTE_RX].BIT.RX.NEWDATA = 0;
    }

    /* Use mailbox search reg. Should be faster if a lot of mailboxes to check. */

}/* end CAN0_RXM0_ISR() */

/*****************************************************************************
* Function name:    CAN_ERS_ISR
* Description  :    CAN Group Error interrupt.
*   				Check which CAN channel is source of interrupt
* Arguments    :    N/A
* Return value :    N/A
*****************************************************************************/
#pragma interrupt	CAN_ERS_ISR(vect=VECT_ICU_GROUPE0, enable)
void CAN_ERS_ISR(void)
{
    /* Error interrupt can have multiple sources. Check interrupt flags to id source. */
    if (IS(CAN0, ERS0))
    {
        LED7 = LED_ON;		/*TODO: additional error handling/cause identification */
        CLR(CAN0, ERS0) = 1;	/* clear interrupts */         
    }
    if (IS(CAN1, ERS1))
    {
        LED7 = LED_ON;		/*TODO: additional error handling/cause identification */	
        CLR(CAN1, ERS1) = 1;	/* clear interrupts */			
    }
    if (IS(CAN2, ERS2))
    {
        LED7 = LED_ON;		/*TODO: additional error handling/cause identification */		
    }

    nop();
}/* end CAN_ERS0_ISR() */

#endif /* USE_CAN_POLL == 0 */

/* eof */