#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum query {INSERT,DELETE,HITMISS};

typedef struct request{
	char address[50];
	char resource[50];
} request;
	request r;

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
		printf("malloc Insert ");
		here = (page *)malloc(sizeof(page));
		strcpy(here->req.address,r.address);
		strcpy(here->req.resource,r.resource);
		if(head==NULL){ head=here; return 0;}
		head->prev = here;
		here->next = head;
		here->prev = NULL;
		head = here;
		return 0;
		case DELETE:
		here = head;
		while(here!=NULL){
			printf("A\n");
			if(!strcmp(here->req.address,r.address) && !strcmp(here->req.resource,r.resource)){
				printf("B\n");
				temp1 = here->prev;
				temp2 = here->next;
				if(temp1!=NULL)temp1->next = temp2;
				if(temp2!=NULL)temp2->prev = temp1;
				printf("A\n");
				free(here);
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
	int i;
	char item[10] = "kumaran";
	for(i=0;i<5;i++){
		printf("\nEnter info to INSERT in slot %d:",i);
		scanf("%s",r.address);
		//strcpy(r.address,item);
		cachemanager(INSERT, r);
	}
	printf("Insrt done");
	page *here = head;
	while(here!=NULL){
		printf("\n%s\n", here->req.address);
		here = here->next;
		if(here!=NULL) printf("Continue\n");
	}
	printf("Traverse done");
	
	printf("\nEnter info to delete:");
	scanf("%s",r.address);
	//strcpy(r.address,item);
	printf("\nDel start\n");
	cachemanager(DELETE,r);
	printf("\nDel done\n");

	here = head;
	while(here!=NULL){
		printf("\n%s\n", here->req.address);
		here = here->next;
	}
	printf("END\n");
	return 0;
}
