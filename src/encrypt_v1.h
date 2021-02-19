#ifndef ENCRYPT_V1_H
#define ENCRYPT_V1_H

#include <cstdio>
#include <string>
#include <cstdlib>
#include <openssl/md5.h>

void encrypt_v1(char* imei, char* resbuf, char *hstr);

#endif // ENCRYPT_V1_H
