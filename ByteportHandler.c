
#include <byteport/byteport_mqtt.h>
#include <byteport/tcp.h>
#include <byteport/device_control_remote.h>
#include <byteport/bp_time.h>
#include <byteport/util.h>

#include <stdarg.h>
#include <inttypes.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "SysDef.h"
#include "lcd_def.h"
#include "Main.h"


/*   Kommentarer o förslag till förbättringar
Gör en bild av Byteport samt en device. Visa funktionalitet (pilar?) inkl terminology som används. Speciellt att namnge dom olika funktionerna
lista funktioner (och kommunikationshåll) och vad resp gör, lite förklaringar ..

Ta fram 2-3 exempel på olika tillämpningar (Arne kan då vara en). beskriv dessa samt visa i kod

Instruktioner + exempel (viktigt) för extract, kompilering, länkning

Denna fil innehåller massa saker som inte alltid behövs? (t.ex MyActions m.fl definitioner). Kanske göra motsvarande filer (dvs rensa onödigt) som 
dom 2-3 exemplena ovan?


*/

const char *byteport_uid       = "acc.broker.byteport.se";  // "acc.broker.byteport.se"
const char *dev_namespace      = "GoldenSpace";  
char       *dev_uid            = "JosefinShip";   // Can be changed later, when we have started and identified..

   
enum MyActions
{
    TA_ANCHUP = 0,
    TA_ANCHDWN = 0,
    TA_ANCHSLWDWN = 0,
    TA_REBOOT = 1337,
    TA_FW_UPGRADE
};

enum MyProperties
{
    TP_STATUS_LAST_REBOOT = 31337,
    TP_STATUS_ENUM,
    TP_STATUS_TEST_FLOAT,
    TP_CUSTOMER_GROUP,
    TP_MY_GROUP
};

// Customer specified properties, added as-is from customers specification.
static dc_field_t customer_props_arr[] =
{
    { .id=100, .field.i = {.min=  0, .max=5, .default_val=0 }, .meta_type=FM_SETTING, .data_type=FD_INT, .name="DebugLvl", .desc="0= None, 1-3" },
    { .id=101, .field.i = {.min= 20, .max=   50, .default_val=30 }, .meta_type=FM_SETTING, .data_type=FD_INT, .name="Reg101", .desc="Description of reg 101" },
    { .id=102, .field.i = {.min= 20, .max=   50, .default_val=30 }, .meta_type=FM_SETTING, .data_type=FD_INT, .name="Reg102", .desc="Description of reg 102" },
    { .id=103, .field.i = {.min= 30, .max=  100, .default_val=50 }, .meta_type=FM_SETTING, .data_type=FD_INT, .name="Reg103", .desc="Description of reg 103" }
};


static const char* mode_enum[] = {"Normal", "Abnormal", "Special"};

// Properties we need but the customer has not specified (incomplete specifiaction never happens right?)
static dc_field_t my_own_props_arr[] =
{
    { .id=TA_ANCHUP,             .meta_type=FM_ACTION, .data_type=FD_INT,    .field.i = { .min=  0, .max=1,  .default_val=0 },                           .name="AnchDwn",       .desc="Anchor up" },
    { .id=TA_ANCHDWN,            .meta_type=FM_ACTION, .data_type=FD_INT,    .field.i = { .min=  0, .max=1,  .default_val=0 },                           .name="AnchUp",        .desc="Anchor down" },
    { .id=TA_ANCHSLWDWN,         .meta_type=FM_ACTION, .data_type=FD_INT,    .field.i = { .min=  0, .max=1,  .default_val=0 },                           .name="AnchSlwUp",     .desc="Anchor slowly down" },
    { .id=TA_REBOOT,             .meta_type=FM_ACTION, .data_type=FD_INT,    .field.i = { .min=  0, .max=1,  .default_val=0 },                           .name="Reboot",         .desc="Trigger a reboot!" },
    { .id=TA_FW_UPGRADE,         .meta_type=FM_ACTION, .data_type=FD_STRING, .field.s = { .val_alloc_sz=256, .default_val="http://path/to/new_fw.zip" }, .name="FW_Update",      .desc="Start a FW upgrade, pass the URL" },
    { .id=TP_STATUS_LAST_REBOOT, .meta_type=FM_STATUS, .data_type=FD_STRING, .field.s = { .val_alloc_sz=256, .default_val="1970-01-01:00:00:00" },       .name="Last_reboot",    .desc="Date of last reboot" },
    { .id=TP_STATUS_ENUM,        .meta_type=FM_STATUS, .data_type=FD_ENUM,   .field.e = { .elements=mode_enum, .sz=VECTSIZE(mode_enum), .default_val = 1 },  .name="Test_enum_mode", .desc="Three enums added as test" },
    { .id=TP_STATUS_TEST_FLOAT,  .meta_type=FM_STATUS, .data_type=FD_FLOAT,  .field.f = { .min=  0, .max=1,  .default_val=0.1 },                         .name="Test_float",     .desc="A value just to test the float type" }
};



