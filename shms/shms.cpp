#include <cstdlib>
#include <iostream>
#include <RF24.h>
#include <huaweimodem.h>
#ifdef HTTPD_SUPPORT // pls don't compile with httpd support, or else the modem will break!!!
#include <httpd.h> // COMPLETELLY INSTABLE!
#endif
#ifdef UDP_SUPPORT
#include <udpclient.h> // UDP Client - WIP!
#endif
#define DESTINATION "0123456789" // this is defined in the code... in the future, we will make this as a variable to be updated... and i'm not too stupid to give away my phone number duh...

using namespace std;
const uint64_t pipes[2] = { 0x9E3DED32E9LL, 0x9AF6ACD8E6LL };

RF24 radio(RPI_V2_GPIO_P1_22, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_8MHZ);

unsigned char deviceTypeGas = 0x0b;
unsigned char deviceTypeWater = 0x0c;
unsigned char deviceTypeTemperature = 0x0d;
unsigned char deviceTypeSmartBracelet = 0x1a;

unsigned char actionIdle = 0x00;
unsigned char actionRegister = 0x01;
unsigned char actionTrigger = 0x02;
unsigned char actionHighTemperatureTrigger = 0xF2;
unsigned char trigPulse = 0x11;
unsigned char trigTemp = 0x12;
unsigned char postPulse = 0x13;
unsigned char postTemp = 0x14;

unsigned char hubID[8] = {0x20, 0x70, 0xC3, 0xC3, 0xAE, 0xA8, 0xB2, 0x73}; // will be read from EEPROM

int stop = 0;
int smsStop = 0;

bool searching = false;

