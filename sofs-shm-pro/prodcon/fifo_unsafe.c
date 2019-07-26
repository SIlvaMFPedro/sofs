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
} FIFO;

/** \brief internal storage region of FIFO type */
static FIFO * fifo;

/* ************************************************* */

/* Initialization of the FIFO */
void fifo_create(void)
{   
    fifo = (FIFO*) malloc (sizeof(FIFO));
    shmid = shmget(key, sizeof(FIFO), 0600 | IPC_CREAT | IPC_EXCL);
    
    if (shmid == -1)
    {
        perror("Fail creating shared data");
        exit(EXIT_FAILURE);
    }

    unsigned int i;
    for (i = 0; i < FIFOSZ; i++)
    {
        fifo->mem[i].id = 99999999;
        fifo->mem[i].value = 99999999;
    }
    fifo->ii = fifo->ri = 0;
    fifo->cnt = 0;

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
    /* detach shared memory from process addressing space */
    shmdt(fifo);
    fifo = NULL;

    /* ask OS to destroy the shared memory */
    shmctl(shmid, IPC_RMID, NULL);
    shmid = -1;
}

/* ************************************************* */

/* Check if FIFO is full */
static bool fifoFull(void)
{
    return fifo->cnt == FIFOSZ;
}

/* ************************************************* */

/* Check if FIFO is empty */
static bool fifoEmpty(void)
{
    return fifo->cnt == 0;
}

/* ************************************************* */

/* Insertion of a pait <id, value> into the FIFO  */
void fifoIn(unsigned int id, unsigned int value)
{
    /* wait while fifo is full */
    while (fifoFull())
    {
        usleep(1000);
    }
    /* Insert pair */
    fifo->mem[fifo->ii].value = value;
    gaussianDelay(1, 0.5);
    fifo->mem[fifo->ii].id = id;
    fifo->ii = (fifo->ii + 1) % FIFOSZ;
    fifo->cnt++;
}

/* ************************************************* */

/* Retrieval of a pair <id, value> from the FIFO */

void fifoOut (unsigned int * idp, unsigned int * valuep)
{
    /* wait while fifo is empty */
    while (fifoEmpty())
    {
        usleep(1001);
    }

    /* Retrieve pair */
    *valuep = fifo->mem[fifo->ri].value;
    fifo->mem[fifo->ri].value = 99999999;
    *idp = fifo->mem[fifo->ri].id;
    fifo->mem[fifo->ri].id = 99999999;
    fifo->ri = (fifo->ri + 1) % FIFOSZ;
    fifo->cnt--;
}

/* ************************************************* */

