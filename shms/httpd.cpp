#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <linux/reboot.h>
#include <sys/reboot.h>
#include <base64.h>
#include <pass.h>
#include <cJSON.h>
//#include <tcpclient.h>
#include <hub.h>

#define CONNMAX 1000
#define BYTES 1024
#define HTTPDVERSION "v1.0-ALPHA"
#define DBLOC "/home/pi/shms/shms"
#define PATH "/home/pi/shms/shms"
#define MAXSENSORS 1024
#define DEBUG
//#define AUTHDEBUG 

char *ROOT;
char PORT[6];
FILE *dbFile;
int listenfd, clients[CONNMAX];
char message[100] = {0};
int sensorIndex = 0;
struct sensor {
  unsigned char* id;
  char* formatID;
  int status;
  int type = 0;
  int action = 0;
  int reset = 0;
} sensors[MAXSENSORS]; // up to MAXSENSORS sensors can be registered with this hub
int registerSensor = 0;
int regMin = 0;
int regSec = 0;
int rT = 0;
int runResetter = 0;

void error(char *);
void startServer(char *);
void respond(int accepter);
int addSensor(unsigned char* id, int status, int action);
void setSensor(sensor s);
void *serverLoop(void *acc);
void startHttpServer();
int getRegistration();
void stopRegistration();
void startRegistration();
void *countdown(void *acc);

/* DB Functions */
void importSensors();
void putSensor(sensor s);
void updateSensors();
char* getString(char* configName);
char* getBytes(char* configName);
/* End */


/* Function: startHttpServer
   Description: Start the HTTP Server as a thread to stop disturbing the main app.
*/

void startHttpServer() {
  //startClient();
  printf("SHMS Embedded HTTPD - version %s\n", HTTPDVERSION);
  #ifdef DEBUG
	printf("compiled with DEBUG mode active\n");
  #endif
  printf("\nThe import Sensor function is SOOOOOOOOO BUGGYYYYYYYY and breaks the modem!!!\n");
  printf("This issue will be fixed in the next versions of the SHMS HTTPD SERVER.\n");
  printf("Until then, PLEASE don't compile me again with the HTTPD server!\nThanks for understanding!\n");
  exit(1);
  importSensors();
  pthread_t thread;
  if (!pthread_create(&thread, NULL, &serverLoop, NULL)) {
     #ifdef DEBUG
     printf("Server thread started.\n");
     #endif
  } else {
     #ifdef DEBUG
     printf("Error starting server thread!\n");
     #endif
  }
}

/* Function: getRegistration
   Description: Returns the registration for the main app.
*/

int getRegistration() {
	return registerSensor;
}

void stopRegistration() {
	registerSensor = 0;
}

void startRegistration() {
	registerSensor = 1;
	regMin = 1;// 01 minutes
	regSec = 0;// 00 seconds
	// these are the start points
	pthread_t threadReg;
  	if (!pthread_create(&threadReg, NULL, &countdown, NULL)) {
	     #ifdef DEBUG
     		printf("Register thread started.\n");
	     #endif
  	} else {
	     #ifdef DEBUG
     		printf("Error starting register thread!\n");
	     #endif
  	}
}

void *serverLoop(void *acc) {
  struct sockaddr_in clientaddr;
  socklen_t addrlen;
  char c;

  ROOT = PATH;
  strcpy(PORT, "80");

  startServer(PORT);
  #ifdef DEBUG
  printf("Embedded C++ HTTPD started at port %s%s%s with root directory as %s%s%s\n", "\033[92m", PORT, "\033[0m", "\033[92m", ROOT, "\033[0m");
  #endif

  // ACCEPT connections
  while (1)
  {
    addrlen = sizeof(clientaddr);
    int connectionAccepted = accept(listenfd, (struct sockaddr *) &clientaddr, &addrlen);

    if (connectionAccepted < 0) {
      #ifdef DEBUG
      printf("accept() error\n");
      #endif
    } else {
      respond(connectionAccepted);
    }
  }
}

void *countdown(void *acc) {
	while (true) {
		if (regSec == 0 && regMin != 0) {
			regMin -= 1;
			regSec = 59;
		} else if (regMin == 0 && regSec == 0) {
			stopRegistration();
			break;
		}
		sleep(1);
		regSec -= 1;
	}
}

