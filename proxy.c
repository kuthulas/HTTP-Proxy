#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define __USE_XOPEN 1
#include <time.h>

enum query {INSERT,HITMISS,DELETE,SHOW};
int root, fdmax;
int npages=0;
int mpages=10;
fd_set tree;

typedef struct request{
	char * address;
	char * resource;
	time_t expires;
	char * accessed;
} request;

typedef struct page{
	request req;
	struct page *next;
	struct page *prev;
} page;

page *head = NULL;

void fixname(char * filename){
	char* x = filename;
	char* y = filename;
	while(*y!=0){
		*x=*y++;
		if(*x!= ' ') x++;
	}
	*x=0;
}

void url2filename(char * filename, request req){
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
	fixname(filename);
}

void serveto(request req, int branch){
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
	fclose(file);
	FD_CLR(branch,&tree);
	close(branch);
}

int cachemanager(enum query q, request r){
	page *here;
	page *temp1,*temp2;

	switch(q){
		case INSERT:
		here = (page *)malloc(sizeof(page));
		here->req.address = r.address;
		here->req.resource = r.resource;
		here->req.expires = r.expires;

		time_t now;
		time(&now);
		struct tm *tme = localtime(&now);		
		char * buffer = malloc(100*sizeof(char));
		strftime(buffer, 80, "%a, %d %b %Y %H:%M:%S %Z",tme);
		here->req.accessed = buffer;

		if(head==NULL) {
			here->next = NULL;
			here->prev = NULL;
			head=here;
		}
		else{
			head->prev = here;
			here->next = head;
			here->prev = NULL;
			head = here;
		}
		npages++;

		if(npages==mpages+1){
			here=head;
			int h;
			for(h=0;h<mpages-1;h++)
				here = here->next;
			here->next = NULL;
			npages = mpages;
		}
		return 0;
		case DELETE:
		here = head;
		while(here!=NULL){
			if(!strcmp(here->req.address,r.address) && !strcmp(here->req.resource,r.resource)){
				temp1 = here->prev;
				temp2 = here->next;
				if(temp1!=NULL)temp1->next = temp2; else head = temp2;
				if(temp2!=NULL)temp2->prev = temp1;
				npages--;
				break;
			}
			here = here->next;
		}
		case HITMISS:
		here = head;
		while(here!=NULL){
			if(!strcmp(here->req.address,r.address) && !strcmp(here->req.resource,r.resource))
			{
				printf("Cache Hit!\n");
				time_t now;
				time(&now);
				struct tm *tme = localtime(&now);

				if(mktime(tme) >= here->req.expires) {
					printf("Expired: Current:%ld, Expiry:%ld\n", mktime(tme), here->req.expires);
					temp1 = here->prev;
					temp2 = here->next;
					if(temp1!=NULL)temp1->next = temp2; else head = temp2;
					if(temp2!=NULL)temp2->prev = temp1;
					npages--;
					return 1;
				}
				else{
					printf("Not expired!\n"); 
					return 0;
				}
			}
			here = here->next;
		}
		return 2;
		case SHOW:
		here = head;
		while(here!=NULL){
			printf("Cache >> \n %s | %s | %lu | %s |\n", here->req.address, here->req.resource, here->req.expires, here->req.accessed);
			here = here->next;
		}
		return 0;
		default:
		return 1;
	}
}

