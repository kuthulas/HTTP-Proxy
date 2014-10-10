#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum query {INSERT,DELETE,HITMISS};

typedef struct request{
	char * address;
	char * resource;
} request;

typedef struct page{
	request req;
	char *modified;
	struct page *next;
	struct page *prev;
} page;

page *head = NULL;

int cachemanager(enum query q, request r){
	page *here = NULL;
	page *temp1,*temp2;
	switch(q){
		case INSERT:
		here = (page *)malloc(sizeof(page));
		here->req.address = r.address;
		here->req.resource = r.resource;
		head->prev = here;
		here->next = head;
		here->prev = NULL;
		head = here;
		return 0;
		case DELETE:
		here = head;
		while(here!=NULL){
			printf("A\n");
			if(here->req.address==r.address && here->req.resource==r.resource){
				printf("B\n");
				temp1 = here->prev;
				temp2 = here->next;
				temp1->next = temp2;
				temp2->prev = temp1;
				printf("A\n");
				free(here);
				free(temp1);
				free(temp2);
				return 0;
			}
			here = here->next;
		}
		return 1;
		case HITMISS:
		here = head;
		while(here!=NULL){
			if(here->req.address==r.address && here->req.resource==r.resource){
				here->prev->next = here->next;
				here->next = head;
				here->prev = NULL;
				head = here;
				return 0;
			}
			here = here->next;
		}
		return 1;
		default:
		return 1;
	}
}

int main(){
	request r;
	int i;
	char *item = "kumaran";
	for(i=0;i<7;i++){
		r.address = item+i;
		cachemanager(INSERT, r);
	}

	page *here = head;
	while(here!=NULL){
		printf("%s\n", here->req.address);
		here = here->next;
	}

	r.address = item+4;
	cachemanager(DELETE,r);

	here = head;
	while(here!=NULL){
		printf("%s\n", here->req.address);
		here = here->next;
	}
}