/*************************************************************************
 *      OneWireHandler.c
 *
 *      Ver  Date       Name Description
 *      W    2006-11-24 AGY  Created.
 *      PA1  2007-10-15 AGY  Changed so that 1-w handling only on request from client
 *			W		 2013-02-19	AGY
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
#include "OneWireHandlerHA7S.h"
#include <wiringPi.h>
//#include <lcd.h>
#include <wiringSerial.h>

//#define LclDbg   

// Declaration of prototypes
int 	    			Scan4Sensors(int fd);
float	    			CalculateMeanValue(int SensorId, float Temp);
int							Reset1W(int fd);
void  					reset1W(int fd);

int 						InitDS2450(int fd, char *Adr);
int 						Read1WADAll(int fd, char *Adr, unsigned char DevType, float *AD);
int 						Read1WTempAll(int fd, char *Adr, unsigned char DevType, float *Temp);
unsigned char 	Read1WParPwr(int fd, char *Adr);
unsigned char		Print1WLcd(int fd, char *Adr, char *Str);
unsigned char 	Write1WLcd(int fd, char *Adr);
unsigned char		Ctrl1WLcd(int fd, char *Adr, int Cmd); // return TRUE or FALSE
int							Read1WCmd(int fd, unsigned char *Str, int ExpNoOfCh);
unsigned char		Search1WDevices(int fd, char Cmd, char *Buf);
int 						A2HexByte(char A1, char A2);
int 						ToHex(char ch);
unsigned char 	Hex2Ascii(unsigned char No);

char            TimeVal0[40];  // Stores time values, to measure executing times
char            TimeVal1[40];  // Stores time values, to measure executing times
char            TimeVal2[40];  // Stores time values, to measure executing times
char            Initiated;     // Indicate if Mean value list is initiated or not.
char            PwrCtrlInit;   // Indicate if Pwr ctrl initiated or not.

// Global variables
struct 					OneWireList_s OneWireList[MAX_NO_OF_DEVICES]; // Maximum no of allowed devices on 1-wire net!
char    				InfoText[100];  // Store info text, Devpath may be long...!
long long 			RT, TM1, TM2, StartCycles, EndCycles;
float						DeltaTime;

void  				* OneWireHandler(enum ProcTypes_e ProcType) {
  FILE             *fp;
  int          	   	fd_HA7S, fd_Client, fd_2own, fd_own, fd_2main, fd_timo;
  unsigned char			Buf[sizeof(union SIGNAL)];
  union SIGNAL 		 *Msg;
  int								Idx, Id;
  int              	DevType;
  char             	ScrPad[100];
  float							CurTemp, CurAD[4];
  char             	Status;

  // Init list, NO sensors present
	Initiated = FALSE;
	PwrCtrlInit = FALSE;
  for (Idx = 0; Idx < MAX_NO_OF_DEVICES; Idx++) {
    OneWireList[Idx].Present = FALSE;
    //OneWireList[Idx].fp      = NULL;
    OneWireList[Idx].DevType = 0x00;  // No sensor or sensortype defined
    OneWireList[Idx].Val[0]  = SENS_DEF_VAL; // Set default value
    OneWireList[Idx].Val[1]  = SENS_DEF_VAL; // Set default value
    OneWireList[Idx].Val[2]  = SENS_DEF_VAL; // Set default value
    OneWireList[Idx].Val[3]  = SENS_DEF_VAL; // Set default value
	}
	
	// Open serial port to HA7S -One Wire
	//if ((fd_HA7S = serialOpen ("/dev/ttyUSB0", 9600)) < 0)  {
	if ((fd_HA7S = serialOpen ("/dev/ttyAMA0", 9600)) < 0)  {
    fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
    printf("Error: Could not open port\r\n");	
    return 1 ;
  }
#ifdef LclDbg
	printf("Port opened ok, status %s : %d \r\n", strerror(errno), errno);	
#endif		
  Scan4Sensors(fd_HA7S);  // Check which sensors are connected
	
  OPEN_PIPE(fd_own,    ONEWIRE_PIPE, O_RDONLY|O_NONBLOCK);
  OPEN_PIPE(fd_2own,   ONEWIRE_PIPE, O_WRONLY);
  OPEN_PIPE(fd_2main,  MAIN_PIPE,    O_WRONLY);
  OPEN_PIPE(fd_timo,   TIMO_PIPE,    O_WRONLY);
	
  if (OneWireList[LCD_1W].Present) {
		Ctrl1WLcd(fd_HA7S, &OneWireList[LCD_1W].Path, LCD_On);
		Ctrl1WLcd(fd_HA7S, &OneWireList[LCD_1W].Path, LCD_Bkl_On);
		Ctrl1WLcd(fd_HA7S, &OneWireList[LCD_1W].Path, LCD_Clear);
		Print1WLcd(fd_HA7S, &OneWireList[LCD_1W].Path, ScrPad);

		Write1WLcd(fd_HA7S, &OneWireList[LCD_1W].Path);
	}
	
	REQ_TIMEOUT(fd_timo, fd_2own, "zzzCheck4 Sensors", SIGzzzCheck4Sensors, 30 Sec); 
	LOG_MSG("Started\n");
	while(TRUE) {
	  WAIT(fd_own, Buf, sizeof(union SIGNAL));
		Msg = Buf;
		switch(Msg->SigNo) {	
			case SIGzzzCheck4Sensors:
				Scan4Sensors(fd_HA7S); // Check if new sensors attached
				REQ_TIMEOUT(fd_timo, fd_2own, "zzzCheck4Sensors", SIGzzzCheck4Sensors, 30 Sec); 
   	  break;
			
 			case SIGReadSensorReq:
//printf("Read sensor sig rec..\r\n");
				fd_Client  = Msg->SensorReq.Client_fd;
				Id = Msg->SensorReq.Sensor;
			  Msg->SensorResp.Sensor  = Id; 
				Msg->SensorResp.Status  = OneWireList[Id].Present; 
				if (OneWireList[Id].Present) {
					switch (OneWireList[Id].DevType) {
						case	DEV_DS1820:
						case	DEV_DS1822:
						case	DEV_DS18B20:
							if (Read1WTempAll(fd_HA7S, &OneWireList[Id].Path, OneWireList[Id].DevType, &OneWireList[Id].Val[0])) {
								if (DebugOn) printf("T:%d %+5.1f %s\r\n", Id, OneWireList[Id].Val[0], OneWireList[Id].Path);	
							}	else {
							  OneWireList[Id].Present = FALSE; // Put sensor off-line if no response recieved
								sprintf(InfoText, "Err: %+5.1f Sensor %d: %s off-line\r\n",  OneWireList[Id].Val[0], OneWireList[Id].Id, OneWireList[Id].SensName);
								LOG_MSG(InfoText);
							}
						break;

						case DEV_DS2450:
							if (Read1WADAll(fd_HA7S, &OneWireList[Id].Path, OneWireList[Id].DevType, &OneWireList[Id].Val[0])) {
								if (DebugOn) printf("AD:%d %5.1f %5.1f %5.1f %5.1f %s\r\n", Id, OneWireList[Id].Val[0], 
																		OneWireList[Id].Val[1],OneWireList[Id].Val[2], OneWireList[Id].Val[3], OneWireList[Id].Path);	
							}	else {
							  OneWireList[Id].Present = FALSE; // Put sensor off-line if no response recieved
								sprintf(InfoText, "Err: %5.1f Sensor %d: %s off-line\r\n", OneWireList[Id].Val[0], OneWireList[Id].Id, OneWireList[Id].SensName);
								LOG_MSG(InfoText);
							}						
						break;
						
						default:
							sprintf(InfoText, "Illegal sensor received: \n");
							CHECK(FALSE, InfoText);
						break;
					} // end switch
				} // End if	
			
				Msg->SensorResp.CmdTime = DeltaTime; // Not used
				Msg->SensorResp.Val[0]  = OneWireList[Id].Val[0]; 
				Msg->SensorResp.Val[1]  = OneWireList[Id].Val[1]; 
				Msg->SensorResp.Val[2]  = OneWireList[Id].Val[2]; 
				Msg->SensorResp.Val[3]  = OneWireList[Id].Val[3]; 
				Msg->SigNo = SIGReadSensorResp;
//printf("Sensor: %d Val: %f\n", Msg->SensorResp.Sensor, Msg->SensorResp.Val[0]);
				SEND(fd_Client, Msg, sizeof(union SIGNAL));
   	  break;
      
  	  default:
     	  sprintf(InfoText, "Illegal signal received: %d\n", Msg->SigNo);
     	  CHECK(FALSE, InfoText);
   	  break;		
	  } // switch
  } // while (TRUE)
}

int 				Scan4Sensors(int fd) {
  int           ExpIdx, Id, Idx, NoOfSensors;
	struct 				OneWireList_s TempOneWireList[MAX_NO_OF_DEVICES]; // Maximum no of allowed devices on 1-wire net!
	int						Found = FALSE;
	int 					No, NotEndOfList;
// Check which sensors that are actually present and put them in list in sorted order (1..n)
		
	Reset1W(fd);
	// Search through list for all connected devices
	Idx = 0; // None found so far...	
	NotEndOfList = TRUE;
	if (Search1WDevices(fd, 'S', &TempOneWireList[Idx++].Path))
		while ((Search1WDevices(fd, 's', &TempOneWireList[Idx++].Path) && (NotEndOfList))) {
			if (Idx == MAX_NO_OF_DEVICES) {
				sprintf(InfoText, "End of list (%d) No sensors found \r\n", Idx);
				LOG_MSG(InfoText);
				NotEndOfList = FALSE;
			}
		}
	
	if (NotEndOfList)   // We went through the list without problems...
		NoOfSensors = --Idx;
	else
		NoOfSensors = MAX_NO_OF_DEVICES-1; // Problems, set list to MAX NO OF DEVICES...although crap in list!
  
/*		
#ifdef LclDbgNotUsed
	printf("No of fnd sensors %d \r\n", NoOfSensors);	
	//Print list to check...
	if 	(!PwrCtrlInit) {
		for (Idx = 0; Idx < NoOfSensors; Idx++) {
			if (Read1WParPwr(fd, TempOneWireList[Idx].Path))
				sprintf(InfoText, "Fnd HA7S Idx: %d:%s ParPwr\r\n", Idx, TempOneWireList[Idx].Path);
			else	
				sprintf(InfoText, "Fnd HA7S Idx: %d:%s ExtPwr\r\n", Idx, TempOneWireList[Idx].Path);
				LOG_MSG(InfoText);
		}
	}
	PwrCtrlInit = TRUE;  // Should only be checked once!			
#endif
*/


  for (Idx = 0; Idx < NoOfSensors; Idx++) { // Search HA7S list and match with expected
  	Found = FALSE;
		for (ExpIdx = 0; ExpIdx < MAX_NO_OF_DEVICES; ExpIdx++) {
		 //printf("Start cmp Idx: %d ExpIdx %d\r\n", Idx, ExpIdx);
			if (!Found) // To be included in for loop...above 2:nd parameter
				if ((strncmp(TempOneWireList[Idx].Path, ExpOneWireList[ExpIdx].Path, 16)) == 0) {
//printf("Match 1-w List        Idx %d : %s\r\n", Idx, TempOneWireList[Idx].Path);
//printf("With  Exp 1-w List ExpIdx %d : %s\r\n", ExpIdx, ExpOneWireList[ExpIdx].Path);
					Found = TRUE;
					TempOneWireList[Idx].Present = TRUE;
					Id = ExpOneWireList[ExpIdx].Id;
					if (!(OneWireList[Id].Present)) { // Check if already in list
						OneWireList[Id].Present = TRUE;
						OneWireList[Id].Id  = Id;
						strncpy(OneWireList[Id].SensName, ExpOneWireList[ExpIdx].SensName, 16);
						OneWireList[Id].DevType = ExpOneWireList[ExpIdx].DevType;

//printf("1-w List     : %s\r\n", TempOneWireList[Id].Path);
//printf("Exp 1-w List : %s\r\n", ExpOneWireList[ExpIdx].Path);
						strncpy(OneWireList[Id].Path, ExpOneWireList[ExpIdx].Path, 16); 
	/*					if (OneWireList[Id].DevType == DEV_DS2450) {
							InitDS2450(fd, OneWireList[Id].Path);  // Initiate sensor = set parameters for AD DS2450						
						}			
*/						
				    if (Read1WParPwr(fd, OneWireList[Id].Path))
							sprintf(InfoText, "Fnd-%d: %s:  %s ParPwr \r\n", OneWireList[Id].Id, OneWireList[Id].SensName, OneWireList[Id].Path);
						else
							sprintf(InfoText, "Fnd-%d: %s:  %s ExtPwr\r\n", OneWireList[Id].Id, OneWireList[Id].SensName, OneWireList[Id].Path);					
						LOG_MSG(InfoText);
					} 
				} // if
			} // if
	}			
 	// Log all found but not defined sensors
		for (Idx = 0; Idx < NoOfSensors; Idx++) {
	//	  printf("Idx %d Present %d \r\n", Idx, TempOneWireList[Idx].Present);
			if (!TempOneWireList[Idx].Present) {
				//	TempOneWireList[Idx].Path[16] = '/0'; // End string
					sprintf(InfoText, "Err: Not def: %d %s \r\n", Idx, TempOneWireList[Idx].Path);
					LOG_MSG(InfoText);
			}
		}	// End for}
	return 1;
}
void  				reset1W(int fd)  {
			int	NoOfRecChar;
			char RecBuf[40];

			serialPutchar(fd,'R'); 
			NoOfRecChar = serialDataAvail(fd);
			RecBuf[0] = serialGetchar(fd);

			if (RecBuf[0] != '\r') {
			  printf("Error at reset : %X Rec: %d\r\n", RecBuf[0], NoOfRecChar);
			}	else
			  printf("Reset ok: %X Rec: %d\r\n", RecBuf[0], NoOfRecChar);
			
}
int  				Reset1W(int fd)  {
			int	NoOfRecChar, Status = TRUE;
			char RecBuf[40];

			serialFlush(fd);
			serialPutchar(fd,'R'); 
			if (Read1WCmd(fd, RecBuf, 1) == 1) {
			//	printf("Reset ok:\r\n");
			}	else	{
			  printf("Error at reset \r\n");
				Status = FALSE;
			}
			return Status;
		}