//start server
void startServer(char *port)
{
  struct addrinfo hints, *res, *p;

  // getaddrinfo for host
  memset (&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if (getaddrinfo( NULL, port, &hints, &res) != 0)
  {
    perror ("getaddrinfo() error");
    exit(1);
  }
  // socket and bind
  for (p = res; p != NULL; p = p->ai_next)
  {
    listenfd = socket (p->ai_family, p->ai_socktype, 0);
    if (listenfd == -1) continue;
    if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
  }
  if (p == NULL)
  {
    perror ("socket() or bind()");
    exit(1);
  }

  freeaddrinfo(res);

  // listen for incoming connections
  if ( listen (listenfd, 1000000) != 0 )
  {
    perror("listen() error");
    exit(1);
  }
}

char *replace_str(char *str, char *orig, char *rep)
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

char *statsRep(char* string) {
  char* tmp = replace_str(string, "{HTTPDVERSION}", HTTPDVERSION);
  tmp = replace_str(tmp, "{PORT}", PORT);
  return tmp;
}

int addSensor(unsigned char* id, int status, int action, int type) {
	int foundID = 0;
	for (int sI = 0; sI < sensorIndex; sI++) {
		for (int iI = 0; iI < 8; iI++) {
			if (sensors[sI].id[iI] == id[iI]) {
				foundID = 1;
			} else {
				foundID = 0;
			}
		}
	}
	if (!foundID) {
		sensors[sensorIndex].id = id;
		sensors[sensorIndex].status = status;
		sensors[sensorIndex].action = action;
		sensors[sensorIndex].type = type;
		#ifdef DEBUG
		printf("Added sensor #%i!\n", sensorIndex);
		#endif
		sensorIndex++;
		updateSensors();
		stopRegistration();
		return 1;
	} else {
		return 0;
	}
}

void setSensor(unsigned char* id, int status, int action) {
	for(int sensorI = 0; sensorI < sensorIndex; sensorI++) {
		int found = 0;
		for(int sIndex = 0; sIndex < 8; sIndex++) {
			if (sensors[sensorI].id[sIndex] == id[sIndex]) {
				found = 1;
			} else {
				found = 0;	
			}
		}
		if (found) {
			#ifdef DEBUG
				printf("found the sensor. setting...\n");
			#endif
			sensors[sensorI].status = status;
			sensors[sensorI].action = action;
			sprintf(message, "TRIGGER %i %02X%02X%02X%02X%02X%02X%02X%02X %02X%02X%02X%02X%02X%02X%02X%02X", sensors[sensorI].type, hubMainID[0], hubMainID[1], hubMainID[2], hubMainID[3], hubMainID[4], hubMainID[5], hubMainID[6], hubMainID[7], id[0], id[1], id[2], id[3], id[4], id[5], id[6], id[7]);
			if (status == 3 && action == 3) {
				sprintf(message, "TEMPTRIGGER %i %02X%02X%02X%02X%02X%02X%02X%02X %02X%02X%02X%02X%02X%02X%02X%02X", sensors[sensorI].type, hubMainID[0], hubMainID[1], hubMainID[2], hubMainID[3], hubMainID[4], hubMainID[5], hubMainID[6], hubMainID[7], id[0], id[1], id[2], id[3], id[4], id[5], id[6], id[7]);
			}
			//sendMessage(message);
			break;
		}
	}
}

char *getR(char* value, char* mesg) {
	char *part = 0;
	part = strtok (mesg, " \t\n");
	part = strtok (NULL, " \t");
	int read = 0;
	int index = 0;
	char* getData = (char*) malloc(256);
	char* f = (char*) malloc(256);
	int i = 0;
	int s = 0;
	for(int word = 0; word < strlen(part); word++) {
		if (read) {
			getData[index] = part[word];
			getData[index+1] = '\0';
			index++;
		}
		if (part[word] == '?') {
			read = 1;
		}
	}
	for(int l = 0; l < strlen(getData); l++) {
		if (getData[l] == '%') {
			if (getData[l+1] == '2' && getData[l+2] == '0') {
				s = l + 3;
				f[i] = ' ';
				i++;
			} else if (getData[l+1] == '3' && getData[l+2] == 'C') {
				s = l + 3;
				f[i] = '<';
				i++;
			} else if (getData[l+1] == '3' && getData[l+2] == 'E') {
				s = l + 3;
				f[i] = '>';
				i++;
			} else {
				f[i] = getData[l];
				i++;
			}
		} else {
			if (l >= s) {
				f[i] = getData[l];
				i++;
			}
		}
	}
	getData[strlen(getData)] = '\0';
	strncpy(getData, f, strlen(f)); // add new formatted data
	if (!read) {
		// not found
		// it may be in the response
		char* c = strtok (NULL, " \t\n\t\n");
		while (c != NULL) {
			c = strtok (NULL, " \t\n\t\n");
			if (strlen(c) == 1) {
				break;
			}
		}
		c = strtok (NULL, " \t\n\t\n");
		if (c != NULL) {
			char* f = (char*) malloc(256);
			int i = 0;
			int s = 0;
			for(int l = 0; l < strlen(c); l++) {
				if (c[l] == '+') {
					f[i] = ' ';
					i++;
				} else {
					f[i] = c[l];
					i++;
				}
			}
			strncpy(getData, f, strlen(f));
		}
	}
	char* result = strtok(getData, "&");
	char* r = (char*) malloc(256);
	int found = 0;
	while (result != NULL) {
		if (strncmp(strtok(result, "="), value, strlen(value)) == 0) {
			char* val = strtok(NULL, "=");
			strncpy(r, val, strlen(val));
			found = 1;
			break;
		}
		result = strtok(NULL, "&");
	}
	if (found) {
		return r;
	} else {
		return "none";
	}
}

// client connection
void respond(int accepter)
{
  int loginGood = 0;
  char mesg[4096], *reqline[3], data_to_send[BYTES], path[4096];
  int rcvd, fd, bytes_read;
  char req[4096];

  memset( (void*)mesg, (int)'\0', 4096 );

  rcvd = recv(accepter, mesg, 4096, 0);

  if (rcvd < 0) {
    fprintf(stderr, ("recv() error\n"));
  } else if (rcvd == 0) {
    fprintf(stderr, "Client disconnected.\n");
  } else {
    strncpy(req, mesg, sizeof(mesg));
    #ifdef DEBUG
	printf("[%s]\n", mesg);
    #endif
    reqline[0] = strtok (mesg, " \t\n");
    if ( strncmp(reqline[0], "GET\0", 4) == 0 | strncmp(reqline[0], "POST\0", 5) == 0 )
    {
      reqline[1] = strtok (NULL, " \t");
      reqline[2] = strtok (NULL, " \t\n");
      if ( strncmp( reqline[2], "HTTP/1.0", 8) != 0 && strncmp( reqline[2], "HTTP/1.1", 8) != 0 )
      {
        write(accepter, "HTTP/1.0 400 Bad Request\n", 25);
      }
      else
      {
	char* get = reqline[1];
	char filtered[256];
	for (int c = 0; c < strlen(get); c++) {
		if (get[c] != '?' && get[c] != '=' && get[c] != '&') {
			filtered[c] = get[c];
			filtered[c+1] = '\0';
		} else {
			break;
		}
	}
        if ( strncmp(filtered, "/\0", 2) == 0 ) {
          strncpy(filtered, "/index.html", 11);
        }
        char* login;
        int found = 0;
        char *data;
        int x = 0;
        while (data != NULL) {
          data = strtok (NULL, " \t\n");
          if (data == NULL) {
            break;
          } else {
            if (strncmp (data, "Authorization:", 14) == 0) {
              found = 1;
              break;
            }
            x++;
          }
        }
        if (!found) {
          #ifdef AUTHDEBUG
	  printf("User must authentificate!\n");
	  #endif
          send(accepter, statsRep("HTTP/1.1 401 Access Denied\nWWW-Authenticate: Basic realm=\"SHMS Embedded C++ HTTPD {HTTPDVERSION}\"\n\n"), 256, 0);
        } else {
          if ( strncmp(strtok (NULL, " \t\n"), "Basic", 2) == 0 ) {
	    #ifdef AUTHDEBUG
            printf("Basic auth...\n");
	    #endif
            login = strtok (NULL, " \t\n");
            unsigned char loginD[256] = {0};
            base64_decode(login, strlen(login), loginD);
            char* user = strtok((char*)loginD, ":");
            char* pass = strtok(NULL, ":");
            if (userExists(user)) {
   	      #ifdef AUTHDEBUG
              printf("User exists... checking password...\n");
	      #endif
              if (pass != NULL) {
                if (!passForUserCorrect(user, pass)) {
	          #ifdef AUTHDEBUG
		  printf("Password Incorrect!\n");
	          #endif
                  send(accepter, statsRep("HTTP/1.1 401 Access Denied\nWWW-Authenticate: Basic realm=\"SHMS Embedded C++ HTTPD {HTTPDVERSION}\"\n\n"), 256, 0);
                } else {
                  loginGood = 1;
		  #ifdef AUTHDEBUG
		  printf("Login Correct!\n");
	          #endif
                }
              } else {
	        #ifdef DEBUG
		printf("No Password specified!\n");
	        #endif
		send(accepter, statsRep("HTTP/1.1 401 Access Denied\nWWW-Authenticate: Basic realm=\"SHMS Embedded C++ HTTPD {HTTPDVERSION}\"\n\n"), 256, 0);
	      }
            } else {
	      #ifdef DEBUG
	      printf("User does not exist!\n");
	      #endif
              send(accepter, statsRep("HTTP/1.1 401 Access Denied\nWWW-Authenticate: Basic realm=\"SHMS Embedded C++ HTTPD {HTTPDVERSION}\"\n\n"), 256, 0);
            }
          } else {
            send(accepter, statsRep("HTTP/1.1 401 Access Denied\nWWW-Authenticate: Basic realm=\"SHMS Embedded C++ HTTPD {HTTPDVERSION}\"\n\n"), 256, 0);
          }
          if (loginGood) {
            if (strncmp(filtered, "/shutdown", 9) == 0) {
              printf("Shutdown in progress...\n");
              sync();
              send(accepter, "HTTP/1.0 200 OK\n", 16, 0);
              shutdown (accepter, SHUT_RDWR);         //All further send and recieve operations are DISABLED...
              close(accepter);
              reboot(LINUX_REBOOT_CMD_POWER_OFF);
            } else if (strncmp(filtered, "/reboot", 7) == 0) {
              printf("Shutdown in progress...\n");
              sync();
              send(accepter, "HTTP/1.0 200 OK\n", 16, 0);
              shutdown (accepter, SHUT_RDWR);         //All further send and recieve operations are DISABLED...
              close(accepter);
              reboot(LINUX_REBOOT_CMD_RESTART);
            } else if (strncmp(filtered, "/setConfig", 10) == 0) {
	      	send(accepter, "HTTP/1.0 200 OK\n\n", 17, 0);
	      	char* config = getR("config", req);
	      	printf("Config: %s\n", config);
	    } else if (strncmp(filtered, "/sensorData", 11) == 0) {
	      	cJSON *root = cJSON_CreateObject();
		cJSON *sensorsJ = NULL;
		cJSON *sensor = NULL;
		cJSON *sensorT = NULL;
		cJSON_AddItemToObject(root, "sensors", sensorsJ = cJSON_CreateObject());
		cJSON_AddNumberToObject(sensorsJ, "registeredSensors", sensorIndex);
		cJSON_AddItemToObject(sensorsJ, "sensor", sensor = cJSON_CreateArray());
		for(int sensorI = 0; sensorI < sensorIndex; sensorI++) {
			cJSON_AddItemToArray(sensor, sensorT = cJSON_CreateObject());
			char idS[16] = {0};
			for (int ls = 0; ls < 8; ls++) {
				sprintf(idS, "%s%02hhX", idS, sensors[sensorI].id[ls]);
			}
			cJSON_AddStringToObject(sensorT, "id", idS);
			cJSON_AddNumberToObject(sensorT, "type", sensors[sensorI].type);
			cJSON_AddNumberToObject(sensorT, "action", sensors[sensorI].action);
			cJSON_AddNumberToObject(sensorT, "status", sensors[sensorI].status);
		}
		unsigned char* json = (unsigned char*) cJSON_Print(root);
		char base[2 * sizeof(json)];
		base64_encode(json, strlen((char*)json), base);
		send(accepter, "HTTP/1.0 200 OK\n\n", 17, 0);
		write(accepter, base, strlen(base));
	    } else if (strncmp(filtered, "/register", 9) == 0) {
	      	send(accepter, "HTTP/1.0 200 OK\n\n", 17, 0);
		if (strncmp(reqline[0], "GET\0", 4) == 0) {
			cJSON *root = cJSON_CreateObject();
			cJSON *reg = NULL;
			cJSON_AddItemToObject(root, "register", reg = cJSON_CreateObject());
			cJSON_AddNumberToObject(reg, "timeLeftMinutes", regMin);
			cJSON_AddNumberToObject(reg, "timeLeftSeconds", regSec);
			if (registerSensor) {
				cJSON_AddTrueToObject(reg, "isRegistering");
			} else {
				cJSON_AddFalseToObject(reg, "isRegistering");
			}
			unsigned char* json = (unsigned char*) cJSON_Print(root);
			char base[2 * sizeof(json)];
			base64_encode(json, strlen((char*)json), base);
			write(accepter, base, strlen(base));
		} else if (strncmp(reqline[0], "POST\0", 5) == 0) {
			char* abort = getR("abort", req);
			if (strncmp(abort, "1", 1) == 0) {
				stopRegistration();
			} else {
				startRegistration();
			}
		}
	    } else {
              strcpy(path, ROOT);
              strcpy(&path[strlen(ROOT)], filtered);
              printf("file: %s\n", path);
              if ( (fd = open(path, O_RDONLY)) != -1 ) {
                send(accepter, "HTTP/1.0 200 OK\n\n", 17, 0);
                while ( (bytes_read = read(fd, data_to_send, BYTES)) > 0 ) {
                  write (accepter, statsRep(data_to_send), bytes_read);
                }
              } else {
                write(accepter, statsRep("HTTP/1.0 404 Not Found\n\n<html><head></head><body><h1>404 Not Found</h1><i>SHMS Embedded C++ HTTPD Server - version {HTTPDVERSION} running on port {PORT}</i></body></html>\n"), 155);
              }
            }
          }
        }
      }
    }
  }

  //Closing SOCKET
  shutdown (accepter, SHUT_RDWR);         //All further send and recieve operations are DISABLED...
  close(accepter);
}


/* STARTING DB STUFF */

void putSensor(sensor s) {
	char location[strlen(DBLOC) + 9];
	strcpy(location, DBLOC);
	strcat(location, "/shmsS.db");
	FILE* dbFile = fopen(location, "wb");
	int d = 3;
	unsigned char tst[4096] = {0x01, 0x10, 0x15}; // header
	for(int z = 0; z < sensorIndex; z++) {
		d++;
		tst[d] = 0x02; // start db name
		d++;
		tst[d] = 0x63; // db name
		d++;
		tst[d] = 0x32; // db name
		d++;
		tst[d] = 0x56; // db name
		d++;
		tst[d] = 0x75; // db name
		d++;
		tst[d] = 0x63; // db name
		d++;
		tst[d] = 0x32; // db name
		d++;
		tst[d] = 0x39; // db name
		d++;
		tst[d] = 0x79; // db name
		d++;
		tst[d] = 0x53; // db name
		d++;
		tst[d] = 0x55; // db name
		d++;
		tst[d] = 0x51; // db name
		d++;
		tst[d] = 0x3D; // db name
		d++;
		tst[d] = 0x03; // end db name
		d++;
		tst[d] = 0x06; // start unsigned char value
		d++;
		for(int i = 0; i < 8; i++) {
			tst[d] = sensors[z].id[i]; // unsigned char value
			d++;
		}
		tst[d] = 0x07; // end unsigned char value
		d++;
		tst[d] = 0x02; // again a new db name
		d++;
		tst[d] = 0x63; // db name
		d++;
		tst[d] = 0x32; // db name
		d++;
		tst[d] = 0x56; // db name
		d++;
		tst[d] = 0x75; // db name
		d++;
		tst[d] = 0x63; // db name
		d++;
		tst[d] = 0x32; // db name
		d++;
		tst[d] = 0x39; // db name
		d++;
		tst[d] = 0x79; // db name
		d++;
		tst[d] = 0x56; // db name
		d++;
		tst[d] = 0x48; // db name
		d++;
		tst[d] = 0x6C; // db name
		d++;
		tst[d] = 0x77; // db name
		d++;
		tst[d] = 0x5A; // db name
		d++;
		tst[d] = 0x51; // db name
		d++;
		tst[d] = 0x3D; // db name
		d++;
		tst[d] = 0x3D; // db name
		d++;
		tst[d] = 0x03; // end db name
		d++;
		tst[d] = 0x06; // again a new unsigned char value
		d++;
		if (sensors[z].type == 0) {
			tst[d] = 0x0b;
		} else if (sensors[z].type == 1) {
			tst[d] = 0x0c;
		} else if (sensors[z].type == 2) {
			tst[d] = 0x0d;
		} else if (sensors[z].type == 3) {
			tst[d] = 0x1a;
		}
		d++;
		tst[d] = 0x07; // end unsigned char value
		d++;
	}
	tst[d] = 0x02; // new db name, but now we start adding our new sensor
	d++;
	tst[d] = 0x63; // db name
	d++;
	tst[d] = 0x32; // db name
	d++;
	tst[d] = 0x56; // db name
	d++;
	tst[d] = 0x75; // db name
	d++;
	tst[d] = 0x63; // db name
	d++;
	tst[d] = 0x32; // db name
	d++;
	tst[d] = 0x39; // db name
	d++;
	tst[d] = 0x79; // db name
	d++;
	tst[d] = 0x53; // db name
	d++;
	tst[d] = 0x55; // db name
	d++;
	tst[d] = 0x51; // db name
	d++;
	tst[d] = 0x3D; // db name
	d++;
	tst[d] = 0x03;
	d++;
	tst[d] = 0x06;
	d++;
	for(int i = 0; i < 8; i++) {
		tst[d] = s.id[i];
		d++;
	}
	tst[d] = 0x07;
	d++;
	tst[d] = 0x02;
	d++;
	tst[d] = 0x63; // db name
	d++;
	tst[d] = 0x32; // db name
	d++;
	tst[d] = 0x56; // db name
	d++;
	tst[d] = 0x75; // db name
	d++;
	tst[d] = 0x63; // db name
	d++;
	tst[d] = 0x32; // db name
	d++;
	tst[d] = 0x39; // db name
	d++;
	tst[d] = 0x79; // db name
	d++;
	tst[d] = 0x56; // db name
	d++;
	tst[d] = 0x48; // db name
	d++;
	tst[d] = 0x6C; // db name
	d++;
	tst[d] = 0x77; // db name
	d++;
	tst[d] = 0x5A; // db name
	d++;
	tst[d] = 0x51; // db name
	d++;
	tst[d] = 0x3D; // db name
	d++;
	tst[d] = 0x3D; // db name
	d++;
	tst[d] = 0x03;
	d++;
	tst[d] = 0x06;
	d++;
	if (s.type == 0) {
		tst[d] = 0x0b;
	} else if (s.type == 1) {
		tst[d] = 0x0c;
	} else if (s.type == 2) {
		tst[d] = 0x0d;
	}
	d++;
	tst[d] = 0x07;
	d++;
	tst[d] = 0x13;
	d++;
	tst[d] = 0x00;
	d++;
	tst[d] = 0x12;
	d++;
	fwrite(&tst, d, 1, dbFile);
	fclose(dbFile);
}

void updateSensors() {
	char location[strlen(DBLOC) + 9];
	strcpy(location, DBLOC);
	strcat(location, "/shmsS.db");
	FILE* dbFile = fopen(location, "wb");
	int d = 3;
	unsigned char tst[8192] = {0x01, 0x10, 0x15}; // header
	for(int z = 0; z < sensorIndex; z++) {
		if (!sensors[z].reset) {
			d++;
			tst[d] = 0x02; // start db name
			d++;
			tst[d] = 0x63; // db name
			d++;
			tst[d] = 0x32; // db name
			d++;
			tst[d] = 0x56; // db name
			d++;
			tst[d] = 0x75; // db name
			d++;
			tst[d] = 0x63; // db name
			d++;
			tst[d] = 0x32; // db name
			d++;
			tst[d] = 0x39; // db name
			d++;
			tst[d] = 0x79; // db name
			d++;
			tst[d] = 0x53; // db name
			d++;
			tst[d] = 0x55; // db name
			d++;
			tst[d] = 0x51; // db name
			d++;
			tst[d] = 0x3D; // db name
			d++;
			tst[d] = 0x03; // end db name
			d++;
			tst[d] = 0x06; // start unsigned char value
			d++;
			for(int i = 0; i < 8; i++) {
				tst[d] = sensors[z].id[i]; // unsigned char value
				d++;
			}
			tst[d] = 0x07; // end unsigned char value
			d++;
			tst[d] = 0x02; // again a new db name
			d++;
			tst[d] = 0x63; // db name
			d++;
			tst[d] = 0x32; // db name
			d++;
			tst[d] = 0x56; // db name
			d++;
			tst[d] = 0x75; // db name
			d++;
			tst[d] = 0x63; // db name
			d++;
			tst[d] = 0x32; // db name
			d++;
			tst[d] = 0x39; // db name
			d++;
			tst[d] = 0x79; // db name
			d++;
			tst[d] = 0x56; // db name
			d++;
			tst[d] = 0x48; // db name
			d++;
			tst[d] = 0x6C; // db name
			d++;
			tst[d] = 0x77; // db name
			d++;
			tst[d] = 0x5A; // db name
			d++;
			tst[d] = 0x51; // db name
			d++;
			tst[d] = 0x3D; // db name
			d++;
			tst[d] = 0x3D; // db name
			d++;
			tst[d] = 0x03; // end db name
			d++;
			tst[d] = 0x06; // again a new unsigned char value
			d++;
			if (sensors[z].type == 0) {
				tst[d] = 0x0b;
			} else if (sensors[z].type == 1) {
				tst[d] = 0x0c;
			} else if (sensors[z].type == 2) {
				tst[d] = 0x0d;
			} else if (sensors[z].type == 3) {
				tst[d] = 0x1a;
			}
			d++;
			tst[d] = 0x07; // end unsigned char value
			d++;
		}
	}
	fwrite(&tst, d, 1, dbFile);
	fclose(dbFile);
}

void importSensors() { // this is a very buggy function
	unsigned char buf[512] = {0};
	char* tmp = (char*) malloc(512);
	char* dbName = (char*) malloc(512); // just to get around the segfaults
	char* dbValue = (char*) malloc(512);
	char* sensorID = (char*) malloc(512);
	char* sensorType = (char*) malloc(512);
	unsigned char* tmpu = (unsigned char*) malloc(512);
	int strLock = 0;
	int uCharLock = 0;
	int started = 0;
	int ended = 0;
	int bytesRead = 0;
	int i = 0;
	int readValue = 0;
	int sensorIDLock = 0;
	int sensorTypeLock = 0;
	int iU = 0;
	char location[strlen(DBLOC) + 9] = {0};
	strcpy(location, DBLOC);
	strcat(location, "/shmsS.db");
	dbFile = fopen(location, "rb");
	fseek (dbFile , 0 , SEEK_END);
  	long dbSize = ftell(dbFile);
  	rewind (dbFile);
	while (bytesRead < dbSize) {
		fread(buf, 1, 1, dbFile);
		bytesRead += 1;
		if (started && strLock && buf[0] == 0x03) {
			base64_decode(tmp, strlen(tmp), (unsigned char*) dbName);
			dbName[strlen(dbName) + 1] = '\0';
			tmp = (char*) malloc(512); // clean up
			i = 0;
			strLock = 0;
		}
		if (started && uCharLock && buf[0] == 0x07) {
			strcpy(dbValue, tmp);
			dbValue[strlen(dbValue) + 1] = '\0';
			if (strncmp(dbName, "sensorID", 8) == 0 && !sensorIDLock && !sensorTypeLock) {
				sensorID = (char*) malloc(256); // before setting a value, we need to clear it
				sensorIDLock = 1;
				strcpy(sensorID, dbValue);
				sensorID[strlen(sensorID) + 1] = '\0';
				#ifdef DEBUG
				printf("sensorID='%s'\n\n", sensorID);
				#endif
			} else if (strncmp(dbName, "sensorType", 10) == 0 && sensorIDLock && !sensorTypeLock) {
				sensorType = (char*) malloc(256); // before setting a value, we need to clear it
				sensorTypeLock = 1;
				strcpy(sensorType, dbValue);
				sensorType[strlen(sensorType) + 1] = '\0';
				#ifdef DEBUG
				printf("sensorType='%s'\n\n", sensorType);
				#endif
			}
			if (sensorIDLock && sensorTypeLock) {
				#ifdef DEBUG
				printf("Sensor type %s('%s')\n", sensorType, sensorID);
				#endif
				sensors[sensorIndex].id = tmpu;
				sensors[sensorIndex].formatID = sensorID;
				if (strncmp(sensorType, "0B", 2) == 0) {
					sensors[sensorIndex].type = 0;
				} else if (strncmp(sensorType, "0C", 2) == 0) {
					sensors[sensorIndex].type = 1;
				} else if (strncmp(sensorType, "0D", 2) == 0) {
					sensors[sensorIndex].type = 2;
				} else if (strncmp(sensorType, "1A", 2) == 0) {
					sensors[sensorIndex].type = 3;
				}
				sensors[sensorIndex].action = 0;
				sensors[sensorIndex].status = 0;
				sensorIndex++;
				sensorIDLock = 0;
				sensorTypeLock = 0;
				free(sensorID);
				free(tmpu);
			}
			free(tmp);
			free(dbName);
			free(dbValue); // clean up
			i = 0;
			iU = 0;
			uCharLock = 0;
			readValue = 0;
		}
		if (started && strLock) {
			tmp[i] = (char)buf[0];
			i++;
		}
		if (started && uCharLock) {
			sprintf(tmp, "%s%02hhX\0", tmp, buf[0]);
			if (!sensorIDLock) {
				tmpu[iU] = buf[0];
				#ifdef DEBUG
				printf("[%02hhX] ", tmpu[iU]);
				#endif
				iU++;
			}
		}
		if (buf[0] == 0x01 && !started) {
			fread(buf, 1, 1, dbFile);
			bytesRead += 1;
			if (buf[0] == 0x10 && !started) {
				fread(buf, 1, 1, dbFile);
				bytesRead += 1;
				if (buf[0] == 0x15 && !started) {
					started = 1;
				}
			}
		}
		if (started && buf[0] == 0x02) {
			strLock = 1;
		}
		if (started && buf[0] == 0x04) {
			strLock = 1;
			readValue = 1;
		}
		if (started && buf[0] == 0x06) {
			uCharLock = 1;
		}
		if (buf[0] == 0x13 && started) {
			fread(buf, 1, 1, dbFile);
			bytesRead += 1;
			if (buf[0] == 0x00 && started) {
				fread(buf, 1, 1, dbFile);
				bytesRead += 1;
				if (buf[0] == 0x12 && started) {
					ended = 1;
					break;
				}
			}
		}
	}
	fclose(dbFile);
}

char* getString(char* name) {
	unsigned char buf[1];
	char* tmp = (char*) malloc(1);
	char* dbName = (char*) malloc(1); // just to get around the segfaults
	char* dbValue = (char*) malloc(1);
	int strLock = 0;
	int uCharLock = 0;
	int started = 0;
	int ended = 0;
	int bytesRead = 0;
	int i = 0;
	int readValue = 0;
	int block = 0;
	char location[strlen(DBLOC) + 9];
	strcpy(location, DBLOC);
	strcat(location, "/shms.db");
	dbFile = fopen(location, "rb");
	fseek (dbFile , 0 , SEEK_END);
  	long dbSize = ftell(dbFile);
  	rewind (dbFile);
	while (bytesRead < dbSize) {
		fread(buf, 1, 1, dbFile);
		bytesRead += 1;
		if (started && strLock && buf[0] == 0x03) {
			base64_decode(tmp, strlen(tmp), (unsigned char*) dbName);
			tmp = (char*) malloc(256); // clean up
			i = 0;
			strLock = 0;
		}
		if (started && strLock && readValue && buf[0] == 0x05 && !block) {
			base64_decode(tmp, strlen(tmp), (unsigned char*) dbValue);
			if (strncmp((char*) dbName, name, strlen((char*) dbName)) == 0) {
				block = 1;
			} else {
				dbValue = (char*) malloc(256);
			}
			tmp = (char*) malloc(256);
			dbName = (char*) malloc(256); // clean up
			i = 0;
			strLock = 0;
			readValue = 0;
		}
		if (started && uCharLock && buf[0] == 0x07 && !block) {
			strncpy(dbValue, tmp, strlen(tmp));
			tmp = (char*) malloc(256);
			dbName = (char*) malloc(256); // clean up
			i = 0;
			uCharLock = 0;
			readValue = 0;
		}
		if (started && strLock && !block) {
			tmp[i] = (char)buf[0];
			i++;
		}
		if (started && uCharLock && !block) {
			sprintf(tmp, "%s%02hhX", tmp, buf[0]);
		}
		if (buf[0] == 0x01 && !started) {
			fread(buf, 1, 1, dbFile);
			bytesRead += 1;
			if (buf[0] == 0x10 && !started) {
				fread(buf, 1, 1, dbFile);
				bytesRead += 1;
				if (buf[0] == 0x15 && !started) {
					started = 1;
				}
			}
		}
		if (started && buf[0] == 0x02) {
			strLock = 1;
		}
		if (started && buf[0] == 0x04) {
			strLock = 1;
			readValue = 1;
		}
		if (started && buf[0] == 0x06) {
			uCharLock = 1;
		}
		if (buf[0] == 0x13 && started) {
			fread(buf, 1, 1, dbFile);
			bytesRead += 1;
			if (buf[0] == 0x00 && started) {
				fread(buf, 1, 1, dbFile);
				bytesRead += 1;
				if (buf[0] == 0x12 && started) {
					ended = 1;
					break;
				}
			}
		}
	}
	fclose(dbFile);
	return dbValue;
}

char* getBytes(char* name) {
	unsigned char buf[1];
	char* tmp = (char*) malloc(1);
	char* dbName = (char*) malloc(1); // just to get around the segfaults
	char* dbValue = (char*) malloc(1);
	int strLock = 0;
	int uCharLock = 0;
	int started = 0;
	int ended = 0;
	int bytesRead = 0;
	int i = 0;
	int readValue = 0;
	int block = 0;
	char location[strlen(DBLOC) + 9];
	strcpy(location, DBLOC);
	strcat(location, "/shms.db");
	dbFile = fopen(location, "rb");
	fseek (dbFile , 0 , SEEK_END);
  	long dbSize = ftell(dbFile);
  	rewind (dbFile);
	while (bytesRead < dbSize) {
		fread(buf, 1, 1, dbFile);
		bytesRead += 1;
		if (started && strLock && buf[0] == 0x03) {
			base64_decode(tmp, strlen(tmp), (unsigned char*) dbName);
			tmp = (char*) malloc(256); // clean up
			i = 0;
			strLock = 0;
		}
		if (started && strLock && readValue && buf[0] == 0x05 && !block) {
			//base64_decode(tmp, strlen(tmp), (unsigned char*) dbValue);
			dbValue = (char*) malloc(256);
			tmp = (char*) malloc(256);
			dbName = (char*) malloc(256); // clean up
			i = 0;
			strLock = 0;
			readValue = 0;
		}
		if (started && uCharLock && buf[0] == 0x07 && !block) {
			strncpy(dbValue, tmp, strlen(tmp));
			if (strncmp((char*) dbName, name, strlen((char*) dbName)) == 0) {
				block = 1;
			} else {
				dbValue = (char*) malloc(256);
			}
			tmp = (char*) malloc(256);
			dbName = (char*) malloc(256); // clean up
			i = 0;
			uCharLock = 0;
			readValue = 0;
		}
		if (started && strLock && !block) {
			tmp[i] = (char)buf[0];
			i++;
		}
		if (started && uCharLock && !block) {
			sprintf(tmp, "%s%02hhX", tmp, buf[0]);
		}
		if (buf[0] == 0x01 && !started) {
			fread(buf, 1, 1, dbFile);
			bytesRead += 1;
			if (buf[0] == 0x10 && !started) {
				fread(buf, 1, 1, dbFile);
				bytesRead += 1;
				if (buf[0] == 0x15 && !started) {
					started = 1;
				}
			}
		}
		if (started && buf[0] == 0x02) {
			strLock = 1;
		}
		if (started && buf[0] == 0x04) {
			strLock = 1;
			readValue = 1;
		}
		if (started && buf[0] == 0x06) {
			uCharLock = 1;
		}
		if (buf[0] == 0x13 && started) {
			fread(buf, 1, 1, dbFile);
			bytesRead += 1;
			if (buf[0] == 0x00 && started) {
				fread(buf, 1, 1, dbFile);
				bytesRead += 1;
				if (buf[0] == 0x12 && started) {
					ended = 1;
					break;
				}
			}
		}
	}
	fclose(dbFile);
	return dbValue;
}
