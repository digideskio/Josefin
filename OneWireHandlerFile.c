/*************************************************************************
 *      OneWireHandler.c
 *
 *      Ver  Date       Name Description
 *      W    2006-11-24 AGY  Created.
 *      PA1  2007-10-15 AGY  Changed so that 1-w handling only on request from client
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
#include <sys/resource.h>
#include <sys/time.h>
//#include <linux/timex.h>   // Measure short time intervals cycles_t get_cycles(void);
#include "SysDef.h"
#include "lcd_def.h"
#include "Main.h"
#include "OneWireHandlerFile.h"

// Declaration of prototypes
char 	          Scan4Sensors(void);
float	          CalculateMeanValue(int SensorId, float Temp);
char						ReadAD(float *AD, FILE *fp, unsigned char DevType, char *SensorPath, char *ScrPAdHex);
char						ReadTemp(float *Temp, FILE *fp, unsigned char DevType, char *SensorPath, char *ScrPadHex);
int 						A2HexByte(char A1, char A2);
int 						ToHex(char ch);
unsigned char 	Hex2Ascii(unsigned char No);
char            TimeVal0[40];  // Stores time values, to measure executing times
char            TimeVal1[40];  // Stores time values, to measure executing times
char            TimeVal2[40];  // Stores time values, to measure executing times
char            Initiated;     // Indicate if Mean value list is initiated or not.

// Global variables
struct 	OneWireList_s OneWireList[MAX_NO_OF_DEVICES]; // Maximum no of allowed devices on 1-wire net!
char    		InfoText[300];  // Store info text, Devpath may be long...!
long long 	RT, TM1, TM2, StartCycles, EndCycles;
float				DeltaTime;

void  				* OneWireHandler(enum ProcTypes_e ProcType) {
  FILE            	*fp;
  int          	   	fd_Client, fd_2own, fd_own, fd_2main, fd_timo;
  unsigned char			Buf[sizeof(union SIGNAL)];
  union SIGNAL 			*Msg;
  int								Idx, Id;
  int              	DevType;
  char             	ScrPad[100];
  float							CurTemp;
  float            	AD[4];
  char             	Status;


  setpriority(PRIO_PROCESS, 0, -20);
  // Init list, NO sensors present
	Initiated = FALSE;
  for (Idx = 0; Idx < MAX_NO_OF_DEVICES; Idx++) {
    OneWireList[Idx].Present = FALSE;
    //OneWireList[Idx].fp      = NULL;
    OneWireList[Idx].DevType = 0x00;  // No sensor or sensortype defined
    OneWireList[Idx].Val[0]  = -99; //SENS_DEF_VALUE;  // Set default value
    OneWireList[Idx].Val[1]  = -99; //SENS_DEF_VALUE;  // Set default value
    OneWireList[Idx].Val[2]  = -99; //SENS_DEF_VALUE;  // Set default value
    OneWireList[Idx].Val[3]  = -99; //SENS_DEF_VALUE;  // Set default value

	}
	printf("1");
  Scan4Sensors();
		printf("2");
  OPEN_PIPE(fd_own,    ONEWIRE_PIPE, O_RDONLY|O_NONBLOCK);
  OPEN_PIPE(fd_2own,   ONEWIRE_PIPE, O_WRONLY);
  OPEN_PIPE(fd_2main,  MAIN_PIPE,    O_WRONLY);
  OPEN_PIPE(fd_timo,   TIMO_PIPE,    O_WRONLY);
//  REQ_TIMEOUT(fd_timo, fd_2own, "OneWire", SIGInitMeasAD, 2 Sec); 
//  REQ_TIMEOUT(fd_timo, fd_2own, "OneWire", SIGInitMeasTemp, 2 Sec); 
  REQ_TIMEOUT(fd_timo, fd_2own, "OneWire", SIGCheckNewSensors, 5 Sec); 
  LOG_MSG("Started\n");
	while(TRUE) {
	  WAIT(fd_own, Buf, sizeof(union SIGNAL));
		Msg = Buf;
    switch(Msg->SigNo) {
     	case SIGReadSensorReq:
     	case SIGReadADReq:
     	case SIGReadTempReq:
       	fd_Client  = Msg->SensorReq.Client_fd;
       	Id = Msg->SensorReq.Sensor;
        if (!OneWireList[Id].Present) {
//					if (DebugOn) printf(" WARN: SenReq %d, not present \r\n",Msg->SensorReq.Sensor); // Sensor not present..
				} else { // Sensor exists, proceed...  
					DevType = OneWireList[Id].DevType;
					for (Idx = 0; Idx < 4; Idx++)  						// Init values
						OneWireList[Id].Val[Idx] = SENS_DEF_VAL;
          switch (DevType) {
            case DEV_DS1820: 												// Temperature sensors
            case DEV_DS1822:
            case DEV_DS18B20:
          /*    READ_TIMER(TM1); */
              if (!(ReadTemp(&CurTemp, fp, DevType, OneWireList[Id].Path, ScrPad))) {
                OneWireList[Id].Present = FALSE; 
                CurTemp = SENS_DEF_VAL;  // Set default value == indicate error
								sprintf(InfoText, "Read temp : %d %s Status : %s \n", Id, OneWireList[Id].Path, (OneWireList[Id].Present) ? "OK" : "ERROR");
                LOG_MSG(InfoText);
		 				  }  
           /*   READ_TIMER(TM2);  */
							DELTA_TIME(DeltaTime, TM2, TM1);
            	OneWireList[Id].Val[0] = CurTemp; 
            break;

            case DEV_DS2450:  // AD-sensor
           /*   READ_TIMER(TM1);  */
							if (!(ReadAD(AD, fp, DevType, OneWireList[Id].Path, ScrPad))) { // If not present then..
               	OneWireList[Id].Present = FALSE; 
                for (Idx = 0; Idx < 4; Idx++)
                  AD[Idx] = SENS_DEF_VAL;  // Set default value == indicate error

								sprintf(InfoText, "Read AD : %d %s Status : %s \n", Id, OneWireList[Id].Path, (OneWireList[Id].Present) ? "OK" : "ERROR");
								LOG_MSG(InfoText);
							}  
           /*   READ_TIMER(TM2);  */
							DELTA_TIME(DeltaTime, TM2, TM1);
							OneWireList[Id].Val[0] = AD[0];
							OneWireList[Id].Val[1] = AD[1];
							OneWireList[Id].Val[2] = AD[2];
							OneWireList[Id].Val[3] = AD[3];
            break;

            default:
              sprintf(InfoText, "Illegal device: %d\n", DevType);
              CHECK(FALSE, InfoText);
            break;
          } // switch
				} // else
      	Msg->SensorResp.Sensor  = Id; 
      	Msg->SensorResp.Status  = OneWireList[Id].Present; 
      	Msg->SensorResp.CmdTime = DeltaTime; 
      	Msg->SensorResp.Val[0]  = OneWireList[Id].Val[0]; 
      	Msg->SensorResp.Val[1]  = OneWireList[Id].Val[1]; 
      	Msg->SensorResp.Val[2]  = OneWireList[Id].Val[2]; 
     	  Msg->SensorResp.Val[3]  = OneWireList[Id].Val[3]; 
     	  Msg->SigNo = SIGReadSensorResp;
