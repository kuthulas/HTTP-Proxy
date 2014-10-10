#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum query {INSERT,SEARCH};
int root, fdmax;
fd_set tree;

struct request{
	char * address;
	char * resource;
};

struct cache{
	struct request req;
	char *modified;
	struct cache *next;
};

struct cache *croot = NULL;

void url2filename(char * filename, struct request req){
	char resource[strlen(req.resource)];
	int i;
	char *s1 = "/";
	char *s2 = "_";
	strcpy(filename, req.address);
	strcpy(resource, req.resource);
	for(i=0;i<strlen(resource);i++) {
		if(strncmp(resource+i,s1,1)==0) strncpy(resource+i,s2,strlen(s2));
	}
	strcat(filename, resource);
}

void serveto(struct request req, int branch){
	char * filename = malloc(strlen(req.address)+strlen(req.resource)+1);
	char buffer[BUFSIZ];
	url2filename(filename, req);
	FILE *file = fopen(filename, "r");
	if(file!=NULL){
		while(fread(buffer,1,BUFSIZ,file)>0){
			if((send(branch, buffer, strlen(buffer), 0))==-1){
				perror("Serve failed!");
			}
		}
	}
}

void cachemanager(struct request req, enum query q){
	switch(q){
		case INSERT:
		break;
		case SEARCH:
		break;
		default:
		break;
	}
}

//----
void patchback(int sroot, struct request req, int branch){
	char * filename = malloc(strlen(req.address)+strlen(req.resource)+1);
	url2filename(filename, req);
	int nbytes;
	char content[BUFSIZ];
	memset(content,0,BUFSIZ);
	FILE *file = fopen(filename,"w");
	while((nbytes = recv(sroot, content, BUFSIZ, 0)) > 0) {
		fprintf(file,"%s",content);
		memset(content, 0, BUFSIZ);
	}
	close(sroot);
	fclose(file);
	if(nbytes < 0) {
		perror("RECV Error!\n");
		exit(0);
	}
	else {
		cachemanager(req, INSERT);
		serveto(req, branch);
	}
}

void dispatch(int sroot, char *message, struct request req, int branch){
	if((send(sroot, message, strlen(message), 0))==-1){
		perror("GET Failed!");
		exit(1);
	}
	else patchback(sroot, req, branch);
}

void GETdressed(int sroot, struct request req, int branch){
	char *resource = req.resource;
	if(resource==NULL) resource = "";
	else if(resource[0]=='/') resource++;
	char *template = "GET /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n";
	char *message = (char *)malloc(strlen(template)-5+strlen(req.address)+strlen(resource)+strlen("ECEN 602"));
	sprintf(message, template, resource, req.address, "ECEN 602");
	dispatch(sroot, message, req, branch);
}

void getfor(struct request req, int branch){
	int rv, sroot;
	struct addrinfo hints, *servinfo, *p;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if ((rv = getaddrinfo(req.address, htons(80), &hints, &servinfo)) != 0) { // PORT!
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(EXIT_FAILURE);
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sroot = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		if (connect(sroot, p->ai_addr, p->ai_addrlen) == -1) {
			close(sroot);
			perror("client: connect");
			continue;
		}
		break;
	}
	freeaddrinfo(servinfo);
	GETdressed(sroot, req, branch);
}

//----

void nexus(char const *target[]){
	struct addrinfo hints, *servinfo, *p;
	int rv, yes=1;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(target[1], target[2], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(EXIT_FAILURE);
	}
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((root = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}
		if (setsockopt(root, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			exit(EXIT_FAILURE);
		}
		if (bind(root, p->ai_addr, p->ai_addrlen) == -1) {
			close(root);
			perror("server: bind");
			continue;
		}
		break;
	}
	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(servinfo); // all done with this structure
	if (listen(root, 10) == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
	FD_SET(root, &tree);
}

struct request decode(char abuffer[]){
	char *token;
	token = strtok(abuffer,"\r\n");
	struct request req;
	while(token != NULL) 
	{
		char *line = malloc(sizeof(char) * strlen(token));
		strcpy(line,token);
		if(strncmp(line,"GET",3)==0) {
			char * getline = line+4;
			getline[strlen(getline)-8] = '\0';
			req.resource = malloc(strlen(getline));
			strncpy(req.resource, getline, strlen(getline));
		}
		else if(strncmp(line,"Host: ",3)==0) req.address = (line+6);
		token = strtok(NULL, "\r\n");
	}
	return req;
}

int hitormiss(struct request req){
	while (croot != NULL)
	{
		if (croot->req.address == req.address && croot->req.resource == req.resource) return 1; // also expiry check and delete from cache
		croot = croot->next;
	}
	return 0;
}

int main(int argc, char const *argv[]){
	if(argc==3){
		int branch, gold, c;
		fd_set reads;
		struct sockaddr_storage address;

		nexus(argv);
		printf("[Proxy server ready!]\n");
		fdmax = root;

		for(;;){
			reads = tree;
			if(select(fdmax+1, &reads, NULL,NULL,NULL)==-1){
				perror("select");
				exit(EXIT_FAILURE);
			}

			for(c=0;c<=fdmax;c++){
				char abuffer[BUFSIZ];
				if(FD_ISSET(c, &reads)){
					if(c==root){
						socklen_t len = sizeof address;
						if((branch = accept(root,(struct sockaddr *)&address,&len))!=-1){
							FD_SET(branch, &tree);
							if(branch > fdmax) fdmax = branch;
						}
					}
					else{
						if((gold=read(c,abuffer,BUFSIZ))>0){
							printf("%s\n", abuffer);
							if(strncmp(abuffer,"GET",3)==0){
								struct request req = decode(abuffer);
								if(hitormiss(req)) serveto(req, c);
								else getfor(req,c);
							}
						}
						else{
							if(gold==0){
								printf("Offline.\n");
							}
							else perror("recv");
							close(c);
							FD_CLR(branch,&tree);
						}
					}
				}
			}
		}
	}
	else printf("Format: %s <server> <port>\n", argv[0]);
	return 0;
}