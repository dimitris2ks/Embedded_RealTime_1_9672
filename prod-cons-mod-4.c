#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>

#define QUEUESIZE 10
#define LOOP 100000
#define PRO_NUM 1

void *producer (void *args);
void *consumer (void *args);

typedef struct {
    void *(*work)(void *);
    void *arg;
    struct timeval start;
} workFunction;

typedef struct {
    workFunction buf[QUEUESIZE];
    long head, tail;
    int full, empty;
    pthread_mutex_t *mut;
    pthread_cond_t *notFull, *notEmpty;
    int *toWrite;
} queue;

queue *queueInit (int *toWrite);
void queueDelete (queue *q);
void queueAdd (queue *q, workFunction in);
void queueDel (queue *q, workFunction *out);


int outCounter;

int main () {

   
    srand(time(NULL));

   
    FILE *fp;
    fp = fopen("assignment_data_4.csv", "w");

    for (int conNum=1; conNum<513; conNum*=2) {

       
        printf("#Cons=%d Started.\n",conNum);

        outCounter = -1;

        
        int *toWrite = (int *)malloc(LOOP*PRO_NUM*sizeof(int));

        
        queue *fifo;
        fifo = queueInit (toWrite);
        if (fifo ==  NULL) {
            fprintf (stderr, "main: Queue Init failed.\n");
            exit (1);
        }

        pthread_t pro[PRO_NUM];
        pthread_t con[conNum];

        
        for (int i=0; i<conNum; i++)
            pthread_create (&con[i], NULL, consumer, fifo);
        for (int i=0; i<PRO_NUM; i++)
            pthread_create (&pro[i], NULL, producer, fifo);

        
        for (int i=0; i<PRO_NUM; i++)
            pthread_join (pro[i], NULL);
        for (int i=0; i<conNum; i++)
            pthread_join (con[i], NULL);

        
        queueDelete (fifo);

        
        for (int i=0; i<LOOP*PRO_NUM; i++)
            fprintf(fp, "%d,", toWrite[i]);
        fprintf(fp, "\n");

        
        free(toWrite);

        
        sleep(0.1);

    }

    
    fclose(fp);

    return 0;
}

void work(void *arg) {
    int *a = (int *)arg;
    double r = 0;
    for (int i=0; i<a[0]; i++)
        r += sin((double)a[i+1]);

    
}

void *producer (void *q) {
    queue *fifo;

    fifo = (queue *)q;

    for (int i=0; i<LOOP; i++) {

        
        int k = (rand() % 101) + 100;
        int *a = (int *)malloc((k+1)*sizeof(int));
        a[0] = k;
        for (int j=0; j<k; j++)
            a[j+1] = k+j;

        
        workFunction in;
        in.work = &work;
        in.arg = a;

        
        pthread_mutex_lock (fifo->mut);

        while (fifo->full) {
            //printf ("producer: queue FULL.\n");
            pthread_cond_wait (fifo->notFull, fifo->mut);
        }
        gettimeofday(&in.start, NULL);
        queueAdd (fifo, in);

        
        pthread_mutex_unlock (fifo->mut);

        
        pthread_cond_signal (fifo->notEmpty);

    }

    return (NULL);
}

void *consumer (void *q) {
    queue *fifo;
    workFunction out;
    struct timeval end;

    fifo = (queue *)q;

    while (1) {

        
        pthread_mutex_lock (fifo->mut);

        
        if (outCounter == LOOP*PRO_NUM-1) {
            pthread_mutex_unlock (fifo->mut);
            break;
        }

        
        outCounter++;

        while (fifo->empty) {
            //printf ("consumer: queue EMPTY.\n");
            pthread_cond_wait (fifo->notEmpty, fifo->mut);
        }
        queueDel (fifo, &out);
        gettimeofday(&end, NULL);
        fifo->toWrite[outCounter] = (int) ((end.tv_sec-out.start.tv_sec)*1e6 + (end.tv_usec-out.start.tv_usec));

        
        pthread_mutex_unlock (fifo->mut);

        
        pthread_cond_signal (fifo->notFull);

        
        out.work(out.arg);

    }

    return (NULL);
}

queue *queueInit (int *toWrite) {
    queue *q;

    q = (queue *)malloc (sizeof (queue));
    if (q == NULL) return (NULL);

    q->empty = 1;
    q->full = 0;
    q->head = 0;
    q->tail = 0;
    q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
    pthread_mutex_init (q->mut, NULL);
    q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
    pthread_cond_init (q->notFull, NULL);
    q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
    pthread_cond_init (q->notEmpty, NULL);

    q->toWrite = toWrite;

    return (q);
}

void queueDelete (queue *q) {
    pthread_mutex_destroy (q->mut);
    free (q->mut);
    pthread_cond_destroy (q->notFull);
    free (q->notFull);
    pthread_cond_destroy (q->notEmpty);
    free (q->notEmpty);

    free (q);
}

void queueAdd (queue *q, workFunction in) {
    q->buf[q->tail] = in;
    q->tail++;
    if (q->tail == QUEUESIZE)
        q->tail = 0;
    if (q->tail == q->head)
        q->full = 1;
    q->empty = 0;

    return;
}

void queueDel (queue *q, workFunction *out) {
    *out = q->buf[q->head];

    q->head++;
    if (q->head == QUEUESIZE)
        q->head = 0;
    if (q->head == q->tail)
        q->empty = 1;
    q->full = 0;

    return;
}