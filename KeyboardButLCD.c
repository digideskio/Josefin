/*************************************************************************
 *      KeyboardBut.c
 *
 *      Ver  Date       Name Description
 *      W    2009-02-26 AGY  Created.
 *      PA1  2016-03-09 AGY  A dedicated version for using kyeboard and LCD buttons (no IO buttons)
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

#elif BB_DEFINED
#include "SimpleGPIO.h"
#endif
#include "SysDef.h"
#include "Main.h"

// All these started by Main.c (bottom of the file...)

// Reads keyboard input, mainly for debugging
void * RdKeyboard(struct ProcState_s *Pstate) {
  int 	fd_main, ret;
  static unsigned char  Buf[sizeof(union SIGNAL)]; // Static due to must be placed in global memory (not stack) otherwise we get OS dump Misaligned data!!!
  union SIGNAL             *Msg;

  fd_main = Pstate->fd.WR_MainPipe;
 // OPEN_PIPE(fd_main, MAIN_PIPE, O_WRONLY);
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
     case 'T':  // Debug level 2 On/Off
     case 't':
   		 if (DebugOn == 2) {
         DebugOn = FALSE;
         LOG_MSG("Debug OFF level 2\n");
       }
       else {
         DebugOn = 2;
         LOG_MSG("Debug ON level 2\n");
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

void * RdLCDButtons(struct ProcState_s *PState) { // Read LCD Buttons
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

  fd_main = PState->fd.WR_MainPipe;
  //sprintf (InfoText, " Main %d  \r\n", fd_main);
  //LOG_MSG(InfoText);
  
  // Now we must wait for the LCD to be initiated...
  idx = 0;
  while (!ProcState.DevLCDDefined) {
    usleep(200000);
    idx++;
    if (idx >= 1000) {
      LOG_MSG("Err: Not able to start LCD Buttons, no LCD found..exit! \r\n"); 
      exit(0);
    }  
  }
  
// We must use the uncached reading to avoid delays
  sprintf(Addr, "%s%s%s%s", OWFS_MP, "uncached/", OneWireList[ProcState.LCD_Id].Path, "/cumulative.ALL"); 
 // printf("Open: %d %s Addr: %s \r\n", ProcState.LCD_Id, OneWireList[ProcState.LCD_Id].Path, Addr);  
 //sleep(10);
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
        if (DebugOn == 2) printf("Op %d: %d, Rgt %d, %d \r\n", OpNo, OpNoOld, RgtNo, RgtNoOld);
        
			  if ((OpNo >= (OpNoOld+2)) && !(OpOn)) { // Button pressed, increased by 2 and released since last time
          OpOn = TRUE;
          OpNoOld = OpNo;
          Msg->SigNo = SIGOpButOn;// Send signal	
 		  	  SEND(fd_main, Msg, sizeof(union SIGNAL));
         // printf("%d: OP LCD Buttons\r\n", OpNo);
	      } else { 
          OpOn = FALSE;
        }
        if ((RgtNo >= (RgtNoOld + 2)) && !(RgtOn)) {// Button pressed, increased by 2 and released since last time
          RgtOn = TRUE;
          RgtNoOld = RgtNo;
  	      Msg->SigNo = SIGRghtButOn;// Send signal	
   		    SEND(fd_main, Msg, sizeof(union SIGNAL));
          //printf("%d: Rgt LCD Buttons\r\n", RgtNo);
        } else {
          RgtOn = FALSE;
        } // else
      } // if
    } // if-else
    fclose(fp);
  } // while
}; 


