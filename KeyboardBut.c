/*************************************************************************
 *      KeyboardBut.c
 *
 *      Ver  Date       Name Description
 *      W    2009-02-26 AGY  Created.
 *
 *************************************************************************/

#include <sys/ioctl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <errno.h>
#include <unistd.h> 
#include <string.h>
#include <malloc.h>
#include <termios.h>
#include <dirent.h>
#include <sys/time.h>
#include <linux/input.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/time.h>
#include "OWHndlrOWFSFile.h"
#ifdef RPI_DEFINED
#include <wiringPi.h>

#elif BB_DEFINED
#include "SimpleGPIO.h"

#endif


#include "SysDef.h"
//#include "KeyboardIO.h"
//#include "TimHndlr.h"
#include "Main.h"
// All these started by Main.c (bottom of the file...)
void * RdButton(enum ProcTypes_e ProcType) { // Not used today as LCD buttons works well!
  // Reads buttons connected to corresponding IO pins
  int 	Idx, ButIdx, fd_I2CKnob, fd_main, fd_timo, fd_OpBut, fd_LftBut, fd_RgtBut, ret;
  char 	data_read[2];
  char 	led[10], OpButton[10], LftButton[10], RgtButton[10];
  char	OpButOn, LftButOn, RgtButOn;
  static unsigned char  Buf[sizeof(union SIGNAL)]; // Static due to must be placed in global memory (not stack) otherwise we get OS dump Misaligned data!!!
  union SIGNAL             *Msg;
  struct  input_event       ie; 
#ifdef RPI_DEFINED
	int 		But_Op    = 8; 			// GPIO 3 header pin 5
	int 		But_Rgt   = 9;    	// SDA0 2 header pin 3

#elif BB_DEFINED
	int 		But_Op    = 67; 			// GPIO 67 header pin 8, P8
	int 		But_Rgt   = 60;     	// GPIO 60 header pin 12, P9
#endif	

  if (ProcType == BF537) {
    strcpy(led, STR_LED_BF537);
    strcpy(OpButton,  STR_OPBUT_BF537);
    strcpy(LftButton, STR_LFTBUT_BF537);
    strcpy(RgtButton, STR_RGTBUT_BF537);
  } 
  else if (ProcType == BF533) {
    strcpy(led, STR_LED_BF533);
    strcpy(OpButton,  STR_OPBUT_BF533);
    strcpy(LftButton, STR_LFTBUT_BF533);
    strcpy(RgtButton, STR_RGTBUT_BF533);
  } else if (ProcType == RPI) {
    
  } else if (ProcType == BB) {
	
  } else if (ProcType == HOSTENV) {
    // Do nothing  printf("Kbd defined HOST\n");
	  }
  else
    CHECK(FALSE, "Unknown processor type\n");

  OPEN_PIPE(fd_main, MAIN_PIPE, O_WRONLY);
	
  OpButOn = FALSE;
  LftButOn = FALSE;
  RgtButOn = FALSE;
// WiringPi initiated by Main.c
#ifdef RPI_DEFINED
	pinMode(But_Op, INPUT);
	pinMode(But_Rgt, INPUT);
#elif BB_DEFINED
	gpio_export(But_Op);
	gpio_export(But_Rgt);
	gpio_set_dir(But_Op, INPUT_PIN);
  
	gpio_set_dir(But_Rgt, INPUT_PIN);
#endif

  LOG_MSG("Button Started\n");
	Idx = 0;
	ButIdx = 0;
  while(TRUE) {
		//Idx++;
    Msg = (void *) Buf; // Set ptr to receiving buffer
    usleep(20000); // Timeout between each scan of keyboard, to be adjusted
#ifdef RPI_DEFINED
// Check if Right button pressed	
    ret = digitalRead(But_Op);
#elif BB_DEFINED
		gpio_get_value(But_Op, &ret);
#endif
  
		//printf(" OP: %d\r", ret);
		if (ret != 0)  // Button is NOT pressed!!
			OpButOn = FALSE;            // Button not pressed
    else if (OpButOn == FALSE)  { // Button pressed and released since last read
      OpButOn = TRUE;
      Msg->SigNo = SIGOpButOn;// Send signal	
      SEND(fd_main, Msg, sizeof(union SIGNAL));
		printf("Oppressed %d\r\n", ret);
		}
		
// Check if Right button pressed			
#ifdef RPI_DEFINED
    ret = digitalRead(But_Rgt);		
#elif BB_DEFINED	
		gpio_get_value(But_Rgt, &ret);
#endif // RPI_DEFINED

		if (ret != 0)	// Button is NOT pressed!!
			RgtButOn = FALSE;            // Button not pressed
    else if (RgtButOn == FALSE)  { // Button pressed and released since last read
      RgtButOn = TRUE;
      Msg->SigNo = SIGRghtButOn;// Send signal	
      SEND(fd_main, Msg, sizeof(union SIGNAL));
		printf("Rgtpressed %d\r\n", ret);
		}
   } // while
	  exit(0);
}; 
// Reads keyboard input, mainly for debugging
void * RdKeyboard(enum ProcTypes_e ProcType) {
  int 	fd_main, ret;
  static unsigned char  Buf[sizeof(union SIGNAL)]; // Static due to must be placed in global memory (not stack) otherwise we get OS dump Misaligned data!!!
  union SIGNAL             *Msg;

  OPEN_PIPE(fd_main, MAIN_PIPE, O_WRONLY);
  LOG_MSG("Kbd Started\n");
  while(TRUE) {
		//Idx++;
    Msg = (void *) Buf; // Set ptr to receiving buffer
 		switch (getchar()) {
     case 'H':  // Help info
     case 'h':
   		 	printf("(O)peration, (R)ight, (D)ebug On/Off \r\n");   	
        printf("\r\n");
		 break;  

     case 'D':  // Debug On/Off
     case 'd':
   		 if (DebugOn == TRUE) {
         DebugOn = FALSE;
         LOG_MSG("Debug OFF\n");
       }
       else {
         DebugOn = TRUE;
         LOG_MSG("Debug ON\n");
       }  
		 break;  
     
   	 case 'O': // Operation, OP pressed
     case 'o':
   		 	Msg->SigNo = SIGOpButOn;// Send signal	
 			 SEND(fd_main, Msg, sizeof(union SIGNAL));
		 break;

		 case 'R': // Right pressed
     case 'r':
    	 	Msg->SigNo = SIGRghtButOn;// Send signal	
   		 SEND(fd_main, Msg, sizeof(union SIGNAL));
		  break;
		 }
   } // while
	  exit(0);
}; 

