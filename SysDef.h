
/*************************************************************************
 *      SysDef.c
 *
 *      Ver  Date       Name Description
 *      W    2006-11-24 AGY  Created.
 *
 *****************************************************************/
#include <pthread.h>
#include "SockSrv.h"
//#define DEBUG 

#define TRUE                   1
#define FALSE                  0
#define mSec               *1000
#define Sec                *1000000
#define SENS_DEF_VAL		  	-99  // Value used when no contact with sensor and as start value

#define WTIME               100  // Waiting time for messages. To be adjusted....
#define PROCESS	             int  // Process or fd for pipe...
#define SIGSELECT            int

 // Define a struct with ALL signal/structs used in the application 
struct	ByteportReport_s {  // Anchor command (Up, Dwn etc)
  SIGSELECT 						SigNo;
  char                  Str[200];  // Actual message to send to Byteport
};
 
struct	ServCmdReq_s {  // Anchor command (Up, Dwn etc)
  SIGSELECT 						SigNo;
  enum ServModes_e      Cmd;
};

struct	ReadADSensorReq_s {
  SIGSELECT 	SigNo;
  int					Client_fd;
  int         Sensor;
};

struct	ReadADSensorResp_s {
  SIGSELECT 	  SigNo;
  int           Sensor;
  char				  Status; // 1 == ok, 0 = error when reading sensor
  //char				Id[16];
  float         Val[3];    // Sensor values 
};
struct	ReadTempSensorReq_s {
  SIGSELECT 	SigNo;
  int					Client_fd;
  int         Sensor;
};

struct	ReadTempSensorResp_s {
  SIGSELECT 	  SigNo;
  int           Sensor;
  char				  Status; // 1 == ok, 0 = error when reading sensor
  //char				Id[16];
  float         Val[3];    // Sensor values 
};
struct	ReadSensorReq_s {
  SIGSELECT 	SigNo;
  int					Client_fd;
  int         Sensor;
};

struct	ReadSensorResp_s {
  SIGSELECT 	  SigNo;
  int           Sensor;
  float   			CmdTime;  // Used to store delta time for executing the commands between START and STOP
	char				  Status; // 1 == ok, 0 = error when reading sensor
  //char				Id[16];
  float         Val[3];    // Sensor values 
};

struct	TempR_s {
  SIGSELECT 	SigNo;
  int					Client_fd;
  int         Sensor;
  char        Status;  // 1 == Everything is ok
  //char				Id[16];
  float				Temp;
};

 struct	ADR_s { 
  SIGSELECT 	SigNo;
  int					Client_fd;
  int         Sensor;
  char        Status;  // 1 == everything is ok
  //char				Id[16];
  float				AD[4];
};
  
    struct KbdIO_s {
      SIGSELECT 	SigNo;
      //char        Msg[10];
    };	

    struct Timeout_s {
      SIGSELECT		    SigNo;
      int			        Client_fd; // filedescriptor for client
      char						ClientName[20]; // Client name
      SIGSELECT       RespSig; // Signal to send to client when timeout expires
      unsigned int    DeltaTime; // Time in usec until timeout
    };	

    union SIGNAL {
      SIGSELECT					  		    SigNo;
      struct ByteportReport_s     ByteportReport;
			struct ServCmdReq_s		    	ServCmdReq;
      struct ReadSensorReq_s      SensorReq;
      struct ReadSensorResp_s     SensorResp;
      struct ReadADSensorReq_s    SensorADReq;
      struct ReadADSensorResp_s   SensorADResp;
      struct ReadTempSensorReq_s  SensorTempReq;
      struct ReadTempSensorResp_s SensorTempResp;
     // struct TempR_s 					TempR;
     // struct ADR_s 					  ADR;
      struct KbdIO_s	    		     KbdIO; 
      struct Timeout_s	           Timo;
			char												 Data[120]; // For debugging..
    };

// Debug tools!!!!
char DebugOn;  // Global variable, may be used by everyone

struct ProcState_s  ProcState; // Write only Main.c, read all

#define DBGON DebugOn = TRUE;
#define DBGOFF DebugOn = FALSE;

// Timer to measure short time intervals. Gives no of cycles. See proc/cpuinfo Core clock (500 MHz?) 
//t of type long long  (number of cycles)
#define ReadCycles(t)		
#define SYSTEM_CLOCK 500000000    
#define TIMER_START(a) ReadCycles(a);
#define TIMER_STOP(a, b, c) 
#define READ_TIMER(a) 
#define DELTA_TIME(a, b, c) 
#define TIMER_READ(a, b, c) 


#define  LOG_MSG(a)              {printf("%s : LOG %s %d %s", now(), __FILE__, __LINE__, a);}

// Request timeout REQ_TIMEOUT(fd_timo, fd_own, Client name, Signal, DeltaTime)
#define	 REQ_TIMEOUT(a, b, c, d, e) {union SIGNAL				*zz_Msg; \
                                 unsigned char zz_Buf[sizeof(union SIGNAL)];\
                                 zz_Msg = (void *) zz_Buf;\
                                 zz_Msg->SigNo = SIGTimoReq;\
                                 zz_Msg->Timo.Client_fd = b;\
                                 strcpy(zz_Msg->Timo.ClientName, c);\
                                 zz_Msg->Timo.RespSig = d;\
                                 zz_Msg->Timo.DeltaTime = e;\
                                 write(a, zz_Msg, sizeof(union SIGNAL));}
 
#define  CHECK(a,b)            {if (!(a)) {                       							    \
                                 printf("%s : CHECK %s %d %s\n", now(), __FILE__, __LINE__, b);\
                                }\
                               }