static dc_field_t root_props_arr[] =
{
    { TP_MY_GROUP,          "Myappprop", "Our own extra settings",                 FD_GROUP, FM_SETTING, .field.g={ .fields=my_own_props_arr,   .sz=VECTSIZE(my_own_props_arr) } },
    { TP_CUSTOMER_GROUP, "Customerprops", "customer settings as they specify them", FD_GROUP, FM_SETTING, .field.g={ .fields=customer_props_arr, .sz=VECTSIZE(customer_props_arr) } }
};

static dc_field_t root_group = { 0, "ROOT", "Root node", .data_type=FD_GROUP, .field.g = { root_props_arr, VECTSIZE(root_props_arr ) }};


//! Command-ack are synchronous in the sense that each byteport command sent byt us should
//! Result in exactly one acq. Sometimes the acq contains data, such as an ack to the echo command.
static void command_acq_cb( void *user_ctx, byteport_cmd_t cmd, byteport_error_code_t rval, const char *reply, int length )
{
    (void)user_ctx;
    (void)reply;
    (void)length;
    if (rval != 0)
      PRINTD( "Command ack: %s, return code: %s", cstringFrom__byteport_cmd(cmd), cstringFrom__byteport_error_code(rval) );
}

//! Asynchronous messages sent from byteport to our us the client.
static void message_cb( void *user_ctx, const char *ascii_json_list, int length )
{
    (void)user_ctx;
    PRINTD("Got message: %.*s", length, ascii_json_list );
}



// Called by byteport service whenever a property (may) have changed
static void property_changed_callback( dc_field_t *root, void *user_ctx )
{
    //PRINTD("PROPERTY (may have) CHANGED CB!");
    char    	       	InfoText[300];      // LOG_MSG Store info text

    (void) user_ctx;
    dc_field_t *customer = dc_get_by_id( root, TP_CUSTOMER_GROUP );

    BP_ASSERT( customer && customer->data_type == FD_GROUP, "" );

    if ( dc_changed_recursive( customer ) )
    {
        dc_field_t *t=dc_get_first( customer );
        for( ; t != NULL; t = dc_get_next(customer, t) )
        {

            if( !dc_changed(t) )
                continue;

            // We know that all customer registers are INT in this example so we need not check the type
            // This call will cleare the changed flag
            int val = dc_get_i( t );
            DebugOn = val;
            sprintf(InfoText, "Debug set: %d \r\n", DebugOn);
            LOG_MSG(InfoText);
            //PRINTD("Property: %s got new value: %i", t->name, val);
        }
    }

}

// Anropas av byteport_service() då en ny action triggas
static void action_triggerd_callback( dc_field_t *action, dc_field_t *root, void *user_ctx )
{
    switch( (enum MyActions) action->id )
    {
    case TA_REBOOT:
        PRINTD( "Action received: Rebooting! Updating status field \"Last reboot\"");
        struct timespec time;
        clock_gettime(CLOCK_REALTIME, &time);
        // Update the status-field which should be seen in the control tab of the device in byteport.
        dc_set_s( dc_get_recursive_by_id(root, TP_STATUS_LAST_REBOOT, -1), ctime( &time.tv_sec ) );

        break;
    case TA_FW_UPGRADE:
        PRINTD( "Action received: Will download firmware from %s", dc_get_s(action) );
        // Insert your code to handle a FW upgrade here
        break;
    }

}


