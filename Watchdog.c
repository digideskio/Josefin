/*************************************************************************
 *      Watchdog.c
 *
 *      Ver  Date       Name Description
 *      R1   2006-11-24 AGY  Created.
 *	    R2   2009-01-24 AGY  Adopted to R8 system (new GPIO used instead of pflags) 	
 *      R3   2013-01-09 AGY  Adopted to RaspberryPi and using OK LED (GPIO 16)
 *			R4	 2014-03-22 AGY  Adopted to Beaglebone Black...i Abisko!
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
// #include <wiringPi.h>  Not used any more AG March 2016
#define OK_LED  47
#endif

#include "SysDef.h"
//#include "TimeoutHandler.h"
#include "Main.h"

void * Watchdog(enum ProcTypes_e ProcType) {
int fd_led;

#ifdef RPI_DEFINED
#define LED_OK "/sys/class/leds/led0/brightness" 
	//pinMode(OK_LED, OUTPUT);
  OPEN_PIPE(fd_led, LED_OK, O_WRONLY|O_NONBLOCK);
#elif BB_DEFINED
#define LED4_MP  "/sys/class/leds/beaglebone:green:usr3/brightness"
	OPEN_PIPE(fd_led, LED4_MP, O_WRONLY|O_NONBLOCK);
#endif
  // Wiringpi lib already initialized by Main program

	LOG_MSG("Started\n"); 
	while (TRUE) {
		write(fd_led, "1", 1);
		usleep (500000);
		write(fd_led, "0", 1);
		usleep(500000);			
				
	}
}		