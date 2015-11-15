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
* File Name	    : switches.c
* Version	    :	
* Tool-Chain    : RX Standard Toolchain 1.0.0
* Platform	    : YRDKRX63N
* Description   : Switch functions for CAN API demo.
*
*******************************************************************************/
/*******************************************************************************
* History : DD.MM.YYYY Version Description
*           05.03.2012         First port to YRDKRX63N
*******************************************************************************/

/*******************************************************************************
*******************************************************************************/
#include <machine.h>
#include <stdio.h>
#include <stdbool.h>

#include "platform.h"
#include "can_api_demo.h"
#include "r_can_api.h"
#include "switches.h"
uint8_t     IST[14] = {0};

/*******************************************************************************
Macro definitions
*******************************************************************************/
/* Logical switch combination bit codes. For use in detecting multiple
   switch press combinations. */
#define     SW_NONE			0x07    /* 0b00000111 No Switch */
#define     SW1_PRESSED	   	0x06    /* 0b00000110 Switch 1 pressed only */
#define     SW2_PRESSED		0x05    /* 0b00000101 Switch 2 pressed only */
#define     SW3_PRESSED		0x03    /* 0b00000011 Switch 3 pressed only */
#define     SW23_PRESSED	0x01    /* 0b00000001 Switch 3 & 2 pressed only */

/* Nr times switch-poll must match. For debounce. */
#define     SW_COUNT  	    2

/* Switch func. table */
#define     SW_TBL_SIZE     6

/* enumerate values for the switch function array indexes. */
enum 
{
    NO_FUNC = 0,
    SW1_FUNC,
    SW2_FUNC,
    SW3_FUNC,
    SW32_FUNC,
    SW23_FUNC                          
};

/*******************************************************************************
Global variables and functions imported (externs)
*******************************************************************************/
/* Data */
extern uint32_t g_can_channel;


/*******************************************************************************
Private global variables and functions
*******************************************************************************/
/* Data */
/* Switch */
static uint8_t g_last_sw_data;         /* Last switch data */
static uint8_t g_fix_sw_data;          /* Fixed switch data */
static uint8_t g_last_fix_sw_data;     /* Last fixed switch data */



char state = 1;
/* state = 1: 1st digit
*  state = 2: 2nd digit
*  state = 3: 3rd digit
*/

/*	initialize each to, e.g., ‘A’	*/
char dig1 = 65;
char dig2 = 65;
char dig3 = 65;


/* Functions */
static uint8_t switches_are_stable(uint8_t);
static uint8_t switch_decode(void);
static void sw_none_function(void);
static void sw1_function(void);
static void sw2_function(void);
static void sw3_function(void);
static void sw23_function(void);
static void sw32_function(void);

/* An array of constant pointers to the switch functions */
static void (* const g_switch_func_table[SW_TBL_SIZE])(void) = {sw_none_function,
                                                                sw1_function, 
                                                                sw2_function, 
                                                                sw3_function, 
                                                                sw32_function, 
                                                                sw23_function };


/*******************************************************************************
* Function name: switches_initialize
* Description  : Initializes push button switches I/O ports. 
* Arguments    : None
* Return Value : None
*******************************************************************************/
void switches_initialize(void)
{
    /* Set switch input ports to GPIO input mode. */ 
    SW1_PMR = 0;
    SW1_PDR = 0;
    
    SW2_PMR = 0;
    SW2_PDR = 0;

    SW3_PMR = 0;
    SW3_PDR = 0;
	
  	lcd_display(LCD_LINE1, "A__");
    
}  /* End of function switches_initialize(). */


