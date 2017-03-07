/*************************************************************************
 *      TimeoutHandler.c
 *
 *      Ver  Date       Name Description
 *      W    2006-11-24 AGY  Created.
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

#include	"SysDef.h"
#include	"Main.h"
// #include	"SysDef.h"

#define TICK_TIME			100000  // usec between each TICK
// Global variables
    
char  InfoText[300];  // To report messages
    
struct TimoRec_s {
  void               *Buff_p;
  int                 Client_fd;
  SIGSELECT   				RespSig;
  unsigned int        DeltaTime;
  char                ClientName[20];
  struct TimoRec_s   *Next_p;
};
	
struct TimoRec_s     *TimoList_p  = NULL; // Linked list of waiting timeouts

void Tick(unsigned int DeltaTime);
void LinkTimoRec(struct  TimoRec_s  *TimoRec_p);
char ChkTimoList(struct  TimoRec_s  *List_p);

void * TimeoutHandler(struct ProcState_s *PState) {
	int 								no, fd_Own;
  unsigned char       Buf[sizeof(union SIGNAL)];
	union SIGNAL				*Msg;
  struct TimoRec_s		*TimoRec_p;
  
  setpriority(PRIO_PROCESS, 0, -20);
  fd_Own = PState->fd.RD_TimoPipe;
  //sprintf (InfoText, " Own %d\r\n", fd_Own);
  //LOG_MSG(InfoText);  
	LOG_MSG("Started\n");

	while(TRUE) {
    while (read(fd_Own, Buf, sizeof(union SIGNAL)) != sizeof(union SIGNAL)) {
      Tick(TICK_TIME);  // Tick clock and check for timeouts
			if (ChkTimoList(TimoList_p) > 20) {			
				sprintf(InfoText, "No of timeouts now HIGH \r\n");
        LOG_MSG(InfoText); sleep(1);
			}
      usleep(TICK_TIME);
    };
    //no = read(fd_own, Buf, sizeof(union SIGNAL)-1);
		Msg = (void *) Buf;
    switch(Msg->SigNo) {
      case SIGTimoReq:
        TimoRec_p = malloc(sizeof(struct TimoRec_s));
        TimoRec_p->DeltaTime = Msg->Timo.DeltaTime;
        TimoRec_p->Client_fd = Msg->Timo.Client_fd;
        TimoRec_p->RespSig = Msg->Timo.RespSig;
        strcpy(TimoRec_p->ClientName, Msg->Timo.ClientName);
        TimoRec_p->Next_p = NULL;
        TimoRec_p->Buff_p = NULL;
 //sprintf(InfoText, "Timo Req: Client: %s Id: %d Time:  %d\n",  TimoRec_p->ClientName, Msg->Timo.Client_fd, TimoRec_p->DeltaTime);
 //LOG_MSG(InfoText);
        LinkTimoRec(TimoRec_p);
			break;

			default:
        sprintf(InfoText, "Illegal signal received: %d\n", Msg->SigNo);
				CHECK(FALSE, InfoText);
				break;
    } // switch
	} // while (TRUE)
};

/* void     usleep(WTIME);
 rex_ReqTimeout(unsigned long DeltaTime, TIMERREF **Ref, void *Sig_p)  {
    struct TimoRec_s            *TimoRec_p;

    if (Sig_p == NULL) {
      Sig_p = ALLOC(sizeof(struct Timeout_s), TIMEOUT);
    }
    rex_ChckLglPtr(Sig_p);
    TimoRec_p = ALLOC(sizeof(struct TimoRec_s), OS_TIMOREC);
    TimoRec_p->Owner     = GET_PROCESS;
    TimoRec_p->Next_p    = NULL;
    TimoRec_p->Buff_p    = Sig_p;
    TimoRec_p->DeltaTime = DeltaTime;
    LinkTimoRec(TimoRec_p);
    *Ref = (TIMERREF *) TimoRec_p;
  } */ /* rex_ReqTimeout */

