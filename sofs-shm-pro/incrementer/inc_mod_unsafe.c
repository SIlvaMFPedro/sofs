/*
 * A simple module
 *
 * A very simple module, with an internal data structure
 * and 3 manipulation functions.
 * The data structure is just a pair of integer variable.
 * The 3 functions are to:
 * - set the variable values;
 * - get the variable values;
 * - increment the variable values.
 *
 * \author (2016) Artur Pereira <artur at ua.pt>
 */

#include "bwdelay.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>

const long key = 0x1111L;

// Se tempo aumentado, passa a ser safe
/* time delay length */
#define SET_TIME 500
#define GET_TIME 500
#define INC_TIME 500

/* Internal data structure */
typedef struct
{
    int var1, var2;
} SharedDataType;

static int shmid = -1;
static SharedDataType * p = NULL;

/* manipulation functions */

void v_create()
{
    /* create the shared memory */
    shmid = shmget(key, sizeof(SharedDataType), 0600 | IPC_CREAT | IPC_EXCL);
    if (shmid == -1)
    {
        perror("Fail creating shared data");
        exit(EXIT_FAILURE);
    }
}

void v_connect()
{
    /* get access to the shared memory */
    shmid = shmget(key, 0, 0);
    if (shmid == -1)
    {
        perror("Fail connecting to shared data");
        exit(EXIT_FAILURE);
    }

    /* attach shared memory to process addressing space */ 
    p = shmat(shmid, NULL, 0);
    if (p == (void*)-1)
    {
        perror("Fail connecting to shared data");
        exit(EXIT_FAILURE);
    }
}

void v_destroy()
{
    /* detach shared memory from process addressing space */
    shmdt(p);
    p = NULL;

    /* ask OS to destroy the shared memory */
    shmctl(shmid, IPC_RMID, NULL);
    shmid = -1;
}

/* set shared data with new values */
void v_set(int value1, int value2)
{
	p->var1 = value1;
    bwDelay(SET_TIME);
	p->var2 = value2;
}

/* get current values of shared data */
void v_get(int * value1p, int * value2p)
{
	*value1p = p->var1;
    bwDelay(GET_TIME);
	*value2p = p->var2;
}

/* increment both variables of the shared data */
void v_inc(void)
{
    p->var1 = p->var1 + 1;
    bwDelay(INC_TIME);
    p->var2 = p->var2 + 1;
}