/*******************************************************************************
* Function name: read_switches
* Description  : Determines which switch or combination of swithces (if any)
*                is currently pressed. Sets a variable with a code that
*                represents the state of the switches, then sends it to the 
*                switches_are_stable() function which will assign that code 
*                to a global if it represents a stable switch state.
* Arguments    : None
* Return Value : None
*******************************************************************************/
void read_switches(void)
{
    uint8_t i, current_sw_pos; 

    /* Build a switch byte pattern */
    current_sw_pos = 0x07;               /* initialize all switch bits to 1 */
    
    if (!SW1)
    {
        current_sw_pos &= SW1_PRESSED;	/* SW1 is pressed. Clear SW1 bit */
    }

    if (!SW2)
    {
        current_sw_pos &= SW2_PRESSED;	/* SW2 is pressed. Clear SW2 bit */
    }

    if (!SW3)
    {
        current_sw_pos &= SW3_PRESSED;	/* SW3 is pressed. Clear SW3 bit */	
    }

    if (switches_are_stable(current_sw_pos)) 
    {
        /* Switch pressed, find out which */
        i = switch_decode();
        
        if (i < SW_TBL_SIZE)
        {   /* Switch function table. Call the corresponding switch func */
            g_switch_func_table[i]();
        }
        else
        {
            app_err_nr |= APP_TABLE_ERR;
        }
    }
    
}  /* End of function read_switches(). */


/*******************************************************************************
* Function name: switches_are_stable
* Description  : Tests for stability of switch state. Performs debounce.
* Arguments    : current_sw_pos - 
*                    Current positions of switches 1-3.
* Return Value : true -
*                    stable change. 
*                false - 
*                    not stable change.
*******************************************************************************/
uint8_t switches_are_stable(uint8_t current_sw_pos)
{
    bool switches_stable = false;
    static uint8_t sw_count = 0;        /* Switch counter */    
    
    /* If switches have changed, time the change. */
    if ((g_fix_sw_data != current_sw_pos) && (g_last_sw_data == current_sw_pos))
    {
        sw_count++;
        
        /* If read_switches are stable, copy value */
        if (sw_count > SW_COUNT)
        {
            g_fix_sw_data = current_sw_pos;
            sw_count = 0;
            switches_stable = true;
        }
    }
    else
    {
        sw_count = 0;
    }
    
    g_last_sw_data = current_sw_pos;
    
    return switches_stable;
    
}/*  End of function switches_are_stable(). */


/*******************************************************************************
* Function name: switch_decode
* Description  : Decodes which switch has been pressed
* Arguments    : None
* Return Value : None
*******************************************************************************/
uint8_t switch_decode(void)
{
    uint8_t switch_func_nr = NO_FUNC;
    
    switch (g_fix_sw_data)
    {
        /* No switch */
        case SW_NONE:
            switch_func_nr = NO_FUNC;
        break;

        /* SW1 */
        case SW1_PRESSED:
            if (SW_NONE == g_last_fix_sw_data)
            {
                switch_func_nr = SW1_FUNC;
            }
            else
            {
                switch_func_nr = NO_FUNC;
            }
        break;

        /* SW2 */
        case SW2_PRESSED:
            /* Don't call Switch 2 again after '23' until user has realeased SW2 */
            if (SW_NONE == g_last_fix_sw_data)
            {
                switch_func_nr = SW2_FUNC;
            }
            else if (SW23_PRESSED == g_last_fix_sw_data)
            {
                switch_func_nr = NO_FUNC; /* SwDummyFunc */
            }
            else
            {
                switch_func_nr = NO_FUNC;
            }
        break;
        
        /* SW3 */
        case SW3_PRESSED:
            /* Don't call Switch 3 after '32' again until user has realeased SW3 */
            if (SW_NONE == g_last_fix_sw_data)
            {
                switch_func_nr = SW3_FUNC;
            }
            else if (SW23_PRESSED == g_last_fix_sw_data)
            {
                switch_func_nr = NO_FUNC; /* SwDummyFunc */
            }
            else
            {
                switch_func_nr = NO_FUNC;
            }
        break;
        
        /* SW1 & SW2 */
        case SW23_PRESSED:
            /* SW1 --> SW2 */
            if (SW3_PRESSED == g_last_fix_sw_data)
            {
                switch_func_nr = SW32_FUNC;
            }
            /* SW2 --> SW1 */
            else if (SW2_PRESSED == g_last_fix_sw_data)
            {
                switch_func_nr = SW23_FUNC;
            }
            else
            {
                switch_func_nr = NO_FUNC;
            }
        break;
        
        default:
            switch_func_nr = NO_FUNC;
        break;
    }
    
    g_last_fix_sw_data = g_fix_sw_data;
    
    return switch_func_nr;
    
} /*  End of function switch_decode(). */