int 				Read1WCmd(int fd, unsigned char *Str, int ExpNoOfCh) {  // return no of rec chars (incl <CR>) -1 = Error
			int		LoopIdx, Idx, AvailCh, SumRecCh;
			int 	BreakIdx = 100000;
			char	Ready = FALSE;
			
			Idx = 0;			
	    LoopIdx = 0; 
			SumRecCh = 0;
			
			memset(Str, '\0', ExpNoOfCh+5);  // Empty string + some more...
			// Read chars until all received or error detected (BreakIdx reached)
			while (!Ready) {
				LoopIdx++;
				if (serialDataAvail(fd) >  0) {
				  Str[Idx++] = serialGetchar(fd);
			//		printf("%d: %c \r\n",Idx,  Str[Idx-1]);
					SumRecCh++;		
					if ((SumRecCh == 1) && (Str[Idx-1] == '\r')) { // Check if end of list
					  Ready = TRUE;	// End of list received, break.		
					//	printf("Read1wCmd: End of list\r\n");
					}	else if (SumRecCh == ExpNoOfCh) {
					  Ready = TRUE; // Normal exit, all ok
					}	else if (SumRecCh > ExpNoOfCh) {  // We have exceeded expected no of chars, error!
						printf("Read1wCmd: Exp %d Rec &d\r\n", ExpNoOfCh, SumRecCh);	
						Ready = TRUE;
						SumRecCh = ExpNoOfCh;		
						serialFlush(fd);  // Empty rest of in-buffer
					}		
				}	else if (LoopIdx > BreakIdx) { // No chars read...just loop until timeout...
					Ready = TRUE; // Timeout..!
					printf("Read1wCmd: Timeout Exp %d Rec %d LoopIdx %d\r\n", ExpNoOfCh, SumRecCh, LoopIdx);	
					SumRecCh = -1;	
					serialFlush(fd);  // Empty in-buffer
				}	
			}			  
			return SumRecCh; // Return no of rec chars OR -1 = Error
		};			
