struct sensor {
  unsigned char* id;
  int status = 0;
  int type = 0;
  int action = 0;
  int reset = 0;
};
void error(char *);
void startServer(char *);
void respond(int accepter);
int addSensor(unsigned char* id, int status, int action, int type);
void setSensor(unsigned char* id, int status, int action);
void *serverLoop(void *acc);
void startHttpServer();
void stopRegistration();
void startRegistration();
int getRegistration();