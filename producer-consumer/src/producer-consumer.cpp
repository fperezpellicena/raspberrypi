//============================================================================
// Name        : producer-consumer.cpp
// Author      : Francisco Pérez Pellicena <fperezpellicena@gmail.com>
// Version     :
// Copyright   : 
// Description : Producer consumer example using a queue
// sudo mkdir /dev/mqueue
// sudo mount -t mqueue none /dev/mqueue
//============================================================================
#include <iostream>
#include <stdlib.h>
#include <mqueue.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#define QUEUE_NAME		"/producer-consumer"
#define QUEUE_SIZE		2
#define QUEUE_OFLAGS	O_RDWR | O_CREAT
#define QUEUE_MODE		S_IRUSR | S_IWUSR
#define QUEUE_PRIO		0

#define MESSAGE			"Hello!"
#define MESSAGE_LENGTH	7

#define PRODUCER_RATE	5

using namespace std;

static pthread_t producer;
static pthread_t consumer;
static mqd_t queue;

static void* produce(void*) {
	while(true) {
		sleep(PRODUCER_RATE);
		mq_send(queue, MESSAGE, MESSAGE_LENGTH, QUEUE_PRIO);
		cout << "Send" << endl;
	}
	pthread_exit(NULL);
}

static void* consume(void*) {
	char message[2 * MESSAGE_LENGTH];
	while(true) {
		mq_receive(queue, message, MESSAGE_LENGTH, NULL);
		cout << "Receive: " << message << endl;
	}
	pthread_exit(NULL);
}

static void signalHandler(int sig, siginfo_t *siginfo, void *context) {
	cout << "Signal received: " << sig << endl;
	mq_close(queue);
	mq_unlink(QUEUE_NAME);
	pthread_cancel(producer);
	pthread_cancel(consumer);
	abort();
}

static void createQueue(void) {
	mq_attr attr;
	attr.mq_flags = 0;
	attr.mq_maxmsg = QUEUE_SIZE;
	attr.mq_msgsize = MESSAGE_LENGTH;
	queue = mq_open(QUEUE_NAME, QUEUE_OFLAGS, QUEUE_MODE, &attr);
}

static void createThreads(void) {
	pthread_create(&producer, NULL, produce, NULL);
	pthread_create(&consumer, NULL, consume, NULL);
}

static void configureSignalHandler(void) {
	struct sigaction act;
	act.sa_sigaction = &signalHandler;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	sigaction(SIGKILL, &act, NULL);
}

int main() {
	configureSignalHandler();
	createQueue();
	createThreads();
	while(true) {
		pause();
	}
	return 0;
}
