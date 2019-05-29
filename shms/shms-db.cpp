#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <base64.h>

#define DBLOC "/home/pi/shms/shms"
#define MAXSENSORS 1024

struct sensor {
  unsigned char* id;
  char* formatID;
  int status;
  int type;
  int action;
  int reset = 0;
} sensors[MAXSENSORS];

FILE *dbFile;

int sensorListIndex = 0;

void importSensors();
void putSensor(sensor s);
void updateSensors();
char* getString(char* configName);
char* getBytes(char* configName);

int main() {
	unsigned char i[8] = {0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78, 0x89};
	unsigned char i2[8] = {0x99, 0xB7, 0x6C, 0x7A, 0xBE, 0x4D, 0x60, 0x64};
	sensors[0].id = i;
	sensors[0].type = 0;
	sensors[1].id = i2;
	sensors[1].type = 0;
	sensorListIndex = 2;
	updateSensors();
	importSensors();
	return 0;
}

void putSensor(sensor s) {
	char location[strlen(DBLOC) + 9];
	strcpy(location, DBLOC);
	strcat(location, "/shmsS.db");
	FILE* dbFile = fopen(location, "wb");
	int d = 3;
	unsigned char tst[4096] = {0x01, 0x10, 0x15}; // header
	for(int z = 0; z < sensorListIndex; z++) {
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
	for(int z = 0; z < sensorListIndex; z++) {
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
			}
			d++;
			tst[d] = 0x07; // end unsigned char value
			d++;
		}
	}
	fwrite(&tst, d, 1, dbFile);
	fclose(dbFile);
}

void importSensors() {
	unsigned char buf[1];
	char* tmp = (char*) malloc(1);
	char* dbName = (char*) malloc(1); // just to get around the segfaults
	char* dbValue = (char*) malloc(1);
	char* sensorID = (char*) malloc(1);
	char* sensorType = (char*) malloc(1);
	unsigned char* tmpu = (unsigned char*) malloc(1);
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
	char location[strlen(DBLOC) + 9];
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
			tmp = (char*) malloc(256); // clean up
			i = 0;
			strLock = 0;
		}
		if (started && uCharLock && buf[0] == 0x07) {
			strncpy(dbValue, tmp, strlen(tmp));
			if (strncmp(dbName, "sensorID", 8) == 0 && !sensorIDLock && !sensorTypeLock) {
				sensorID = (char*) malloc(256); // before setting a value, we need to clear it
				sensorIDLock = 1;
				strncpy(sensorID, dbValue, strlen(dbValue));
				printf("sensorID='%s'\n\n", sensorID);
			} else if (strncmp(dbName, "sensorType", 10) == 0 && sensorIDLock && !sensorTypeLock) {
				sensorType = (char*) malloc(256); // before setting a value, we need to clear it
				sensorTypeLock = 1;
				strncpy(sensorType, dbValue, strlen(dbValue));
				printf("sensorType='%s'\n\n", sensorType);
			}
			if (sensorIDLock && sensorTypeLock) {
				printf("Sensor type %s('%s')\n", sensorType, sensorID);
				sensors[sensorListIndex].id = tmpu;
				sensors[sensorListIndex].formatID = sensorID;
				if (strncmp(sensorType, "0B", 2) == 0) {
					sensors[sensorListIndex].type = 0;
				} else if (strncmp(sensorType, "0C", 2) == 0) {
					sensors[sensorListIndex].type = 1;
				} else if (strncmp(sensorType, "0D", 2) == 0) {
					sensors[sensorListIndex].type = 2;
				}
				sensors[sensorListIndex].action = 0;
				sensors[sensorListIndex].status = 0;
				sensorListIndex++;
				sensorIDLock = 0;
				sensorTypeLock = 0;
				sensorID = (char*) malloc(256);
				tmpu = (unsigned char*) malloc(256);
			}
			tmp = (char*) malloc(256);
			dbName = (char*) malloc(256);
			dbValue = (char*) malloc(256); // clean up
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
			sprintf(tmp, "%s%02hhX", tmp, buf[0]);
			if (!sensorIDLock) {
				tmpu[iU] = buf[0];
				printf("[%02hhX] ", tmpu[iU]);
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