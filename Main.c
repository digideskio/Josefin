/*************************************************************************
 *      Main.c
 *
 *      Ver  	Date    	Name Description
 *        		20061124 	AGY  Created.
 *      			20130219 	AGY  New version for RaspberryPi. Slight mod needed! 
 *      w			20140214 	AGY  New version adding BeagleBone support 
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
#include <sys/resource.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

#ifdef RPI_DEFINED
#include <wiringPi.h>
#include <wiringSerial.h>
#include "lcd_def.h" 
#include <lcd.h>
#elif BB_DEFINED
#include "WiringBBB.h"
#endif

#include "SysDef.h"
#include "Main.h"
 
#include "KeyboardIO.h"
#include "TimeoutHandler.h"
#include "OneWireHandlerOWFSFile.h"
//#include "SocketServer.h"

void * RdKeyboardKnob(enum ProcTypes_e ProcType);
void * RdKeyboardBut(enum ProcTypes_e ProcType);
void * OneWireHandler(enum ProcTypes_e ProcType);
void * TimeoutHandler(enum ProcTypes_e ProcType);
void * Watchdog(enum ProcTypes_e ProcType);
void * SockServer(enum ProcTypes_e ProcType);
void   LCDDisplayUpdate(struct ProcState_s *PState);
float  GetWaterLevel(float Level);
float  GetDieselLevel(float Level);
void   InitProc(struct ProcState_s *Pstate);
void   OpButPressed(struct ProcState_s *PState);
void   RghtButPressed(struct ProcState_s *PState);
void   LftButPressed(struct ProcState_s *PState);
void   BuildBarText(char *Str, float Level, float Resolution);
void   QuitProc(void);
 
// Define global variables


char    InfoText[300];
char    LCDText[100];// Holds the text currently displayed on LCD, with some extra bytes so we don't overwrite..
char    ProtectionText[100]; // A test to see if problem disappeares
long long TM2Temp, TM1Temp, TM1AD, TM2AD, TMLCD1, TMLCD2;
float		DeltaTime;

char    DbgTest=0;
int     ret;
//struct FilterQueue_s 	FQueue[5]; //NO_OF_ELEM_IN_FILTERQUEU] 

struct  FQ_s  { // used to make a smoother presentation
	float     ADWater;
	float     ADDiesel;
	float     ADBatVoltF;
} FQueue[NO_OF_ELEM_IN_FILTERQUEU] ;

int    main(int argc, char *argv[]) {
	union SIGNAL			 		*Msg;
	unsigned char       	Buf[sizeof(union SIGNAL)];
  char                	TimeStamp[100], TimeStampStop[100];
  unsigned char       	UpdateInterval, Idx;
	enum ProcTypes_e    	ProcessorType;

//DebugOn = TRUE;   // Start in Debug mode

	//ProcState.ModeState      = Water;          // Set initial value, for test purpose. TBD later
  ProcState.ModeState      = MainMode;          // Set initial value, for test purpose. TBD later 
	ProcState.ServMode       = AnchStop;         	// Set initial value
  ProcState.MinOutTemp     = SENS_DEF_VAL;
  ProcState.MaxOutTemp     = SENS_DEF_VAL;
  ProcState.OutTemp        = SENS_DEF_VAL;
  ProcState.MinSeaTemp     = SENS_DEF_VAL;
  ProcState.MaxSeaTemp     = SENS_DEF_VAL;
  ProcState.SeaTemp        = SENS_DEF_VAL;
  ProcState.MinRefrigTemp  = SENS_DEF_VAL;
  ProcState.MaxRefrigTemp  = SENS_DEF_VAL;
  ProcState.RefrigTemp     = SENS_DEF_VAL;
  ProcState.MinBoxTemp     = SENS_DEF_VAL;
  ProcState.MaxBoxTemp     = SENS_DEF_VAL;
  ProcState.BoxTemp        = SENS_DEF_VAL;
  ProcState.MinWaterTemp   = SENS_DEF_VAL;
  ProcState.MaxWaterTemp   = SENS_DEF_VAL;
  ProcState.WaterTemp      = SENS_DEF_VAL;
  ProcState.WaterLevel     = SENS_DEF_VAL;
	ProcState.HWTemp         = SENS_DEF_VAL;
  ProcState.DieselLevel    = SENS_DEF_VAL;
  ProcState.BatVoltS       = 13.5; //SENS_DEF_VAL; Start value, avoids div by 0!
  ProcState.BatVoltF       = 13.5; //SENS_DEF_VAL; Start value, avoids div by 0!
  ProcState.BatAmpS        = SENS_DEF_VAL;
  ProcState.BatAmpF        = SENS_DEF_VAL;
  ProcState.fd.lcd         = 0;
  ProcState.fd.own         = 0;
  ProcState.fd.ToOwn       = 0;
  ProcState.fd.timo        = 0;
  ProcState.fd.sens        = 0;
  ProcState.fd.kbdBut      = 0;
  ProcState.fd.kbdKnob     = 0;
  ProcState.UpdateInterval = 12;   // Timeout intervall for data & display update 

// Initiate filter queue
	for (Idx = 0; Idx < NO_OF_ELEM_IN_FILTERQUEU; Idx++) {
		FQueue[Idx].ADDiesel		= SENS_DEF_VAL;
		FQueue[Idx].ADWater 		= SENS_DEF_VAL;
		FQueue[Idx].ADBatVoltF 	= SENS_DEF_VAL;
	} 
#ifdef RPI_DEFINED
	if (wiringPiSetup () == -1)  // Initiate WiringPi lib
    exit (1) ;  
	ProcState.fd.lcd = lcdInit (4, 20, 4, 11, 10, 0,1,2,3,0,0,0,0) ; // Initiate LCD driver
	// Note, HW wiring must correspond to this...
#elif BB_DEFINED
//ProcState.fd.lcd = lcdInit (4, 20, 4, P8_3, P8_4, P8_5, P8_11, P8_12, P8_14,P8_12,0,0,0) ; // Initiate LCD driver

//P8_14 ==> DB7
//P8_12 ==> DB6
//P8_11 ==> DB5
//P8_5  ==> DB4 
//P8_4  ==> RS
//P8_3  ==> E


printf("Display updated\r\n");

#endif		
#ifdef LCD_PRESENT
   memset(LCDText, ' ', 100); // Clear display buffer
#ifdef RPI_DEFINED
	//  LCD_CTRL(ProcState.fd.lcd, LCD_Reset);
    LCD_CLEAR(ProcState.fd.lcd);
    LCD_HOME(ProcState.fd.lcd);	
//	LCD_CTRL(ProcState.fd.lcd, LCD_On); 
//	LCD_CTRL(ProcState.fd.lcd, LCD_Cursor_On);
#elif BB_DEFINED

#endif
#endif  
  InitProc(&ProcState);
 // signal(SIGINT, QuitProc);  // Handles CTRL-C command
#ifdef LCD_PRESENT
// Note: Line 2 & 3 must be swapped..!
  sprintf(&LCDText[0], "Josefin started      ");
  sprintf(&LCDText[40]," Ver: %s              ", __DATE__);
  sprintf(&LCDText[20],"                     ");
  sprintf(&LCDText[60]," Golding production  ");
 // LCD_WRITE(ProcState.fd.lcd, 1, 1, LCDText);

#ifdef RPI_DEFINED	
	lcdPosition (ProcState.fd.lcd, 0, 0) ;
	for (Idx = 0; Idx < 80; Idx++)
	  lcdPutchar(ProcState.fd.lcd, LCDText[Idx]);		
	//lcdPrintf(ProcState.fd.lcd, LCDText);
 
	//ret = write(ProcState.fd.lcd, LCDText, 80);
	// printf("LCD Write: %d bytes\r\n", ret);
#elif BB_DEFINED

#endif	// BB/RPI_DEFINED
#endif // LCD_DEFINED
  REQ_TIMEOUT(ProcState.fd.timo, ProcState.fd.ToOwn, "MainInitTOut", SIGInitMeasTempOut, 3 Sec);  
	REQ_TIMEOUT(ProcState.fd.timo, ProcState.fd.ToOwn, "MainInitTBox", SIGInitMeasTempBox, 8 Sec); 
  REQ_TIMEOUT(ProcState.fd.timo, ProcState.fd.ToOwn, "MainInitTRefrig", SIGInitMeasTempRefrig, 3 Sec); 
  REQ_TIMEOUT(ProcState.fd.timo, ProcState.fd.ToOwn, "MainInitTWater", SIGInitMeasTempWater, 15 Sec);
	REQ_TIMEOUT(ProcState.fd.timo, ProcState.fd.ToOwn, "MainInitTHWater", SIGInitMeasTempHW, 15 Sec);
	REQ_TIMEOUT(ProcState.fd.timo, ProcState.fd.ToOwn, "MainInitTSea", SIGInitMeasTempSea, 20 Sec); 
 // REQ_TIMEOUT(ProcState.fd.timo, ProcState.fd.ToOwn, "MainInitADInt", SIGInitMeasADInt, 2 Sec); 
  REQ_TIMEOUT(ProcState.fd.timo, ProcState.fd.ToOwn, "MainInitADExt", SIGInitMeasADExt, 10 Sec); 
	sprintf(InfoText, "Josefin started Ver:  %s\n", __DATE__);
  LOG_MSG(InfoText);
  while (TRUE) {
    WAIT(ProcState.fd.own, Buf, sizeof(union SIGNAL));
//if (Msg->SigNo == 10) {DbgTest = 1;}
		
		Msg = Buf;
 //if (DbgTest == 1) {printf("2: %d\r\n", Msg->SigNo);usleep(200000);}
   switch(Msg->SigNo) {
      case SIGInitMeasTempBox:  // Initiate loop to read Temperature sensors
        Msg->SigNo = SIGReadSensorReq;
        Msg->SensorReq.Client_fd = ProcState.fd.ToOwn;
        Msg->SensorReq.Sensor = BOX_TEMP;
        SEND(ProcState.fd.sens, Msg, sizeof(union SIGNAL));
      break;
      case SIGInitMeasTempRefrig:  // Initiate loop to read Temperature sensors
        Msg->SigNo = SIGReadSensorReq;
        Msg->SensorReq.Client_fd = ProcState.fd.ToOwn;
        Msg->SensorReq.Sensor = REFRIG_TEMP;
        SEND(ProcState.fd.sens, Msg, sizeof(union SIGNAL));
      break;
      case SIGInitMeasTempWater:  // Initiate loop to read Temperature sensors
        Msg->SigNo = SIGReadSensorReq;
        Msg->SensorReq.Client_fd = ProcState.fd.ToOwn;
        Msg->SensorReq.Sensor = WATER_TEMP;
        SEND(ProcState.fd.sens, Msg, sizeof(union SIGNAL));
      break;
      case SIGInitMeasTempSea:  // Initiate loop to read Temperature sensors
        Msg->SigNo = SIGReadSensorReq;
        Msg->SensorReq.Client_fd = ProcState.fd.ToOwn;
        Msg->SensorReq.Sensor = SEA_TEMP;
        SEND(ProcState.fd.sens, Msg, sizeof(union SIGNAL));
      break; 
      case SIGInitMeasTempHW:  // Initiate loop to read Temperature sensors
        Msg->SigNo = SIGReadSensorReq;
        Msg->SensorReq.Client_fd = ProcState.fd.ToOwn;
        Msg->SensorReq.Sensor = HWATER_TEMP;
        SEND(ProcState.fd.sens, Msg, sizeof(union SIGNAL));
      break; 
      case SIGInitMeasTempOut:  // Initiate loop to read Temperature sensors
        Msg->SigNo = SIGReadSensorReq;
        Msg->SensorReq.Client_fd = ProcState.fd.ToOwn;
        Msg->SensorReq.Sensor = OUT_TEMP;
        SEND(ProcState.fd.sens, Msg, sizeof(union SIGNAL));
      break;
			case SIGInitMeasADInt:  // Initiate loop to read AD sensors
        Msg->SigNo = SIGReadSensorReq;
        Msg->SensorReq.Client_fd = ProcState.fd.ToOwn;
        Msg->SensorReq.Sensor = ADINT;
        SEND(ProcState.fd.sens, Msg, sizeof(union SIGNAL));
      break;
			case SIGInitMeasADExt:  // Initiate loop to read AD sensors
        Msg->SigNo = SIGReadSensorReq;
        Msg->SensorReq.Client_fd = ProcState.fd.ToOwn;
        Msg->SensorReq.Sensor = ADEXT;
        SEND(ProcState.fd.sens, Msg, sizeof(union SIGNAL));
      break;

      case SIGReadSensorResp:
				if (DebugOn) {		
					printf(" SenRsp %d: %10.5f sec V1: %f V2: %f  Status: %s \r\n", 
								 Msg->SensorResp.Sensor, Msg->SensorResp.CmdTime,
								 Msg->SensorResp.Val[0], Msg->SensorResp.Val[1],
								 Msg->SensorResp.Status ? "OK" : "ERROR");
				}
        switch (Msg->SensorResp.Sensor) {
					case OUT_TEMP: 
						ProcState.OutTemp = Msg->SensorResp.Val[0];
						if (Msg->SensorResp.Status) { // Valid data recieved
						  if ((ProcState.MinOutTemp == SENS_DEF_VAL) || (ProcState.OutTemp < ProcState.MinOutTemp))
							  ProcState.MinOutTemp   = Msg->SensorResp.Val[0];
						  if ((ProcState.MaxOutTemp == SENS_DEF_VAL) || (ProcState.OutTemp > ProcState.MaxOutTemp))
							  ProcState.MaxOutTemp   = Msg->SensorResp.Val[0];
						} // No valid data
					  REQ_TIMEOUT(ProcState.fd.timo, ProcState.fd.ToOwn, "MainInitTOut", SIGInitMeasTempOut, 30 Sec);
            LCDDisplayUpdate(&ProcState);
					break;
					case BOX_TEMP: 
						ProcState.BoxTemp = Msg->SensorResp.Val[0];  
						if (Msg->SensorResp.Status) { // Valid data recieved
						  if ((ProcState.MinBoxTemp == SENS_DEF_VAL) || (ProcState.BoxTemp < ProcState.MinBoxTemp))
							  ProcState.MinBoxTemp   = Msg->SensorResp.Val[0];
						  if ((ProcState.MaxBoxTemp == SENS_DEF_VAL) || (ProcState.BoxTemp > ProcState.MaxBoxTemp))
							  ProcState.MaxBoxTemp   = Msg->SensorResp.Val[0];
						}
					  REQ_TIMEOUT(ProcState.fd.timo, ProcState.fd.ToOwn, "MainInitTBox", SIGInitMeasTempBox, 15 Sec);
            LCDDisplayUpdate(&ProcState);
					break;
					case REFRIG_TEMP:
						ProcState.RefrigTemp = Msg->SensorResp.Val[0]; 
						if (Msg->SensorResp.Status) { // Valid data recieved
						  if ((ProcState.MinRefrigTemp == SENS_DEF_VAL) || (ProcState.RefrigTemp < ProcState.MinRefrigTemp))
							  ProcState.MinRefrigTemp   = Msg->SensorResp.Val[0];
						  if ((ProcState.MaxRefrigTemp == SENS_DEF_VAL) || (ProcState.RefrigTemp > ProcState.MaxRefrigTemp))
							  ProcState.MaxRefrigTemp   = Msg->SensorResp.Val[0];
						}
					  REQ_TIMEOUT(ProcState.fd.timo, ProcState.fd.ToOwn, "MainInitTRefrig", SIGInitMeasTempRefrig, 20 Sec);
            LCDDisplayUpdate(&ProcState);
					break;
          case WATER_TEMP: 
            ProcState.WaterTemp = Msg->SensorResp.Val[0];
            if (Msg->SensorResp.Status) { // Valid data recieved
              if ((ProcState.MinWaterTemp == SENS_DEF_VAL) || (ProcState.WaterTemp < ProcState.MinWaterTemp))
                ProcState.MinWaterTemp   = Msg->SensorResp.Val[0];
              if ((ProcState.MaxWaterTemp == SENS_DEF_VAL) || (ProcState.WaterTemp > ProcState.MaxWaterTemp))
                ProcState.MaxWaterTemp   = Msg->SensorResp.Val[0];
            }
					  REQ_TIMEOUT(ProcState.fd.timo, ProcState.fd.ToOwn, "MainInitTWater", SIGInitMeasTempWater, 20 Sec);
            LCDDisplayUpdate(&ProcState);
					break;	
          case HWATER_TEMP: 
            ProcState.HWTemp = Msg->SensorResp.Val[0];
            if (Msg->SensorResp.Status) { // Valid data recieved
              if ((ProcState.MinHWTemp == SENS_DEF_VAL) || (ProcState.HWTemp < ProcState.MinHWTemp))
                ProcState.MinHWTemp   = Msg->SensorResp.Val[0];
              if ((ProcState.MaxHWTemp == SENS_DEF_VAL) || (ProcState.HWTemp > ProcState.MaxHWTemp))
                ProcState.MaxHWTemp   = Msg->SensorResp.Val[0];
            }
					  REQ_TIMEOUT(ProcState.fd.timo, ProcState.fd.ToOwn, "MainInitHW", SIGInitMeasTempHW, 20 Sec);

            LCDDisplayUpdate(&ProcState);
					break;
					case SEA_TEMP:    
  					ProcState.SeaTemp= Msg->SensorResp.Val[0];
						if (Msg->SensorResp.Status) { // Valid data recieved
						  if ((ProcState.MinSeaTemp == SENS_DEF_VAL) || (ProcState.SeaTemp < ProcState.MinSeaTemp))
							  ProcState.MinSeaTemp   = Msg->SensorResp.Val[0];
						  if ((ProcState.MaxSeaTemp == SENS_DEF_VAL) || (ProcState.SeaTemp > ProcState.MaxSeaTemp))
							  ProcState.MaxSeaTemp   = Msg->SensorResp.Val[0];
						} 
					  REQ_TIMEOUT(ProcState.fd.timo, ProcState.fd.ToOwn, "MainInitTSea", SIGInitMeasTempSea, 40 Sec);
            LCDDisplayUpdate(&ProcState);
					break;
					case ADINT:    
/*						if (Msg->SensorResp.Status) { // Valid data recieved
							ProcState.BatVoltF    = 11* (Msg->SensorResp.Val[0]) + 0.65;  //Correction due to ...
//sprintf(InfoText, "BatF %f AD %6.3f\n", ProcState.BatVoltF, Msg->SensorResp.Val[0]);
//LOG_MSG(InfoText);
					  }		
					  REQ_TIMEOUT(ProcState.fd.timo, ProcState.fd.ToOwn, "MainInitADInt", SIGInitMeasADInt, 5 Sec);
//          TIMER_START(TM1AD);
            LCDDisplayUpdate(&ProcState); 
*/				break;
					case ADEXT: 
						if (Msg->SensorResp.Status) { // Valid data recieved
							// Filter data to get better readings with less variation
							if (FQueue[0].ADWater == SENS_DEF_VAL) { // Not initiated yet, do it!
								for (Idx = 0; Idx < NO_OF_ELEM_IN_FILTERQUEU; Idx++) {
									FQueue[Idx].ADDiesel    = Msg->SensorResp.Val[0];  								
									FQueue[Idx].ADWater     = Msg->SensorResp.Val[1];
									FQueue[Idx].ADBatVoltF  = Msg->SensorResp.Val[2];							
								}	// for
							} // if
							// Move all data up 1 position in queue
							for (Idx = (NO_OF_ELEM_IN_FILTERQUEU - 1); Idx > 0; Idx--) {
							//	printf("Idx: %d\r\n", Idx);
								FQueue[Idx].ADDiesel    = FQueue[Idx - 1].ADDiesel;  								
								FQueue[Idx].ADWater     = FQueue[Idx - 1].ADWater;
								FQueue[Idx].ADBatVoltF  = FQueue[Idx - 1].ADBatVoltF;							
							}	// for
							FQueue[0].ADDiesel    = Msg->SensorResp.Val[0];								
							FQueue[0].ADWater     = Msg->SensorResp.Val[1];
							FQueue[0].ADBatVoltF  = Msg->SensorResp.Val[2];						
							Msg->SensorResp.Val[0] = 0; 							
							Msg->SensorResp.Val[1] = 0; 							
							Msg->SensorResp.Val[2] = 0; 							
							
							// Calculate mean value of all elem in filter queue
							for (Idx = 0; Idx < NO_OF_ELEM_IN_FILTERQUEU; Idx++) {
								//printf("ADD %d: %4f ", Idx, FQueue[Idx].ADDiesel);

								Msg->SensorResp.Val[0] += FQueue[Idx].ADDiesel; 							
								Msg->SensorResp.Val[1] += FQueue[Idx].ADWater; 							
								Msg->SensorResp.Val[2] += FQueue[Idx].ADBatVoltF; 							
							}	// for
							//printf("\r\n");
							Msg->SensorResp.Val[0] = Msg->SensorResp.Val[0]/(NO_OF_ELEM_IN_FILTERQUEU); 							
							Msg->SensorResp.Val[1] = Msg->SensorResp.Val[1]/(NO_OF_ELEM_IN_FILTERQUEU); 							
							Msg->SensorResp.Val[2] = Msg->SensorResp.Val[2]/(NO_OF_ELEM_IN_FILTERQUEU);							
//printf("Wavg: %4f WLatest: %4f \r\n", Msg->SensorResp.Val[1], FQueue[0].ADWater);
						
						  ProcState.BatVoltF      = Msg->SensorResp.Val[2];
			 			  ProcState.ADWaterLevel  = Msg->SensorResp.Val[1];
							ProcState.ADDieselLevel = Msg->SensorResp.Val[0];

						//	printf("ADD: %f ADW: %f ADB: %f \r\n", Msg->SensorResp.Val[0], Msg->SensorResp.Val[1],Msg->SensorResp.Val[2]);				
							
							// If we dont get correct readout of battery voltage, use default value!
              if (ProcState.BatVoltF < 11) ProcState.BatVoltF = 13;
// Get corresponding Water and Diesel level, scale towards 13. Volt (used for table in Excel-sheet)
     //         ProcState.BatVoltF = 13; // Hard code this!!

							ProcState.WaterLevel   =  GetWaterLevel(ProcState.ADWaterLevel) * 13 / ProcState.BatVoltF;
							ProcState.DieselLevel  =  GetDieselLevel(ProcState.ADDieselLevel) * 13 / ProcState.BatVoltF;
						}   
						if ((ProcState.ModeState == Water) || (ProcState.ModeState == Diesel) ) { // Fast update
					     REQ_TIMEOUT(ProcState.fd.timo, ProcState.fd.ToOwn, "MainInitADExtFst", SIGInitMeasADExt, 1 Sec);
							// if (DebugOn) printf("Fast update M:%d \r\n",ProcState.ModeState);
						} else { // Slow update
					     REQ_TIMEOUT(ProcState.fd.timo, ProcState.fd.ToOwn, "MainInitADExtSlw", SIGInitMeasADExt, 12 Sec);
							// if (DebugOn) printf("Slow update M:%d \r\n",ProcState.ModeState );
					  }
