/*************************************************************************
 *      OneWireHandler.h
 *
 *      Ver  Date       Name Description
 *      R1    2006-11-24 AGY  Created for Blackfin BF533, 537
 *      W     2013-02-19 AGY  Adapted for RaspberryPi
 *
 *************************************************************************/


// Define allowed device types, add new and change in code...
#define DEV_NO_DEV					0x00    // No sensor defined
#define DEV_DS1820         	0x10		// Temperature sensor, old model
#define DEV_DS2450         	0x20		// AD sensor
#define DEV_DS1822         	0x22    // Temperature sensor, old model
#define DEV_DS18B20					0x28    // Temperature sensor

/*#define LCD_ON						1    		// LCD-1W
#define LCD_OFF							2    		// LCD-1W
#define LCD_BKL_ON					3    		// LCD-1W
#define LCD_BKL_OFF					4    		// LCD-1W
#define LCD_CLEAR						5    		// LCD-1W
*/
 /* Declaration of types */
  
struct OneWireList_s {
	 char						  Present; // Boolean indicating if sensor exists or not
   char             Id;
   //FILE						 *fp;  // File pointer to sensor
   char             SensName[16];
   unsigned char    DevType;
   char             Path[25];
   float						Val[4];   // Read values	
};


struct ExpOneWireList_s {
   char             Id;
   char             SensName[16];
   unsigned char    DevType;
   char             Path[25];	
};

 static const struct ExpOneWireList_s ExpOneWireList[] = {
      /************************************************************/
      // Expected devices, if found here the defined name is used
      // (Id, "Your own sensor name", ROM type, "Hex name")
      // Note, several sensors may share the same Id, the last existing 
      // in the list below is used!  
      //
      // Ids must be 0..n in any order but with no gaps in the list!!
      /************************************************************/
 
      {0, "T-ute ", 0x28,  "80000000f6ebcd28"},	// Inne
      {0, "T-ute ", 0x28,  "45000000F7140A28"},  // Ute-givare båt    
			{0, "T-ute ", 0x28,  "80000000F7BAE628"},  // Inne på board		
			{0, "T-ute ", 0x28,  "9D00000115C4E728"},  // Inne, ny own pwrd

      {1, "T-box ", 0x10,  "36000800B62E2A10"},  // Båt-Box
      {1, "T-box ", 0x28,  "19000000FEF37E28"},	// Inne test, ny Own pwr
      {1, "T-box ", 0x28,  "21000001B8098028"},	// Inne test		 
      {1, "T-box ", 0x28,  "00000115fedb28"},	// Inne test
			
      {2, "T-kyl ", 0x28,  "C6000000C7A59128"}, // Båt-kyl
      {2, "T-kyl ", 0x28,  "19000000F6EBCD28"},	// Inne test   
			{2, "T-kyl ", 0x10,  "370000003478A710"},	// Inne test

      {3, "T-VV  ", 0x28,  "000001b8098028"},
      {3, "T-VV  ", 0x28,  "B4000001B7D01E28"}, // Inne
			{3, "T-VV  ", 0x10,  "0F000800AAA2A410"}, // VV-Båt
	/*   //{3, "Temp frys", 0x28, "000000f7140a28"},	
      {4, "AD Int   ", 0x20,  "00000009a97f20"}, // AD Battery 1 & 2
      {4, "AD Int   ", 0x20,  "00000009afba20"},
      {4, "AD Int   ", 0x20,  "00000009c78520"}, Not used 090507, AGY
*/
      {5, "AD Ext", 0x20,  "B70000001437CF20"}, // Ny-version Inne AD Water & diesel
			{5, "AD Ext", 0x20,  "0000000648ef20"}, // AD Water & diesel
			{5, "AD Ext", 0x20,  "000000094c9420"}, //  
      {5, "AD Ext", 0x20,  "EA00000006549120"},  // AD i båt
			{5, "AD Ext", 0x20,  "E7000000066A6220"}, // AD ext inne

//    {2, "Temp kort" ,0x28,  "000000f7bae628"},
//    {2, "Temp kort", 0x28,  "000000fef37e28"},
//    {2, "Temp kort", 0x28,  "00000115bfc028"},

      {6, "T-hav ", 0x28,  "000000c7e85b28"}, // Tveksam funktion?
      {6, "T-hav ", 0x22,  "0000001aa55922"}, // Gammal givare, bytes
      {6, "T-hav ", 0x22,  "0000000017C69F22"}, // BÅt-Gammal givare, bytes
      {6, "T-hav ", 0x28,  "80000000f6ebcd28"}, // Inne
			
      {7, "T-Vtn ", 0x28,  "80000000f6ebcd28"}, // Inne  

			{8, "LCD-1 ", 0xFF,  "420001000007EBFF"} // Inne LCD-1

    };
		
#define EXP_NO_OF_DEVICES ((sizeof(ExpOneWireList)/sizeof(ExpOneWireList[0])))
#define MAX_NO_OF_DEVICES 25  // Defines maximum Id no!!


// external functions defined in crcutil.c
void setcrc16(void);
unsigned short docrc16(unsigned short cdata);
void setcrc8(void);
unsigned char docrc8(unsigned char x);
