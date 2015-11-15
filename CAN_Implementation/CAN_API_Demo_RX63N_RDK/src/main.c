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
* File Name    : main.c
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
*         : 05.03.2012         Ported to YRDKRX63N from RSK RX63N
*******************************************************************************/

/*******************************************************************************
Includes	<System Includes> , "Project Includes"
*******************************************************************************/
#include <machine.h>
#include <stdint.h>

#include "platform.h"
#include "switches.h"
#include "can_api_demo.h"

/*******************************************************************************
Macro definitions
*******************************************************************************/
#define LED_DELAY   0x00080000 /* Adjust this to set the LED blinking rate. */

/*******************************************************************************
Private global variables and functions
*******************************************************************************/
/* Functions */
static void blink_leds(uint32_t nr_led_flashes);


/*******************************************************************************
* Function name:    main
* Description  : 	main function for Can API demo program. Initializes the LCD,
*                   flashes some LEDs, then calls the Can API demo program.
* Arguments    :    none
* Return value : 	none
*******************************************************************************/
void main(void)
{
    /* Setup LCD. */
    lcd_initialize();

    /* Display splash screen. */
   // lcd_display(LCD_LINE1, "Lab 2 ");
    //lcd_display(LCD_LINE4, "Sean");
    //lcd_display(LCD_LINE5, "Tanmay");
    //lcd_display(LCD_LINE4, "CAN Demo");

    /* Blink board LEDs. */
    blink_leds(20);

    switches_initialize(); 

    /* Run the CAN API Demo. */
    can_api_demo();
    
} /* End function main(). */


/*******************************************************************************
* Function name:    blink_leds
* Description  : 	Blinks some of the board LEDs a specified number of times.
* Arguments    :    none
* Return value : 	none
*******************************************************************************/
void blink_leds(uint32_t nr_led_flashes)
{
    uint32_t i = LED_DELAY;
    uint32_t t;

    LED4 = LED_ON;
    LED5 = LED_ON;
    LED6 = LED_ON;
    LED7 = LED_ON;

    for (t = 0; t < nr_led_flashes; t++)
    {
        while(i--)
        {
            /* Empty spin loop. */
        }

        i = LED_DELAY;

        LED4 ^= 1; /* Toggle the LED state. */
        LED5 ^= 1;
        LED6 ^= 1;
        LED7 ^= 1;
    }
    
    /* Make sure all the LEDs end up in the off state. */
    LED4 = LED_OFF;
    LED5 = LED_OFF;
    LED6 = LED_OFF;
    LED7 = LED_OFF;
    
} /* End function blink_leds(). */

/* eof */