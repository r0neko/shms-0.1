#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <huaweimodem.h>
#include <uri.h>
#include <ctype.h>

#define NODEBUG

int voiceEnabled = 0;
int voiceDisabled = 1;
int ready = 0;
int error = 1;

int voiceRate = 8000;
int bitDepth = 16;
int framePeriod = 20;
int voiceStatus = 0;
char* modemManufacturer;
char* hardwareRevision;
int rssiRaw;
int pinVerification;

int fd;
int fdAudio;
FILE *rawAudio;
int endoffile = 0;
int callendStop = 0;
int isRinging = 1;
pthread_t voicenotify;
int nomodem = 0;

pcm_data_t audio;

int set_interface_attribs (int fd, int speed, int parity) {
  struct termios tty;
  memset (&tty, 0, sizeof tty);
  if (tcgetattr (fd, &tty) != 0)
  {
    printf("error %d from tcgetattr\n", errno);
    return -1;
  }
  cfsetospeed (&tty, speed);
  cfsetispeed (&tty, speed);
  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
  // disable IGNBRK for mismatched speed tests; otherwise receive break
  // as \000 chars
  tty.c_iflag &= ~IGNBRK;         // disable break processing
  tty.c_lflag = 0;                // no signaling chars, no echo,
  // no canonical processing
  tty.c_oflag = 0;                // no remapping, no delays
  tty.c_cc[VMIN]  = 0;            // read doesn't block
  tty.c_cc[VTIME] = 0;            // 0.5 seconds read timeout

  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
  tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
  tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
  tty.c_cflag |= parity;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CRTSCTS;

  if (tcsetattr (fd, TCSANOW, &tty) != 0)
  {
    printf("error %d from tcsetattr\n", errno);
    return -1;
  }
  if (tcsetattr (fdAudio, TCSANOW, &tty) != 0)
  {
    printf("error %d from tcsetattr\n", errno);
    return -1;
  }
  return 0;
}

void set_blocking (int fd, int should_block) {
  struct termios tty;
  memset (&tty, 0, sizeof tty);
  if (tcgetattr (fd, &tty) != 0)
  {
    printf("error %d from tggetattr\n", errno);
    return;
  }

  tty.c_cc[VMIN]  = should_block ? 1 : 0;
  tty.c_cc[VTIME] = 0;

  if (tcsetattr (fd, TCSANOW, &tty) != 0) {
    printf("error %d setting term attributes\n", errno);
  }
  if (tcsetattr (fdAudio, TCSANOW, &tty) != 0) {
    printf("error %d setting audio term attributes\n", errno);
  }
}

char *rs(char *str, char *orig, char *rep)
{
  static char buffer[4096];
  char *p;

  if (!(p = strstr(str, orig))) // Is 'orig' even in 'str'?
    return str;

  strncpy(buffer, str, p - str); // Copy characters from 'str' start to 'orig' st$
  buffer[p - str] = '\0';

  sprintf(buffer + (p - str), "%s%s", rep, p + strlen(orig));

  return buffer;
}

void str_append(char* s, char c)
{
        int len = strlen(s);
        s[len] = c;
        s[len+1] = '\0';
}

char* readPCUI(int removeOK = 0) {
  char* buf = (char*) malloc(256);
  read(fd, buf, 256);
  #ifdef DEBUG
  printf("[huaweimodem serialRead(int)] Received '%s' from Modem\n", buf);
  #endif
  char* answ = (char*) malloc(256);
  int c = 0;  // block read of newlines to make the response prettier
  for (int z = 0; z < strlen(buf); z++) {  // AT response prettyfier
	if (buf[z] == '\n' && !c) { // is the char a new line?
		c = 1;  // block read of the next nl char
		str_append(answ, '\\');
		str_append(answ, 'n');
	} else if (buf[z] == '\r' && !c) {  // if the char a carriage return
		c = 1;  // block read of the next CR char
		str_append(answ, '\\');
		str_append(answ, 'n');
	} else {  // else if a real char
		if (buf[z] != '\n' && buf[z] != '\r') {  // if (the char is not a newline, nor a carriage return)
			str_append(answ, buf[z]); // print the char
		}
		if (c) { // if the block IS active
			c = 0; // disable it
		}
	}
  }
  answ = rs(answ, "ERROR\\n", "ERROR");
  answ = rs(answ, "OK\\n", "OK");

  if (removeOK) {
      answ = rs(answ, "OK", "");
      answ = rs(answ, "\\n\\nERROR", "");
      answ = rs(answ, "\\n\\nOK", "");
  }
  return answ;
}

char* readPCUIBlock(int rO) {
  char* buf = (char*) malloc(256);
  while (strlen(buf) < 10) {
    strcpy(buf, readPCUI(rO));
  }
  #ifdef DEBUG
  printf("[huaweimodem readPCUIBlock] Received '%s'", buf);
  #endif
  return buf;
}