/* void rex_Cancel(TIMERREF *Ref)  {
    struct TimoRec_s            *TimoRec_p;
    struct TimoRec_s            *Prev_p;
    struct TimoRec_s            *Tmp_p;

    TimoRec_p = (struct TimoRec_s *) Ref;  // Ptr to be deleted 
    Prev_p    = NULL;
    Tmp_p     = NULL;
    if (TimoList_p == TimoRec_p) {       // First in list 
      TimoList_p = TimoRec_p->Next_p;
      Tmp_p = TimoList_p->Next_p;
      if ( Tmp_p != NULL) {
        Tmp_p->DeltaTime += TimoRec_p->DeltaTime;  // Adjust relative timers 
      }
      if (TimoRec_p->Buff_p != NULL) {   // Free buffers 
        FREE(TimoRec_p->Buff_p);
      }
      FREE(TimoRec_p);
    }
    else {                              // Somewhere in list
      Tmp_p  = TimoList_p;
      Prev_p = TimoList_p;
      while ((Tmp_p != NULL) && (Tmp_p != TimoRec_p)) {  // Search linked list 
        Prev_p = Tmp_p;
        Tmp_p  = Tmp_p->Next_p;
      }
      if ((Tmp_p == NULL) && (Tmp_p != TimoRec_p)) {  // Found or end of list 
        CHECK(FALSE, "Timout Ref not found\r\n");
      }
      else {                            // Found, remove record 
        Prev_p->Next_p = TimoRec_p->Next_p;  // Re-link 
        Tmp_p = Prev_p->Next_p;
        if ( Tmp_p != NULL) {
          Tmp_p->DeltaTime += read(fd_own, Buf, sizeof(union SIGNAL)TimoRec_p->DeltaTime;  // Adjust relative timers 
        }
        if (TimoRec_p->Buff_p != NULL)  // Free buffers
          FREE(TimoRec_p->Buff_p);
        FREE(TimoRec_p);
      }
    }	  WAIT(fd_own, Buf, sizeof(union SIGNAL));

 } // rex_Cancel 

*/
void Tick(unsigned int TickTime)  {
    struct TimoRec_s          *Tmp_p;
    union SIGNAL							*Msg;
    char											 Buf[sizeof(union SIGNAL)];

    if (TimoList_p != NULL) {
     if (TimoList_p->DeltaTime <= TickTime) {
       while ((TimoList_p != NULL) &&
              (TimoList_p->DeltaTime <= TickTime)) {
         TickTime -= TimoList_p->DeltaTime;
          // TBD Write  SEND(&TimoList_p->Buff_p, TimoList_p->Owner);
         Tmp_p = TimoList_p;
         Msg = (void *) Buf;
		     Msg->SigNo = Tmp_p->RespSig;
	
			   strcpy(Msg->Timo.ClientName, Tmp_p->ClientName);
         SEND(Tmp_p->Client_fd, Msg, sizeof(union SIGNAL));
//sprintf(InfoText, "Send Timeout to %s Id: %d\n", Tmp_p->ClientName, Tmp_p->Client_fd);
//LOG_MSG(InfoText);
         TimoList_p = TimoList_p->Next_p;
         free(Tmp_p);
        } /* while */
        if (TimoList_p != NULL) 
					TimoList_p->DeltaTime -= TickTime;
      } else /* if */
        TimoList_p->DeltaTime -= TickTime;
    } /* if */
}      /* rex_Tick */
void LinkTimoRec(struct TimoRec_s *TimoRec_p)  {
    char                         NotLinked = TRUE;
    struct TimoRec_s            *Tmp_p;
    struct TimoRec_s            *Prev_p;

    if (TimoList_p == NULL) {
      TimoList_p = TimoRec_p;           /* Empty list, insert first element */
 // sprintf(InfoText, "1:th DeltaTime %d\n", TimoRec_p->DeltaTime);
  //LOG_MSG(InfoText);
    }
    else if (TimoList_p->DeltaTime > TimoRec_p->DeltaTime) { /* Insert first in list */
      TimoList_p->DeltaTime -= TimoRec_p->DeltaTime;  /* Decrease DeltaTime in List  */
      TimoRec_p->Next_p = TimoList_p;
      TimoList_p = TimoRec_p;
    }
    else {
      Prev_p = TimoList_p;
      Tmp_p  = TimoList_p;
      while (NotLinked) {
        if (Tmp_p == NULL) {     /* Last in list */
          Prev_p->Next_p = TimoRec_p;
          NotLinked = FALSE;
        }
        else if (Tmp_p->DeltaTime <= TimoRec_p->DeltaTime) { /* Walk through list */
          TimoRec_p->DeltaTime -= Tmp_p->DeltaTime;
          Prev_p = Tmp_p;
          Tmp_p  = Tmp_p->Next_p;
        }
        else {                    /* Link in list */
          Prev_p->Next_p = TimoRec_p;
          TimoRec_p->Next_p = Tmp_p;
          Tmp_p->DeltaTime -= TimoRec_p->DeltaTime;
          NotLinked = FALSE;
   // sprintf(InfoText, "In list DeltaTime %d\n", TimoRec_p->DeltaTime);
    //LOG_MSG(InfoText);
       }   /* else */
      }  /* while */
    }  /* else */
  } /* LinkTimoRec */
char ChkTimoList(struct TimoRec_s *List_p)  {
	  char                         Idx;
    struct TimoRec_s            *Tmp_p;

		Idx = 0;
		Tmp_p = TimoList_p;
    while (Tmp_p != NULL) {
      Tmp_p = Tmp_p->Next_p;
			Idx++;
		}	
		return Idx;
} /* ChkTimoList */