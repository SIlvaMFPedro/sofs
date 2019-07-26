/**
 * @file
 *
 * \brief A producer-consumer application, implemented using processes.
 *
 * \remarks The return status of the threads are ignored
 *
 * \author (2016) Artur Pereira <artur at ua.pt>
 */

#include  <stdio.h>
#include  <stdlib.h>
#include  <libgen.h>
#include  <unistd.h>
#include  <sys/wait.h>
#include  <sys/types.h>
#include  <pthread.h>
#include  <math.h>

#include  "fifo.h"
#include  "delays.h"

#define USAGE "Synopsis: %s [options]\n"\
    "\t----------+--------------------------------------------\n"\
    "\t  Option  |          Description                       \n"\
    "\t----------+--------------------------------------------\n"\
    "\t -i num   | number of iterations (dfl: 10; max: 100)   \n"\
    "\t -p num   | number of 'processes' (dfl: 4; max 100)    \n"\
    "\t -h       | this help                                  \n"\
    "\t----------+--------------------------------------------\n"

/* Argument value for producer and consumer processes */
typedef struct
{
    unsigned int id;      ///< process id
    unsigned int niter;   ///< number of iterations
} ARGV;

/* launcher of a process to run a given routine */
void proc_create(int * pidp, void* (*routine)(void*), ARGV *arg)
{
    int pid = fork();
    switch (pid)
    {
        case -1:
            fprintf(stderr, "Fail launching process\n");
            exit(EXIT_FAILURE);

        case 0:
            break;

        default:
            *pidp = pid;
            return;
    }

    /* child side: run given routine */
    routine(arg);
}


/* ******************************************************* */

/* The producer thread */
void *producer(void *argp)
{
    /* cast argument to real type */
    ARGV* argv = (ARGV*)argp;

    /* make the job */
    unsigned int i;
    for (i = 0; i < argv->niter; i++)
    {
        /* retrieve an item from the fifo */
        unsigned int id = argv->id;
        unsigned int value = i * 10000 + id;
        fifoIn(id, value);

        /* do something else */
        gaussianDelay(10, 5);

        /* print them */
        printf("\e[32;01mThe value %05u was produced by thread P%u!\e[0m\n", value, id);
    }

    printf("Producer %u is quiting\n", argv->id);
    exit (EXIT_SUCCESS);
}

/* ******************************************************* */

/* The consumer thread */
void *consumer(void *argp)
{
    /* cast argument to real type */
    ARGV* argv = (ARGV*)argp;

    /* make the job */
    unsigned int i;
    for (i = 0; i < argv->niter; i++)
    {
        /* do something else */
        gaussianDelay(10, 5);

        /* retrieve an item from the fifo */
        unsigned int pid, value;
        fifoOut(&pid, &value);

        /* print them */
        if (value == 99999999 || pid == 99999999 || (value % 100) != pid)
            printf("\e[31;01mThe value %05u was produced by thread P%u and consumed by thread C%u!\e[0m\n",
                    value, pid, argv->id);
        else
            printf("\e[34;01mThe value %05u was produced by thread P%u and consumed by thread C%u!\e[0m\n",
                    value, pid, argv->id);
    }

    printf("Consumer %u is quiting\n", argv->id);
    exit (EXIT_SUCCESS);
}

/* ******************************************************* */

/*   main thread: it starts the simulation and generates the producer and consumer threads */
int main(int argc, char *argv[])
{
    /* */
    int niter = 10;  /* default number of iterations */
    int nproc = 4;      /* default number of processes */

    /* processamento da linha de comando */
    const char *optstr = "i:p:h";

    int option;
    while ((option = getopt(argc, argv, optstr)) != -1)
    {
        switch (option)
        {
            case 'i':
                niter = atoi(optarg);
                break;

            case 'p':
                nproc = atoi(optarg);
                break;

            case 'h':
                printf(USAGE, basename(argv[0]));
                return 0;
                break;

            default:
                fprintf(stderr, "Opção não válida\n");
                fprintf(stderr, USAGE, basename(argv[0]));
                return 1;
        }
    }

    /* create the shared data structure */
    fifo_create();
    /* connect to the shared data structure */
    fifo_connect();
    
    /* start random generator */
    srand(getpid());

    /* launching the consumers */
    int cthr[nproc];   /* consumers' ids */
    ARGV carg[nproc];        /* consumers' args */
    printf("Launching %d consumer threads, each performing %d iterations\n", nproc, niter);
    unsigned int i;
    for (i = 0; i < nproc; i++)
    {	
        carg[i].id = i;
        carg[i].niter = niter;
        proc_create(&cthr[i], consumer, &carg[i]);
        printf("Process %d launched\n", cthr[i]);
    }

    /* launching the producers */
    int pthr[nproc];   /* producers' ids */
    ARGV parg[nproc];        /* producers' args */
    printf("Launching %d producer threads, each performing %d iterations\n", nproc, niter);
    //unsigned int id;
    for (i = 0; i < nproc; i++)
    {
        parg[i].id = i;
        parg[i].niter = niter;
        proc_create(&pthr[i], producer, &parg[i]);
        printf("Process %d launched\n", pthr[i]);
    }

    /* wait for processes to conclude */
    int status[nproc];
    for (i = 0; i < nproc; i++)
    {
        waitpid(pthr[i], &status[i], 0);
        printf("Process %d returned\n", pthr[i]);
        
    }

    for (i = 0; i < nproc; i++)
    {
        waitpid(cthr[i], &status[i], 0);
        printf("Process %d returned\n", cthr[i]);
    }
    
    fifo_destroy();

    return EXIT_SUCCESS;
}

