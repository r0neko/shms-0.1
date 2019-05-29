#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <base64.h>

#define ALGOVER "v1.0-Alpha"

char* encryptData(char* data) {
	char* formattedS = (char*) malloc(strlen(data));
	for(int l = 0; l < strlen(data); l++) {
		char x = (char)(data[l]+2*2);
		formattedS[l] = x;
	}
	formattedS[strlen(data)] = '\0';
	return formattedS;
}

char* decryptData(char* data) {
	char* formattedD = (char*) malloc(strlen(data));
	for(int l = 0; l < strlen(data); l++) {
		char x = (char)(data[l]-2*2);
		formattedD[l] = x;
	}
	formattedD[strlen(data)] = '\0';
	return formattedD;
}

int userExists(char* user) {
	int exist = 0;
	FILE* file = fopen("/etc/.spasswd", "r");
	char formatted[strlen(user)] = {0};
	char logindata[256] = {0};
	for(int l = 0; l < strlen(user); l++) {
		char x = (char)(user[l]+6);
		formatted[l] = x;
	}
	formatted[strlen(user)] = '\0';
	while (fgets(logindata, sizeof(logindata), file)) {
		char* x = strtok(logindata, "\n");
		unsigned char loginD[256] = {0};
		base64_decode(x, strlen(x), loginD);
		if (strncmp((char*) loginD, formatted, sizeof(user)) == 0) {
			exist = 1;
		}
	}
	fclose(file);
	return exist;
}

int passForUserCorrect(char* user, char* password) {
	int exist = 0;
	FILE* file = fopen("/etc/.spasswd", "r");
	char formattedUser[strlen(user)] = {0};
	char formattedPassword[strlen(password)] = {0};
	char formatted[strlen(user) + 1 + strlen(password)] = {0};
	char logindata[256] = {0};
	for(int l = 0; l < strlen(user); l++) {
		char x = (char)(user[l]+6);
		formattedUser[l] = x;
	}
	for(int l = 0; l < strlen(password); l++) {
		char x = (char)(password[l]+6);
		formattedPassword[l] = x;
	}
	formattedUser[strlen(user)] = '\0';
	formattedPassword[strlen(password)] = '\0';
	strcat((char *)formatted, formattedUser);
	strcat((char *)formatted, (char*) ":");
	strcat((char *)formatted, formattedPassword);
	while (fgets(logindata, sizeof(logindata), file)) {
		char* x = strtok(logindata, "\n");
		unsigned char loginD[256] = {0};
		base64_decode(x, strlen(x), loginD);
		if (strncmp((char*) loginD, formatted, sizeof(formatted)) == 0) {
			exist = 1;
		}
	}
	fclose(file);
	return exist;
}