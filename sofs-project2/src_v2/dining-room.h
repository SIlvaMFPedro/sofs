/**
 * \brief Dining room data structures
 *  
 * \author Miguel Oliveira e Silva - 2016
 */

#ifndef DINING_ROOM_H
#define DINING_ROOM_H

#include "simulation.h"


typedef struct _DiningRoom_ {
   int pizza;                  // number of pizza meals available in dining room [0;NUM_PIZZA]
   int spaghetti;              // number of spaghetti meals available in dining room [0;NUM_SPAGHETTI]
   int cleanForks;             // number of clean forks available in dining room [0;NUM_FORKS]
   int cleanKnives;            // number of clean knives available in dining room [0;NUM_KNIVES]
   int dirtyForks;             // number of dirty forks in dining room [0;NUM_FORKS]
   int dirtyKnives;            // number of dirty knives in dining room [0;NUM_KNIVES]
   int dirtyForksInWaiter;     // number of dirty forks in waiter (i.e. the dirty forks that are being washed)
   int dirtyKnivesInWaiter;    // number of dirty knives in waiter (i.e. the dirty knives that are being washed)
   int dead_philosophers; 	   // number of dead philosophers
} DiningRoom;

typedef struct _Threads_ {
	pthread_cond_t *philosophers_cond;		/* ponto de sincronizaçao entre os filosofos */
	pthread_once_t initial;					/* flag de inicialização */
	pthread_cond_t wakeup_waiter;/* variavel de condicao que controla a utilizacao do waiter */

	pthread_mutex_t waiter; /* mutex que controla a utilizacao do waiter */

	pthread_mutex_t accessPizza; /* mutex que controla o acesso a pizza */
	pthread_mutex_t accessSpaghetti; /* mutex que controla o acesso ao esparguete */
	pthread_mutex_t accessForks; /* mutex que controla o acesso aos garfos */
	pthread_mutex_t accessKnives; /* mutex que controla o acesso as facas */
	pthread_mutex_t accessDirtyForks; /* mutex que controla o acesso aos garfos sujos */
	pthread_mutex_t accessDirtyKnives; /* mutex que controla o acesso as facas sujas */
	pthread_mutex_t accessKill; /* mutex que controla o acesso a matar o filosofo ahahah */

	pthread_mutex_t accessReqPizza; /* mutex que controla o acesso aos pedidos de pizza */
	pthread_mutex_t accessReqSpaghetti; /* mutex que controla o acesso aos pedidos de spaghetti */
	pthread_mutex_t accessReqCutlery; /* mutex que controla o acesso aos pedidos de limpeza dos talheres */
 	pthread_mutex_t pizza_requests_without_need; /* mutex que controla os pedidos de pizza para os filosofos seguintes */
 	pthread_mutex_t spaghetti_requests_without_need; /* mutex que controla os pedidos de spaghetti para os filosofos seguintes */
	pthread_mutex_t cutlery_requests_without_need; /* mutex que controla os pedidos de cutlery para os filosofos seguintes */

}Threads;

void init_philosophers(void);

void init_dinner(void);

void diningRoom_init(void);

void init_pizza(void);

void init_spaghetti(void);

void init_forks(void);

void init_knives(void);

/* Philospher actions */

void get_pizza(int id);

void get_spaghetti(int id);

void get_two_forks(int id);

void get_fork_knife(int id);

void drop_two_forks(int id);

void drop_fork_knife(int id);


/* Waiter Actions */

void get_cutlery(void);

void replenish_cutlery(int * cleans);

void add_pizza(void);

void add_spaghetti(void);

/* Actions to be performed on philosophers */

void kill_phil(int id);

void signal_philosopher(int id);

void wait_philosopher(int id, pthread_mutex_t* access);

/* Actions to be performed on the waiter */

void signal_waiter(void);

void wait_waiter(void);

#endif

