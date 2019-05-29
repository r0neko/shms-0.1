#define FRAME_SIZE 320
typedef struct pcm_data {
	char *data;
	int length;
} pcm_data_t;

void sendSMS(char* text, char* phoneNumber);
void writePCUI(char *message, int flushSerial);
void writeAudioIO(char* audioF);
char* readPCUI(int removeOK);
char* readPCUIBlock(int rO);
char* manufacturer();
void modemBegin();
int dial(char *phone);
void sendSMS(char* text, char* phoneNumber);
int simStatus();
char* imsi();
char* imei();
char* manufacturer();