//printf("Sensor: %d Val: %f\n", Msg->SensorResp.Sensor, Msg->SensorResp.Val[0]);
     	  SEND(fd_Client, Msg, sizeof(union SIGNAL));
   	  break;
      
   	  case SIGCheckNewSensors:
//TIMER_START(TM1);
     	  Scan4Sensors();
		  //if (DebugOn) { TIMER_STOP("Scan", TM2, TM1);}
        REQ_TIMEOUT(fd_timo, fd_2own, "OneWire Check new sensors", SIGCheckNewSensors, 30 Sec); 
   	  break;

  	  default:
     	  sprintf(InfoText, "Illegal signal received: %d\n", Msg->SigNo);
     	  CHECK(FALSE, InfoText);
   	  break;		
	  } // switch
  } // while (TRUE)
}


char 						Scan4Sensors(void) {
  int              fp, Idx, Id;

// Check which sensors that are actually present and put them in list in sorted order (1..n)
  errno = 0;
  for (Idx = 0; Idx < EXP_NO_OF_DEVICES; Idx++) {
    if((fp = fopen(ExpOneWireList[Idx].Path, "r")) != NULL)  {
      fclose(fp);
      Id = ExpOneWireList[Idx].Id;
      if (!(OneWireList[Id].Present)) {
        OneWireList[Id].Present = TRUE;
        OneWireList[Id].Id  = Id;
        strncpy(OneWireList[Id].SensName, ExpOneWireList[Idx].SensName, 16);
        OneWireList[Id].DevType = ExpOneWireList[Idx].DevType;
        strncpy(OneWireList[Id].Path, ExpOneWireList[Idx].Path, 100); 
        sprintf(InfoText, "Fnd-%d: %s:%s\n", OneWireList[Id].Id, OneWireList[Id].SensName, OneWireList[Id].Path);
        LOG_MSG(InfoText);
      }  
    } else {// End if   
     printf("ERROR: %s  Sensor :  %s\n", strerror(errno), ExpOneWireList[Idx].Path);
      errno = 0;
    }
  } // End for
}

