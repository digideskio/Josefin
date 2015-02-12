/*************************************************************************
 *      OneWireHandler.h
 *
 *      Ver  Date       Name Description
 *      W    2006-11-24 AGY  Created.
 *
 *************************************************************************/







// Define allowed device types, add new and change in code...
#define DEV_NO_DEV					0x00    // No sensor defined
#define DEV_DS1820         	0x10		// Temperature sensor, old model
#define DEV_DS2450         	0x20		// AD sensor
#define DEV_DS1822         	0x22    // Temperature sensor, old model
#define DEV_DS18B20					0x28    // Temperature sensor

 /* Declaration of types */
 
struct OneWireList_s {
	 char						  Present; // Boolean indicating if sensor exists or not
   char             Id;
   //FILE						 *fp;  // File pointer to sensor
   char             SensName[16];
   unsigned char    DevType;
   char             Path[100];
   float						Val[4];   // Read values	
};


struct ExpOneWireList_s {
   char             Id;
   char             SensName[16];
   unsigned char    DevType;
   char             Path[100];	
};

 static const struct ExpOneWireList_s ExpOneWireList[] = {
      /************************************************************/
      // Expected devices, if found here the defined name is used
      // (Id, "Your own sensor name", ROM type, "path")
      // Note, several sensors may share the same Id, the last existing 
      // in the list below is used!  
      //
      // Ids must be 0..n in any order but with no gaps in the list!!
      /************************************************************/

      {0, "T-ute ", 0x28,  "/sys/devices/w1_bus_master1/28-000000f6ebcd/w1_slave"},	// Inne
      {0, "T-ute ", 0x28,  "/sys/devices/w1_bus_master2/28-000000f6ebcd/w1_slave"}, // Inne

      {0, "T-ute ", 0x28,  "/sys/devices/w1_bus_master1/28-000000f7140a/w1_slave"},
      {0, "T-ute ", 0x28,  "/sys/devices/w1_bus_master2/28-000000f7140a/w1_slave"},

      {1, "T-box ", 0x10,  "/sys/devices/w1_bus_master1/10-000800b62e2a/w1_slave"},
      {1, "T-box ", 0x10,  "/sys/devices/w1_bus_master2/10-000800b62e2a/w1_slave"},

     // {1, "Temp box ", 0x28,  "/sys/devices/w1_bus_master1/28-000000f6ebcd/w1_slave"},	// Inne test
     // {1, "Temp box ", 0x28,  "/sys/devices/w1_bus_master2/28-000000f6ebcd/w1_slave"},	// Inne test
      {1, "T-box ", 0x28,  "/sys/devices/w1_bus_master3/28-00000115fedb/w1_slave"},	// Inne test

      {2, "T-kyl ", 0x28,  "/sys/devices/w1_bus_master1/28-000000c7a591/w1_slave"},
      {2, "T-kyl ", 0x28,  "/sys/devices/w1_bus_master2/28-000000c7a591/w1_slave"},

      {2, "T-kyl ", 0x28,  "/sys/devices/w1_bus_master1/28-000000f6ebcd/w1_slave"},	// Inne test
      {2, "T-kyl ", 0x28,  "/sys/devices/w1_bus_master2/28-000000f6ebcd/w1_slave"},	// Inne test

      {3, "T-VV  ", 0x28,  "/sys/devices/w1_bus_master1/28-000001b80980/w1_slave"},
      {3, "T-VV  ", 0x28,  "/sys/devices/w1_bus_master2/28-000001b80980/w1_slave"},
      {3, "T-VV  ", 0x28,  "/sys/devices/w1_bus_master1/28-000001b7d01e/w1_slave"}, // Inne
      {3, "T-VV  ", 0x28,  "/sys/devices/w1_bus_master2/28-000001b7d01e/w1_slave"}, // Inne
			{3, "T-VV  ", 0x10,  "/sys/devices/w1_bus_master1/10-000800aaa2a4/w1_slave"}, // Båt
      {3, "T-VV  ", 0x10,  "/sys/devices/w1_bus_master2/10-000800aaa2a4/w1_slave"}, // Båt
 /*   //{3, "Temp frys", 0x28, "/sys/devices/w1_bus_master2/28-000000f7140a/w1_slave"},	
      {4, "AD Int   ", 0x20,  "/sys/devices/w1_bus_master4/20-00000009a97f/voltages"}, // AD Battery 1 & 2
      {4, "AD Int   ", 0x20,  "/sys/devices/w1_bus_master4/20-00000009afba/voltages"},
      {4, "AD Int   ", 0x20,  "/sys/devices/w1_bus_master4/20-00000009c785/voltages"}, Not used 090507, AGY
*/
      {5, "AD Ext", 0x20,  "/sys/devices/w1_bus_master1/20-000000066a62/voltages"}, // Inne AD Water & diesel
      {5, "AD Ext", 0x20,  "/sys/devices/w1_bus_master2/20-000000066a62/voltages"}, // Inne AD Water & diesel
      {5, "AD Ext", 0x20,  "/sys/devices/w1_bus_master1/20-0000000648ef/voltages"}, // AD Water & diesel
      {5, "AD Ext", 0x20,  "/sys/devices/w1_bus_master2/20-0000000648ef/voltages"}, // AD Water & diesel
      {5, "AD Ext", 0x20,  "/sys/devices/w1_bus_master1/20-000000094c94/voltages"}, // 
      {5, "AD Ext", 0x20,  "/sys/devices/w1_bus_master2/20-000000094c94/voltages"}, // 
      {5, "AD Ext", 0x20,  "/sys/devices/w1_bus_master1/20-000000065491/voltages"},
      {5, "AD Ext", 0x20,  "/sys/devices/w1_bus_master2/20-000000065491/voltages"},

//    {2, "Temp kort" ,0x28,  "/sys/devices/w1_bus_master3/28-000000f7bae6/w1_slave"},
//    {2, "Temp kort", 0x28,  "/sys/devices/w1_bus_master3/28-000000fef37e/w1_slave"},
//    {2, "Temp kort", 0x28,  "/sys/devices/w1_bus_master3/28-00000115bfc0/w1_slave"},

      {6, "T-hav ", 0x28,  "/sys/devices/w1_bus_master1/28-000000c7e85b/w1_slave"}, // Tveksam funktion?
      {6, "T-hav ", 0x28,  "/sys/devices/w1_bus_master2/28-000000c7e85b/w1_slave"},
      {6, "T-hav ", 0x22,  "/sys/devices/w1_bus_master1/22-0000001aa559/w1_slave"}, // Gammal givare, bytes
      {6, "T-hav ", 0x22,  "/sys/devices/w1_bus_master2/22-0000001aa559/w1_slave"},

      {6, "T-hav ", 0x22,  "/sys/devices/w1_bus_master1/22-00000017c69f/w1_slave"}, // Gammal givare, bytes
      {6, "T-hav ", 0x22,  "/sys/devices/w1_bus_master2/22-00000017c69f/w1_slave"},

      {6, "T-hav ", 0x28,  "/sys/devices/w1_bus_master1/28-000000f6ebcd/w1_slave"}, // Inne
      {6, "T-hav ", 0x28,  "/sys/devices/w1_bus_master2/28-000000f6ebcd/w1_slave"}, // Inne
			
			{7, "T-Vtn ", 0x28,  "/sys/devices/w1_bus_master1/28-000000f6ebcd/w1_slave"}, // Inne
      {7, "T-Vtn ", 0x28,  "/sys/devices/w1_bus_master2/28-000000f6ebcd/w1_slave"} // Inne

    };
		
#define EXP_NO_OF_DEVICES ((sizeof(ExpOneWireList)/sizeof(ExpOneWireList[0])))
#define MAX_NO_OF_DEVICES 30  // Defines maximum Id no!!


// external functions defined in crcutil.c
void setcrc16(void);
unsigned short docrc16(unsigned short cdata);
void setcrc8(void);
unsigned char docrc8(unsigned char x);
