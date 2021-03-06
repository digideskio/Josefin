
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
#include "OWHndlrOWFSFile.h"

// Declaration of prototypes

char 	          LCD1W_Write(int LCD_Id, int Line, char *Msg);
char 	          Scan4Sensors(void);
float	          CalculateMeanValue(int SensorId, float Temp);
//char						ReadAD(float *AD, FILE *fp, unsigned char DevType, char *SensorPath, char *ScrPAdHex);
char						ReadADALL(float *AD, int Factor, FILE *fp, unsigned char DevType, char *SensorPath, char *ScrPAdHex);
char						ReadTemp(float *Temp, FILE *fp, unsigned char DevType, char *SensorPath, char *ScrPadHex);
int 						A2HexByte(char A1, char A2);
int 						ToHex(char ch);
unsigned char 	Hex2Ascii(unsigned char No);
char            TimeVal0[40];  // Stores time values, to measure executing times
char            TimeVal1[40];  // Stores time values, to measure executing times
char            TimeVal2[40];  // Stores time values, to measure executing times
char            Initiated;     // Indicate if Mean value list is initiated or not.

// Global variables

char    		InfoText[300];  // Store info text, Devpath may be long...!
long long 	RT, TM1, TM2, StartCycles, EndCycles;
float				DeltaTime;