//sprintf(InfoText, "BatF  %7.3f AD: %7.3f \n", ProcState.BatVoltS, Msg->SensorResp.Val[3]);
//LOG_MSG(InfoText);
						
            LCDDisplayUpdate(&ProcState);
					break; // ADExt
					default: 
						CHECK(FALSE, "Undefined sensor...\n");
					break;  	
				}
#ifdef LCD_PRESENT
       LCDDisplayUpdate(&ProcState);
#else
        if (Msg->SensorResp.Sensor == WATER_TEMP) {// Just to secure only 1 line when no display present
          LCDDisplayUpdate(&ProcState);
        }  

#endif
      break;

      case SIGOpButOn:
//if  (DbgTest == 1) {printf("3: %d\r\n", Msg->SigNo);usleep(200000);}
        OpButPressed(&ProcState);
        LCDDisplayUpdate(&ProcState);
	    break;
      case SIGLftButOn:
	//printf("Left button presssed Msg: %s\n");
        RghtButPressed(&ProcState);
//        LftButPressed(&ProcState); // Due to problems reading Left/Right. Step always Right!!!
        LCDDisplayUpdate(&ProcState);
      break;
      case SIGRghtButOn:
	//printf("Right button presssed \n");
        RghtButPressed(&ProcState);
        LCDDisplayUpdate(&ProcState);
      break;
			
      case SIGServCmdReq: 
				switch (Msg->ServCmdReq.Cmd) {				
					case Dwn:
						printf("Main: Anchor Down \r\n");
					break;
					case SlwUp:		
						printf("Main: Anchor Slow Up \r\n");
					break;
					case Up:
						printf("Main: Anchor Up \r\n");
					break;
					case AnchStop:	
						printf("Main: Anchor STOP \r\n");
					break;
					case SetTime:
						printf("Main: Set Time \r\n");
					break;
					default:
						sprintf(InfoText, "Illegal server cmd received: %d\n", Msg->ServCmdReq.Cmd);
						CHECK(FALSE, InfoText);
					break; 
				}	// End switch		
			break;
				
      default:
        sprintf(InfoText, "Illegal signal received: %d\n", Msg->SigNo);
        CHECK(FALSE, InfoText);
      break;
    } // Switch
	}  // While
}
float  GetWaterLevel(float Level) {
  unsigned char         NotFound, Idx;
  float                 K, m, Amount;

  NotFound = TRUE;
 // CHECK(Level > 0, StrErr_c) // " Error, too low AD value\r\n");
  for ( Idx = 0; NotFound; Idx++) {
    if ( Level > Lvl2Water[Idx].Level) {
//if (DebugOn) printf("Water***: Idx: %d Searched Level: %5.2f TblLevel: %5.2f Water: %5.2f \r\n", Idx, Level, Lvl2Water[Idx].Level, Lvl2Water[Idx].Amount);
      //-----------------2005-02-26 13:01-----------------
      //  Just loop until we pass the searched level
      //  We start at lowest value and search until we pass
      //  Then the searched value in between Idx and Idx-1
      //  --------------------------------------------------
    } else {
//		   if (DebugOn) printf("Water***: Last Idx: %d Searched Level: %5.2f TblLevel: %5.2f Water: %5.2f \r\n", Idx, Level, Lvl2Water[Idx].Level, Lvl2Water[Idx].Amount);
       NotFound = FALSE;
    }  // if else 
  }    // for loop 
  Idx--;      // Decrement counter, otherwise for loop leaves with +1 
  K = (Lvl2Water[Idx].Amount - Lvl2Water[Idx-1].Amount) / (Lvl2Water[Idx].Level - Lvl2Water[Idx-1].Level);  // Slope.. 
  m =  Lvl2Water[Idx].Amount - K * Lvl2Water[Idx].Level;
  Amount  =  K * Level + m;                         // Y = K*X + m 
/*  if ( DebugOn) {                   // Use this to calibrate when water is available...
    sprintf(InfoText, "Idx: %d PrevList Lvl: %f Water: %u \r\n CurrList Lvl: %f Water: %f \r\n Level:        %d Water: %d \r\n", Idx, Lvl2Water[Idx-1].Level, Lvl2Water[Idx-1].Amount, Lvl2Water[Idx].Level, Lvl2Water[Idx].Amount, Level, (short) Amount);
    LOG_MSG(InfoText);  
  }*/
  return Amount;
}
float  GetDieselLevel(float Level) {
  unsigned char         NotFound, Idx;
  float                 K, m, Amount;

  NotFound = TRUE;
 // CHECK(Level > 0, StrErr_c) // " Error, too low AD value\r\n");
  for ( Idx = 0; NotFound; Idx++) {
    if ( Level > Lvl2Diesel[Idx].Level) {
      //-----------------2005-02-26 13:01-----------------
      //  Just loop until we pass the searched level
      //  We start at lowest value and search until we pass
      //  Then the searched value in between Idx and Idx-1
      //  --------------------------------------------------
    } else {
       NotFound = FALSE;
    }  // if else 
  }    // for loop 
  Idx--;      // Decrement counter, otherwise for loop leaves with +1 
  K = (Lvl2Diesel[Idx].Amount - Lvl2Diesel[Idx-1].Amount) / (Lvl2Diesel[Idx].Level - Lvl2Diesel[Idx-1].Level);  // Slope.. 
  m =  Lvl2Diesel[Idx].Amount - K * Lvl2Diesel[Idx].Level;
  Amount  =  K * Level + m;                         // Y = K*X + m 
 /* if (DebugOn) {                   // Use this to calibrate when water is available...
    sprintf(InfoText, "Idx: %d PrevList Lvl: %f Diesel: %f \r\n CurrList Lvl: %f Diesel: %f \r\n Level:        %d Diesel: %d \r\n", Idx, Lvl2Diesel[Idx-1].Level, Lvl2Diesel[Idx-1].Amount, Lvl2Diesel[Idx].Level, Lvl2Diesel[Idx].Amount, Level, (short) Amount);
    LOG_MSG(InfoText);  
  }*/
  return Amount;
}
void   LCDDisplayUpdate(struct ProcState_s *PState) {
	/* 20101129 Note, must fill the text string (20 chars with this display)
	otherwise dummy chars will destroy presentation
	*/
	
#define	Line1   0
#define	Line2  20
#define	Line3  40
#define Line4  60 
	short   Resolution;
  char    Indicator;

//TIMER_START(TMLCD1);
memset(LCDText, ' ', 80); // Clear display buffer
//TIMER_STOP("LCD memset", TMLCD2, TMLCD1);
if  (DbgTest == 1) {printf("DispRoutine entered \r\n");usleep(200000);}  
	switch(PState->ModeState) {
		case MainMode:          //   "12345678911234567892"
      if (PState->OutTemp == SENS_DEF_VAL)
        sprintf(&LCDText[Line1], "Temperatur     --.-    ");
      else
        sprintf(&LCDText[Line1], "Temperatur    %+5.1f   ", PState->OutTemp);
      if (PState->RefrigTemp == SENS_DEF_VAL)
        sprintf(&LCDText[Line2], " Kyl           --.-    ");
      else
        sprintf(&LCDText[Line2], " Kyl          %+5.1f   ", PState->RefrigTemp);
      if (PState->BoxTemp == SENS_DEF_VAL) 
        sprintf(&LCDText[Line3], " Box           --.-    ");
      else        
        sprintf(&LCDText[Line3], " Box          %+5.1f   ", PState->BoxTemp);
      if (PState->WaterLevel == SENS_DEF_VAL)
        sprintf(&LCDText[Line4], "Vatten      -- [180]   ");
      else if (PState->WaterLevel <= MAX_WATER_LEVEL)
        sprintf(&LCDText[Line4], "Vatten    %3.0f [180]  ", PState->WaterLevel);
      else
        sprintf(&LCDText[Line4], "Vatten         > Max   ");
   break;

    case Temperature:       //   "12345678911234567892"
      if (PState->OutTemp == SENS_DEF_VAL)
        sprintf(&LCDText[Line1], "Temperatur     --.-      ");
      else
        sprintf(&LCDText[Line1], "Temperatur    %+5.1f      ", PState->OutTemp);
      if (PState->SeaTemp == SENS_DEF_VAL)
        sprintf(&LCDText[Line2], " Hav           --.-      ");
      else        
        sprintf(&LCDText[Line2], " Hav          %+5.1f   ", PState->SeaTemp);
      if (PState->HWTemp == SENS_DEF_VAL)
        sprintf(&LCDText[Line3], " Varme         --.-     ");
      else
        sprintf(&LCDText[Line3], " Varme        %+5.1f   ", PState->HWTemp);
      if (PState->WaterTemp == SENS_DEF_VAL)
        sprintf(&LCDText[Line4], " Vatten        --.-    ");
      else        
        sprintf(&LCDText[Line4], " Vatten       %+5.1f   ", PState->WaterTemp);
    break;

    case MinMaxTemp:        //   "12345678911234567892"
      sprintf(&LCDText[Line1],   "Temp    Min     Max  ");
			if (PState->OutTemp == SENS_DEF_VAL)  // No value received...
        sprintf(&LCDText[Line2], " Ute    --.-    --.-    ");
      else
        sprintf(&LCDText[Line2], " Ute   %+5.1f   %+5.1f  ", PState->MinOutTemp, PState->MaxOutTemp);
      if (PState->RefrigTemp == SENS_DEF_VAL)
        sprintf(&LCDText[Line3], " Varme  --.-    --.-     ");
      else
        sprintf(&LCDText[Line3], " Varme %+5.1f   %+5.1f  ", PState->MinHWTemp, PState->MaxHWTemp);
      if (PState->SeaTemp == SENS_DEF_VAL)
        sprintf(&LCDText[Line4], " Vatten --.-    --.-      ");
      else        
        sprintf(&LCDText[Line4], " Vatten%+5.1f   %+5.1f   ", PState->MinWaterTemp, PState->MaxWaterTemp);
			break;

    case Water:             //    "12345678911234567892"
//printf("1 W-Level: %6.void3f \r\n", PState->WaterLevel);
// First write temperature
      if (PState->WaterTemp == SENS_DEF_VAL) {
        sprintf(&LCDText[Line1],  "Vatten        --.-   ");
//printf("Water received\n");
			}
      else      
        sprintf(&LCDText[Line1],  "Vatten      %+5.1f      ", PState->WaterTemp);
// Then write water volume
      if (PState->WaterLevel == SENS_DEF_VAL) {
        sprintf(&LCDText[Line2],  "  --.-  [180] Liter     ");
// Clear screen
        sprintf(&LCDText[Line3],  "                    ");// Clear screen
        sprintf(&LCDText[Line4],  "                    ");// Clear screen  
      }
      else if (PState->WaterLevel <= MAX_WATER_LEVEL) {  
        sprintf(&LCDText[Line2],  "  %-3.0f  [180] Liter    ", PState->WaterLevel);
				Resolution = MAX_WATER_LEVEL/20;  // Max water in liters / nr of chars on display line
// Clear screen
        sprintf(&LCDText[Line3],  "                    ");// Clear screen
        sprintf(&LCDText[Line4],  "                    ");// Clear screen  
        if (DebugOn) {
          BuildBarText(&LCDText[Line3], PState->WaterLevel, Resolution);  
					sprintf(&LCDText[Line4]," AD : %6.3f [mV]       ", PState->ADWaterLevel);
        } else {
          BuildBarText(&LCDText[Line3], PState->WaterLevel, Resolution);  
          BuildBarText(&LCDText[Line4], PState->WaterLevel, Resolution);  
        }
      } else {
          sprintf(&LCDText[Line2],"Error: Water > MAX      ");
          sprintf(&LCDText[Line3],"  %-3.0f  [180] Liter   ", PState->WaterLevel);
					sprintf(&LCDText[Line4]," AD : %6.3f [mV]        ", PState->ADWaterLevel);

//printf("W-Level: %f \r\n", PState->WaterLevel);
//printf("AD-Level: %f \r\n", PState->ADWaterLevel);
      }
    break;

    case Diesel:             //   "12345678911234567892"
      sprintf(&LCDText[Line1],    "Diesel               ");
// Then write diesel volume
      if (PState->DieselLevel == SENS_DEF_VAL) {
         sprintf(&LCDText[Line2], "  --.-  [280] Liter  ");
// Clear screen
        sprintf(&LCDText[Line3],  "                     ");// Clear screen
        sprintf(&LCDText[Line4],  "                     ");// Clear screen  
      }
      else if (PState->DieselLevel <= MAX_DIESEL_LEVEL) {  
        sprintf(&LCDText[Line2],  "  %-3.0f  [280] Liter     ", PState->DieselLevel);
			  Resolution = MAX_DIESEL_LEVEL/20;  // Max diesel in liters / nr of chars on display line
// Clear screen
        sprintf(&LCDText[Line3],  "                     ");// Clear screen
        sprintf(&LCDText[Line4],  "                     ");// Clear screen 
        if (DebugOn) {
          BuildBarText(&LCDText[Line3], PState->DieselLevel, Resolution);
					sprintf(&LCDText[Line4]," AD : %6.3f [mV]        ", PState->ADDieselLevel);
        } else {
          BuildBarText(&LCDText[Line3], PState->DieselLevel, Resolution);
          BuildBarText(&LCDText[Line4], PState->DieselLevel, Resolution);
        }
      } else {
          sprintf(&LCDText[Line2],"Error: Diesel > MAX    ");
          sprintf(&LCDText[Line3],"  %-3.0f  [280] Liter   ", PState->DieselLevel);
					sprintf(&LCDText[Line4]," AD : %6.3f [mV]        ", PState->ADDieselLevel);
      }
    break;
 
    case SysInfo:
      if (DebugOn) { //          "12345678911234567892"
        sprintf(&LCDText[Line1], "SysInfo     Dbg ON  ");
			} else {
        sprintf(&LCDText[Line1], "SysInfo     Dbg OFF ");
			}
      if (PState->BatVoltS == SENS_DEF_VAL)
        sprintf(&LCDText[Line2], " Str  --.-- V        ");
      else
        sprintf(&LCDText[Line2], " Str %5.1f V         ", PState->BatVoltS);
      if (PState->BatVoltF == SENS_DEF_VAL)
        sprintf(&LCDText[Line3], " Fbr  --.-- V        ");
      else 
        sprintf(&LCDText[Line3], " Fbr %5.1f V         ", PState->BatVoltF);
      sprintf(&LCDText[Line4], "                        ");
 
			break;

    default:
      sprintf(InfoText, "Undefined state %d\n          ", PState);
      CHECK(FALSE, InfoText);
    break;
  }  // End of switch  

#ifdef OWLCD_PRESENT
int	idxx, fd1WLCD1, fd1WLCD2, fd1WLCD3, fd1WLCD4, fd1WLCDon;
char  Addr[100], Addr2[100], Temp2[20];

  OPEN_PIPE(fd1WLCD1,  "/mnt/1wire/FF.EB0700000100/line20.0", O_WRONLY|O_NONBLOCK);
  OPEN_PIPE(fd1WLCD2,  "/mnt/1wire/FF.EB0700000100/line20.1", O_WRONLY|O_NONBLOCK);
  OPEN_PIPE(fd1WLCD3,  "/mnt/1wire/FF.EB0700000100/line20.2", O_WRONLY|O_NONBLOCK);
  OPEN_PIPE(fd1WLCD4,  "/mnt/1wire/FF.EB0700000100/line20.3", O_WRONLY|O_NONBLOCK);
   OPEN_PIPE(fd1WLCDon, "/mnt/1wire/FF.EB0700000100/LCDon", O_WRONLY|O_NONBLOCK);
/*
	 sprintf(Addr, "/mnt/1wire/FF.EB0700000100/line20.0");
	if((fd1WLCD = fopen(Addr, "r")) == NULL)  {
		sprintf(InfoText, "ERROR: %s %d Can not open file %s \n", strerror(errno), errno, Addr);
		CHECK(FALSE, InfoText);
		// fclose(fp); Don't close, fp == NULL!!
	}
	printf("Open OK: %s %d %s\r\n", strerror(errno), errno, Addr);

	fd1WLCD = open (Addr, O_WRONLY|O_NONBLOCK);
	printf("Open: %s %d %s\r\n", strerror(errno), errno, Addr);

	sprintf(Addr2, "/mnt/1wire/FF.EB0700000100/LCDon");
	fd1WLCD2 = open (Addr2, "w");
	printf("Open: %s %d %s\r\n", strerror(errno), errno, Addr2);

	*/  
	//fprintf(fd1WLCD2, "1");
	//  memcpy(&Temp2, &LCDText[Line1], 20);
	//	Temp2[19] = '\0';
	/*	fprintf(fd1WLCD, "xxxxxxxxxxxxxxxxxxxxxxxxxxxx");
		printf("Write Resp: %s %d \r\n", strerror(errno), errno, Addr2);

		printf("LCD opened ok: %s\r\n", Temp2);
		*/
   // sprintf(Temp2, "Hejsan Arne");
		  //fputc(LCDText[idxx], fd1WLCD);
		write(fd1WLCD1, &LCDText[Line1], 20);
		write(fd1WLCD2, &LCDText[Line2], 20); 
		write(fd1WLCD3, &LCDText[Line3], 20); 
		write(fd1WLCD4, &LCDText[Line4], 20); 
	close (fd1WLCD1);
	close (fd1WLCD2);
	close (fd1WLCD3);
	close (fd1WLCD4);
	close (fd1WLCDon);
	
#endif 
 
#ifdef LCD_PRESENT
  //TIMER_START(TMLCD1);

// Fix display problem in R8 system, line 2 & 3 switched, switch back!!!
  char Temp[20];
  memcpy(&Temp, &LCDText[Line2], 20);
  memcpy(&LCDText[Line2], &LCDText[Line3], 20);
  memcpy(&LCDText[Line3], &Temp, 20);
	
//{TIMER_STOP("LCD memcpy", TMLCD2, TMLCD1);}
//sleep(5);
 // LCD_WRITE(PState->fd.lcd, 1, 1, LCDText);	
 int i;
 #ifdef RPI_DEFINED
 lcdPosition (PState->fd.lcd, 0, 0);
 for (i = 0; i < 80; i++) 
   lcdPutchar (PState->fd.lcd, LCDText[i]) ;
#elif BB_DEFINED

#endif
if (ret < 0) printf("LCD Write 2: %d bytes\r\n", ret);

	if (DebugOn) {
		printf("%s\n", LCDText); // Print on screen also
	}
	
#else
  printf(" %s \n", LCDText);
#endif
}  // End of function LCDDisplayUpdate 
void   OpButPressed(struct ProcState_s *PState)    {

  switch(PState->ModeState) {
    case Temperature:
      PState->ModeState = MinMaxTemp;
    break;

    case MinMaxTemp:         // Clear Min & Max vaules
      PState->MinWaterTemp     = PState->WaterTemp;
      PState->MaxWaterTemp     = PState->WaterTemp;
      PState->MinHWTemp        = PState->HWTemp;
      PState->MaxHWTemp        = PState->HWTemp;
      PState->MinRefrigTemp    = PState->RefrigTemp;
      PState->MaxRefrigTemp    = PState->RefrigTemp;
      PState->MinWaterTemp     = PState->WaterTemp;
      PState->MaxWaterTemp     = PState->WaterTemp;
      PState->MinSeaTemp       = PState->SeaTemp;
      PState->MaxSeaTemp       = PState->SeaTemp;
      PState->MinOutTemp       = PState->OutTemp;
      PState->MaxOutTemp       = PState->OutTemp;
      PState->MinBoxTemp       = PState->BoxTemp;
      PState->MaxBoxTemp       = PState->BoxTemp;
    break;

    case SysInfo:
       if (DebugOn == TRUE) {
         DebugOn = FALSE;
         LOG_MSG("Debug OFF\n");
       }
       else {
         DebugOn = TRUE;
         LOG_MSG("Debug ON\n");
       }
    break;

    default: // Test, reset LCD display

    break;
  }
} // OP button pressed 
void   LftButPressed(struct ProcState_s *PState)    {

  switch (PState->ModeState) {
    case SysInfo    : PState->ModeState = Diesel;      break;
    case Diesel     : PState->ModeState = Water;       break;
    case Water      : PState->ModeState = Temperature; break;
    case Temperature: PState->ModeState = MainMode;    break;
    case MainMode   : PState->ModeState = SysInfo;		 break;
    case MinMaxTemp : PState->ModeState = Temperature; break;
 //   case Water      : PState->ModeState = Battery;     break;
    default:
      sprintf(InfoText, "Undefined state %d\n", PState);
      CHECK(FALSE, InfoText);
    break;
  } // switch
}  // MiButPressed 
void   RghtButPressed(struct ProcState_s *PState)     {

  switch (PState->ModeState) {
    case MainMode   : PState->ModeState = Temperature; break;
    case Temperature: PState->ModeState = Water;	     break;
    case Water      : PState->ModeState = Diesel;      break;
    case Diesel     : PState->ModeState = SysInfo;     break;
    case SysInfo    : PState->ModeState = MainMode;		 break;
    case MinMaxTemp : PState->ModeState = Temperature; break;
//    case Battery    : PState->ModeState = Water;       break;
    default:
      sprintf(InfoText, "Undefined state %d\n", PState);
      CHECK(FALSE, InfoText);
    break;
  } // switch 
} // PlButPressed
void   BuildBarText(char * Str, float Level, float Resolution)    {
  char Idx, ScreenPos;

  ScreenPos = Level / Resolution;   
 /* for (Idx = 0; Idx < ScreenPos; Idx++) {
   // Str[Idx] = 0xBC;
    Str[Idx] = 'x';
  }
*/ 
 if (ScreenPos <= 20) 
  // memset(Str, 0xBC, ScreenPos);
   memset(Str, 0x3E, ScreenPos);
 else
   printf("Severe error...\r\n"); 

}  // BuildBarText