int main(int argc, char** argv) {
	modemBegin();
	int s = simStatus();
	if(s == 0) { 
		printf("SIM found. IMSI = %s\n", imsi());
	} else {
		printf("SIM failure. Forced disable!\n");
		stop = 1;
		smsStop = 1;
	}
	#ifdef HTTPD_SUPPORT
	printf("Start the HTTP Server(INSTABLE)\n");
	startHttpServer();
	#endif
	#ifdef UDP_SUPPORT
		printf("Start UDP Client...\n");
		initUDP();
		registerHubToServer(hubID);
	#endif
	uint8_t len;
	printf("Initiate RF...\n");
	radio.begin();
	radio.enableDynamicPayloads();
	radio.setAutoAck(1);
	radio.enableAckPayload();
	radio.setRetries(15,15);
	radio.setDataRate(RF24_250KBPS);
	radio.setPALevel(RF24_PA_MAX);
	radio.setChannel(89);
	radio.setCRCLength(RF24_CRC_8);
	radio.openWritingPipe(pipes[1]);
	radio.openReadingPipe(1,pipes[0]);
	if (!radio.isPVariant()) {
		printf("RF Failure! Can't find RF!\n");
		return 1;
	} else {
		radio.startListening();
		radio.printDetails();
		delay(1);
		while(1) {
			#ifdef HTTPD_SUPPORT
			searching = getRegistration();
			#endif
			char receivePayload[11];
			uint8_t pipe = 1;
			unsigned char id[8];
			unsigned char type;
			float temp = 0;
			int t = 0;
			unsigned char action;
			radio.startListening();
			while (radio.available(&pipe)) {
				len = radio.getDynamicPayloadSize();
				radio.read(receivePayload, len);
				printf("len: %i\n", len);
				if(len == 0) continue;

				for(int x = 0; x < len; x++) {
					//printf(" %02X", receivePayload[x]);
				}
				//printf("\n\n");
				for(int x = 0; x < 8; x++) {
					id[x] = receivePayload[x];
					//printf(" 0x%02X", receivePayload[x]);
				}
				type = receivePayload[8];
				//printf("type=0x%02X\n", type);
				if (type != deviceTypeTemperature) {
					action = receivePayload[9];
				}
				int stype = -1;
				if (type == deviceTypeGas) {
					//printf("gasaio\n");
					stype = 0;
				}
				if (type == deviceTypeWater) {
					//printf("waterL\n");
					stype = 1;
				}
				if (type == deviceTypeTemperature) {
					//printf("temperature\n");
					stype = 2;
				}
				else if (type == deviceTypeSmartBracelet) {
					stype = 3;
					//printf("sbrac\n");
				}
				if (type == deviceTypeSmartBracelet) {
					if(action == postTemp) {
						unsigned char temperature[(strlen(receivePayload) - 10)] = {0};	
						strncpy ((char*)temperature, receivePayload + 10, (strlen(receivePayload) - 10));
						float temperatura = atof((char*)temperature);
						printf("Temperature is %2.1f degrees C\n", temperatura);
						#ifdef UDP_SUPPORT
							udpPostSB(hubID, 0, temperatura, id, stype); 
						#endif
						smsStop = 0;
					}
					if(action == postPulse) {
						unsigned char pulse[(strlen(receivePayload) - 10)] = {0};
						strncpy ((char*)pulse, receivePayload + 10, (strlen(receivePayload) - 10));
						int pulseB = atoi((char*)pulse);
						printf("Pulse is %i BPM\n", pulseB);
						#ifdef UDP_SUPPORT
							udpPostSB(hubID, 1, pulseB, id, stype);
						#endif
					}
					if(action == trigTemp) {
						#ifdef UDP_SUPPORT
							udpPostSB(hubID, 2, 0, id, stype);
						#endif
					}
					if(action == trigPulse) {
						#ifdef UDP_SUPPORT
							udpPostSB(hubID, 3, 0, id, stype);
						#endif
					}
					if(action == 0x12) {
						if(!smsStop) {
            	printf("S-a inregistrat alerta din partea unei bratari! Trimitere SMS!");
            	sendSMS("ALERTA COD 1 - SALONUL 1 PATUL 1", DESTINATION);
              smsStop = 1;
            }
					}
					//printf("action=0x%02x\n", action);
				}
				if (action == actionRegister && searching) {
					printf("REGISTER HUB\n");
					unsigned char registerAccept[18];
					registerAccept[0] = 0x05;
					for (int idi = 0; idi < 8; idi++) {
						registerAccept[idi + 1] = id[idi];
					}
					for (int idh = 8; idh < 16; idh++) {
						registerAccept[idh] = hubID[idh - 8];
					}
					int status = 0;
					int stype = -1;
					if (type == deviceTypeGas) {
						printf("Gas sensor AIO\n");
						stype = 0;
					}
					if (type == deviceTypeWater) {
						printf("Water Level Sensor\n");
						stype = 1;
					}
					if (type == deviceTypeTemperature) {
						printf("Temperature Sensor\n");
						stype = 2;
					}
					else if (type == deviceTypeSmartBracelet) {
						stype = 3;
						printf("Smart Bracelet\n");
					}
					int action = 1;
					#ifdef HTTPD_SUPPORT
					int stat = addSensor(id, status, action, stype);
					printf("%i\n", stat);
					if (stat) {
						radio.stopListening();
						radio.write(registerAccept, sizeof(registerAccept));
					}
					#endif
				}
				if (action == actionTrigger && type != deviceTypeSmartBracelet) {
					int status = 2;
					int action = 2;
					#ifdef HTTPD_SUPPORT
					setSensor(id, status, action);
					#endif
					if (!stop) {
						printf("DECLANSAT! POSIBIL INCENDIU! INITIERE APEL TELEFONIC!\n\n");
						int dR = dial(DESTINATION);
						printf("dial says %i\n");
						if (!dR) {
							stop = 1;
						}
					}
				} else if (action == actionHighTemperatureTrigger && type != deviceTypeSmartBracelet) {
					int status = 3;
					int action = 3;
					#ifdef HTTPD_SUPPORT
					setSensor(id, status, action);
					#endif
					if (!stop) {
						printf("TEMPERATURA MARE DETECTATA!!! POSIBIL INCENDIU! INITIERE APEL TELEFONIC!\n");
						int dR = dial(DESTINATION);
						if (!dR) {
							stop = 1;
						}
					}
				} else if (action == actionIdle && type != deviceTypeSmartBracelet) {
					if(stop) {
						stop = 0;
						printf("Received Idle Action from the sensor.\n");
					}
				}
				receivePayload[len]=0;
			}
		}
		delayMicroseconds(20);
	}
}