void  				* OneWireHandler(struct ProcState_s *PState) {
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
// Setup all Signal pipes for communication
  fd_own   = PState->fd.RD_OWPipe;
  fd_2own  = PState->fd.WR_OWPipe;
  fd_2main = PState->fd.WR_MainPipe;
  fd_timo  = PState->fd.WR_TimoPipe;
  //sprintf (InfoText, " Own %d ToOwn %d Main %d Timo %d \r\n", fd_own, fd_2own, fd_2main, fd_timo);
  //LOG_MSG(InfoText);
  
  
 // OPEN_PIPE(fd_own,    ONEWIRE_PIPE, O_RDONLY|O_NONBLOCK);
 // OPEN_PIPE(fd_2own,   ONEWIRE_PIPE, O_WRONLY);
 // OPEN_PIPE(fd_2main,  MAIN_PIPE,    O_WRONLY);
 // OPEN_PIPE(fd_timo,   TIMO_PIPE,    O_WRONLY);
//  REQ_TIMEOUT(fd_timo, fd_2own, "OneWire", SIGInitMeasAD, 2 Sec); 
//  REQ_TIMEOUT(fd_timo, fd_2own, "OneWire", SIGInitMeasTemp, 2 Sec); 
  REQ_TIMEOUT(fd_timo, fd_2own, "OneWire", SIGCheckNewSensors, 5 Sec); 
  LOG_MSG("Started\n");

	while(TRUE) {
	  WAIT(fd_own, Buf, sizeof(union SIGNAL));
  	Msg = (void *) Buf;
		fp = NULL;
//		sprintf(InfoText, "Rec Sig: %d \n", Msg->SigNo );
//   LOG_MSG(InfoText); //sleep(1);
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
						//	DELTA_TIME(DeltaTime, TM2, TM1);
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
							//DELTA_TIME(DeltaTime, TM2, TM1);
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
     	  sprintf(InfoText, "Illegal signal received: %d MsgLen: %d Data: %x %x %x %x\n", Msg->SigNo, sizeof(Msg), Msg->Data[0], Msg->Data[1],Msg->Data[2],Msg->Data[3]);
     	  CHECK(FALSE, InfoText);
   	  break;		
	  } // switch
  } // while (TRUE)
}
char 						Set1WLCDOn(int LCD_Id) {
  int           fp;
	char					Addr[300];

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
	char					Addr[300];

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
	char					Addr[300];

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
	char						Addr[300];

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
  int              Idx, Id, PwrMode;
  FILE             *fp;
	char							Addr[300], PwrAddr[300], line[10];

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
				
	      // Check and print power mode of device
        PwrMode = 0; // Default, not power mode
        sprintf(PwrAddr, "%s%s", Addr, "/power");  
 	      if((fp = fopen(PwrAddr, "r")) != NULL)  {
          if (fgets(line, 5, fp) != NULL) {// Read 1:th line of info from device
            //printf("%s \r\n", line);
            if (line[0] != '0')
	            PwrMode = 1;
          }  
		    } 				
				// Print info about found sensor
				sprintf(InfoText, "Fnd[%d] %s:%s Fct:%df Pwr: %d\n", OneWireList[Id].Id, 
                                                             OneWireList[Id].SensName, 
                                                             OneWireList[Id].Path, 
                                                             OneWireList[Id].Data,
                                                             PwrMode);
				LOG_MSG(InfoText);
// Check if we have the master OWFS device attached == This is "JosefinShip"!
				if (!strncmp(OneWireList[Id].Path, JosefinShipID, sizeof(JosefinShipID))) { // Check if we have master ID connected
				  sprintf(ProcState.DeviceName,"%s", "JosefinShip");
					sprintf(InfoText, "Defined %s \r\n", ProcState.DeviceName);
					LOG_MSG(InfoText);		
				} 

				if (DEV_LCD == OneWireList[Id].DevType) {  // Initiate all 1W LCDs
					ProcState.DevLCDDefined = TRUE;
          ProcState.LCD_Id = Id;
					Set1WLCDOn(OneWireList[Id].Id);   // Turn Display ON
					Set1WLCDBlkOn(OneWireList[Id].Id); // Turn backlight ON
					sprintf(InfoText, "%s initiated\r\n", OneWireList[Id].SensName);
					LOG_MSG(InfoText);
				}
      }  
    } else {// End if   
     //printf("ERROR: %s  Sensor :  %s\n", strerror(errno), ExpOneWireList[Idx].Path);
      errno = 0;
    }
  } // End for
}
char			ReadADALL(float *AD, int ConvLevelAD, FILE *fp, unsigned char DevType, char *SensorPath, char *ScrPadHex) {
  int 						idx, ADno, ADidx;
	char   					ADReadIdx;
  char						Status;
  unsigned int 		lastcrc16;
  unsigned char		ScrPad[24]; 					
  char	        	*Str, Address[300], TempLine[300], line[300], ADline[4][20];
  float						templong; 
	
 
 // Set default values
	Status = FALSE;
	ADReadIdx = 6;  // Set number of re-tries at AD read out
  for (idx = 0; idx < 4; idx++)
    AD[idx] = SENS_DEF_VAL;
  // We should use uncached here to get fastest possible response!
   //sprintf(Address, "%s%s%s%s", OWFS_MP, "uncached/", SensorPath, "/volt.ALL");  //Note, change to "volt2." for 2.55 V conversion
   sprintf(Address, "%s%s%s", OWFS_MP, SensorPath, "/volt.ALL");  //Note, change to "volt2." for 2.55 V conversion

 // printf ("AD Fact: %3.2f %s\r\n", ConvLevelAD, SensorPath);
	
	if((fp = fopen(Address, "r")) == NULL)  {
		sprintf(InfoText, "ERROR: %s %d Cannot open file %s \n", strerror(errno), errno, Address);
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
        // Extract each AD value as text from the read string
				//sprintf(InfoText, "Read AD-All %s Len: %d\n", line, strlen(line));
				//LOG_MSG(InfoText);
        idx = 0;
        ADidx = 0;
        ADno =0;
        for (idx = 0; idx < (int) strlen(line); idx++) {
          //printf(" %d : %c\r\n", idx, line[idx]);
          if (line[idx] == ',') {
            ADline[ADno] [ADidx] = '\0';
            ADno++; 
            ADidx = 0;             
          } else
            ADline[ADno] [ADidx++] = line[idx];                         
        }
        ADline[ADno] [ADidx] = '\0';
        //
        //printf("%s : %s : %s : %s \r\n", ADline[0], ADline[1], ADline[2], ADline[3]);   
			fclose(fp);	
			switch ( ConvLevelAD) {		// Caluclate AD value based on type of AD we use	
				case 3:  // Old own built type used in Josefin today
					AD[0] = 2 * atof(&ADline[0][0]);
					AD[1] = 2 * atof(&ADline[1][0]);
					AD[2] = 3 * atof(&ADline[2][0]); 
					AD[3] = 3 * atof(&ADline[3][0]);  // Not used today
				break;
				
				case 5:  // New type, bought from M-Teknik 
					AD[0] = 5 * atof(&ADline[0][0]);
					AD[1] = 5 * atof(&ADline[1][0]);
					AD[2] = 5 * atof(&ADline[2][0]);
					AD[3] = 5 * atof(&ADline[3][0]);  // Not used today			
				break;
				
				default:
					AD[0] = atof(&ADline[0][0]);
					AD[1] = atof(&ADline[1][0]);
					AD[2] = atof(&ADline[2][0]);
					AD[3] = atof(&ADline[3][0]);  // Not used today		
					sprintf(InfoText, " Err: Unknown AD conversion factor\r\n");
 					LOG_MSG(InfoText);
				break;
			}
			
			if (DebugOn) {
				sprintf(InfoText, "Fact: %d T:%-40.40s \r\n", ConvLevelAD, line);
				LOG_MSG(InfoText);
				sprintf(InfoText, "[V]: AD0 %10.3f AD1 %10.3f AD2 %10.3f AD3 %10.3f\r\n", AD[0], AD[1], AD[2], AD[3]);
				LOG_MSG(InfoText);
			}	
		} 
	}
	return Status;
}
		
/*
char 						ReadAD(float *AD, FILE *fp, unsigned char DevType, char *SensorPath, char *ScrPadHex) {
  int 						idx;
  char						Status;
  unsigned int 		lastcrc16;
  unsigned char		ScrPad[24]; 					
  char	        	AddrChA[300], AddrChB[300], AddrChC[300], AddrChD[300], line[300];
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
*/
char 						ReadTemp(float *Temp, FILE *fp, unsigned char DevType, char *SensorPath, char *ScrPadHex) {
   //char             InfoText[100];
	 int 							idx;
	 char							Status;
	 unsigned char 		lastcrc8;
	 unsigned char		ScrPad[10];		// Scratchpad
	 float  					hi_precision; // Calculated temperature in Centigrade							
   //FILE	            *fp;
   char	            Addr[300], line[300];

  Status = FALSE;
  *Temp = SENS_DEF_VAL; // Default, indicates error!
 
	sprintf(Addr, "%s%s%s", OWFS_MP, SensorPath, "/temperature");
  //sprintf(Addr, "%s%s%s%s", OWFS_MP, "uncached/", SensorPath, "/temperature"); // Uncached version
	
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