void writePCUI(char *message, int flushSerial) {
  char messageF[strlen(message) + 3];  // We define a variable where we keep the message to send, plus carriage return and new line for the modem
  sprintf(messageF, "%s\r\n", message);  // dirty hack, but it will work for this...
  unsigned char msg[strlen(message) + 2]; // We define a variabile where to hold the message to send to the modem
  strncpy((char *)msg, messageF, strlen(messageF));  // copy the message formatted into unsigned char format
  #ifdef DEBUG
  printf("[huaweimodem serialWrite(char*, int)] Sending '%s' to Modem...\n", messageF);
  #endif
  write (fd, msg, strlen(messageF));
  #ifdef DEBUG
  printf("[huaweimodem serialWrite(char*, int)] Sent '%s' to Modem\n", messageF);
  #endif
  usleep(((strlen(messageF) + 2) + 800) * 100);
  if (flushSerial) {
    tcflush(fd, TCIOFLUSH);
  }
}

void dwritePCUI(char *message, int flushSerial) {
  unsigned char msg[strlen(message) + 1]; // We define a variabile where to hold the message to send to the modem
  strcpy((char *)msg, message);  // copy the message formatted into unsigned char format
  #ifdef DEBUG
  printf("[huaweimodem serialWrite(char*, int)] Sending '%s' to Modem...\n", msg);
  #endif
  write (fd, msg, strlen(message));
  #ifdef DEBUG
  printf("[huaweimodem serialWrite(char*, int)] Sent '%s' to Modem\n", msg);
  #endif
  usleep(((strlen(message) + 2) + 800) * 100);
  if (flushSerial) {
    tcflush(fd, TCIOFLUSH);
  }
}

void writeAudioIO(char** audioF, int size) {
  write (fdAudio, audioF, 320);
  usleep(framePeriod * 10000);
}

void* voiceWarnThread(void *args) {
  pcm_data_t* audio = (pcm_data_t*) args;
  int l = audio->length;
  int n = 0, offset = 0, i = 0;
  for(i = 0; i <= l; i += FRAME_SIZE) {
	if (callendStop) {
		break;
	}
	n = write(fdAudio, &audio->data[offset], FRAME_SIZE);
	if (n < FRAME_SIZE) {
		printf("%s: wrote only %d of %d bytes\n", __FUNCTION__, n, FRAME_SIZE);
	}
	offset += FRAME_SIZE;
	usleep(1000 * framePeriod);
  }
  if (!callendStop) {
	sleep(1);
  	writePCUI("AT+CHUP", 1);
  	endoffile = 1;
  }
  callendStop = 0;
}

int dial(char *phone) {
  if (!nomodem) {
    endoffile = 0;
    callendStop = 0;
    writePCUI("AT", 1);
    char dialCMD[20] = {0};
    sprintf(dialCMD, "ATD%s;", phone);
    char* callStat = (char*) malloc(256);
    writePCUI(dialCMD, 0);
    char* resp = (char*) malloc(10);
    strcpy(resp, readPCUIBlock(1));
    printf("%s\n", resp);
    char *cst = strtok(resp, "\\n");
    cst = strtok(cst, ":");
    char* cmd = cst;
    cst = strtok(cst, ":");
    int isResponse = 0;
    if (strncmp(cst, "^ORIG", 5) != 0) {
	printf("Looks like was an error... Let's resend the call command...\n");
	writePCUI(dialCMD, 0);
    	char* resp = (char*) malloc(10);
    	strcpy(resp, readPCUIBlock(1));
    	printf("%s\n", resp);
    	cst = strtok(resp, "\\n");
    	cst = strtok(cst, ":");
	if (strcmp(cst, "^ORIG") == 0) {
		isResponse = 1;
	} else {
		printf("Failed to dial!\n");
		return 1;
	}
    }
    printf("CallResponse='%s'\n", cmd);
    if (strcmp(cmd, "^ORIG") == 0 || isResponse) {
      usleep(500);
      writePCUI("AT^DDSETEX=2", 1);
      printf("[huaweimodem.c dial(char*)] Started Calling...\n");
      isRinging = 1;
      while (!endoffile) {
        callStat = readPCUIBlock(1);
        if (strlen(callStat) > 5) {
          char *comm;
          comm = strtok(callStat, "\\n");
	  comm = strtok(comm, ":");
          if (strcmp(comm, "^CONF") == 0) {
            printf("[huaweimodem.c dial(char*)] Ringing...\n");
          }
          else if (strcmp(comm, "^CEND") == 0) {
            if (isRinging) {
              printf("[huaweimodem.c dial(char*)] Call Rejected!\n");
              break;
            } else {
              printf("[huaweimodem.c dial(char*)] Call is ended.\n");
              callendStop = 1;
              break;
            }
          }
          else if (strcmp(comm, "^CONN") == 0) {
            sleep(1);
            isRinging = 0;
            printf("[huaweimodem.c dial(char*)] Call answered. Creating thread to play 16-Bit WAV RAW PCM...\n");
            int err = pthread_create(&voicenotify, NULL, &voiceWarnThread, (void *)&audio);
            if (err != 0) {
              printf("\n[huaweimodem.c dial(char*)] can't create voice thread :[%s]", strerror(err));
            } else {
              printf("\n[huaweimodem.c dial(char*)] Voice thread created successfully\n");
            }
          }
        }
      }
    }
    if (isRinging) {
	return 1; // call rejected
    }
    return 0;
  } else {
	printf("[huaweimodem.c dial(char*)] Avoiding phone call, because there is NO modem detected!\n");
  }
  return 1;
}

