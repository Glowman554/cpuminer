#include "server.h"

char *ROOT;
int listenfd, clients[CONNMAX];
void error(char *);
void startServer(char *);
void respond(int);

int server_main(void* arg) {
	struct sockaddr_in clientaddr;
	socklen_t addrlen;
	char c;    

	char* PORT = "5050";
	ROOT = getenv("PWD");

	int slot = 0;
		
	applog(LOG_INFO, "Server started at port no. %s with root directory as %s\n", PORT, ROOT);

	for (int i = 0; i < CONNMAX; i++) {
		clients[i] = -1;
	}

	startServer(PORT);

	while (1) {
		addrlen = sizeof(clientaddr);
		clients[slot] = accept(listenfd, (struct sockaddr *) &clientaddr, &addrlen);

		if (clients[slot]<0) {
			error("accept() error");
		} else {
			if (fork() == 0) {
				respond(slot);
			}
		}

		while (clients[slot] != -1) {
			slot = (slot + 1) % CONNMAX;
		}
	}

	return 0;
}

void server_thread() {
	pthread_t server_thread;
	
	int err = pthread_create(&server_thread, NULL, server_main, NULL);
	if(err) {
		applog(LOG_ERR, "Oh no server thread is ded");
	}
}

//start server
void startServer(char *port) {
	struct addrinfo hints, *res, *p;

	memset (&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo( NULL, port, &hints, &res) != 0) {
		perror("getaddrinfo() error");
		pthread_exit("getaddrinfo() error");
	}

	for (p = res; p!=NULL; p=p->ai_next) {
		listenfd = socket(p->ai_family, p->ai_socktype, 0);
		if (listenfd == -1) {
			continue;
		}
		if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) {
			break;
		}
	}

	if (p==NULL) {
		perror("socket() or bind()");
		pthread_exit("socket() or bind()");
	}

	freeaddrinfo(res);

	// listen for incoming connections
	if (listen (listenfd, 1000000) != 0 ) {
		perror("listen() error");
		pthread_exit("listen() error");
	}
}

//client connection
void respond(int n) {
	char mesg[99999], *reqline[3], data_to_send[BYTES], path[99999], buffer[99999];
	int rcvd, fd, bytes_read;

	memset((void*)mesg, (int) '\0', 99999 );

	rcvd = recv(clients[n], mesg, 99999, 0);

	if (rcvd<0) {
		fprintf(stderr, "recv() error\n");
	} else if (rcvd==0) {
		fprintf(stderr, "Client disconnected upexpectedly.\n");
	} else {
		//printf("%s", mesg);
		reqline[0] = strtok(mesg, " \t\n");
		if (strncmp(reqline[0], "GET\0", 4)==0 ) {
			reqline[1] = strtok(NULL, " \t");
			reqline[2] = strtok(NULL, " \t\n");
			if (strncmp(reqline[2], "HTTP/1.0", 8) != 0 && strncmp(reqline[2], "HTTP/1.1", 8) != 0) {
				write(clients[n], "HTTP/1.0 400 Bad Request\n", 25);
			} else {

				if (strncmp(reqline[1], "/\0", 2) == 0) {
					reqline[1] = "/index.html";
				}

				if(strcmp(reqline[1], "/miner_info") == 0) {
					send(clients[n], "HTTP/1.0 200 OK\n\n", 17, 0);

					sprintf(buffer, "{\n"
									"	\"opt_n_threads\": %d,\n"
									"	\"num_processors\": %d,\n"
									"	\"rpc_url\": \"%s\",\n"
									"	\"rpc_user\": \"%s\",\n"
									"	\"algo_name\": \"%s\"\n"
									"}", opt_n_threads, num_processors, rpc_url, rpc_user, algo_name);

					write(clients[n], buffer, strlen(buffer));

				} else if (strcmp(reqline[1], "/miner_data") == 0) {
					send(clients[n], "HTTP/1.0 200 OK\n\n", 17, 0);

					sprintf(buffer, "{\n"
									"	\"khash_total\": %f,\n"
									"	\"khash_thread_avg\": %f\n"
									"}", khash_total, khash_thread_avg);

					write(clients[n], buffer, strlen(buffer));
				} else {
					strcpy(path, ROOT);
					strcpy(&path[strlen(ROOT)], reqline[1]);
					applog(LOG_INFO, "file: %s", path);

					if((fd = open(path, O_RDONLY)) != -1) {
						send(clients[n], "HTTP/1.0 200 OK\n\n", 17, 0);

						while ((bytes_read = read(fd, data_to_send, BYTES)) > 0) {
							write(clients[n], data_to_send, bytes_read);
						}
					} else {
						write(clients[n], "HTTP/1.0 404 Not Found\n", 23);
					}	
				};
			}
		}
	}

	shutdown (clients[n], SHUT_RDWR);
	close(clients[n]);
	clients[n] = -1;
}