#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

int root;

// Listen back from the server
void patchback(char filename[]){
	int nbytes;
	int start=0;
	char content[BUFSIZ];
	memset(content,0,BUFSIZ);
	char *bodystart = NULL;
	char *name = basename(filename); // Filename to save the file locally
	if(strcmp(name,"")==0) name = "index.html";
	FILE *file = fopen(name,"w");
	while((nbytes = recv(root, content, BUFSIZ, 0)) > 0) {
		if(start==0) { // Remove header from the received HTTP message
			bodystart = strstr(content, "\r\n\r\n");
			if(bodystart!=NULL) {
				fprintf(file,"%s",bodystart+4);
				start = 1;
			}
		}
		else fprintf(file,"%s",content); // Save just the body
		memset(content, 0, BUFSIZ);
	}
	close(root);
	fclose(file);
	if(nbytes < 0) {
		perror("RECV Error!\n");
		exit(0);
	} 
	else printf("GET Success! Saved in %s\n", name);
}

// Send message to proxy server
void dispatch(char *message, char filename[]){
	if((send(root, message, strlen(message), 0))==-1){
		perror("GET Failed!");
		exit(1);
	}
	else patchback(filename);
}

// Format the GET message using command-line input parameters
void GETdressed(const char *url){
	if(strncmp(url,"http://",7)==0) url+=7;
	if(strncmp(url,"https://",8)==0) url+=8;
	char urld[strlen(url)];
	strcpy(urld,url);
	const char *resource = strstr(url,"/");
	if(resource==NULL) resource = url; 
	else if(resource[0]=='/') resource++; 
	char filename[strlen(resource)];
	memcpy(filename, resource, strlen(resource));
	filename[strlen(resource)]='\0';
	char *address = strtok(urld,"/");
	char *template = "GET /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n";
	char *message = (char *)malloc(strlen(template)-5+strlen(address)+strlen(resource)+strlen("ECEN 602"));
	sprintf(message, template, resource, address, "ECEN 602");
	dispatch(message, filename);
}

// Make connection to proxy server
void nexus(char const *target[]){
	int rv;
	struct addrinfo hints, *servinfo, *p;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if ((rv = getaddrinfo(target[1], target[2], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(EXIT_FAILURE);
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((root = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		if (connect(root, p->ai_addr, p->ai_addrlen) == -1) {
			close(root);
			perror("client: connect");
			continue;
		}
		break;
	}
	freeaddrinfo(servinfo);
	GETdressed(target[3]);
}

int main(int argc, char const *argv[]){
	if(argc==4) nexus(argv);
	else {printf("Format: %s <address> <port> <url>\n", argv[0]); exit(EXIT_FAILURE);}
	return 0;
}
