/*************************************************************************
 *      Watchdog.c
 *
 *      Ver  Date       Name Description
 *      R1   2006-11-24 AGY  Created.
 *	    R2   2009-01-24 AGY  Adopted to R8 system (new GPIO used instead of pflags) 	
 *      R3   2013-01-09 AGY  Adopted to RaspberryPi and using OK LED (GPIO 16)
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
#ifdef RPI_DEFINED
#include <wiringPi.h>

#elseif BB_DEFINED
#include "BeagleGPIO/beaglebone_gpio.h"
#endif

#include "SysDef.h"
//#include "KeyboardIO.h"
#include "TimeoutHandler.h"

#include "Main.h"

#define INPUT 0
#define OUTPUT 1
#define OK_LED  16

void * Watchdog(enum ProcTypes_e ProcType) {
  // Wiringpi lib already initialized by Main program
	pinModeGpio(OK_LED, OUTPUT);
	LOG_MSG("Started\n"); 

	while (TRUE) {
		digitalWriteGpio(OK_LED, 0);
		delay (500);
		digitalWriteGpio(OK_LED, 1);
		delay (500);				
	}
}		