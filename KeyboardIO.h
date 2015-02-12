/*************************************************************************
 *      KeyBoard.h
 *
 *      Ver  Date       Name Description
 *      W    2006-11-24 AGY  Created.
 *      R1   2009-01-19 AGY  Changed to /dev/event0 for uclinux R8	
 *
 *************************************************************************/


#define INPUT_DEVICE           		"/dev/event0"
#define SET_FIO_DIR                	1  // Peripheral Flag Direction Register
#define SET_FIO_POLAR              	2  // Flag Source Polarity Register
#define SET_FIO_EDGE               	3  // Flag Source Sensitivity Register
#define SET_FIO_BOTH               	4  // Flag Set on BOTH Edges Register
#define SET_FIO_INEN								5  // Flag Input Enable Register 

	
#define INPUT												0
#define OUTPUT											1
#define ACTIVEHIGH_RISINGEDGE				0
#define ACTIVELOW_FALLINGEDGE				1
#define LEVEL												0
#define EDGE												1

#define SINGLEEDGE									0
#define BOTHEDGES										1

#define INPUT_DISABLE								0
#define INPUT_ENABLE								1




