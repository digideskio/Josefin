/************************************************************
*
* Copyright (C) 2006, Analog Devices. All Rights Reserved
*
* FILE lcd_def.h
* PROGRAMMER(S): Arne Gylling
*
* $Id: lcd_def.h,v 1.1 2006/11/15 11:16:14 arne Exp $
*
* DATE OF CREATION: Nov. 13 2006
*
* SYNOPSIS:
*
* DESCRIPTION: TWI LCD definition file

* CAUTION:    
**************************************************************
* MODIFICATION HISTORY:
* 13.11.2006 11:00  lcd_def.h Created. (Arne Gylling) 
************************************************************
*
* This program is free software; you can distribute it and/or modify it
* under the terms of the GNU General Public License (Version 2) as
* published by the Free Software Foundation.
*
* This program is distributed in the hope it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
* for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
*
************************************************************/

// LCD definitions for 1-wire operation, note we re-use some from lcd_def

#define LCD_Bkl_On					30   		// LCD-1W
#define LCD_Bkl_Off					31   		// LCD-1W



// Function command codes for io_ctl.

#define LCD_On									 1
#define LCD_Off									 2
#define LCD_Clear								 3
#define LCD_Reset								 4
#define LCD_Cursor_Left					 5
#define LCD_Cursor_Right	 			 6
#define LCD_Disp_Left		 				 7
#define LCD_Disp_Right					 8
#define LCD_Set_Cursor					10
#define LCD_Home								11
#define LCD_Curr_Controller			12
#define LCD_Cursor_Off					14
#define LCD_Cursor_On						15
#define LCD_Set_Cursor_Pos			17
#define LCD_Blink_Off           18
#define LCD_Contr               19

/* new definitions for RapberryPi 20121222*/
#define LCD_CTRL(a,b)	  lcdClear(a);  // a= file descriptor
#define LCD_HOME(a) 	  lcdHome(a);  // Home (position 1, 1)
#define LCD_CLEAR(a) 	  lcdClear(a); // Clear (position 1, 1 and clear screen)
#define LCD_POS(a,b,c)	lcdPosition(a,c-1, b-1); // LCD_POS: Position cursor, fd, y, x) , y = column[1..20], x= row[1..4]

#define LCD_WRITE(a, b, c, d)	{ lcdPosition(a,c-1,b-1); lcdPuts(a, d);}

// Same as LCD_WRITE but here you specify number of bytes to write
#define LCD_WRITEN(a, b, c, d, e)	{ \
  int zzIdxLCD; \
  lcdPosition(a,c-1,b-1); \
	for (zzIdxLCD = 0; zzIdxLCD <= e; zzIdxLCD++)  \
	  lcdPutchar(a, d[zzIdxLCD]); \
	}

				

// LCD_WRITE: Position on screen and write text
// Note, position 1, 1 has adress 00, so c - 1, see code above,  must be used to set postion 1, 1.
// a = file descriptor, b = line/row (normally 1..2 or 4 on LCD displays)
// c = column (normally 1..20 on LCD type displays)
// d = text
// Example Write "Hello world" on line 2, position 1
// LCD_WRITE (fd, 2, 1, "Hello world");




