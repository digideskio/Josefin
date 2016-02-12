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
#define DEV_LCD							0xFF    // LCD module

#define JosefinShipID				"28.91A5C7000000"   // Select one 1Wire ID as the ID for that this must be JosefinShip (byteport reporting)
																								// otherwise, if not found, its JosefinSim

#define OWFS_MP							"/mnt/1wire/"   // Mount point for OWFS filesystem

// Definitions for 1W LCD
#define LCD1								8
#define LCD2								9
#define LCD1W_WRITE(a,b, c)		LCD1W_Write(a,b,c);   // a=LCD Id, b=Line, c=Text msg max 20 char

/*#define LCD_ON						1    		// LCD-1W
#define LCD_OFF							2    		// LCD-1W
#define LCD_BKL_ON					3    		// LCD-1W
#define LCD_BKL_OFF					4    		// LCD-1W
#define LCD_CLEAR						5    		// LCD-1W
*/
 /* Declaration of types */
 
extern char 	          Set1WLCDOn(int LCD_Id); // Turn Display ON
extern char 	          Set1WLCDBlkOn(int LCD_Id); // Turn backlight ON
extern char							Set1WLCDBlkOff(int LCD_Id); // Turn backlight OFF 

struct OneWireList_s {
	 char						  Present; // Boolean indicating if sensor exists or not
   char             Id;
   //FILE						 *fp;  // File pointer to sensor
   char             SensName[16];
   unsigned char    DevType;
   char             Path[55];
   float						Val[4];   // Read values
	 int  						Data;  // Normally not used but for AD for ex you can select Conversion factor
};