void patchback(int sroot, request req, int branch, int status){
	char * filename = malloc(strlen(req.address)+strlen(req.resource)+1);
	url2filename(filename, req);
	int nbytes;
	int iscode=0;
	int isexpire=0;
	char *code = NULL;
	char *expires = NULL;
	char *token1 = NULL;
	char *token2 = NULL;

	char content[BUFSIZ];
	memset(content,0,BUFSIZ);
	char str[10];
	sprintf(str,"temp%d",branch);
	FILE *file = fopen(str,"w");
	while((nbytes = recv(sroot, content, BUFSIZ, 0)) > 0) {
		if(iscode==0){
			token1 = strtok(strdup(content),"\r\n");
			*(token1+12)='\0';
			code=token1+9;
			iscode=1;
		}
		if(isexpire==0){
			token2 = strtok(strdup(content),"\r\n");
			while(token2!=NULL){
				if(strncmp(token2, "Expires: ", 9)==0) {
					expires = token2+9;
					isexpire = 1;
				}
				token2 = strtok(NULL, "\r\n");
			}
		}
		fprintf(file,"%s",content);
		memset(content, 0, BUFSIZ);
	}
	fclose(file);
	struct tm tme;
	bzero((void *)&tme, sizeof(struct tm));
	if(expires!=NULL && strcmp(expires,"-1")!=0) {
		strptime(expires, "%a, %d %b %Y %H:%M:%S %Z", &tme);
		req.expires = mktime(&tme);
	}
	else req.expires = 0;

	close(sroot);
	
	printf("Received: %s\n", code);
	if(nbytes < 0) {
		perror("RECV Error!\n");
		exit(0);
	}
	else {
		if(strcmp(code,"304")!=0) {
			if(access( filename, F_OK ) != -1) if(remove(filename)!=0) printf("Cache: File delete error!\n");
			if(rename(str,filename)!=0) printf("Cache: File rename error!\n");
		}
		else {
			cachemanager(DELETE,req);
		}
		cachemanager(INSERT,req);
		serveto(req, branch);
	}
}

void dispatch(int sroot, char *message, request req, int branch, int status){
	if((send(sroot, message, strlen(message), 0))==-1){
		perror("GET Failed!");
		exit(1);
	}
	else patchback(sroot, req, branch, status);
}

void GETdressed(int sroot, request req, int branch, int status){
	char *resource = req.resource;
	if(resource==NULL) resource = "";
	else if(resource[0]=='/') resource++;
	char * message;
	page* here = head;
	
	while(here!=NULL){
		if(!strcmp(here->req.address,req.address) && !strcmp(here->req.resource,req.resource)) break;
		here=here->next;
	}

	printf("status %d\n", status);
	if(status == 0 && here!=NULL){
		printf("Conditional GET\n");
		char *template = "GET /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: %s\r\nIf-Modified-Since: %s\r\n\r\n";
		message = (char *)malloc(strlen(template)+6+strlen(req.address)+strlen(resource)+strlen("ECEN602")+strlen(req.accessed));
		sprintf(message, template, resource, req.address, "ECEN602", here->req.accessed);
	}
	else{
		printf("GET\n");
		char *template = "GET /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n";
		message = (char *)malloc(strlen(template)-5+strlen(req.address)+strlen(resource)+strlen("ECEN602"));
		sprintf(message, template, resource, req.address, "ECEN602");
	}
	dispatch(sroot, message, req, branch, status);
}

void getfor(request req, int branch, int status){
	int rv, sroot;
	struct addrinfo hints, *servinfo, *p;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if ((rv = getaddrinfo(req.address, "80", &hints, &servinfo)) != 0) {
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
	GETdressed(sroot, req, branch, status);
}

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
	freeaddrinfo(servinfo);
	if (listen(root,12) == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
	FD_SET(root, &tree);
}

request decode(char abuffer[]){
	char *token;
	token = strtok(strdup(abuffer),"\r\n");
	struct request req;
	while(token != NULL) 
	{
		char *line = malloc(sizeof(char) * strlen(token));
		line = token;
		if(strncmp(line,"GET",3)==0) {
			char * getline = line+4;
			getline[strlen(getline)-8] = '\0';
			req.resource = getline;
		}
		else if(strncmp(line,"Host: ",5)==0) req.address = line+6;
		token = strtok(NULL, "\r\n");
	}
	fixname(req.resource);
	fixname(req.address);
	req.expires=0;
	req.accessed="Never";
	return req;
}

int main(int argc, char const *argv[]){
	if(argc==3){
		int branch, gold, c;
		fd_set reads;
		struct sockaddr_storage address;

		nexus(argv);
		printf("Proxy server ready!\n");
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
							if(strncmp(abuffer,"GET",3)==0){
								request req = decode(abuffer);
								printf("Address: %s\n", req.address);
								printf("Resource: %s\n", req.resource);
								int status = cachemanager(HITMISS,req);
								getfor(req,c,status);
							}
						}
						else{
							if(gold==0) printf("Offline.\n");
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