/*************************************************************************
 *      Keyboard.c
 *
 *      Ver  Date       Name Description
 *      W    2006-11-24 AGY  Created.
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

#include "SysDef.h"
#include "KeyboardIO.h"
#include "TimeoutHandler.h"

#include "Main.h"

#define KnobUsed
// Defined if you use knob as input otherwise comment out (== buttons on board used instead)

void * RdKeyboardKnob(enum ProcTypes_e ProcType) {
  int 	fd_I2CKnob, fd_main, fd_timo, fd_OpBut, fd_LftBut, fd_RgtBut, ret;
  char 	data_read[2];
  char 	led[10], OpButton[10], LftButton[10], RgtButton[10];
  char	OpButOn, LftButOn, RgtButOn;
  static unsigned char  Buf[sizeof(union SIGNAL)]; // Static due to must be placed in global memory (not stack) otherwise we get OS dump Misaligned data!!!
  union SIGNAL             *Msg;
  struct  input_event       ie; 

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
  }
  else if (ProcType == HOSTENV) {
    // Do nothing  printf("Kbd defined HOST\n");
  }
  else
    CHECK(FALSE, "Unknown processor type\n");

  OPEN_PIPE(fd_main, MAIN_PIPE, O_WRONLY);
  LOG_MSG("Started\n");
  OpButOn = FALSE;
  LftButOn = FALSE;
  RgtButOn = FALSE;

  

//  OPEN_DEV(fd_I2CKnob, INPUT_DEVICE, O_RDWR,0); 
//sleep(2);
//printf("Open knob\n");
  fd_I2CKnob = open (INPUT_DEVICE, O_RDWR, 0);
  if(fd_I2CKnob<0) {
    printf("Could not open %s Errno %d %s\n", INPUT_DEVICE, errno, strerror(errno));
  }
//sleep(2);
//printf("Knob open\n");
 
  while(TRUE) {
//printf("Prev Buf: %X   Buf[0] %X Msg: %X \n", Buf, &Buf[0], Msg);
    Msg = Buf; // Set ptr to receiving buffer
//printf("After Buf: %X   Buf[0] %X Msg: %X \n", Buf, &Buf[0], Msg);

    read(fd_I2CKnob, &ie, sizeof (ie));  // Halts until new cmd received from knob
printf("Knob cmr rec..\n");
		if (ie.code != 0) {
	   // printf("Got %d from knob\n", ie.code); sleep(1);
    }
    if(ie.value && ie.type) {
      switch(ie.code) {
	     case(0x1e):  // Left pressed
          //printf("Left pressed\n"); 
	        Msg->SigNo = SIGLftButOn;// Send signal	
	        //strcpy(Msg->KbdIO.Msg, "Left_pressed");
 	        SEND(fd_main, Msg, sizeof(union SIGNAL));
	     break;
	     case(0x30):  // Right pressed
         //printf("Right pressed\n");sleep(2);
	       Msg->SigNo = SIGRghtButOn;// Send signal	
	       //strcpy(Msg->KbdIO.Msg, "Right_pressed");
	       SEND(fd_main, Msg, sizeof(union SIGNAL));
	     break;
	     case(0x1c):  // Enter pressed
         //printf("Enter pressed\n");sleep(2);
	       Msg->SigNo = SIGOpButOn;// Send signal	
	       //strcpy(Msg->KbdIO.Msg, "OP_pressed");
 	       SEND(fd_main, Msg, sizeof(union SIGNAL));
	     break;
	     default:
         printf("Err: ?? pressed\n");
	     break;
      } // switch
      ret= 0;
    } // if 
  }

  close(fd_OpBut);
  close(fd_LftBut);
  close(fd_RgtBut);
  exit(0);
}; 
