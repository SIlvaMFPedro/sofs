/*
 *  @brief A simple FIFO, whose elements are pairs of integers,
 *      one being the id of the producer and the other the value produced
 *
 * @remarks safe, bust waiting version
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
#include <pthread.h>

#include "fifo.h"
#include "delays.h"

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
static FIFO fifo;

/** \brief locking flag which warrants mutual exclusion inside the monitor */
static pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER, not_full = PTHREAD_COND_INITIALIZER;

/* ************************************************* */

/* Initialization of the FIFO */
void fifoInit(void)
{
    pthread_mutex_lock(&accessCR);

    unsigned int i;
    for (i = 0; i < FIFOSZ; i++)
    {
        fifo.mem[i].id = 99999999;
        fifo.mem[i].value = 99999999;
    }
    fifo.ii = fifo.ri = 0;
    fifo.cnt = 0;

    pthread_mutex_unlock(&accessCR);
}

/* ************************************************* */

/* Check if FIFO is full */
static bool fifoFull(void)
{
    return fifo.cnt == FIFOSZ;
}

/* ************************************************* */

/* Check if FIFO is empty */
static bool fifoEmpty(void)
{
    return fifo.cnt == 0;
}

/* ************************************************* */

/* Insertion of a pait <id, value> into the FIFO  */
void fifoIn(unsigned int id, unsigned int value)
{
    pthread_mutex_lock(&accessCR);

    while (fifoFull()){
        pthread_cond_wait(&not_full, &accessCR);
    }

    /* wait while fifo is full */
    /*while (fifoFull())
    {
        pthread_mutex_unlock(&accessCR);
        usleep(1000);
        pthread_mutex_lock(&accessCR);
    }*/

    /* Insert pair */
    fifo.mem[fifo.ii].value = value;
    gaussianDelay(1, 0.5);
    fifo.mem[fifo.ii].id = id;
    fifo.ii = (fifo.ii + 1) % FIFOSZ;
    fifo.cnt++;

    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&accessCR);
}

/* ************************************************* */

/* Retrieval of a pair <id, value> from the FIFO */

void fifoOut (unsigned int * idp, unsigned int * valuep)
{
    pthread_mutex_lock(&accessCR);

    /* wait while fifo is empty */
    /*while (fifoEmpty())
    {
        pthread_mutex_unlock(&accessCR);
        usleep(1000);
        pthread_mutex_lock(&accessCR);
    }*/


    while (fifoEmpty()){
        pthread_cond_wait(&not_empty, &accessCR);
    }


    /* Retrieve pair */
    *valuep = fifo.mem[fifo.ri].value;
    fifo.mem[fifo.ri].value = 99999999;
    *idp = fifo.mem[fifo.ri].id;
    fifo.mem[fifo.ri].id = 99999999;
    fifo.ri = (fifo.ri + 1) % FIFOSZ;
    fifo.cnt--;

    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&accessCR);
}

/* ************************************************* */

