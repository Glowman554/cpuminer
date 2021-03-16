#ifndef _SERVER_H_
#define _SERVER_H_


#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<signal.h>
#include<fcntl.h>

#include <pthread.h>

#include "miner-info.h"

#include "miner.h"

#define CONNMAX 9999999
#define BYTES 1024

void server_thread();

#endif