/*******************************************************************************
* Function name: sw1_function
* Description  : Switch 1. Transmits a test CAN frame.
*                A-D Demo.
* Arguments    : None
* Return Value : None               
*******************************************************************************/
void sw1_function(void)
{
	if ( state == 1 ) 
	{  
		if ( ++dig1 > 90 ){ dig1 = 65; } /*  Rollover condition for state1 in  SW1*/
		sprintf((char *)IST, "%c%c%c",dig1,95,95);            
    	lcd_display(LCD_LINE1, IST);
	}		
	if ( state == 2 )
	{
		if ( ++dig2 > 90 ){ dig2 = 65; } /*  Rollover condition for state2 in  SW1*/
		sprintf((char *)IST, "%c%c%c", dig1, dig2,95 );            
    	lcd_display(LCD_LINE1, IST);
	}
	if ( state == 3 )
	{
		if ( ++dig3 > 90 ){ dig3 = 65; } /*  Rollover condition for state3 in  SW1*/
		sprintf((char *)IST, "%c%c%c", dig1, dig2, dig3 );            
    	lcd_display(LCD_LINE1, IST);
	}
	
 } /*  End of function sw1_function(). */


/*******************************************************************************
* Function name: sw2_function
* Description  : Switch 2. Shows test frame CAN TxID. 
* Arguments    : None
* Return Value : None
*******************************************************************************/
void sw2_function(void)
{
	if ( state == 1 )
	{
		if ( --dig1 < 65 ){ dig1 = 90; } /*  Rollover condition for state1 in  SW2*/
		sprintf((char *)IST, "%c%c%c",dig1,95,95);       
    	lcd_display(LCD_LINE1, IST);
	}		
	if ( state == 2 )
	{
		if ( --dig2 < 65 ){ dig2 = 90; } /*  Rollover condition for state1 in  SW2*/
		sprintf((char *)IST, "%c%c%c", dig1, dig2,95 );            
    	lcd_display(LCD_LINE1, IST);
	}
	if ( state == 3 )
	{
		if ( --dig3 < 65 ){ dig3 = 90; } /*  Rollover condition for state1 in  SW2*/
		sprintf((char *)IST, "%c%c%c", dig1, dig2, dig3 );            
    	lcd_display(LCD_LINE1, IST);
	}
}/*  End of function sw2_function(). */


/*******************************************************************************
* Function name: sw3_function
* Description  : Switch 3. Shows test frame CAN RxID. 
*                If you are using a CAN filter mask, the incoming data ID 
*                may have overwritten what was set by the user with SW2 & 3.
* Arguments    : None
* Return Value : None                
*******************************************************************************/
void sw3_function(void)
{
	if ( state++ == 3 )          //  Time to send
	{            /* Tx Data frame is initialized */
		g_tx_dataframe.data[0]=dig1;
		g_tx_dataframe.data[1]=dig2;
		g_tx_dataframe.data[2]=dig3;
        /* Reinitialization to State 1*/
		state = 1;
		/* Reinitialization of display variables*/
		dig1 = dig2 = dig3 = 65;
		sprintf((char *)IST, "%c%c%c",dig1,95,95);       
  		lcd_display(LCD_LINE1, IST);

	}
	else
	{
		if ( state == 2 ) 	/*Display  Condition */
		{
			sprintf((char *)IST, "%c%c%c",dig1,dig2,95);       
  		  	lcd_display(LCD_LINE1, IST);
		}
		if ( state == 3 ) /*Display  Condition */
		{          
			sprintf((char *)IST, "%c%c%c",dig1,dig2,dig3);       
  		  	lcd_display(LCD_LINE1, IST);
		}
	}
	
     #if TEST_FIFO
        uint8_t	i = 0;
    #endif
    
    
    
	
	
	
	
	
    if (FRAME_ID_MODE == STD_ID_MODE )
    {
        R_CAN_TxSet(0, CANBOX_TX, &g_tx_dataframe, DATA_FRAME);
		
    }
    else
    {   /* Extended ID mode. */
	
        R_CAN_TxSetXid(0, CANBOX_TX, &g_tx_dataframe, DATA_FRAME); 
		
    }
	  /* Transmission Code in FIFO Mode */ 
    #if TEST_FIFO
    /* Send three bytes to fill FIFO. */
    for (i == 0; i < 3; i++)
    {
        if (FRAME_ID_MODE == STD_ID_MODE )
        {
            R_CAN_TxSetFifo(g_can_channel, &tx_fifo_dataframe, DATA_FRAME);
		
		}
        else
        {
            R_CAN_TxSetFifoXid(g_can_channel, &tx_fifo_dataframe, DATA_FRAME);
        }
    }
    
    #ifdef USE_CAN_POLL
    /* This flag will not be set by TX FIFO interrupt, so set it here. */
    tx_fifo_flag = 1;
    #endif
    #endif
   
    
} /*  End of function sw3_function(). */