void * RdLCDButtons(enum ProcTypes_e ProcType) { // Read LCD Buttons
// We use the Display board for this. Each "click" on a button generates an increase of 2.
// So there is a logic for checking that we actually got an increase of 2 and that the button was released 
// before next "click"
  FILE   *fp;
  int 	 RgtOn, OpOn, Initiated, idx, OpNo, OpNoOld, RgtNo, RgtNoOld, Status, fd_main, ret;
  static unsigned char  Buf[sizeof(union SIGNAL)]; // Static due to must be placed in global memory (not stack) otherwise we get OS dump Misaligned data!!!
  union SIGNAL             *Msg;

  unsigned char Ch, Addr[200], line[80], OpStr[20], RgtStr[20], InfoText[300];  
  
  Initiated = FALSE;
  OpOn = FALSE;
  RgtOn = FALSE;
  OpNo = 0; RgtNo = 0;  

  OPEN_PIPE(fd_main, MAIN_PIPE, O_WRONLY);
  
  // Now we must wait for the LCD to be initiated...
  while (ProcState.LCD_Id == 0) {sleep(1);}
  
// We must use the uncached reading to avoid delays
  sprintf(Addr, "%s%s%s%s", OWFS_MP, "uncached/", OneWireList[ProcState.LCD_Id].Path, "/cumulative.ALL"); 
 // printf("Open: %d %s Addr: %s \r\n", ProcState.LCD_Id, OneWireList[ProcState.LCD_Id].Path, Addr);  
  LOG_MSG("LCD Buttons Started\n");
  
  while(TRUE) {
    Msg = (void *) Buf; // Set ptr to receiving buffer
    usleep(20000); // Timeout between each scan of keyboard, to be adjusted
    if((fp = fopen(Addr, "r")) == NULL)  {
	    sprintf(InfoText, "ERROR: %s %d Cannot open file %s \n", strerror(errno), errno, Addr);
	    CHECK(FALSE, InfoText); sleep(10);
	    // fclose(fp); Don't close, fp == NULL!!
	    return;
   	} else { // File opened, continue execution..
	    idx = 0;
      // Read the string for the buttons; format: example   2343,45, 678, 998
      // String incremented by 2 each time corresponding button pressed
      // Read Op string-button
    //while (fgets(line, 20, fp) != NULL)
      //  printf("%s \r\n");
      while( ( OpStr[idx++] = fgetc(fp) ) != ',') {} //{if (DebugOn == 1) printf("%c", OpStr[idx-1]);usleep(2000);}
      OpNo = atoi(OpStr);
     // printf("Op: %d OpStr: %s \n\r", OpNo, OpStr);
      idx = 0;
      // Read Right string-button
      while( ( RgtStr[idx++] = fgetc(fp) ) != ',') {} // {if (DebugOn == 1) printf("%c", RgtStr[idx-1]); usleep(2000);} 
      RgtNo = atoi(RgtStr);
      if (!Initiated) {  // Dont read first time we enter loop, uninitated values...
        OpNoOld = OpNo;
        RgtNoOld = RgtNo;
        Initiated = TRUE;
      } else { 
			  if ((OpNo == (OpNoOld+2)) && !(OpOn)) { // Button pressed, increased by 2 and released since last time
          OpOn = TRUE;
          OpNoOld = OpNo;
          Msg->SigNo = SIGOpButOn;// Send signal	
 		  	  SEND(fd_main, Msg, sizeof(union SIGNAL));
          //printf("%d: OP LCD Buttons\r\n", OpNo);
	      } else { 
          OpOn = FALSE;
        }
        if ((RgtNo == (RgtNoOld + 2)) && !(RgtOn)) {// Button pressed, increased by 2 and released since last time
          RgtOn = TRUE;
          RgtNoOld = RgtNo;
  	      Msg->SigNo = SIGRghtButOn;// Send signal	
   		    SEND(fd_main, Msg, sizeof(union SIGNAL));
         // printf("%d: Rgt LCD Buttons\r\n", RgtNo);
        } else {
          RgtOn = FALSE;
        } // else
      } // if
    } // if-else
    fclose(fp);
  } // while
}; 