void   InitProc(struct ProcState_s *PState) {
  
  int ret, rc;
  enum ProcTypes_e    ProcessorType;
#ifdef BF537_DEFINED
  ProcessorType = BF537;
  LOG_MSG("BF537 defined\n");
#elif RPI_DEFINED
  ProcessorType = RPI;
  LOG_MSG("RaspberryPi defined\n");
#elif BB_DEFINED
  ProcessorType = BB;
  LOG_MSG("BeagleBone defined\n");
#elif HOST_DEFINED
  ProcessorType = HOSTENV;
  LOG_MSG("HOST defined\n");
#else
  sprintf(InfoText, "Unknown processort type defined: %d \n", ProcessorType);
  CHECK(FALSE, InfoText);
#endif  
     
  remove(KBD_PIPE);
  remove(ONEWIRE_PIPE);	
  remove(MAIN_PIPE);
  remove(TIMO_PIPE);
  umask(0);
  mknod(KBD_PIPE,  S_IFIFO|0666, 0); 
  mknod(ONEWIRE_PIPE, S_IFIFO|0666, 0); 
  mknod(MAIN_PIPE, S_IFIFO|0666, 0); 
  mknod(TIMO_PIPE, S_IFIFO|0666, 0); 

  OPEN_PIPE(PState->fd.own, MAIN_PIPE, O_RDONLY|O_NONBLOCK);
  OPEN_PIPE(PState->fd.ToOwn, MAIN_PIPE, O_WRONLY);

  ret= pthread_create( &PState->Thread.Timeout,  NULL, TimeoutHandler,  (void *) ProcessorType);
  if (ret != 0)  printf("%s %d %s open error %s\n", __FILE__, __LINE__, "Timout thread", strerror(errno)); 
  errno = 0;
  
  ret = pthread_create( &PState->Thread.KbdBut,      NULL, RdKeyboardBut,      (void *) ProcessorType);
  if (ret != 0) printf("%s %d %s open error %s\n", __FILE__, __LINE__, "Kbd button thread", strerror(errno)); 
  errno = 0;

  ret = pthread_create( &PState->Thread.OneWire,  NULL, OneWireHandler,  (void *) ProcessorType);
  if (ret != 0) printf("%s %d %s open error %s\n", __FILE__, __LINE__, "OneWire thread", strerror(errno)); 
  errno = 0;

  ret = pthread_create( &PState->Thread.WDog,     NULL, Watchdog,        (void *) ProcessorType);
  if (ret != 0)  printf("%s %d %s open error %s\n", __FILE__, __LINE__, "Watchdog thread", strerror(errno)); 
  errno = 0;

	ret = pthread_create( &PState->Thread.SockServ,     NULL, SockServer,        (void *) ProcessorType);
  if (ret != 0)  printf("%s %d %s open error %s\n", __FILE__, __LINE__, "Socket server", strerror(errno)); 
  errno = 0;

  //sleep(2); // Wait until all threads are ready, i.e have opened all resources
  
  OPEN_PIPE(PState->fd.timo, TIMO_PIPE, O_WRONLY);
  OPEN_PIPE(PState->fd.sens, ONEWIRE_PIPE, O_WRONLY);

}
void   QuitProc(void) { // Read current timestamp

 printf("Ctrl-c received\n");
#ifdef LCD_PRESENT
 close(ProcState.fd.lcd);
#endif 
// close(ProcState.fd.kbdKnob);
 close(ProcState.fd.kbdBut);
 close(ProcState.fd.timo);
 close(ProcState.fd.sens);
 close(ProcState.fd.ToOwn);
 close(ProcState.fd.own);
 //pthread_cancel(ProcState.Thread.Timeout);
 //pthread_cancel(ProcState.Thread.Kbd);
 //pthread_cancel(ProcState.Thread.OneWire);
 pthread_exit(ProcState.Thread.WDog);
 remove(KBD_PIPE);
 remove(ONEWIRE_PIPE);	
 remove(MAIN_PIPE);
 remove(TIMO_PIPE);
 sleep(2);
 printf("Main Exit\n");

 exit(0);
}
