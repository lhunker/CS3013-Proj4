/*
Lukas Hunker
mailboxs.C
The class definition for a mailbox class
Used to send messages between threads with mutex
*/
#include "mailboxs.h"


/*
constructor
Params:
	n - the number of mailboxs to create
*/
mailboxs::mailboxs(int n){
	boxs = new struct msg * [n];
	//initialize semaphores
	sendsem = new sem_t [n];
	recvsem = new sem_t [n];
	for(int i = 0; i < n; i++){
		 sem_init(&sendsem[i], 0, 1);
		 sem_init(&recvsem[i], 0, 0);
	}
	size = n;
}

/*
SendMsg
Sends a message to the mailbox specified by iTO
Params:
	iTo - The mailbox to send the message to
	Msg - a pointer to the message being sent
*/
void mailboxs::SendMsg(int iTo, struct msg *Msg){
	sem_wait(&sendsem[iTo]);
	boxs[iTo] = Msg;
	sem_post(&recvsem[iTo]);
}

/*
RecvMsg
Retrives a message from the mailbox
Params:
	iFrom - the mailbox to retrieve from
	msg - a pointer to where to save the message
*/
void mailboxs::RecvMsg(int iFrom, struct msg *msg){
	sem_wait(&recvsem[iFrom]);
	*msg = *boxs[iFrom];
	delete boxs[iFrom];
	sem_post(&sendsem[iFrom]);
}

/*
destructor - destroys class
*/
mailboxs::~mailboxs(){
	for(int i = 0; i < size; i++){
		sem_destroy(&sendsem[i]);
		sem_destroy(&recvsem[i]);
	}
	delete sendsem, recvsem, boxs;
}