/*******************************************************************************
* Function name: sw23_function
* Description  : First switch 2 then switch 3 pressed. Increments CAN TX-ID.
* Arguments    : None
* Return Value : None
*******************************************************************************/
void sw23_function(void)
{
    /* Inc. transmit ID */
    g_tx_dataframe.id++;
    
    if (FRAME_ID_MODE == STD_ID_MODE)
    {
        if (g_tx_dataframe.id > 0x7FF)
        {
            g_tx_dataframe.id = 0x01;
        }        		
    }
    else
    {   /* Extended or Mixed ID mode. */
        if (g_tx_dataframe.id > 0x1FFFFFFF)
        {
            g_tx_dataframe.id = 0x01;
        }            
    }    
    
    sw2_function();
    
} /*  End of function sw23_function(). */


/*******************************************************************************
* Function name: sw32_function
* Description  : First switch 3 then switch 2 pressed. Increments CAN RX-ID.
* Arguments    : None
* Return Value : None
*******************************************************************************/
void sw32_function(void)
{
    /* When adjusting rx id, stop all transmission 
    abort_trm_can(CANBOX_TX);   */

    /* If InvalData is flagged we are still receiving. Skip and come back. */
    if (CAN0.MCTL[CANBOX_RX].BIT.RX.INVALDATA == 0 )
    {
        /* Stop receiving */
        CAN0.MCTL[CANBOX_RX].BYTE = 0;
      
        g_rx_dataframe.id++;        /* Increment receive ID */

        if (FRAME_ID_MODE == STD_ID_MODE)
        {
            if (g_rx_dataframe.id > 0x7FF)
            {
                g_rx_dataframe.id = 0x01;
            }        		
        }
        else
        {   /* Extended or Mixed ID mode. */
            if (g_rx_dataframe.id > 0x1FFFFFFF)
            {
                g_rx_dataframe.id = 0x01;
            }            
        } 

        /* Set new ID and start receiving */
        if (FRAME_ID_MODE == STD_ID_MODE)
        {		
            R_CAN_RxSet(g_can_channel, CANBOX_RX, g_rx_dataframe.id, DATA_FRAME);
        }
        else
        {   /* Extended or Mixed ID mode. */
            R_CAN_RxSetXid(g_can_channel, CANBOX_RX, g_rx_dataframe.id, DATA_FRAME);            
        }
        
        sw3_function();
    }
} /* End of function sw32_function(). */


/*******************************************************************************
* Function name: sw_none_function
* Description  : Does nothing. This function will run if no switch is pressed, 
*                or there is no desirable switch function to run. 
* Arguments    : None
* Return Value : None               
*******************************************************************************/
void sw_none_function(void)
{
    nop();
}

/* eof */