unsigned char		Search1WDevices(int fd, char Cmd, char *Buf)  {
			int	NoOfRecChar;
			unsigned char	Status = TRUE;

			// Valid Cmd: 'S', 's', 'F', 'f'
			serialPutchar(fd,Cmd); 
		//	printf("Search: Cmd %c \r\n", Cmd);
			NoOfRecChar = Read1WCmd(fd, Buf, 17);  // Read also expected <CR>	
			if (NoOfRecChar == -1)  { // Error!
				Status = FALSE;
			  printf("Err-Search Cmd: %c Rec: %d Adr: %s\r\n", Cmd, NoOfRecChar, Buf);
			}	else if (NoOfRecChar == 1) {
				Status = FALSE;  // End of list reached
			} else {// All ok!
				Buf[16] = '\0'; // End string gracefully!
#ifdef LclDbg
				printf("Search: Cmd %c Ch: %d Buf: %s\r\n", Cmd, NoOfRecChar, Buf);
#endif				
			}
		return Status;
}	
int					Read1WTempAll(int fd, char *Adr, unsigned char DevType, float *Temp) {
			int		NoOfRecChar;
			char	Buf[40], ScrPad[8];
		  int 	Status;
			float hi_precision, temp2; // Calculated temperature in Centigrade			

			Status = FALSE;
			*Temp = SENS_DEF_VAL; // Set default value
			sprintf(Buf, "A%s\r", Adr); // Syntax = A + Id for sensor + <CR>	
			serialPuts (fd, Buf); 
			NoOfRecChar = Read1WCmd(fd, Buf, 17); 
//printf("1 %d: Adr: %s : %s: Status: %d \r\n", NoOfRecChar, Adr, Buf, (strncmp(Adr, Buf, 16)));
			if ((strncmp(Adr, Buf, 16)) != 0) {  // Check correct response, should be our adress
					sprintf(InfoText, "Err-ReadTemp %d Exp: %s Rec: %s\r\n", NoOfRecChar, Adr, Buf);
					LOG_MSG(InfoText);			
			} else { // All ok, 
//printf("1.1 %d: %s\r\n", NoOfRecChar, Buf);
				serialPuts (fd, "W0144\r");
				NoOfRecChar = Read1WCmd(fd, Buf, 3); 
//printf("2 %d: %C %C %#X Status: %d\r\n", NoOfRecChar, Buf[0], Buf[1], Buf[2], strncmp("44", Buf, 2));
				if ((strncmp("44", Buf, 2)) != 0) { // Check correct response, should be 44
					sprintf(InfoText, "Err-ReadTemp %d Exp:44 Rec: %s\r\n", NoOfRecChar, Buf);
					LOG_MSG(InfoText);
				} else { // All ok, proceed
					delay(800); // Wait for conversion to be completed, takes 750 ms
					serialPutchar (fd, 'M'); // Send match ROM command, response should be our own adress ID
					NoOfRecChar = Read1WCmd(fd, Buf, 17);
					if ((strncmp(Adr, Buf, 16)) != 0) {  // Check correct response, should be our adress
//						Buf[NoOfRecChar-1] = '\0';  // Set end of text at correct position
						sprintf(InfoText, "Err-ReadTemp &d Exp: %s Rec: %s\r\n", NoOfRecChar, Adr, Buf);
						LOG_MSG(InfoText);
					} else { // All ok, proceed
							serialPuts (fd, "W0ABEFFFFFFFFFFFFFFFFFF\r"); // Read data from adressed device
							NoOfRecChar = Read1WCmd(fd, Buf, 21);
							if (strncmp(Buf, "BEFFFFFFF", 9) == 0) {  // Check correct response
//								Buf[NoOfRecChar-1] = '\0';  // Set end of text at correct position
								sprintf(InfoText, "Err-ReadTemp Sensor missing? Rec %d: %s\r\n", NoOfRecChar, Buf);
								LOG_MSG(InfoText);
							} else { // All ok, proceed
								ScrPad[0] = A2HexByte(Buf[4], Buf[5]);
								ScrPad[1] = A2HexByte(Buf[2], Buf[3]);		
								ScrPad[6] = A2HexByte(Buf[14], Buf[15]);		
								ScrPad[7] = A2HexByte(Buf[16], Buf[17]);									
								Status = TRUE;
								if ((DevType == DEV_DS18B20) || (DevType == DEV_DS1822)) {
									short int temp2 = (ScrPad[0] << 8) | ScrPad[1];
									*Temp = temp2 / 16.0;
								} else { // Dev 18S20 == OLD version-best not to use!!!	
								  LOG_MSG("Avoid using DS18S20 (0x10) type of Temperature sensor!!!\r\n");
									if( ScrPad[0] == 0 )  {
										*Temp = (int) ScrPad[1] >> 1;
									} else {
										*Temp = -1 * (int) (0x100-ScrPad[1]) >> 1;
									} /* Negative temp calculation */
									// Below for high precision calculation 
								//	*Temp -= 0.25;
								//	hi_precision = (int) ScrPad[7]- (int) ScrPad[6];
								//	hi_precision = hi_precision / (int) ScrPad[7];
									//hi_precision = hi_precision / 16.0;
									
printf("T1 %2.3f T2 %2.3f\r\n ", *Temp, hi_precision);
	
								//	short int temp = (ScrPad[0] << 8) | ScrPad[1];
									//*Temp += temp2;		
							}
						}	
					}
			}		
		}
		

		if (*Temp == 85) {// +85 temp,sensor have just started should not occur, powering problems..
			//printf("85 temp received!!!\n");
			sprintf(InfoText, "Err-ReadTemp %s \r\n", Adr);
			LOG_MSG(InfoText);
			*Temp = SENS_DEF_VAL;
			Status = FALSE;		// Set status FALSE so that the value is not reported
		}			
		return Status;
};
int					InitDS2450(int fd, char *Adr) {  // Not used, to much troubles...

/* Setup DS2450 to use all channels, 5.12, conversion AD 15 bits (should be decreased?? Check with Anders L, compare with manual
   Note, all results from AD should now be multiplied by a factor 2 (for 5.12 V AD)
	 Remeber to set adress 1C to 40H if VCC powered
**/

			int		NoOfRecChar;
			char	Buf[40];
		  int Status;

// Set 1C to 40 Hex for VCC powered device, Select device			
			sprintf(Buf, "A%s\r", Adr); // Syntax = A + Id for sensor + <CR>	
			serialPuts (fd, Buf); 
			NoOfRecChar = Read1WCmd(fd, Buf, 17); 
			if ((strncmp(Adr, Buf, 16)) != 0) {  // Check correct response, should be our adress
				sprintf(InfoText, "Err-SetupAD %d Exp: %s Rec: %s\r\n", NoOfRecChar, Adr, Buf);
				LOG_MSG(InfoText);			
			} else { // All ok, proceed 			
				serialPuts (fd, "W045540\r");  // Setup ch AD-D (16 bit conversion, 5.12 V) 
				NoOfRecChar = Read1WCmd(fd, Buf, 9); 
printf("Set VCC pwr %d:Status: %d\r\n", NoOfRecChar, strncmp("5540", Buf, 4));
				if ((strncmp("5540", Buf, 4)) != 0) { // Check correct response
					sprintf(InfoText, "Err-Setup AD-D Exp: 5540, %d Rec: %s\r\n", NoOfRecChar, Buf);
					LOG_MSG(InfoText);
				} else {
					sprintf(InfoText, "DS2450 Initiated ok \r\n");
					LOG_MSG(InfoText);			
				}				  
			}	
			
			
			//			printf("Initiate DS2450 \r\n");
			Status = TRUE;  // All ok to start with
// Select device
			sprintf(Buf, "A%s\r", Adr); // Syntax = A + Id for sensor + <CR>	
			serialPuts (fd, Buf); 
			NoOfRecChar = Read1WCmd(fd, Buf, 17); 
			if ((strncmp(Adr, Buf, 16)) != 0) {  // Check correct response, should be our adress
					sprintf(InfoText, "Err-SetupAD %d Exp: %s Rec: %s\r\n", NoOfRecChar, Adr, Buf);
					LOG_MSG(InfoText);			
			} else { // All ok, proceed 
// Now setup channels	
				serialPuts (fd, "W045509000D\r");  // Setup ch AD-A (16 bit conversion, 5.12 V) 
				NoOfRecChar = Read1WCmd(fd, Buf, 9); 
printf("AD-A %d: Status: %d\r\n", NoOfRecChar,strncmp("5509000D", Buf, 8));
				if ((strncmp("5509000D", Buf, 5)) != 0) { // Check correct response
					sprintf(InfoText, "Err-Setup AD-A Exp: 5509000D, %d Rec: %s\r\n", NoOfRecChar, Buf);
					LOG_MSG(InfoText);
				}
			}
			
			
	// read reg 09H		
			sprintf(Buf, "A%s\r", Adr); // Syntax = A + Id for sensor + <CR>	
			serialPuts (fd, Buf); 
			NoOfRecChar = Read1WCmd(fd, Buf, 17); 
			if ((strncmp(Adr, Buf, 16)) != 0) {  // Check correct response, should be our adress
					sprintf(InfoText, "Err-SetupAD %d Exp: %s Rec: %s\r\n", NoOfRecChar, Adr, Buf);
					LOG_MSG(InfoText);			
			} else { // All ok, proceed 
// Now setup channels	
				serialPuts (fd, "W04AA0900FF\r");  // 
				NoOfRecChar = Read1WCmd(fd, Buf, 8); 
printf("\r\n Read AD-A %d: %s Status: %d\r\n", NoOfRecChar, Buf, strncmp("AA0900D8", Buf, 8));
				if ((strncmp("AA09008D", Buf, 5)) != 0) { // Check correct response
					sprintf(InfoText, "Err-Setup AD-A Exp: 5509008D, %d Rec: %s\r\n", NoOfRecChar, Buf);
					LOG_MSG(InfoText);
				}
			}		
			
			
			
			
// Select device			
			sprintf(Buf, "A%s\r", Adr); // Syntax = A + Id for sensor + <CR>	
			serialPuts (fd, Buf); 
			NoOfRecChar = Read1WCmd(fd, Buf, 17); 
			if ((strncmp(Adr, Buf, 16)) != 0) {  // Check correct response, should be our adress
				sprintf(InfoText, "Err-SetupAD %d Exp: %s Rec: %s\r\n", NoOfRecChar, Adr, Buf);
				LOG_MSG(InfoText);			
			} else { // All ok, proceed 
				serialPuts (fd, "W04550A00CD\r");  // Setup ch AD-B (16 bit conversion, 5.12 V) 
				NoOfRecChar = Read1WCmd(fd, Buf, 9); 
	printf("AD-B %d: Status: %d\r\n", NoOfRecChar, strncmp("550A00CD", Buf, 8));
				if ((strncmp("550A00CD", Buf, 5)) != 0) { // Check correct response
					sprintf(InfoText, "Err-Setup AD-B Exp: 550A00CD, %d Rec: %s\r\n", NoOfRecChar, Buf);
					LOG_MSG(InfoText);
				}
			}
// Select device			
			sprintf(Buf, "A%s\r", Adr); // Syntax = A + Id for sensor + <CR>	
			serialPuts (fd, Buf); 
			NoOfRecChar = Read1WCmd(fd, Buf, 17); 
			if ((strncmp(Adr, Buf, 16)) != 0) {  // Check correct response, should be our adress
				sprintf(InfoText, "Err-SetupAD %d Exp: %s Rec: %s\r\n", NoOfRecChar, Adr, Buf);
				LOG_MSG(InfoText);			
			} else { // All ok, proceed 		
				serialPuts (fd, "W04550D00CD\r");  // Setup ch AD-C (16 bit conversion, 5.12 V) 
				NoOfRecChar = Read1WCmd(fd, Buf, 9); 
printf("AD-C %d: Status: %d\r\n", NoOfRecChar, strncmp("550D00CD", Buf, 8));
				if ((strncmp("550D00CD", Buf, 5)) != 0) { // Check correct response
					sprintf(InfoText, "Err-Setup AD-C Exp: 550D00CD, %d Rec: %s\r\n", NoOfRecChar, Buf);
					LOG_MSG(InfoText);
				}
			}
// Select device			
			sprintf(Buf, "A%s\r", Adr); // Syntax = A + Id for sensor + <CR>	
			serialPuts (fd, Buf); 
			NoOfRecChar = Read1WCmd(fd, Buf, 17); 
			if ((strncmp(Adr, Buf, 16)) != 0) {  // Check correct response, should be our adress
				sprintf(InfoText, "Err-SetupAD %d Exp: %s Rec: %s\r\n", NoOfRecChar, Adr, Buf);
				LOG_MSG(InfoText);			
			} else { // All ok, proceed 			
				serialPuts (fd, "W04550F00CD\r");  // Setup ch AD-D (16 bit conversion, 5.12 V) 
				NoOfRecChar = Read1WCmd(fd, Buf, 9); 
printf("AD-D %d:Status: %d\r\n", NoOfRecChar, strncmp("550F00CD", Buf, 8));
				if ((strncmp("550F00CD", Buf, 5)) != 0) { // Check correct response
					sprintf(InfoText, "Err-Setup AD-D Exp: 550F00CD, %d Rec: %s\r\n", NoOfRecChar, Buf);
					LOG_MSG(InfoText);
				} 		  
			}	
	
				reset1W(fd);
			
			return Status;
};
int					Read1WADAll(int fd, char *Adr, unsigned char DevType, float *AD)  {  // return TRUE or FALSE
/*The HA7S can read/write to the DS2450. Below are some instructions on how to read the DS2450’s 4 channels
A710000000116C220 'Address device (of course you would substitute the 8 byte ROMId of your DS2450)
Write configuration data to page 1 using the HA7E's 'W'rite block command, as follows:
W0155 'Puts the DS2450 into 'Write memory' mode 
W0A080008FFFFFF00FFFFFF 'Enables channel A with 8 bit resolution W0A080008FFFFFF00FFFFFF 'Enables channel B with 8 bit resolution
 W0A080008FFFFFF00FFFFFF 'Enables channel C with 8 bit resolution W0A080008FFFFFF00FFFFFF 'Enables channel D with 8 bit resolution
2. Second, you need to tell the DS2450 to perform an A->D conversion on its input channels. 
This is done by issuing the Convert command to the DS2450 as follows:
A710000000116C220 'Resets the bus, and addresses the DS2450 (of course you would substitute the 8 byte ROMId of your DS2450) W053C0F00FFFF 
'Tells the DS2450 to perform an A->D conversion on all 4 input channels.
Now, you need to wait for 100ms with no activity on the 1-Wire bus so the DS2450 can derive power from the bus while performing the A->D conversions.
3. The third step is to read the converted values out of the DS2450, as follows:
A710000000116C220 'Resets the bus, and addresses the DS2450 (of course you would substitute the 8 byte ROMId of your DS2450)
 W0BAA0000FFFFFFFFFFFFFFFF 'Reads the first 8 bytes from memory page 0
A sample response might be: AA 0000 [00CF 00C9 0000 0000] FFFF
The data for each channel is represented in two bytes, LSB first. Data for channel 1 begins at byte #3 (0 based) in the returned response.

LSB  MSB
0x00 0x00 Channel 1  Channel 1 = 0 V ... 0 * (5.10 / 65535) = 0 V
0x00 0xCF Channel 2  Channel 2 = 52992 ... 52292 * (5.10 / 65535) = 4.069 V 
0x00 0xC9 Channel 3  Channel 3 = 51456 ... 51456 * (5.10 / 65535) = 4.004 V
0x00 0x00 Channel 4  Channel 4 = 0 V ... 0 * (5.10 / 65535) = 0 V
*/
			int		NoOfRecChar;
			char	Buf[40], ScrPad[10];
		  int Status;
			char Idx;
			
			Status = FALSE;  			
			for (Idx = 0; Idx < 3; Idx++) // Set default value
				AD[Idx] = SENS_DEF_VAL;
				
			sprintf(Buf, "A%s\r", Adr); // Syntax = A + Id for sensor + <CR>	
			serialPuts (fd, Buf); 
			NoOfRecChar = Read1WCmd(fd, Buf, 17); 
//printf("1 %d: Adr: %s : %s: Status: %d \r\n", NoOfRecChar, Adr, Buf, (strncmp(Adr, Buf, 16)));
			if ((strncmp(Adr, Buf, 16)) != 0) {  // Check correct response, should be our adress
					sprintf(InfoText, "Err-ReadAD %d Exp: %s Rec: %s\r\n", NoOfRecChar, Adr, Buf);
					LOG_MSG(InfoText);			
			} else { // All ok, 
//printf("1.1 %d: %s\r\n", NoOfRecChar, Buf);
				serialPuts (fd, "W053C0F00FFFF\r");
				NoOfRecChar = Read1WCmd(fd, Buf, 11); 
//printf("2 %d: %s Status: %d\r\n", NoOfRecChar, Buf, strncmp("3C0F0", Buf, 5));
				if ((strncmp("3C0F0", Buf, 5)) != 0) { // Check correct response, should be 44
					sprintf(InfoText, "Err-ReadAD %d Exp:3C0F0 Rec: %s\r\n", NoOfRecChar, Buf);
					LOG_MSG(InfoText);
				} else { // All ok, proceed
					delay(200); // Wait for conversion to be completed, takes 150 ms
					serialPutchar (fd, 'M'); // Send match ROM command, response should be our own adress ID
					NoOfRecChar = Read1WCmd(fd, Buf, 17);
					if ((strncmp(Adr, Buf, 16)) != 0) {  // Check correct response, should be our adress
//						Buf[NoOfRecChar-1] = '\0';  // Set end of text at correct position
						sprintf(InfoText, "Err-ReadAD %d Exp: %s Rec: %s\r\n", NoOfRecChar, Adr, Buf);
						LOG_MSG(InfoText);
					} else { // All ok, proceed
							serialPuts (fd, "W0BAA0000FFFFFFFFFFFFFFFF\r"); // Read data from adressed device						
							NoOfRecChar = Read1WCmd(fd, Buf, 23);
//printf("3 %d: %s Status: %d\r\n", NoOfRecChar, Buf, strncmp("AA0000FFFFFFFF", Buf, 10));
							if (strncmp(Buf, "AA0000FFFFFFFF", 13) == 0) {  // Check correct response
//								Buf[NoOfRecChar-1] = '\0';  // Set end of text at correct position
								sprintf(InfoText, "Err-ReadAD Sensor missing? Rec %d: %s\r\n", NoOfRecChar, Buf);
								LOG_MSG(InfoText);
							} else { // All ok, proceed
								ScrPad[0] = A2HexByte(Buf[2], Buf[3]);
								ScrPad[1] = A2HexByte(Buf[4], Buf[5]);	
								
								ScrPad[2] = A2HexByte(Buf[6], Buf[7]);   // Read AD0
								ScrPad[3] = A2HexByte(Buf[8], Buf[9]);
								
								ScrPad[4] = A2HexByte(Buf[10], Buf[11]); // Read AD1
								ScrPad[5] = A2HexByte(Buf[12], Buf[13]);
								
								ScrPad[6] = A2HexByte(Buf[14], Buf[15]);  // Read AD2
								ScrPad[7] = A2HexByte(Buf[16], Buf[17]);	
								
								ScrPad[8] = A2HexByte(Buf[18], Buf[19]);  // Read AD3
								ScrPad[9] = A2HexByte(Buf[20], Buf[21]);	
		
								float templong;
									for (Idx = 1; Idx < 5; Idx++) {
										templong = ((ScrPad[2*Idx + 1] << 8) | ScrPad[2*Idx]) & 0x0000FFFF;
										AD[Idx-1] = (float) (templong /65535.0) * 2.56;  //5.12; //2.56; // Depends on setup in InitDS2450 procedure
									} // for
								printf("Read by AD0: %5.3f AD1: %5.3f AD2: %5.3f AD3: %5.3f \r\n", AD[0], AD[1], AD[2], AD[3]);									
								Status = TRUE;
							}	
						}
				}		
			}	
			return Status;
};
unsigned char		Read1WParPwr(int fd, char *Adr) {  // return TRUE or FALSE
			int		NoOfRecChar;
			char	Buf[40];
			int Status = FALSE;
			
				sprintf(Buf, "A%s\r", Adr); // Syntax = A + Id for sensor + <CR>					serialPuts (fd, Adr);
				serialPuts (fd, Buf); 
				NoOfRecChar = Read1WCmd(fd, Buf, 17);  // Read 16 expected + <CR>
				if ((strncmp(Adr, Buf, 16)) != 0) {  // Check correct response, should be our adress
					sprintf(InfoText, "Err-ReadTemp Exp: % Rec: %s\r\n", Adr, Buf);
					LOG_MSG(InfoText);			
				} else { // All ok, 
					serialPuts (fd, "W02B4FF\r");
					NoOfRecChar = Read1WCmd(fd, Buf, 5);
				//	printf("Par pwr: %c %c\r\n", Buf[2], Buf[3]);
					if ((Buf[2] == '0') && (Buf[3] == '0'))
						Status = TRUE;
					else
						Status = FALSE;	
				}
			return Status;
};
unsigned char		Ctrl1WLcd(int fd, char *Adr, int Cmd) {  // return TRUE or FALSE
			int		NoOfRecChar;
			char	Buf[40];
			int Status = FALSE;
			
				sprintf(Buf, "A%s\r", Adr); // Syntax = A + Id for sensor + <CR>					serialPuts (fd, Adr);
				serialPuts (fd, Buf); 
				NoOfRecChar = Read1WCmd(fd, Buf, 17);  // Read 16 expected + <CR>
//printf("1 %d: \r\n Adr: %s \r\n Exp: %s: \r\n Status: %d \r\n", NoOfRecChar, Adr, Buf, (strncmp(Adr, Buf, 16)));
				if ((strncmp(Adr, Buf, 16)) != 0) {  // Check correct response, should be our adress
					sprintf(InfoText, "Err-CtrlLcd %d Exp: %s Rec: %s\r\n", NoOfRecChar, Adr, Buf);
					LOG_MSG(InfoText);			
				} else { // All ok, 
						serialPutchar (fd, 'M'); // Send match ROM command, response should be our own adress ID
						NoOfRecChar = Read1WCmd(fd, Buf, 17);
						if ((strncmp(Adr, Buf, 16)) != 0) {  // Check correct response, should be our adress
							sprintf(InfoText, "Err-CtrlLcd %d Exp: %s Rec: %s\r\n", NoOfRecChar, Adr, Buf);
							LOG_MSG(InfoText);					
						} else { // All ok proceed

					switch (Cmd) {
						case LCD_On: 
							serialPuts (fd, "W0103\r");// LCD ON
							NoOfRecChar = Read1WCmd(fd, Buf, 3);
							if ((strncmp("03", Buf, 2)) != 0) { // Check correct response, should be 03
								sprintf(InfoText, "Err-CtrlLcd %d Exp:03 Rec: %s\r\n", NoOfRecChar, Buf);
								LOG_MSG(InfoText);
							} else {
								sprintf(InfoText, "CtrlLcd %d Exp:03 Rec: %s\r\n", NoOfRecChar, Buf);
								LOG_MSG(InfoText);
							  Status = TRUE; 
							}
						break;
						case LCD_Off:
							serialPuts (fd, "W0105\r");// LCD ON
							NoOfRecChar = Read1WCmd(fd, Buf, 3);
							if ((strncmp("05", Buf, 2)) != 0) { // Check correct response, should be 05
								sprintf(InfoText, "Err-CtrlLcd %d Exp:05 Rec: %s\r\n", NoOfRecChar, Buf);
								LOG_MSG(InfoText);
							} else {
									sprintf(InfoText, "CtrlLcd %d Exp:05 Rec: %s\r\n", NoOfRecChar, Buf);
								LOG_MSG(InfoText);
						  Status = TRUE; 
							}						
						break;
						case LCD_Bkl_On:
							serialPuts (fd, "W0108\r");// LCD backlight ON
							NoOfRecChar = Read1WCmd(fd, Buf, 3);
							if ((strncmp("08", Buf, 2)) != 0) { // Check correct response, should be 08
								sprintf(InfoText, "Err-CtrlLcd %d Exp:08 Rec: %s\r\n", NoOfRecChar, Buf);
								LOG_MSG(InfoText);
							} else {
								sprintf(InfoText, "CtrlLcd %d Exp:08 Rec: %s\r\n", NoOfRecChar, Buf);
								LOG_MSG(InfoText);
							  Status = TRUE; 
							}						
						break;
						case LCD_Bkl_Off:
							serialPuts (fd, "W0107\r");// LCD backlight Off
							NoOfRecChar = Read1WCmd(fd, Buf, 3);
							if ((strncmp("07", Buf, 2)) != 0) { // Check correct response, should be 07
								sprintf(InfoText, "Err-CtrlLcd %d Exp:07 Rec: %s\r\n", NoOfRecChar, Buf);
								LOG_MSG(InfoText);
							} else {
								sprintf(InfoText, "CtrlLcd %d Exp:07 Rec: %s\r\n", NoOfRecChar, Buf);
								LOG_MSG(InfoText);
							  Status = TRUE; 
							}						
						break;
					case LCD_Clear:
							serialPuts (fd, "W0149\r");// LCD clear
							NoOfRecChar = Read1WCmd(fd, Buf, 3);
							if ((strncmp("49", Buf, 2)) != 0) { // Check correct response, should be 07
								sprintf(InfoText, "Err-CtrlLcd %d Exp:49 Rec: %s\r\n", NoOfRecChar, Buf);
								LOG_MSG(InfoText);
							} else {
								sprintf(InfoText, "CtrlLcd %d Exp:49 Rec: %s\r\n", NoOfRecChar, Buf);
								LOG_MSG(InfoText);
							  Status = TRUE; 
							}						
						break;

						default:
								sprintf(InfoText, "Err-CtrlLcd Illegal code sent %d\r\n", Cmd);
								LOG_MSG(InfoText);							
							  Status = FALSE; 					
						break;
										
					} // End switch
				}
			}
		return Status;
}; 
unsigned char		Print1WLcd(int fd, char *Adr, char *Str) {  // return TRUE or FALSE
			int		NoOfRecChar;
			char	Buf[100];
			int Idx, Status = FALSE;
			
				sprintf(Buf, "A%s\r", Adr); // Syntax = A + Id for sensor + <CR>					serialPuts (fd, Adr);
				serialPuts (fd, Buf); 
				NoOfRecChar = Read1WCmd(fd, Buf, 17);  // Read 16 expected + <CR>
//printf("1 %d: \r\n Adr: %s \r\n Exp: %s: \r\n Status: %d \r\n", NoOfRecChar, Adr, Buf, (strncmp(Adr, Buf, 16)));
			if ((strncmp(Adr, Buf, 16)) != 0) {  // Check correct response, should be our adress
					sprintf(InfoText, "Err-PrintLcd Exp: %s Rec: %s\r\n", Adr, Buf);
					LOG_MSG(InfoText);			
				} else { // All ok, 
				  Ctrl1WLcd(fd, Adr, LCD_Clear);
					serialPutchar (fd, 'M'); // Send match ROM command, response should be our own adress ID
					NoOfRecChar = Read1WCmd(fd, Buf, 17);
//printf("2 %d: \r\n Adr: %s \r\n Exp: %s: \r\n Status: %d \r\n", NoOfRecChar, Adr, Buf, (strncmp(Adr, Buf, 16)));
					if ((strncmp(Adr, Buf, 16)) != 0) {  // Check correct response, should be our adress
						sprintf(InfoText, "Err-PrintLCD %d Exp: %s Rec: %s\r\n", NoOfRecChar, Adr, Buf);
						LOG_MSG(InfoText);					
					} else { // All ok proceed
					  for (Idx = 0; Idx < 40; Idx++) {
						  serialPuts(fd, "W021241\r");
							NoOfRecChar = Read1WCmd(fd, Buf, 5);
//printf("3 %d: Exp: %s Status: %d \r\n", NoOfRecChar, Buf);

						  //Buf[Idx] = A2HexByte(Str[Idx], Str[Idx+1]);
							serialPutchar (fd, 'M'); // Send match ROM command, response should be our own adress ID
							NoOfRecChar = Read1WCmd(fd, Buf, 17);
						}			
				}	
			}	
		return Status;
};
unsigned char		Write1WLcd(int fd, char *Adr) {  // return TRUE or FALSE
			int		NoOfRecChar;
			char	Buf[40];
			int Status = FALSE;
			
				sprintf(Buf, "A%s\r", Adr); // Syntax = A + Id for sensor + <CR>					serialPuts (fd, Adr);
				serialPuts (fd, Buf); 
				NoOfRecChar = Read1WCmd(fd, Buf, 17);  // Read 16 expected + <CR>
//printf("1 %d: \r\n Adr: %s \r\n Exp: %s: \r\n Status: %d \r\n", NoOfRecChar, Adr, Buf, (strncmp(Adr, Buf, 16)));
			if ((strncmp(Adr, Buf, 16)) != 0) {  // Check correct response, should be our adress
					sprintf(InfoText, "Err-WriteLcd Exp: %s Rec: %s\r\n", Adr, Buf);
					LOG_MSG(InfoText);			
				} else { // All ok, 
					serialPuts (fd, "W064E005468697320\r");// Write to LCD scratchpad "Message xxx
					NoOfRecChar = Read1WCmd(fd, Buf, 13);
//printf("2 %d: \r\n Adr: %s \r\n Exp: %s: \r\n Status: %d \r\n", NoOfRecChar, Adr, Buf, (strncmp(Adr, Buf, 16)));
					if ((strncmp("4E005468697320", Buf, 5)) != 0) { // Check correct response
						sprintf(InfoText, "Err-WriteLcd %d Exp:4E005468697320 Rec: %s\r\n", NoOfRecChar, Buf);
						LOG_MSG(InfoText);
					} else {
						delay(20);
						serialPutchar (fd, 'M'); // Send match ROM command, response should be our own adress ID
						NoOfRecChar = Read1WCmd(fd, Buf, 17);
//printf("3 %d: \r\n Adr: %s \r\n Exp: %s: \r\n Status: %d \r\n", NoOfRecChar, Adr, Buf, (strncmp(Adr, Buf, 16)));
						if ((strncmp(Adr, Buf, 16)) != 0) {  // Check correct response, should be our adress
							sprintf(InfoText, "Err-WriteLCD %d Exp: %s Rec: %s\r\n", NoOfRecChar, Adr, Buf);
							LOG_MSG(InfoText);					
						} else { // All ok proceed
							serialPuts (fd, "W0148\r"); // Write LCD scratchpad to LCD
							NoOfRecChar = Read1WCmd(fd, Buf, 3);
							if ((strncmp("48", Buf, 2)) != 0) {  // Check correct response
								sprintf(InfoText, "Err-WriteLCD %d Exp:48 Rec: %s\r\n", NoOfRecChar, Buf);
								LOG_MSG(InfoText);										
							} else {
							  Status = TRUE;
								//printf ("Write LCD ok\r\n");
							}
						}	
					}	
				}	
			return Status;
};
int 				A2HexByte(char A1, char A2) {
	int HexByte;

	HexByte = 16 * ToHex(A1) + ToHex(A2);  	//printf("A1 : %c A2: %c Hex %02X\n", A1, A2, HexByte);
  return HexByte;
}
unsigned char 		Hex2Ascii(unsigned char No) {

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
int 	          	ToHex(char ch) {
//------------------------------------------------------------------------
// Convert 1 hex character to binary. 
//  If not a hex character then return 0.

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
