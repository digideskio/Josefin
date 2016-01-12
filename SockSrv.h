/*************************************************************************
 *
 *      Ver  Date       Name Description
 *      W    2013-03-02 AGY  Created.
 *
 *************************************************************************/

 
 enum ServModes_e {Dwn, SlwUp, Up, AnchStop, SetTime, GetSensData, Dbg, ExitConnection};
 
 struct ComBuf_s {
	enum ServModes_e	ServMode;
		//union SendBuf_s	{
  float      MinOutTemp;     
  float      MaxOutTemp;   
  float      OutTemp;  
  float      MinSeaTemp; 
  float      MaxSeaTemp;   
  float      SeaTemp;       
  float      MinRefrigTemp; 
  float      MaxRefrigTemp;  
  float      RefrigTemp;     
  float      MinBoxTemp;  
  float      MaxBoxTemp;     
  float      BoxTemp;       
  float      MinWaterTemp;
  float      MaxWaterTemp;
  float      WaterTemp;
  float      WaterLevel;
	float      HWTemp;
  float      DieselLevel;
  float      BatVoltS; 
  float      BatVoltF; 
  float      BatAmpS;
  float      BatAmpF;
	
			//char		Buf[1024];
		//} SendBuf;	
		
 };