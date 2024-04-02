
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 10
#define MAX_PRODUCERS 16
#define MAX_CONSUMERS 16

int *buffer;
//item that is produced in the buffer and not consumed yet
int itemCount = 0;
//current consumer index
int consumerIndex = 0;
//current producerIndex
int producerIndex = 0;
//argv
//the number of items produced by each producer
int numProducedItems; //i
int bufferSize; //b
int producerDelay; //d
int consumerDelay; //d
int numProducers; //p
int numConsumers; //c


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t notfull = PTHREAD_COND_INITIALIZER;
pthread_cond_t notempty = PTHREAD_COND_INITIALIZER;

void* producer(void* arg) {
    int producer_thread_num = *((int*)arg);
    int itemProduced = 0;
    for (int i = 0; i < numProducedItems; i++) {  
        //usleep(producerDelay * 500000);
        pthread_mutex_lock(&mutex);  
        //buffer is full
        while (itemCount == bufferSize) {
            pthread_cond_wait(&notfull, &mutex);
        }
        //produce an item
        int item_num = (numProducedItems*producer_thread_num) + itemProduced;
        itemProduced++;
        if(itemProduced==numProducedItems) itemProduced=0;
        buffer[producerIndex] = item_num;
        producerIndex++;
        //restart from buffer 0
        if(producerIndex==bufferSize) producerIndex =0;
        itemCount++;
        printf("producer_%d produced item %d\n", producer_thread_num, item_num);
        pthread_cond_signal(&notempty);
        pthread_mutex_unlock(&mutex);
        //0.5s delay = 500,000 ms
        usleep(producerDelay * 500000); // If the value is 0, then the call has no effect.
    }

    //free the malloc in main
    free(arg);
    return NULL;
}

void* consumer(void* arg) {
    int consumer_thread_num = *((int*)arg);
    //each consuming for p*i/c
    for (int i = 0; i < numProducers*numProducedItems/numConsumers; i++) {
        //usleep(consumerDelay * 500000);
        pthread_mutex_lock(&mutex);
        //buffer is empty
        while (itemCount == 0) {
            pthread_cond_wait(&notempty, &mutex);
        }
        //comsume an item
        int item_num = buffer[consumerIndex];
        consumerIndex++;
        itemCount--;
        if(consumerIndex==bufferSize) consumerIndex=0;
        printf("consumer_%d consumed item %d\n", consumer_thread_num, item_num);
        pthread_cond_signal(&notfull);
        pthread_mutex_unlock(&mutex);
        usleep(consumerDelay * 500000);
         // Delay for the consumer
    }
    //free the malloc in main
    free(arg);
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <producers> <consumers> <items> <buffer_size> <delay>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    numProducers = atoi(argv[1]);
    numConsumers = atoi(argv[2]);
    numProducedItems = atoi(argv[3]);
    bufferSize = atoi(argv[4]);
    int delay = atoi(argv[5]);
    //The number of consumers should always be 
    //smaller than the number of total items being produced (i.e. c < p*i)
    if (numConsumers >= numProducers * numProducedItems) {
        fprintf(stderr, "Error: The number of consumers should always be smaller than the number of total items being produced.\n");
        exit(EXIT_FAILURE);
    }
    if(bufferSize > MAX_BUFFER_SIZE){
        fprintf(stderr, "Error: Max buffer size is 10.\n");
        exit(EXIT_FAILURE);
    }
    
    buffer = (int *)malloc(bufferSize * sizeof(int));
    //If d == 0, there is a 0.5 s delay for consumer threads after a consumer consumes an item 
    //and before a new consumption.
    if (delay == 0) {
        consumerDelay = 1;
        producerDelay = 0;
    }
    //If d == 1 there is a 0.5 s delay for producer threads 
    //after a producer produces an item and before a new production. 
    else if (delay == 1) {
        consumerDelay = 0;
        producerDelay = 1;
    } 
    //Any other input for d indicates no delay for both
    else {
        consumerDelay = 0;
        producerDelay = 0;
    }


    pthread_t producers[MAX_PRODUCERS];
    pthread_t consumers[MAX_CONSUMERS];

    for (int i = 0 ; i <numProducers ; i++) {
        int* producer_thread_num = malloc(sizeof(int));
        *producer_thread_num = i;
        pthread_create(&producers[i], NULL, producer, producer_thread_num);
    }

    for (int i = 0; i < numConsumers; i++) {
        int* consumer_thread_num = malloc(sizeof(int));
        *consumer_thread_num = i;
        pthread_create(&consumers[i], NULL, consumer, consumer_thread_num);
    }

    for (int i = 0; i < numProducers; i++) {
        pthread_join(producers[i], NULL);
    }

    for (int i = 0; i < numConsumers; i++) {
        pthread_join(consumers[i], NULL);
    }
    free(buffer);
    return 0;
}