char 						ReadAD(float *AD, FILE *fp, unsigned char DevType, char *SensorPath, char *ScrPadHex) {
  int 						idx;
  char						Status;
  unsigned int 		lastcrc16;
  unsigned char		ScrPad[24]; 					
  char	        	line[100];
  float						templong; 

  // Set default values in case of error...
  for (idx = 0; idx < 4; idx++)
    AD[idx] = SENS_DEF_VAL;
  Status = FALSE;
  if((fp = fopen(SensorPath, "r")) == NULL)  {
    /* No file to read */
    //sprintf(*ScrPad, "0000000000000000");
    sprintf(InfoText, "ERROR: %s %d Can not open file %s \n", strerror(errno), errno, SensorPath);
    CHECK(FALSE, InfoText);
    // fclose(fp);  Don't close, fp == NULL!!
    return Status;
  } else
	if (fgets(line, 80, fp) == NULL) {// Read 1:th line of info from device
    fclose(fp);
    sprintf(InfoText, "Read error %s \n", SensorPath);
    LOG_MSG(InfoText);
    return Status;
  } else {
    fclose(fp);
	  ScrPad[0] = A2HexByte(line[0],  line[1]); 	// LSB Temp
		ScrPad[1] = A2HexByte(line[3],  line[4]); 	// LSB Temp
	  ScrPad[2] = A2HexByte(line[6],  line[7]); 	// LSB Temp
	  ScrPad[3] = A2HexByte(line[9],  line[10]); 	// LSB Temp
		ScrPad[4] = A2HexByte(line[12], line[13]); 	// LSB Temp
	  ScrPad[5] = A2HexByte(line[15], line[16]); 	// LSB Temp
	  ScrPad[6] = A2HexByte(line[18], line[19]); 	// LSB Temp
		ScrPad[7] = A2HexByte(line[21], line[22]); 	// LSB Temp
	  ScrPad[8] = A2HexByte(line[24], line[25]); 	// LSB Temp
 
		setcrc16();  // Check CRC
	  for( idx = 0; idx < 9; idx++ ) {
		  lastcrc16 = docrc16( ScrPad[idx] );
    }

//    sprintf(ScrPadHex, "ScrPad: %02X [%02X] %02X %02X %02X %02X %02X %02X %02X %02X\n", lastcrc16, ScrPad[8], ScrPad[7],ScrPad[6], ScrPad[5], ScrPad[4], ScrPad[3], ScrPad[2], ScrPad[1], ScrPad[0]);
 //LOG_MSG(ScrPadHex);  
    if (lastcrc16 != 0xB001) {   // verify crc16 is correct
      Status = TRUE;
      for (idx = 0; idx < 4; idx++) {
        templong = ((ScrPad[2*idx + 1] << 8) | ScrPad[2*idx]) & 0x0000FFFF;
        AD[idx] = (float) (templong /65535.0) * 5.12; //2.56; // 5.12;
      } // for
		} // if crc correct
  }	
  return Status;
} 	

