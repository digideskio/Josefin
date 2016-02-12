/*************************************************************************
 *      Main.h
 *
 *      Ver  Date       Name Description
 *      W    2006-11-24 AGY  Created.
 *
 *************************************************************************/

// Define devices, pipes etc.
#define LCD 					  "/dev/lcd"
#define KBD_PIPE			  "/tmp/kbdpipe"
#define ONEWIRE_PIPE	  "/tmp/onewirepipe"
#define MAIN_PIPE			  "/tmp/mainpipe"
#define TIMO_PIPE			  "/tmp/timopipe"
#define BYTEPORTFOLDER	"/tmp/byteport"

#define LCDBlkOnTimerVal  60  // No of minutes before turning off inactive backlight

#define MAX_WATER_LEVEL		180
#define MAX_DIESEL_LEVEL	280

#define OUT_TEMP	        	0 
#define BOX_TEMP    	    	1
#define REFRIG_TEMP	      	2
#define HWATER_TEMP	      	3  // Hot-water from engine-for heating
#define ADINT     	        4  // 4 sensors in all on this device each registers voltage values
#define ADEXT    	        	5  // 4 sensors in all on this device each registers voltage values
#define SEA_TEMP	        	6
#define WATER_TEMP	      	7
#define LCD_1W							8

#define STR_BF533						"BF533"
#define STR_BF537						"BF537"
#define STR_LED_BF533				"/dev/pf2"
//#define STR_LED_BF537		"/dev/pf6"
#define STR_LED_BF537				"/dev/gpio7" 
// No 2-4 used for LED and 5-6,8 for buttons on BF533 
// No 6-9 used for LED and 2-5 for buttons on BF537 

#define STR_OPBUT_BF533	  	"/dev/pf6"  // PPI pin 21
#define STR_LFTBUT_BF533		"/dev/pf5"  // PPI pin 22
#define STR_RGTBUT_BF533		"/dev/pf8"  // PPI pin 19
#define STR_OPBUT_BF537	  	"/dev/gpio5"  // pin ???
#define STR_LFTBUT_BF537		"/dev/gpio3"
#define STR_RGTBUT_BF537		"/dev/gpio2"

// Search paramaters to get CPU info
#define SEARCH          	"model name"   /* Changed from CPU to this in uClinux R8 system */
#define PATH_CPUINFO    	"/proc/cpuinfo"



enum ProcTypes_e {BF533, BF537, RPI, BB, HOSTENV};
/* Declaration of types */

//#define NO_OF_MODES (3)  /* Change this vaule so it is the same as no of enum types -1, see below! */
enum ModeState_e  {/*Scan,*/  MainMode, Temperature, MinMaxTemp, Water, Diesel, SysInfo};

#define NO_OF_ELEM_IN_FILTERQUEU  5

struct  FilterQueu_s {
	float     ADWater;
	float     ADDiesel;
	float     ADBatVoltF;
};

struct ConvList_s {
   float    Level;
   float    Amount;
};


 static const struct ConvList_s Lvl2Water [] = {
       /************************************************************
        * Define AD level and corresponding Water amount [liters]  *
        * Points (a1, b1), (a2, b2)...
        * Y = K * X + m
        * K = (b2 - b1) / (a2 - a1)
        * m = b1 - K * a1
        ************************************************************/
       /*(a1, b1)
        *(a2, b2)
         Values at 13.00 V, see Excel sheet for measured values      */
         {0,       0},     /* Min level & guard value */
         {1.43,    0},
         {1.88,   13},
         {2.55,   23},
         {2.97,   33},  // Visar 1/4 tank på mätaren
         {3.20,   43},
         {3.43,   53},
         {3.53,   63},
         {3.55,   73},  // Visar 1/2 tank på mätaren
         {3.76,   83},
         {3.82,   93},
         {4.26,  103},
         {4.30,  113},  // Visar 3/4 tank på mätaren
         {4.33,  123},
         {4.40,  133},
         {4.44,  143},
         {7.39,  153},
         {7.48,  163},
         {7.51,  173},
         {7.59,  180},
         {20.0,  180}      /* Guard & Max value = Full tank  */
         };


