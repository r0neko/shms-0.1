#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define SERVER "UN IP INVIZIBIL"
#define BUFLEN 512  //Max length of buffer
#define PORT 3546   //The port on which to send data

char buf[BUFLEN];
struct sockaddr_in si_other;
int s;
socklen_t slen = sizeof(si_other); // define some things here

int newMessage = 0;
int inTunnel = 0;
int listening = 0;
char* messReceived = (char*) malloc(BUFLEN);

//prototypes
void beforeExit(void);
void* listenFct(void* a);

void die(char *s)
{
  perror(s);
  exit(1);
}

void sendUDPMessage(char* message) {
  if (sendto(s, message, strlen(message), 0, (struct sockaddr *) &si_other, slen) == -1) {
    die("sendto()");
  }
}

void* listenFct(void* a) {
  while (1) {
    memset(buf, '\0', BUFLEN);

    if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen) == -1) {
      die("recvfrom()");
    }

    char* message = strtok(buf, " ");
    if (strcmp(message, "SHMS/1.0") == 0) {
      message = strtok(NULL, " ");
      if(message != NULL) {
      	if (strncmp(message, "QUERY_ALIVE", 11) == 0) {
        	//sendUDPMessage("SHMS/1.0 QAL_ACCEPTED");
        	printf("This is a QueryAlive... don't panic...\n");
      	} else if (strncmp(message, "INCOMING_CONNECTION", 19) == 0) {
		//printf("Seems like we got a new UDP tunnel :) Yay!\n");
		inTunnel = 1;
     	} else if (strncmp(message, "UNREGISTER", 10) == 0) {
		//printf("The client seems to leave! We are again alone! :(\n");
		inTunnel = 0;
      	} else {
        	if (!newMessage && listening) {
          		strcpy(messReceived, buf);
          		newMessage = 1;
        	}
      	}
      }
    }
  }
}

void initUDP() {
  atexit(beforeExit);
  if ( (s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
    die("socket");
  }

  memset((char *) &si_other, 0, sizeof(si_other));
  si_other.sin_family = AF_INET;
  si_other.sin_port = htons(PORT);

  if (inet_aton(SERVER , &si_other.sin_addr) == 0)
  {
    fprintf(stderr, "inet_aton() failed\n");
    exit(1);
  }
  pthread_t listenThread;
  int err = pthread_create(&listenThread, NULL, &listenFct, NULL);
  if (err == 0) {
    printf("\n[UDP] UDP Thread started!\n");
  } else {
    printf("\n[UDP] UDP Thread FAILED TO start!\n");
    exit(1);
  }
}

char* readUDPMessage() {
  char* messageR = (char*) malloc(512);
  newMessage = 0;
  listening = 1;
  while (!newMessage && inTunnel) {
    continue;
  }
  if (newMessage && inTunnel) {
    newMessage = 0;
    strcpy(messageR, messReceived);
    free(messageR);
    return messageR;
  }
  listening = 0;
  free(messageR);
  return "";
}

void beforeExit(void) {
  sendUDPMessage("SHMS/1.0 UNREGISTER");
  close(s);
  printf("Goodbye world! Here we go!\n");
}

void registerHubToServer(unsigned char* hubID) {
  char* regID = (char*) malloc(16);
  char* udpM = (char*) malloc(512);
  for (int x = 0; x < 8; x++) {
    sprintf(regID, "%s%02X\0", regID, hubID[x]);
  }
  sprintf(udpM, "SHMS/1.0 REGISTERH %s", regID);
  //printf("%s\n", udpM);
  sendUDPMessage(udpM); // register to the server
}

void udpPostSB(unsigned char* id, int value, float v, unsigned char* sid, int type) {
  char* hid = (char*) malloc(8);
  char* sensorid = (char*) malloc(8);
  char* updateStatus = (char*) malloc(512);
  char* udpM = (char*) malloc(512);
  for (int x = 0; x < 8; x++) {
    sprintf(hid, "%s%02X\0", hid, id[x]);
    sprintf(sensorid, "%s%02X\0", sensorid, sid[x]);
  }
  if(value == 0) {
    strcpy(updateStatus, "TEMP\0");
    sprintf(udpM, "SHMS/1.0 %s UPDATE %s %s %2.1f %i\0", hid, sensorid, updateStatus, v, type);
  } else if (value == 1) {
    strcpy(updateStatus, "PULSE\0");
    sprintf(udpM, "SHMS/1.0 %s UPDATE %s %s %i %i\0", hid, sensorid, updateStatus, (int)v, type);
  } else if (value == 2) {
    strcpy(updateStatus, "TEMP\0");
    sprintf(udpM, "SHMS/1.0 %s WARNING %s %s %i\0", hid, sensorid, updateStatus, type);
  } else if (value == 3) {
    strcpy(updateStatus, "PULSE\0");
    sprintf(udpM, "SHMS/1.0 %s WARNING %s %s %i\0", hid, sensorid, updateStatus, type);
  }
  if (inTunnel) {
    //printf("%s\n\n\n", udpM);
    sendUDPMessage(udpM);
  }
}