char 						ReadTemp(float *Temp, FILE *fp, unsigned char DevType, char *SensorPath, char *ScrPadHex) {
   //char             InfoText[100];
	 int 							idx;
	 char							Status;
	 unsigned char 		lastcrc8;
	 unsigned char		ScrPad[10];		// Scratchpad
	 float  					hi_precision; // Calculated temperature in Centigrade							
   //FILE	            *fp;
   char	            line[80];

  Status = FALSE;
  *Temp = SENS_DEF_VAL; // Default, indicates error!
  if((fp = fopen(SensorPath, "r")) == NULL)  {
    /* No file to read */
    //sprintf(*ScrPad, "0000000000000000");
    sprintf(InfoText, "ERROR: %s %d Can not open file %s \n", strerror(errno), errno, SensorPath);
    CHECK(FALSE, InfoText);
    // fclose(fp); Don't close, fp == NULL!!
   return Status;
  } else 
	if (fgets(line, 80, fp) == NULL) {// Read 1:th line of info from device
    fclose(fp);
    sprintf(InfoText, "Read error %x %s \n", fp, SensorPath);
    LOG_MSG(InfoText);
  } else {
    fclose(fp);
		ScrPad[0] = A2HexByte(line[0],  line[1]); 	// LSB Temp
	  ScrPad[1] = A2HexByte(line[3],  line[4]); 	// LSB Temp
	  ScrPad[2] = A2HexByte(line[6],  line[7]); 	// LSB Temp
		ScrPad[3] = A2HexByte(line[9],  line[10]); 	// LSB Temp
	  ScrPad[4] = A2HexByte(line[12], line[13]); 	// LSB Temp
	  ScrPad[5] = A2HexByte(line[15], line[16]); 	// LSB Temp
		ScrPad[6] = A2HexByte(line[18], line[19]); 	// LSB Temp
	  ScrPad[7] = A2HexByte(line[21], line[22]); 	// LSB Temp
	  ScrPad[8] = A2HexByte(line[24], line[25]); 	// LSB Temp

		setcrc8();  // Check CRC
	  for( idx = 0; idx < 9; idx++ ) {
		  lastcrc8 = docrc8( ScrPad[idx] );
    } 
		if ((lastcrc8 != 0x00) || (ScrPad[5] != 0xFF) /*|| (ScrPad[4] != 0x7F)*/) {	// If we suspect invalid data
      Status = FALSE;
	    sprintf(ScrPadHex, "ScrPad: %02X [%02X] %02X %02X %02X %02X %02X %02X %02X %02X", lastcrc8, ScrPad[8], ScrPad[7],ScrPad[6], ScrPad[5], ScrPad[4], ScrPad[3], ScrPad[2], ScrPad[1], ScrPad[0]);
      printf("CRC Error:  %s\n", ScrPadHex);
		} else { // Correct CRC received, calculate temperature!
	    Status = TRUE;
	    if ((DevType == DEV_DS18B20) || (DevType == DEV_DS1822)) {
				short int temp2 = (ScrPad[1] << 8) | ScrPad[0];
     	  *Temp = temp2 / 16.0;
			} else { // Dev 18S20
				if( ScrPad[1] == 0 )  {
		  	  *Temp = (int) ScrPad[0] >> 1;
	 		  } else {
					*Temp = -1 * (int) (0x100-ScrPad[0]) >> 1;
	 		  } /* Negative temp calculation */
	 			*Temp -= 0.25;
	 		  hi_precision = (int) ScrPad[7] - (int) ScrPad[6];
	 			hi_precision = hi_precision / (int) ScrPad[7];
	 			*Temp = *Temp + hi_precision;
			}
		}
  }
if (*Temp == 85) {
// +85 temp indicates that the sensor have just started so it should not occur unless you have some powering problems..
//printf("85 temp received!!!\n");
	Status = FALSE;		// Set status FALSE so that the value is not reported
}
	return Status;
}
int 						A2HexByte(char A1, char A2) {
	int HexByte;

	HexByte = 16 * ToHex(A1) + ToHex(A2);  	//printf("A1 : %c A2: %c Hex %02X\n", A1, A2, HexByte);
  return HexByte;
}
unsigned char 	Hex2Ascii(unsigned char No) {

//------------------------------------------------------------------------
// Convert 1 hex character to Ascii.  

   switch(No) {
			case  0: return '0'; break;
			case  1: return '1'; break;
			case  2: return '2'; break;
			case  3: return '3'; break;
			case  4: return '4'; break;
			case  5: return '5'; break;
			case  6: return '6'; break;
			case  7: return '7'; break;
			case  8: return '8'; break;
			case  9: return '9'; break;
			case 10: return 'A'; break;
			case 11: return 'B'; break;
			case 12: return 'C'; break;
			case 13: return 'D'; break;
			case 14: return 'E'; break;
			case 15: return 'F'; break;
			default: 
				sprintf(InfoText, "Error: Illegal Number %d\n", No);
        CHECK(FALSE, InfoText);
      	return 0;
			break;
		}
}
int 	          ToHex(char ch) {
//------------------------------------------------------------------------
// Convert 1 hex character to binary.  If not a hex character then
// return 0.
//
   if ((ch >= '0') && (ch <= '9'))
      return ch - 0x30;
   else if ((ch >= 'A') && (ch <= 'F'))
      return ch - 0x37;
   else if ((ch >= 'a') && (ch <= 'f'))
      return ch - 0x57;
   else
	  sprintf(InfoText, "Error: Illegal char %X\n", ch);
    CHECK(FALSE, InfoText);
   return 0;
};
