#SHMS v1.0 ALPHA
#System app build system
CC = arm-linux-gnueabihf-g++
CFLAGS = -Ofast -Wno-write-strings -pthread -I/usr/local/include/RF24/.. -I.. -L/usr/local/lib -L. -I.

all: shms

shms:  huaweimodem.o udpclient.o
	$(CC) $(CFLAGS) shms.cpp -lrf24 -ludp -lhuaweimodem -o shmsd -DGUDP_SUPPORT

testM: huaweimodem.o
	$(CC) $(CFLAGS) test.c -lhuaweimodem -o test

huaweimodem.o:  uri.o huaweimodem.cpp huaweimodem.h 
	$(CC) $(CFLAGS) -c huaweimodem.cpp
	ar rcs libhuaweimodem.a uri.o huaweimodem.o

base64.o:  base64.cpp base64.h 
	$(CC) $(CFLAGS) -c base64.cpp
	ar rcs libb64.a base64.o

pass.o:  pass.cpp pass.h 
	$(CC) $(CFLAGS) -c pass.cpp
	ar rcs libpass.a pass.o

httpd.o: base64.o pass.o cJSON.o udpclient.o httpd.cpp httpd.h # httpd here is very very very buggy! it must be fixed!!! the importSensors() function breaks the memory, thuse rendering the content of the modem chars with garbage! for now we leave this off...
	echo "[!WARNING!] Compiling SHMSD with HTTPD(INSTABLE) breaks HUAWEIMODEM driver! malloc breaks..."
	$(CC) $(CFLAGS) -c httpd.cpp
	ar rcs libhttpd.a base64.o pass.o cJSON.o udpclient.o httpd.o

cJSON.o: cJSON.cpp cJSON.h 
	$(CC) $(CFLAGS) -c cJSON.cpp
	ar rcs libcJSON.a cJSON.o

udpclient.o: udpclient.c udpclient.h
	echo "[!WARNING!] UDP Client is STILL UNFINISHED! THERE CAN BE SOME FAILURES! YOU HAVE BEEN WARNED!"
	$(CC) $(CFLAGS) -c udpclient.c -lpthread
	ar rcs libudp.a udpclient.o

testudp:  udpclient.c udpclient.h
	$(CC) $(CFLAGS) udpclient.c -o udpclient

uri.o: uri.cpp uri.h 
	$(CC) $(CFLAGS) -c uri.cpp
	ar rcs liburi.a uri.o

clean: 
	$(RM) shmsd *.o *~ *.a

mrproper: clean
