/*************************************************************************
 *      SysDef.c
 *
 *      Ver  Date       Name Description
 *      W    2006-11-24 AGY  Created.
 *
 *************************************************************************/


#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <getopt.h>
#include <signal.h>
#include <sys/poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h> 
#include <string.h>
#include <pthread.h>
#include <linux/input.h>
#include "sys/stat.h"
#include <malloc.h>

// Prototypes 
  char * now(void);


char * now() { // Read current timestamp
  static char fTime[100];
  struct tm *newtime;
  time_t long_time;

  time( &long_time );                // Get time as long integer. 
  newtime = localtime( &long_time ); // Convert to local time. 
  sprintf(fTime, "%04d%02d%02d %02d:%02d:%02d",newtime->tm_year+1900,newtime->tm_mon+1,newtime->tm_mday,newtime->tm_hour,newtime->tm_min,newtime->tm_sec);
  return fTime;
};
