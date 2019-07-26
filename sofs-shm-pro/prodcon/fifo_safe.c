/*
 *  @brief A simple FIFO, whose elements are pairs of integers,
 *      one being the id of the producer and the other the value produced
 *
 * @remarks Unsafe, blocking, busy waiting version
 *
 *  The following operations are defined:
 *     \li insertion of a value
 *     \li retrieval of a value.
 *
 * \author (2016) Artur Pereira <artur at ua.pt>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#include "fifo.h"
#include "delays.h"


#include <stdlib.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <pthread.h>

const long key = 0x1111L;

static int shmid = -1;

/** \brief internal storage size of <em>FIFO memory</em> */
#define  FIFOSZ         5

/*
 *  \brief Data structure.
 */
typedef struct
{ 
    unsigned int ii;   ///< point of insertion
    unsigned int ri;   ///< point of retireval
    unsigned int cnt;  ///< number of items stored
    struct 
    {
        unsigned int id;     ///< id of the producer
        unsigned int value;  ///< value stored
    } mem[FIFOSZ];           ///< storage memory
    unsigned int semid;
} FIFO;

/** \brief internal storage region of FIFO type */
static FIFO * fifo = NULL;

/* ************************************************* */

static void lock()
{
    struct sembuf down = {0, -1, 0};
    if (semop(fifo->semid, &down, 1) == -1)
    {
        perror("lock");
        exit(EXIT_FAILURE);
    }
}

static void unlock()
{
    struct sembuf up = {0, 1, 0};
    if (semop(fifo->semid, &up, 1) == -1)
    {
        perror("unlock");
        exit(EXIT_FAILURE);
    }
}

static void down_emptyness()
{
    struct sembuf down = {1, -1, 0};
    if (semop(fifo->semid, &down, 1) == -1)
    {
        perror("lock");
        exit(EXIT_FAILURE);
    }
}

static void up_emptyness()
{
    struct sembuf up = {1, 1, 0};
    if (semop(fifo->semid, &up, 1) == -1)
    {
        perror("unlock");
        exit(EXIT_FAILURE);
    }
}

static void down_fullness()
{
    struct sembuf down = {2, -1, 0};
    if (semop(fifo->semid, &down, 1) == -1)
    {
        perror("lock");
        exit(EXIT_FAILURE);
    }
}

static void up_fullness()
{
    struct sembuf up = {2, 1, 0};
    if (semop(fifo->semid, &up, 1) == -1)
    {
        perror("unlock");
        exit(EXIT_FAILURE);
    }
}

/* Initialization of the FIFO */
void fifo_create(void)
{   
    shmid = shmget(key, sizeof(FIFO), 0600 | IPC_CREAT | IPC_EXCL);
    
    if (shmid == -1)
    {
        perror("Fail creating shared data");
        exit(EXIT_FAILURE);
    }

    /* attach shared memory to process addressing space */ 
    fifo = shmat(shmid, NULL, 0);
    if (fifo == (void*)-1)
    {
        perror("Fail connecting to shared data");
        exit(EXIT_FAILURE);
    }

    /* create access locker */
    fifo->semid = semget(key, 3, 0600 | IPC_CREAT | IPC_EXCL);
    if (fifo->semid == -1)
    {
        perror("Fail creating locker semaphore");
        exit(EXIT_FAILURE);
    }

    unlock();
    for (int i=0;i<FIFOSZ; i++)
        up_emptyness();


    unsigned int i;
    for (i = 0; i < FIFOSZ; i++)
    {
        fifo->mem[i].id = 99999999;
        fifo->mem[i].value = 99999999;
    }
    fifo->ii = fifo->ri = 0;
    fifo->cnt = 0;

    /* detach shared memory from process addressing space */
    shmdt(fifo);
    fifo = NULL;

}

void fifo_connect()
{
    /* get access to the shared memory */
    shmid = shmget(key, 0, 0);
    if (shmid == -1)
    {
        perror("Fail connecting to shared data");
        exit(EXIT_FAILURE);
    }

    /* attach shared memory to process addressing space */ 
    fifo = shmat(shmid, NULL, 0);
    if (fifo == (void*)-1)
    {
        perror("Fail connecting to shared data");
        exit(EXIT_FAILURE);
    }
}

void fifo_destroy()
{

    /* destroy the locker semaphore */
    semctl(fifo->semid, 0, IPC_RMID, NULL);

    /* detach shared memory from process addressing space */
    shmdt(fifo);
    fifo = NULL;

    /* ask OS to destroy the shared memory */
    shmctl(shmid, IPC_RMID, NULL);
    shmid = -1;
}

/* ************************************************* */

/* Insertion of a pait <id, value> into the FIFO  */
void fifoIn(unsigned int id, unsigned int value)
{
    down_emptyness();
    lock();
    /* Insert pair */
    fifo->mem[fifo->ii].value = value;
    gaussianDelay(1, 0.5);
    fifo->mem[fifo->ii].id = id;
    fifo->ii = (fifo->ii + 1) % FIFOSZ;
    fifo->cnt++;
    unlock();
    up_fullness();
}

/* ************************************************* */

/* Retrieval of a pair <id, value> from the FIFO */

void fifoOut (unsigned int * idp, unsigned int * valuep)
{
    down_fullness();
    lock();
    /* Retrieve pair */
    *valuep = fifo->mem[fifo->ri].value;
    fifo->mem[fifo->ri].value = 99999999;
    *idp = fifo->mem[fifo->ri].id;
    fifo->mem[fifo->ri].id = 99999999;
    fifo->ri = (fifo->ri + 1) % FIFOSZ;
    fifo->cnt--;
    unlock();
    up_emptyness();
}

/* ************************************************* */