#ifdef DEBUG
#define  OPEN_DEV(a,b, c, d)   {errno = 0;\
                               a = open(b, c, d);  \
                               if (a <= 0) {\
                                 printf("%s : %s %d %s open error %d\n", now(), __FILE__, __LINE__, b, strerror(errno)); \
                               } else { \
                                 printf("%s %d %s ",__FILE__,  __LINE__, b);	\
                                 printf("open with ID: %d ", a);	\
                                 printf("Errcode: %d Errtext: %s\n", errno, strerror(errno));	\
                                }\
                               }

#define  OPEN_PIPE(a,b, c)    {errno = 0;\
                              a = open(b, c);  \
															if (a <= 0) {\
                                printf("%s : %s %d %s open error %d %s\n", now(),  __FILE__, __LINE__, b, errno, strerror(errno)); \
															} else { \
                                printf("%s %d %s ",__FILE__,  __LINE__, b);	\
                                printf("open with ID: %d ", a);	\
                                printf("Errcode: %d Errtext: %s\n", errno, strerror(errno));	\
															 }\
                              }

#else
#define  OPEN_DEV(a,b, c, d)   {errno = 0;\
                               a = open(b, c, d);  \
                               if (a <= 0) {\
                                 printf("fd: %d %s : %s %d %s open %d %s\n", a, now(), __FILE__, __LINE__, b, errno, strerror(errno)); \
                                }\
                               }
    
#define  OPEN_PIPE(a,b, c)    {errno = 0;\
                              a = open(b, c);  \
															if (a <= 0) {\
                                printf("fd: %d %s : %s %d %s open %d %s\n", a, now(), __FILE__, __LINE__, b, errno, strerror(errno)); \
															 }\
                              }
#endif
// a = fd, b = pipe name, c = perms

#define  START_PROC(a,b,c,d)  {CHECK((pthread_create(a,b,c,d) != 0), "Unable to start %d");}

// a = filedescriptor, b = Message
#define   tWAIT(a, b, c)        {select(c) \
/*printf(".");*/\
                               }	



#define   WAIT(a, b, c)        {while(read(a, b , c)!= (int) c) {  \
                                 usleep(WTIME);\
															  } \
                               }	

// a = filedescriptor, b = Message
#define   SENDa(a,b)            CHECK(write(a, b, sizeof(b) != -1), "Unable to SEND\r\n");\
                                 mutex.Lock();\
                                 b = NULL;\
                                 mutex.Unlock();
																 														
#define   SEND(a,b, c)          if (a > 0) {CHECK(write(a, b, c) == c, "SEND error \r\n");}\
                                 else \
                                  printf("Err fd: %d %s : %s %s open %d %s\n", a, now(), __FILE__, __LINE__, b, " Not valid process to send to");                                 
                                //  usleep(50000);}		
// Since I added Curl (Byteport) I have had problems with Illegal Signals received, probably due to 
// some memory over write/timing problems. After adding the small usleep it works again.														


// Define signals and owner
#define  ALL                     (0)

// Keyboard.c
#define  SIGOpButOn             (10)  // Operation button pressed
#define  SIGLftButOn            (12)  // Left button pressed
#define  SIGRghtButOn           (13)  // Right button pressed

// OneWireHandler.c
#define  SIGReadTempReq         (20)  // Request to read temperature
#define  SIGReadTempResp        (21)  // Response to Temperature read request
#define  SIGReadADReq           (22)  // Request to read AD
#define  SIGReadADResp          (23)  // Response to AD read request
#define  SIGReadSensorReq       (24)  // Read sensor request
#define  SIGReadSensorResp      (25)  // Read sensor response
#define  SIGCheckNewSensors     (26)  // Scan for new sensors
#define  SIGzzzReadTemp         (27)  // Read temperature, internal SIGNAL 
#define  SIGzzzReadAD 			    (28)  // Read AD, internal SIGNAL
#define  SIGzzzCheck4Sensors    (29)  // Check for new sensors, INTERNAL SIGNAL

// TimeoutHandler.c
#define  SIGTimoReq             (30)  // Timeout request
#define  SIGTimoResp            (31)  // Timeout response

// Main.c
#define  SIGInitMeasTempOut     (40)  // Initiate measurments from Temperature sensors
#define  SIGInitMeasTempBox     (41)  // Initiate measurments from Temperature sensors
#define  SIGInitMeasTempRefrig  (42)  // Initiate measurments from Temperature sensors
#define  SIGInitMeasTempWater   (43)  // Initiate measurments from Temperature sensors
#define  SIGInitMeasTempSea     (44)  // Initiate measurments from Temperature sensors
#define  SIGInitMeasTemp        (45)  // Initiate measurments from Temperature sensors
#define  SIGInitMeasADInt       (46)  // Initiate measurments from AD sensors
#define  SIGInitMeasADExt       (47)  // Initiate measurments from AD sensors
#define  SIGInitMeasTempHW      (48)  // Initiate measurments from Temperature sensors
#define  SIGServCmdReq	        (49)  // Server command
#define  SIGMinuteTick	        (50)  // Minute tick counter, for backlight off.
#define  SIGInitByteportReport	(51)  // Send report to Byteport
#define  SIGSecondTick	        (52)  // Second tick, for process LCDT.

// ByteportHandler
#define  SIGByteportInit 	      (60)  // Init Byteport.
#define  SIGByteportReport	    (61)  // Report to Byteport.
#define  SIGByteportSrv    	    (62)  // Server cmd to ByteportHandler.




    