struct ExpOneWireList_s {
   char             Id;
   char             SensName[16];
   unsigned char    DevType;
   char             Path[55];	
	 int  						Data;  // Normally not used but for AD for ex you can select Conversion factor
};


 static const struct ExpOneWireList_s ExpOneWireList[] = {
      /************************************************************/
      // Expected devices, if found here the defined name is used
      // (Id, "Your own sensor name", ROM type, "Hex name", "Factor")
      // Note, several sensors may share the same Id, the last existing 
      // in the list below is used!  
			// Syntax: 28.xxx  == using usb and OWFS
			//				 800xxxxxxxx28 == using HA7S
      // I have therefore included both definitions in the structure 
			// below so you can use either. Remember to add for both for new sensors
			// New (not defined) sensors can be found in /mnt/1wire (mount point)
      // Ids must be 0..n in any order but with no gaps in the list!!
      //
      // Factor: 
      // For AD (type 20) factor gives division factor
      // For Temperature sensors marked "1" = Using 3 wires, better performance
      // Not converted marked "0" = Using 2 wires only
      /************************************************************/
 
 
      {0, "T-ute ", 0x28,  "28.CDEBF6000000", 1},	// Inne, utegivare på ute-väggen    
			{0, "T-ute ", 0x28,  "28.1ED0B7010000", 0},	// Inne
			
      {0, "T-ute ", 0x28,  "45000000F7140A28", 0},  // Ute-givare båt    
      {0, "T-ute ", 0x28,  "28.0A14F7000000", 0},  // Ute-givare båt    
			{0, "T-ute ", 0x28,  "80000000F7BAE628", 0},  // Inne på board		
			{0, "T-ute ", 0x28,  "28.E7C415010000", 0},  // USB-OFS Inne, ny own pwrd

      {1, "T-box ", 0x10,  "36000800B62E2A10", 0},  // HA7S Båt-Box     
			{1, "T-box ", 0x10,  "10.2A2EB6000800", 0},   // USB-OWFS Båt-Box
      {1, "T-box ", 0x28,  "28.7EF3FE000000", 0},	 // Inne test, ny Own pwr
      {1, "T-box ", 0x28,  "28.99D3B7010000", 0},	 // Inne test, ny ej konverterad Power
      {1, "T-box ", 0x28,  "21000001B8098028", 0},	// Inne test		 
      {1, "T-box ", 0x28,  "00000115fedb28", 0},	// Inne test
			
      {2, "T-kyl ", 0x28,  "C6000000C7A59128", 0}, // HA7S Båt-kyl
      {2, "T-kyl ", 0x28,  "28.91A5C7000000", 0},  // USB-OWFS Båt-kyl
      {2, "T-kyl ", 0x28,  "28.CD14B8010000", 1},  // Inne test USB-OWFS Båt-kyl, konverterad     
      {2, "T-kyl ", 0x28,  "19000000F6EBCD28", 0},	// Inne test   
			{2, "T-kyl ", 0x10,  "370000003478A710", 0},	// Inne test	
			{2, "T-kyl ", 0x28,  "28.5BE8C7000000", 0},	// Inne test

      {3, "T-VV  ", 0x28,  "000001b8098028", 0},
      {3, "T-VV  ", 0x28,  "B4000001B7D01E28", 0}, // Inne
			{3, "T-VV  ", 0x10,  "0F000800AAA2A410", 0}, // VV-Båt
	/*   //{3, "Temp frys", 0x28, "000000f7140a28", 0},	
      {4, "AD Int   ", 0x20,  "00000009a97f20", 0}, // AD Battery 1 & 2
      {4, "AD Int   ", 0x20,  "00000009afba20", 0},
      {4, "AD Int   ", 0x20,  "00000009c78520", 0}, Not used 090507, AGY
*/
      {5, "AD Ext", 0x20,  "20.CF3714000000", 5}, // USB-OWFS Ny-version Inne AD Water & diesel, factor 5
			{5, "AD Ext", 0x20,  "20.EF4806000000", 5}, // AD Water & diesel
			{5, "AD Ext", 0x20,  "000000094c9420", 5}, //  
      {5, "AD Ext", 0x20,  "20.9149650000000", 3.14},  // AD i båt     
			{5, "AD Ext", 0x20,  "20.915406000000", 3.14},  // USB-OWFS AD i båt
			{5, "AD Ext", 0x20,  "20.626A06000000", 3.14}, // AD ext inne (på trä kortet)

//    {2, "Temp kort" ,0x28,  "000000f7bae628", 0},
//    {2, "Temp kort", 0x28,  "000000fef37e28", 0},
//    {2, "Temp kort", 0x28,  "00000115bfc028", 0},

      {6, "T-hav ", 0x28,  "000000c7e85b28", 0}, // Tveksam funktion?
      {6, "T-hav ", 0x22,  "0000001aa55922", 0}, // Gammal givare, bytes
      {6, "T-hav ", 0x22,  "0000000017C69F22", 0}, // HA7S BÅt-Gammal givare, bytes
      {6, "T-hav ", 0x22,  "22.9FC61700000", 0}, // USB-OWFS BÅt-Gammal givare, bytes
      {6, "T-hav ", 0x28,  "80000000f6ebcd28", 0}, // Inne
			
      {7, "T-Vtn ", 0x28,  "80000000f6ebcd28", 0}, // Inne  

			{8, "LCD-1 ", 0xFF,  "420001000007EBFF", 0},// Inne LCD-1 HA7S format
      {8, "LCD-1 ", 0xFF,  "FF.EB0700000100", 0}, // Inne LCD-1 BIG OWFS format			  	
      {8, "LCD-1 ", 0xFF,  "FF.750800000100", 0}  // Inne LCD-2 BIG OWFS format

    };
		
#define EXP_NO_OF_DEVICES ((sizeof(ExpOneWireList)/sizeof(ExpOneWireList[0])))
#define MAX_NO_OF_DEVICES 25  // Defines maximum Id no!!

// Global varaiable
 struct 	OneWireList_s OneWireList[MAX_NO_OF_DEVICES]; // Maximum no of allowed devices on 1-wire net!


// external functions defined in crcutil.c
void setcrc16(void);
unsigned short docrc16(unsigned short cdata);
void setcrc8(void);
unsigned char docrc8(unsigned char x);
