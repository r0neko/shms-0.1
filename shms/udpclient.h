void initUDP();
char* readUDPMessage();
void registerHubToServer(unsigned char* hubID);
void udpPostSB(unsigned char* id, int value, float v, unsigned char* sid, int type);
void sendUDPMessage(char* message);