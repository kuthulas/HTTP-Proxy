#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int root;

void patchback(){
	int nbytes;
	char content[BUFSIZ];
	memset(content,0,BUFSIZ);
	FILE *file = fopen("resource.html","w");
	while((nbytes = recv(root, content, BUFSIZ, 0)) > 0) {
		fprintf(file,"%s",content);
		memset(content, 0, BUFSIZ);
	}
	close(root);
	fclose(file);
	if(nbytes < 0) {
		perror("RECV Error!\n");
		exit(0);
	} 
	else printf("GET Success! Saved in resource.html\n");
}

void dispatch(char *message){
	if((send(root, message, strlen(message), 0))==-1){
		perror("GET Failed!");
		exit(1);
	}
	else patchback();
}

void GETdressed(const char *url){
	char urld[strlen(url)];
	strcpy(urld,url);
	char *resource = strstr(url,"/");
	if(resource==NULL) resource = "";
	else if(resource[0]=='/') resource++;
	char *address = strtok(urld,"/");
	char *template = "GET /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n";
	char *message = (char *)malloc(strlen(template)-5+strlen(address)+strlen(resource)+strlen("ECEN 602"));
	sprintf(message, template, resource, address, "ECEN 602");
	dispatch(message);
}

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