/*************************************************************************
 *      OneWireHandler.c
 *
 *      Ver  Date       Name Description
 *      PA0  2006-11-24 AGY  Created.
 *      PA1  2007-10-15 AGY  Changed so that 1-w handling only on request from client
 *			PA3	 2014-01-10 AGY  Added OWFS file system, read 1_wire directly from filesystem  
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
#include "OneWireHandlerOWFSFile.h"

// Declaration of prototypes

char 	          LCD1W_Write(int LCD_Id, int Line, char *Msg);
char 	          Scan4Sensors(void);
float	          CalculateMeanValue(int SensorId, float Temp);
char						ReadAD(float *AD, FILE *fp, unsigned char DevType, char *SensorPath, char *ScrPAdHex);
char						ReadADALL(float *AD, float Factor, FILE *fp, unsigned char DevType, char *SensorPath, char *ScrPAdHex);
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


  //setpriority(PRIO_PROCESS, 0, -20);
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


  Scan4Sensors();
//	if (OneWireList[8].Present)  // Init 1W LCD if present
//	  Init1WLCD(8);
		

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
		Msg = (void *) Buf;
		fp = NULL;
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
								if (!(ReadADALL(AD, OneWireList[Id].Data, fp, DevType, OneWireList[Id].Path, ScrPad))) { //Test 
							//	if (!(ReadAD(AD, fp, DevType, OneWireList[Id].Path, ScrPad))) { // If not present then..
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
char 						Set1WLCDOn(int LCD_Id) {
  int           fp;
	char					Addr[100];

if (!OneWireList[LCD_Id].Present) { // Check that LCD attached
		sprintf(InfoText, "Err LCD%d Not initiated\r\n", LCD_Id);
		LOG_MSG(InfoText);
	}	else {
  	sprintf(Addr, "%s%s%s", OWFS_MP, OneWireList[LCD_Id].Path,"/LCDon"); 
    OPEN_PIPE(fp, Addr,	O_WRONLY|O_NONBLOCK);
  	write(fp, "1", 1); // Turn LCD On
  	if (errno != 0)
  		printf("Write	error LCDon: %s %d\r\n", strerror(errno), errno);
  	close(fp);
  	usleep(50000); // We need a delay before next command to LCD
  	close(fp);
	}
}
char 						Set1WLCDBlkOn(int LCD_Id) {
  int           fp;
	char					Addr[100];

	if (!OneWireList[LCD_Id].Present) { // Check that LCD attached
		sprintf(InfoText, "Err LCD%d Backlight ON, not initiated\r\n", LCD_Id);
		LOG_MSG(InfoText);
	}	else {
	  sprintf(Addr, "%s%s%s", OWFS_MP, OneWireList[LCD_Id].Path,"/backlight"); 
    OPEN_PIPE(fp, Addr, O_WRONLY|O_NONBLOCK);
	  write(fp, "1", 1); // Turn backlight On
	  if (errno != 0)
	  	 printf("Write error LCDbcklgt: %s %d\r\n", strerror(errno), errno);
	  usleep(5000);
	  close(fp);
	}
}
char 						Set1WLCDBlkOff(int LCD_Id) {
  int           fp;
	char					Addr[100];

	if (!OneWireList[LCD_Id].Present) {  // Check that LCD attached
		sprintf(InfoText, "Err LCD%d Backlight OFF, not initiated\r\n", LCD_Id);
		LOG_MSG(InfoText);
	}	else {
	  sprintf(Addr, "%s%s%s", OWFS_MP, OneWireList[LCD_Id].Path,"/backlight"); 
    OPEN_PIPE(fp, Addr, O_WRONLY|O_NONBLOCK);
	  write(fp, "0", 1); // Turn backlight OFF
	  if (errno != 0)
	  	 printf("Write error LCDbcklgt: %s %d\r\n", strerror(errno), errno);
	  usleep(5000);
	  close(fp);
	}

}
char 						LCD1W_Write(int LCD_Id, int Line, char *Msg) {
  int             fp, Id;
	char						Addr[100];

	if (!OneWireList[LCD_Id].Present) { // Check that LCD attached
		sprintf(InfoText, "Err LCD%d Write, not initiated\r\n", LCD_Id);
		LOG_MSG(InfoText);
	}	else {
		errno = 0;	
		switch(Line) {
			case 1: 
				sprintf(Addr, "%s%s%s", OWFS_MP, OneWireList[LCD_Id].Path, "/line20.0");	
				OPEN_PIPE(fp, Addr, 	O_WRONLY|O_NONBLOCK);
				write(fp, Msg, 20);
				close(fp);
			break;
			case 2:  
				sprintf(Addr, "%s%s%s", OWFS_MP, OneWireList[LCD_Id].Path, "/line20.1");		
				OPEN_PIPE(fp, Addr, 	O_WRONLY|O_NONBLOCK);
				write(fp, Msg, 20);
				close(fp);
			break;
			case 3: 
				sprintf(Addr, "%s%s%s", OWFS_MP, OneWireList[LCD_Id].Path, "/line20.2");		
				OPEN_PIPE(fp, Addr, 	O_WRONLY|O_NONBLOCK);
				write(fp, Msg, 20);
				close(fp);
			break;
			case 4: 	
				sprintf(Addr, "%s%s%s", OWFS_MP, OneWireList[LCD_Id].Path, "/line20.3");		
				OPEN_PIPE(fp, Addr, 	O_WRONLY|O_NONBLOCK);
				write(fp, Msg, 20);
				close(fp);
			break;
			default:	sprintf(InfoText, "Illegal LCD line: %d\n", Line);
								CHECK(FALSE, InfoText);
			break;
		}
		
		usleep(500);
		if (errno != 0)
			printf("Write error LCD: %s Msg: %s\r\n", strerror(errno), Msg);
	}
}

char 						Scan4Sensors(void) {
  int              Idx, Id;
  FILE             *fp;
	char							Addr[100];

// Check which sensors that are actually present and put them in list in sorted order (1..n)
  errno = 0;
  for (Idx = 0; Idx < (int) EXP_NO_OF_DEVICES; Idx++) {
		sprintf(Addr, "%s%s", OWFS_MP, ExpOneWireList[Idx].Path); 

		//printf(Addr);  // For debugging of new sensors
		//printf("-\r\n");
		
    if((fp = fopen(Addr, "r")) != NULL)  {
      fclose(fp);		
	//printf("Open ok: %s\r\n", Addr);  // For debugging of new sensors
      Id = ExpOneWireList[Idx].Id;

      if (!(OneWireList[Id].Present)) {
        OneWireList[Id].Present = TRUE;
        OneWireList[Id].Id  = Id;
        strncpy(OneWireList[Id].SensName, ExpOneWireList[Idx].SensName, 16);
        strncpy(OneWireList[Id].Path, ExpOneWireList[Idx].Path, 100); 
        OneWireList[Id].DevType = ExpOneWireList[Idx].DevType;
				OneWireList[Id].Data    = ExpOneWireList[Idx].Data;

        sprintf(InfoText, "Fnd[%d] %s:%s Fct:%3.2f\n", OneWireList[Id].Id, OneWireList[Id].SensName, OneWireList[Id].Path, OneWireList[Id].Data );
        LOG_MSG(InfoText);
				if (DEV_LCD == OneWireList[Id].DevType) {  // Initiate all 1W LCDs
				  ProcState.DevLCDDefined = TRUE;
	        Set1WLCDOn(OneWireList[Id].Id);   // Turn Display ON
	        Set1WLCDBlkOn(OneWireList[Id].Id); // Turn backlight ON
					sprintf(InfoText, "%s initiated\r\n", OneWireList[Id].SensName);
					LOG_MSG(InfoText);
				}
      }  
    } else {// End if   
    // printf("ERROR: %s  Sensor :  %s\n", strerror(errno), ExpOneWireList[Idx].Path);
      errno = 0;
    }
  } // End for
}
char			ReadADALL(float *AD, float ConvLevelAD, FILE *fp, unsigned char DevType, char *SensorPath, char *ScrPadHex) {
  int 						idx;
	char   					ADReadIdx;
  char						Status;
  unsigned int 		lastcrc16;
  unsigned char		ScrPad[24]; 					
  char	        	Address[200], line[200];
  float						templong; 
	
  // Set default values
	Status = FALSE;
	ADReadIdx = 6;  // Set number of re-tries at AD read out
  for (idx = 0; idx < 4; idx++)
    AD[idx] = SENS_DEF_VAL;
  sprintf(Address, "%s%s%s", OWFS_MP, SensorPath, "/volt.ALL");  //Note, change to "volt2." for 2.55 V conversion
 // printf ("AD Fact: %3.2f %s\r\n", ConvLevelAD, SensorPath);
	
	// read Channel A
	if((fp = fopen(Address, "r")) == NULL)  {
		sprintf(InfoText, "ERROR: %s %d Cannot open file %s \n", strerror(errno), errno, SensorPath);
		CHECK(FALSE, InfoText);
		// fclose(fp); Don't close, fp == NULL!!
		return Status;
	} else {
		while (ADReadIdx > 0) {
			if (fgets(line, 200, fp) != NULL) { // Read 1:th line of info from device, if ok continue
				ADReadIdx = 0; // Break while loop
				Status = TRUE;
			} else	{			
				usleep(5000);  // Wait and try again	
				//sprintf(InfoText, "%d: Read AD again %x %s \n", (6- ADReadIdx),fp, SensorPath);
   			//LOG_MSG(InfoText);
				ADReadIdx--;
			}
		}	// End while
		if (!Status) { // ERROR: Not able to read AD...
			fclose(fp);
			sprintf(InfoText, "Read error %x %s %s\n", fp, SensorPath, line);
			LOG_MSG(InfoText);
		} else {  // Everything just fine...continue
			//	sprintf(InfoText, "Read AD-All %s\n", line);
			//	LOG_MSG(InfoText);
			fclose(fp);	
			AD[0] = ConvLevelAD * atof(&line[3]);
			AD[1] = ConvLevelAD * atof(&line[13]);
			AD[2] = ConvLevelAD * atof(&line[26]);
			AD[3] = ConvLevelAD * atof(&line[40]);
			//sprintf(InfoText, "Fact: %f %s :AD0 %10.6f AD1 %10.6f AD2 %10.6f AD3 %10.6f\r\n", ConvLevelAD, line, AD[0], AD[1], AD[2], AD[3]);	
			//LOG_MSG(InfoText);
		} 
	}
	return Status;
}
	
	

char 						ReadAD(float *AD, FILE *fp, unsigned char DevType, char *SensorPath, char *ScrPadHex) {
  int 						idx;
  char						Status;
  unsigned int 		lastcrc16;
  unsigned char		ScrPad[24]; 					
  char	        	AddrChA[100], AddrChB[100], AddrChC[100], AddrChD[100], line[80];
  float						templong, ConvLevelAD;  // Note, see ReadAD all for use of ConvLevel, read from OneWireList..7 Apr 2015
	
  // Set default values
	Status = FALSE;
  for (idx = 0; idx < 4; idx++)
    AD[idx] = SENS_DEF_VAL;
  Status = FALSE;
  sprintf(AddrChA, "%s%s%s", OWFS_MP, SensorPath, "/volt.A");  //Note, change to "volt2." for 2.55 V conversion
	sprintf(AddrChB, "%s%s%s", OWFS_MP, SensorPath, "/volt.B");
	sprintf(AddrChC, "%s%s%s", OWFS_MP, SensorPath, "/volt.C");
	sprintf(AddrChD, "%s%s%s", OWFS_MP, SensorPath, "/volt.D");

	// read Channel A
	if((fp = fopen(AddrChA, "r")) == NULL)  {
		sprintf(InfoText, "ERROR: %s %d Can not open file %s \n", strerror(errno), errno, SensorPath);
		CHECK(FALSE, InfoText);
		// fclose(fp); Don't close, fp == NULL!!
		return Status;
	} else {
		if (fgets(line, 80, fp) == NULL) {// Read 1:th line of info from device
			fclose(fp);
			sprintf(InfoText, "Read error %x %s %s\n", fp, SensorPath, line);
			LOG_MSG(InfoText);
		} else {
			fclose(fp);	
			//Status = TRUE;  // I printed the sequence to get start adress, see for loop below
			AD[0] = ConvLevelAD * atof(&line[3]);
			//printf("%s :AD0 %10.6f \r\n", line, AD[0]);

			// Continue by reading Channel B
			if((fp = fopen(AddrChB, "r")) == NULL)  {
				sprintf(InfoText, "ERROR: %s %d Can not open file %s \n", strerror(errno), errno, SensorPath);
				CHECK(FALSE, InfoText);
				// fclose(fp); Don't close, fp == NULL!!
				return Status;
			} else {
				if (fgets(line, 80, fp) == NULL) {// Read 1:th line of info from device
					fclose(fp);
					sprintf(InfoText, "Read error %x %s \n", fp, SensorPath);
					LOG_MSG(InfoText);
				} else {
					fclose(fp);	
					Status = TRUE;  // I printed the sequence to get start adress, see for loop below
					AD[1] = ConvLevelAD * atof(&line[3]);
					//printf("%s :AD1 %10.6f \r\n", line, AD[1]);
				}
			}
		
			// Continue by reading Channel C
			if((fp = fopen(AddrChC, "r")) == NULL)  {
				sprintf(InfoText, "ERROR: %s %d Can not open file %s \n", strerror(errno), errno, SensorPath);
				CHECK(FALSE, InfoText);
				// fclose(fp); Don't close, fp == NULL!!
				return Status;
			} else {
				if (fgets(line, 80, fp) == NULL) {// Read 1:th line of info from device
					fclose(fp);
					sprintf(InfoText, "Read error %x %s \n", fp, SensorPath);
					LOG_MSG(InfoText);
				} else {
					fclose(fp);	
					Status = TRUE;  // I printed the sequence to get start adress, see for loop below
					AD[2] = ConvLevelAD * atof(&line[3]);
					//printf("%s :AD2 %10.6f \r\n", line, AD[2]);
					Status = TRUE; // All sensors read correctly
				}
			}
			
		} 
	return Status;
	}
	
} 	

char 						ReadTemp(float *Temp, FILE *fp, unsigned char DevType, char *SensorPath, char *ScrPadHex) {
   //char             InfoText[100];
	 int 							idx;
	 char							Status;
	 unsigned char 		lastcrc8;
	 unsigned char		ScrPad[10];		// Scratchpad
	 float  					hi_precision; // Calculated temperature in Centigrade							
   //FILE	            *fp;
   char	            Addr[100], line[80];

  Status = FALSE;
  *Temp = SENS_DEF_VAL; // Default, indicates error!
 
	sprintf(Addr, "%s%s%s", OWFS_MP, SensorPath, "/temperature");
	
  if((fp = fopen(Addr, "r")) == NULL)  {
    /* No file to read */
    //sprintf(*ScrPad, "0000000000000000");
    sprintf(InfoText, "ERROR: %s %d Can not open file %s \n", strerror(errno), errno, SensorPath);
    CHECK(FALSE, InfoText);
    // fclose(fp); Don't close, fp == NULL!!
   return Status;
  } else 
		if (fgets(line, 20, fp) == NULL) {// Read 1:th line of info from device
			fclose(fp);
			sprintf(InfoText, "Read error %x %s \n", fp, SensorPath);
			LOG_MSG(InfoText);
  } else {
		fclose(fp);	
		Status = TRUE;
		*Temp = atof(line);
		//printf("Temp %5.2f %s\r\n", *Temp, line);
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
