/*************************************************************************
 *      Socket server.c
 *
 *      Ver  Date       Name Description
 *      w    2013-03-02 AGY  Created.
 *
 *************************************************************************/
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <fcntl.h>

//#include "SocketServer.h"
#include "SysDef.h"
#include "Main.h"

void * SockServer(enum ProcTypes_e ProcType) {
		int 						fd_2main, sockfd = 0, n = 0;
    char 						c, recvBuff[sizeof(struct ComBuf_s)];
	  unsigned char  	Buf[sizeof(union SIGNAL)]; 
		union SIGNAL   *Msg;
	
		struct ComBuf_s  *ComBuf;

		char	InfoText[100];
    int ExitConn, listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr; 
    char sendBuff[sizeof(struct ComBuf_s)];
    time_t ticks; 
 
    memset(recvBuff, '0',sizeof(recvBuff));
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)    {
        printf("\n Error : Could not create socket \n");
        return 1;
    } 

    memset(&serv_addr, '0', sizeof(serv_addr)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5123); 

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

    listen(listenfd, 10); 	
		OPEN_PIPE(fd_2main, MAIN_PIPE, O_WRONLY);
		LOG_MSG("Started\r\n");		
 
 		listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5123); 

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

    listen(listenfd, 10); 
		Msg = Buf;
		ComBuf = (struct ComBuf_s *) malloc(sizeof(struct ComBuf_s));
    while (TRUE) {	
			ExitConn = FALSE;
			connfd = accept(listenfd, (struct sockaddr*)NULL, NULL); 
			printf("Connected Id: %d \r\n", connfd); // Connected! Wait for messages..
			while (((n = read (connfd, recvBuff, sizeof(recvBuff))) > 0) && (!ExitConn)) {
				//ComBuf = recvBuff; 

				memcpy(ComBuf, recvBuff, sizeof(struct ComBuf_s));
				//printf("ModeState %d\r\n", ComBuf->ModeState);
				switch (ComBuf->ServMode) {
					case Dbg:	
					  if (DebugOn) {
						  DebugOn = FALSE;
							printf("Dbg OFF\r\n");
						} else {
							DebugOn = TRUE;
							printf("Dbg ON\r\n"); 
						}
					break;					
					case Dwn:	
						Msg->SigNo = SIGServCmdReq;// Send signal	
						Msg->ServCmdReq.Cmd = Dwn; 
						SEND(fd_2main, Msg, sizeof(union SIGNAL));
					break;					
					case Up:	
						Msg->SigNo = SIGServCmdReq;// Send signal	
						Msg->ServCmdReq.Cmd = Up; 
						SEND(fd_2main, Msg, sizeof(union SIGNAL));
						//printf("Up ordered\r\n");
					break;
					case SlwUp:	
						Msg->SigNo = SIGServCmdReq;// Send signal	
						Msg->ServCmdReq.Cmd = SlwUp; 
						SEND(fd_2main, Msg, sizeof(union SIGNAL));
						//printf("Slow up ordered\r\n");
					break;
					case AnchStop:	
						Msg->SigNo = SIGServCmdReq;// Send signal	
						Msg->ServCmdReq.Cmd = AnchStop; 
						SEND(fd_2main, Msg, sizeof(union SIGNAL));
						//printf("Slow up ordered\r\n");
					break;
					case SetTime:	
						//printf("Set time ordered\r\n");
						Msg->SigNo = SIGServCmdReq;// Send signal	
						Msg->ServCmdReq.Cmd = SetTime; 
						SEND(fd_2main, Msg, sizeof(union SIGNAL));

						//settime(&ticks);
					break;
					case GetSensData:	
						printf("Get sensor data ordered\r\n");
						ComBuf->OutTemp = ProcState.OutTemp;	
						ComBuf->BoxTemp = ProcState.BoxTemp;
						ComBuf->RefrigTemp = ProcState.RefrigTemp;	
						ComBuf->WaterLevel = ProcState.WaterLevel;
						ComBuf->DieselLevel = ProcState.DieselLevel;	
						memcpy(sendBuff, ComBuf,sizeof(struct ComBuf_s));
						n = write(connfd, sendBuff, sizeof(struct ComBuf_s)); 
						if (n < 0) printf("Write error: %d\r\n", n);						
					break;					
					case ExitConnection:	
						printf("Exit ordered, Good bye!!\r\n");
						ExitConn = TRUE;
					break;					
					default:		
						recvBuff [n] = 0;
						printf("Illegal selection: %d Msg: %X\r\n", n, recvBuff[0]);
					break;
				}

				ticks = time(NULL);
				snprintf(sendBuff, sizeof(sendBuff), "%.24s\r\n", ctime(&ticks));
				//write(connfd, sendBuff, strlen(sendBuff)); 
			}  
			close(connfd);
			printf("Connection closed\r\n");
		}
}




 