// Take care of the log callbacks
static void custom_logger( void* ctx, int lvl, const char *file, int line, const char *fmt, ...)
{
    if( lvl <= BP_NOTICE)
    {
        va_list argp;
        va_start(argp, fmt);
        (void) ctx;

        FILE *output = lvl == BP_ERROR ? stderr
                                       : stdout;

        fflush(stdin);
        fflush(stdout);

        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        struct tm nt;
        localtime_r( &now.tv_sec, &nt );

        const char *file_no_path = strrchr(file, '/');

        fprintf( output, "%04i-%02i-%02i %02i:%02i:%02i.%03i %s:%i\t",
                 nt.tm_year+1900, nt.tm_mon+1, nt.tm_mday,
                 nt.tm_hour, nt.tm_min, nt.tm_sec, (int)(now.tv_nsec/1000000),
                 file_no_path ? file_no_path+1 : file, line);
        vfprintf(output, fmt, argp );

        fflush(output);

        va_end(argp);
    }
}

void  				* BPHandler(struct ProcState_s *PState) {
    
    int          	   	Initiated = FALSE, fd_Client, fd_ToOwn, fd_own, fd_2main, fd_timo;
    unsigned char			Buf[sizeof(union SIGNAL)];
    union SIGNAL 			*Msg;
    int   rv;
    char csv[200]; 
    char    	       	InfoText[300];      // LOG_MSG Store info text
      
    // Setup transport layer
    bp_tcp_t tcp;
    bp_tcp_init( &tcp );
    
    bp_tcp_set_host(&tcp, byteport_uid, BP_MQTT_TCP_PORT);  // AG Set Byteport instance to use, normally byteport.se
    tcp.log_cb = custom_logger;


    // Setup byteport
    byteport_mqtt_t bp;
    byteport_mqtt_init( &bp );
    byteport_mqtt_set_logger( &bp, custom_logger, NULL );

    // MQTT Credentials
    //mqtt_setup_user( &bp.mqtt, "GoldSpace Josefin", "GoldingProd", "byteport" );  // AG vad är detta? Något jag skall sätta?
    
    mqtt_setup_user( &bp.mqtt, "BP_MQTT_CONSOLE_EXAMPLE", "developer", "byteport" );
    mqtt_set_keepalive_interval( &bp.mqtt, 0 );

    byteport_mqtt_setup_credentials( &bp, dev_namespace, dev_uid  );
    byteport_mqtt_register_callbacks( &bp, command_acq_cb, NULL, message_cb, NULL);

    mqtt_acquire_transport( &bp.mqtt, bp_tcp_get_transport(&tcp) );

    // Setup the device control system
    dc_field_init_static_tree( &root_group );
    dc_set_root( &bp.dc, &root_group );

    dc_set_prop_cb( &bp.dc, property_changed_callback, NULL);
    dc_set_act_cb(  &bp.dc, action_triggerd_callback, NULL);

    // Use heartbeat as a connect command


    struct timespec t;
    bp_clock_gettime_real( &t);

    // Setup own id & libbyteport instance to use
    byteport_error_code_t bp_err = BP_ERR_GENERAL_ERROR;
    
    fd_own   = PState->fd.RD_BPRepPipe;
    fd_ToOwn = PState->fd.WR_BPRepPipe;
    fd_2main = PState->fd.WR_MainPipe;
    fd_timo  = PState->fd.WR_TimoPipe;
    //REQ_TIMEOUT(fd_timo, fd_ToOwn, "ByteportSrv", SIGByteportSrv, 3 Sec); 
    
    rv = 0; // Set as fault to start with..     
    //LOG_MSG ("Enter loop \r\n");
    byteport_mqtt_cmd_heartbeat( &bp, NULL);  // Removes warnings..
    LOG_MSG("Started\r\n");    
    while(TRUE) {
        // Select with a timeout of one second. The timeout is needed so service can be called every now and then
        // and send keepalives.
      fd_set readfds;
      struct timeval timeout = { 5, 0 };  // Changed to 5 sec

        // ---- SELECT() -------------------------------------------------------------------------
        // ---- Wait for two alternative input sources. in this example they are:
        // ---- We use a timeout so our polling handler (SERVICE and STORE_TIME) can do their polling
      FD_ZERO( &readfds );
      byteport_mqtt_fd_set( &bp, &readfds );  // ... The byteport mqtt socket (if connected) and
      FD_SET( fd_own, &readfds );       
       
      //LOG_MSG ("Enter select\r\n");       
      int sel = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout) ;
      if( sel < 0 ) {
            PRINTD("Error %s", strerror(errno) );
            usleep(1000*1000);
            continue;
      }
       
        // ---- Handle event: SERVICE-------------------------------------------------------------------------
        // ---- Always perform a servie, regardles if select returned because of timeout or not
        // ---- Service internally checks the readfds pointer and will not do anything unecessary if there is nothing to do
      bp_err = byteport_mqtt_service( &bp, &readfds);
      //LOG_MSG ("Proceed from select\r\n");       

      if( bp_err < 0 )
        DBG_ERR("%s Warning during service: %i %s", now(), bp_err, cstringFrom__byteport_error_code(bp_err) );



        // ---- Handle event: READ_PIPE-------------------------------------------------------------------------
        // ---- Handle out other input source, if select indicates that there is something to read
        // ---- If there are more pipes we select on, just add more if-statemes as this one
      if( FD_ISSET( fd_own, &readfds) )   {
        WAIT(fd_own, Buf, sizeof(union SIGNAL));
  	    Msg = (void *) Buf;
		  
        //LOG_MSG ("Rec Byteport signal \r\n");       

        switch(Msg->SigNo) {
          case SIGByteportInit:
            if (!strcmp(dev_uid, Msg->ByteportReport.Str)) {
              byteport_mqtt_setup_credentials( &bp, dev_namespace, "JosefinSim"  );
              LOG_MSG(" Reporting as JosefinSim initiated\r\n");
            } else {
              byteport_mqtt_setup_credentials( &bp, dev_namespace, "JosefinShip" );
              LOG_MSG(" Reporting as JosefinShip initiated\r\n");
            }            
          break;        
          case SIGByteportReport:
          //LOG_MSG("SIG: Report rec\n");
            if( byteport_mqtt_cmd_ready( &bp ) ){
 
              snprintf( csv, sizeof(csv), Msg->ByteportReport.Str  );//"OutTemp=%i", (int)(10) );
              bp_err = byteport_mqtt_cmd_store_csv( &bp, NULL, csv );
              
              //printf(" Send to byteport: %s %d\r\n", csv, sizeof(Msg->ByteportReport.Str));
                if( bp_err < 0 ) {
                    PRINTD(" %s Error sending timestamp: %i %s", now(), bp_err, cstringFrom__byteport_error_code(bp_err) );
                } else {
                    dc_field_t *f = dc_get_recursive_by_id( bp.dc.root, 100, -1);
                    dc_set_i( f, t.tv_sec % 1000 );                 


                } // Else

            } // end if
            if (DebugOn == 1) { sprintf(InfoText, "%s \r\n", csv); LOG_MSG(InfoText);}  
          break;
          case SIGByteportSrv:
            //LOG_MSG("SIG: Timeout rec\n"); 
 
            REQ_TIMEOUT(fd_timo, fd_ToOwn, "ByteportSrv", SIGByteportSrv, 3 Sec); 
          break;
          default:
     	      sprintf(InfoText, "Illegal signal received: %d MsgLen: %d Data: %x %x %x %x\n", Msg->SigNo, sizeof(Msg), Msg->Data[0], Msg->Data[1],Msg->Data[2],Msg->Data[3]);
     	      CHECK(FALSE, InfoText);
          break; 
      } // End switch   
    }
 


    // ---- POLLING: RECONNECT.
    // ---- Try to reconnect if possible.
    // ---- This is OPTIONAL, any command will try to reconnect (NOTE: service() on the other hand will NEVER try to reconnect )
    // ---- But if we are not connected we will not receive any device-control messages either so we strive to be connected when possible.
    if( !byteport_mqtt_is_connected( &bp) ) {
      PRINTD("Connecting to mqtt server...");
      bp_err=byteport_mqtt_cmd_heartbeat( &bp, NULL);
      if( bp_err < 0 )
        DBG_ERR("Failed to connect: %i %s", bp_err, cstringFrom__byteport_error_code(bp_err) );
    } 
  }  // While
}

  