void sendSMS(char* text, char* phoneNumber) {
	if(!nomodem) {
		writePCUI("AT+CMGF=1", 1);
		char* msg = (char*) malloc(20);
		sprintf(msg, "AT+CMGS=\"%s\"", phoneNumber);
		writePCUI(msg, 0);
		dwritePCUI(text, 0);
		writePCUI("\x1A\0", 0); // ctrl-z sends sms
		char* response = readPCUIBlock(1); // wait until message sent
		if (strstr(response, "+CME ERROR") != NULL) {
			char* errCode = strtok(response, ":");
			errCode = strtok(NULL, ": ");
			printf("[ERROR] Received error code %s from modem!\n", errCode);
		}
	}
}

int simStatus() {
	if (!nomodem) {
		writePCUI("AT+CPIN?", 0);
		char* simStatus = strtok(readPCUI(1), "\\n");
		if (strstr(simStatus, "+CME ERROR") != NULL) {
			char* errCode = strtok(simStatus, ":");
			errCode = strtok(NULL, ": ");
			if (strcmp(errCode, "13") == 0) {
				printf("(U)SIM Failure. (U)SIM May be missing.\n");
				return 1;
			}
		} else {
			char* err = strtok(simStatus, ":");
			err = strtok(NULL, ":");
			if (strcmp(err, " SIM PIN") == 0) {
				printf("PIN NEEDED\n");
				return 2;
			} else {
				return 0;
			}
		}
	} else {
		printf("MODEM not detected!");
		return 1;
	}
}

char* imsi() {
	if (!nomodem) {
		writePCUI("AT+CIMI", 0);
		char* imsi = strtok(readPCUI(1), "\\n");
		if (strstr(imsi, "+CME ERROR") == NULL) {
			 return imsi;
		}
		if (strstr(imsi, "+CME ERROR") != NULL) {
			char* errCode = strtok(imsi, ":");
			errCode = strtok(NULL, ": ");
			if (strcmp(errCode, "13") == 0) {
				printf("(U)SIM Failure. (U)SIM May be missing.\n");
				return "000000000000000000";
			}
		}
	} else {
		return "0000000000000000";
	}
}

char* imei() {
	if (!nomodem) {
		writePCUI("AT+CGSN", 0);
		char* imei = strtok(readPCUI(1), "\\n");
		if (strstr(imei, "+CME ERROR") == NULL) {
			 return imei;
		}
		if (strstr(imei, "+CME ERROR") != NULL) {
			 return "0000000000000000";
		}
	} else {
		return "0000000000000000";
	}
}

char* manufacturer() {
	if (!nomodem) {
		writePCUI("AT+GMI", 0);
		char* man = strtok(readPCUI(1), "\\n");
		if (strstr(man, "+CME ERROR") == NULL) {
			return man;
		} else 	{
			return "ERRMAN";
		}
	} else {
		return "0000000000000000";
	}
}
	
int get_pcm_frames(char *audio_file, pcm_data_t *audio)
{
	int file_length	 = 0, silent_frame = 0, bytes_read = 0;

	FILE *file = fopen(audio_file, "r");

	if (file == NULL)
	{
		printf("%s: error opening file %s. %s\n", __FUNCTION__, audio_file, strerror(errno));
		return 0;
	}

	fseek(file, 0, SEEK_END);
	file_length = ftell(file);
	rewind(file);

	silent_frame = FRAME_SIZE - (file_length % FRAME_SIZE);
	audio->data = (char*) malloc(file_length + silent_frame);
	audio->length = file_length + silent_frame;

	if ((bytes_read	= fread(audio->data, 1, file_length, file)) != file_length)
	{
		printf("%s: couldn't read the whole audio file. %s\n", __FUNCTION__, strerror(errno));
		return 0;
	}

	memset(&audio->data[file_length], '\0', silent_frame);

	return 1;
}