#define NO_OF_ITEMS_IN_W_CONV_LIST ((sizeof(Lvl2Water)/sizeof(Lvl2Water[0])))

static const struct ConvList_s Lvl2Diesel [] = {
       /************************************************************
        * Define AD level and corresponding Diesel amount [liters]  *
        * Points (a1, b1), (a2, b2)...
        * Y = K * X + m
        * K = (b2 - b1) / (a2 - a1)
        * m = b1 - K * a1
        ************************************************************/
       /*(a1, b1)
        *(a2, b2)                    
       Använder samma värden som för vattentanken men uppskalat * 280/180 = 1.5555  */
//     Reserv = 40 liter
//
//
//
         {0,      0},     /* Min level & guard value */
         {1.35,  13},
         {1.53,  15},
         {2.82,  30},
         {3.23,  47},  // Visar 1/4 tank på mätaren
         {3.50,  62},
         {3.71,  78},
         {3.88,  93},
         {4.29, 112},  // Visar 1/2 tank på mätaren  // Kalibrerad vid 13.5V
         {4.5,  162},  // Kalibrerad vid 13.5V
         {4.6,  170},  // Visar 3/4 tank på mätaren
         {4.83, 212},  // Kalibrerad vid 13.5V
         {7.34, 280},  // Kalibrerad vid 13.5V
         {20.0, 280}   // Guard & Max value = Full tank
         };
 




#define NO_OF_ITEMS_IN_D_CONV_LIST ((sizeof(Lvl2Diesel)/sizeof(Lvl2Diesel[0])))


struct ProcState_s {
	enum ModeState_e ModeState;	
	enum ServModes_e ServMode;
  float         MinOutTemp;
  float         MaxOutTemp;
  float         OutTemp;
  float					MinSeaTemp;
  float					MaxSeaTemp;
  float					SeaTemp;
  float         MinRefrigTemp;
  float         MaxRefrigTemp;
  float         RefrigTemp;
  float	        MinBoxTemp;
  float	        MaxBoxTemp;
  float	        BoxTemp;
  float         MinWaterTemp;	
  float         MaxWaterTemp;	
  float         WaterTemp; 
	float         HWaterTemp; 
	float         HWTemp;	
	float         MinHWTemp;	
	float         MaxHWTemp;	
  float         BatVoltS;  // Voltage Start battery
  float         BatVoltF;  // Voltage Förbruknings battery
  float         BatAmpS;
  float         BatAmpF;
  float         ADWaterLevel;
  float         WaterLevel;
  float         ADDieselLevel;
  float         DieselLevel;
	char					DevLCDDefined;
  int           LCD_Id;  // Id no to LCD in OneWireList
  float         UpdateInterval;
	char					LCDBlkOnTimer; // Time in minutes until backligt is turned off
  char					DeviceName[16]; // The name is defined in OneWireHandlerOWFSFile.c. The name to use for Byteport reporting 
 struct fd_s {  // Filedescriptors
    int           lcd;
    int           timo;
    int           sens;
    int           own;
    int           ToOwn;
    int           kbdKnob;
    int           kbdBut;
		FILE				  *OutTemp;
		FILE					*RefrigTemp;
		FILE					*BoxTemp;
		FILE					*DieselLevel;
		FILE					*WaterLevel;
		FILE					*HWaterTemp;
		FILE					*WaterTemp;
		FILE					*SeaTemp;
		FILE					*BatVoltF;
								
  } fd;  
  struct thread_s {
    pthread_t     KbdKnob;
    pthread_t     Kbd;
    pthread_t     LCDKbd;
    pthread_t     Button;
    pthread_t     OneWire;
    pthread_t     Timeout;
    pthread_t     WDog;
    pthread_t     SockServ;
  } Thread; 

};



