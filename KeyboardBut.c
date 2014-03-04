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
#ifdef RPI_DEFINED
#include <wiringPi.h>

#elif BB_DEFINED
#include "BeagleGPIO/BeagleBone_gpio.h"

#endif


#include "SysDef.h"
//#include "KeyboardIO.h"
#include "TimeoutHandler.h"
#include "Main.h"

void * RdKeyboardBut(enum ProcTypes_e ProcType) {
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
  struct gpioID	But_Op;
  struct gpioID	But_Rgt;
	
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
	//pinMode(But_Op, 1, INPUT);
	//pinMode(But_Rgt,1,  INPUT);
#endif

  LOG_MSG("Started\n");
	Idx = 0;
	ButIdx = 0;
  while(TRUE) {
		//Idx++;
    Msg = Buf; // Set ptr to receiving buffer
    usleep(20000); // Timeout between each scan of keyboard, to be adjusted

// Check if Right button pressed	
    ret = digitalRead(But_Op);
		//printf(" OP: %d\r", ret);
		if (ret != 0)  // Button is NOT pressed!!
			OpButOn = FALSE;            // Button not pressed
    else if (OpButOn == FALSE)  { // Button pressed and released since last read
      OpButOn = TRUE;
      Msg->SigNo = SIGOpButOn;// Send signal	
      SEND(fd_main, Msg, sizeof(union SIGNAL));
		//  printf("Oppressed %d\r\n", ret);
		}
		
// Check if Right button pressed			
    ret = digitalRead(But_Rgt);
		if (ret != 0)	// Button is NOT pressed!!
			RgtButOn = FALSE;            // Button not pressed
    else if (RgtButOn == FALSE)  { // Button pressed and released since last read
      RgtButOn = TRUE;
      Msg->SigNo = SIGRghtButOn;// Send signal	
      SEND(fd_main, Msg, sizeof(union SIGNAL));
		//  printf("Rgtpressed %d\r\n", ret);
		}

   } // while
	  exit(0);
}; 