int playMessage(pcm_data_t *audio) {
	int n = 0, offset = 0, i = 0;
	for(i = 0; i <= audio->length; i += FRAME_SIZE) {
		n = write(fdAudio, &audio->data[offset], FRAME_SIZE);

		if (n < FRAME_SIZE) {
			printf("%s: wrote only %d of %d bytes\n", __FUNCTION__, n, FRAME_SIZE);
		}

		offset += FRAME_SIZE;
		usleep(1000 * framePeriod);
	}
}

void test() {
	int err = pthread_create(&voicenotify, NULL, &voiceWarnThread, (void *)&audio);
            if (err != 0) {
              printf("\n[huaweimodem.c dial(char*)] can't create voice thread :[%s]", strerror(err));
            } else {
              printf("\n[huaweimodem.c dial(char*)] Voice thread created successfully\n");
            }
}

void downloadTTS(char* text) {
  int socket_desc;
  char messageTTS[strlen(url_encode(text)) + 285];
  char* c = (char*) malloc(256);
  char* soundFile;
  char buf[256];
  char *filename = "tts.wav";
  int total_len = 0;
  int lenfile = 0;
  int i;
  int len;
  FILE *file = NULL;
  struct sockaddr_in server;
  socket_desc = socket(AF_INET , SOCK_STREAM , 0);
  if (socket_desc == -1)
  {
    printf("Could not create socket\n");
  }
  server.sin_addr.s_addr = inet_addr("192.168.2.52"); // Server #1 IP
  server.sin_family = AF_INET;
  server.sin_port = htons( 80 );
  remove(filename);
  file = fopen(filename, "a+b");

  if (file == NULL) {
     printf("File could not opened");
  } else {
     printf("File opened successfully!\n");
  }

  if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0) {
    printf("connect error\n");
  } else {
    printf("connect success\n");
  }
  sprintf(messageTTS, "GET /getTTS.php?text=%s HTTP/1.1\r\nHost: shmsintranet.local\r\nConnection: keep-alive\r\nCache-Control: max-age=0\r\nUpgrade-Insecure-Requests: 1\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/63.0.3239.132 Safari/537.36\r\nAccept: */*;q=0.8\r\n\r\n", url_encode(text));
  if ( send(socket_desc , messageTTS , strlen(messageTTS) , 0) < 0) {
    printf("Send failed\n");
    shutdown(socket_desc, SHUT_RDWR);
    close(socket_desc);
  } else {
    unsigned char buffer[1];
    while (strcmp(buf, "\r\n")) {
      for (i = 0; strcmp(buf + i - 2, "\r\n"); i++) {
        recv(socket_desc, buf + i, 1, 0);
        buf[i + 1] = '\0';
      }
      if (strstr(buf, "HTTP/") == buf) {
        fputs(strchr(buf, ' ') + 1, stdout);
        if (strcmp(strchr(buf, ' ') + 1, "200 OK\r\n")) {
          printf("ERROR: No Exists...\n");
        }
      }
      if (strstr(buf, "Content-Length:") == buf) {
        *strchr(buf, '\r') = '\0';
        lenfile = atoi(strchr(buf, ' ') + 1);
      }
    }
    if (lenfile > 10) {
    	for (int i = 0, k = 0, m = 0; i < lenfile; i++, k++) {
    	  recv(socket_desc, buffer , 1 , 0);
	  fwrite(buffer, 1, 1, file);
       	  if (k == total_len) {
    	    m++;
    	    k = 0;
    	  }
    	}
    } else {
    	printf("ERROR: Empty WAV file");
	exit(1);
    }
    shutdown(socket_desc, SHUT_RDWR);
    close(socket_desc);
    fclose(file);
  }
}

void modemBegin() {
	char *portname = "/dev/ttyUSB2";
  	fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
  	if (fd < 0) {
   	 	printf("error %d opening command port %s: %s\n", errno, portname, strerror (errno));
    		nomodem = 1;
  	}
  	char *portnameA = "/dev/ttyUSB1";
  	fdAudio = open (portnameA, O_RDWR | O_NOCTTY | O_SYNC);
  	if (fdAudio < 0) {
    		printf("error %d opening AUDIO port %s: %s\n", errno, portnameA, strerror (errno));
    		nomodem = 1;
  	}
  	set_interface_attribs (fd, B9600, 0);
	set_interface_attribs (fdAudio, B9600, 0);
  	set_blocking (fd, 1);
	writePCUI("ATE0", 1);
	writePCUI("AT+CMEE=1", 1);
	writePCUI("AT^CURC=0", 1);
	//downloadTTS("Atenție! A fost detectat un incendiu în salonul 3, situat la etajul 2, direcția nord-est. Vă rugăm să interveniți!"); // hardcoded shit
  	get_pcm_frames("/home/pi/shms/shms/tts.wav", &audio);
	//printf("The simcard imsi is: %s\n", imsi()); // debug crap
}
