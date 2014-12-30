//============================================================================
// Name        : producer-consumer.cpp
// Author      : Francisco Pérez Pellicena <fperezpellicena@gmail.com>
// Version     :
// Copyright   : 
// Description : Producer consumer example using a queue
//============================================================================

#include <stdlib.h>
#include <mqueue.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#define QUEUE_DIR			"/tmp/producer-consumer"
#define QUEUE_DIR_PERMS		(S_IRWXU | S_IRWXG | S_IRWXO)

#define QUEUE_NAME			"/producer-consumer"
#define QUEUE_SIZE			2
#define QUEUE_OFLAGS		(O_RDWR | O_CREAT)
#define QUEUE_MODE			(S_IRUSR | S_IWUSR)
#define QUEUE_PRIO			0

#define MESSAGE				"Hello!"
#define MESSAGE_LENGTH		7

#define PRODUCER_RATE		2

using namespace std;

static pthread_t producer;
static pthread_t consumer;
static mqd_t queue;

static void* produce(void*) {
	while(true) {
		if (mq_send(queue, MESSAGE, MESSAGE_LENGTH, QUEUE_PRIO) == -1) {
			printf("Error sending message to the queue");
			break;
		}
		printf("Producer thread sent: %s", MESSAGE);
		sleep(PRODUCER_RATE);
	}
	pthread_exit(NULL);
}

static void* consume(void*) {
	char message[2 * MESSAGE_LENGTH];
	while(true) {
		if (mq_receive(queue, message, MESSAGE_LENGTH, NULL) == -1) {
			perror("Error receiving message from the queue");
			break;
		}
		printf("Consumer thread received: %s", message);
	}
	pthread_exit(NULL);
}

static void signalHandler(int sig, siginfo_t *siginfo, void *context) {
	// Close queue
	mq_close(queue);
	mq_unlink(QUEUE_NAME);
	rmdir(QUEUE_DIR);
	// Cancel threads
	pthread_cancel(producer);
	pthread_cancel(consumer);
	abort();
}

static void createQueue(void) {
	mq_attr attr;
	attr.mq_flags = 0;
	attr.mq_maxmsg = QUEUE_SIZE;
	attr.mq_msgsize = MESSAGE_LENGTH;
	if ((queue = mq_open(QUEUE_NAME, QUEUE_OFLAGS, QUEUE_MODE, &attr)) == -1) {
		perror("Error creating queue");
		exit(EXIT_FAILURE);
	}
	if (mkdir(QUEUE_DIR, QUEUE_DIR_PERMS) == -1) {
		perror("Error creating queue directory");
		exit(EXIT_FAILURE);
	}
}

static void createThreads(void) {
	if (pthread_create(&producer, NULL, produce, NULL) != 0) {
		perror("Error creating producer thread");
		exit(EXIT_FAILURE);
	}
	if (pthread_create(&consumer, NULL, consume, NULL) != 0) {
		perror("Error creating consumer thread");
		exit(EXIT_FAILURE);
	}
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
