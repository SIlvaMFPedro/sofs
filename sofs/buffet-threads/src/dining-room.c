/**
 * \brief Dining room
 *  
 * \author Miguel Oliveira e Silva - 2016
 		   Eduardo Reis Silva nºmec. 76354
 		   Nuno Filipe Sousa Capela nº mec. 76385
 		   Pedro Marques Ferreira da Silva nºmec. 72645
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>
#include <string.h>
#include <locale.h>
#include "parameters.h"
#include "dining-room.h"
#include "philosopher.h"
#include "simulation.h"
#include "waiter.h"
#include "logger.h"

#include <pthread.h>


extern Simulation* sim;

static pthread_cond_t *philosophers_cond;		/* array de variaveis de condição que controlam o adormecer e acordar cada filosofo */
static pthread_once_t initial = PTHREAD_ONCE_INIT;					/* flag de inicialização */
static pthread_cond_t wakeup_waiter = PTHREAD_COND_INITIALIZER;/* variavel de condicao que controla a utilizacao do waiter */

static pthread_mutex_t waiter = PTHREAD_MUTEX_INITIALIZER; /* mutex que controla a utilizacao do waiter */

static pthread_mutex_t accessPizza = PTHREAD_MUTEX_INITIALIZER; /* mutex que controla o acesso a pizza */
static pthread_mutex_t accessSpaghetti = PTHREAD_MUTEX_INITIALIZER; /* mutex que controla o acesso ao esparguete */
static pthread_mutex_t accessForks = PTHREAD_MUTEX_INITIALIZER; /* mutex que controla o acesso aos garfos */
static pthread_mutex_t accessKnives = PTHREAD_MUTEX_INITIALIZER; /* mutex que controla o acesso as facas */
static pthread_mutex_t accessDirtyForks = PTHREAD_MUTEX_INITIALIZER; /* mutex que controla o acesso aos garfos sujos */
static pthread_mutex_t accessDirtyKnives = PTHREAD_MUTEX_INITIALIZER; /* mutex que controla o acesso as facas sujas */
static pthread_mutex_t accessKill = PTHREAD_MUTEX_INITIALIZER; /* mutex que controla o acesso a matar o filosofo ahahah */

void init_philosophers(void){
	unsigned int phil;

	for(phil=0; phil < sim->params->NUM_PHILOSOPHERS; phil++){
		sim->philosophers[phil]->state = P_BIRTH;
		sim->philosophers[phil]->meal = P_NONE;
		sim->philosophers[phil]->cutlery[0] = P_NOTHING;
		sim->philosophers[phil]->cutlery[1] = P_NOTHING;
	}

}

void init_dinner(void){

	init_knives();
   	init_forks();
   	init_spaghetti();
   	init_pizza();
   	init_kill();

   	philosophers_cond = (pthread_cond_t*)mem_alloc(sizeof(pthread_cond_t)*sim->params->NUM_PHILOSOPHERS);

	unsigned int phil;

	for(phil=0; phil < sim->params->NUM_PHILOSOPHERS; phil++){
		sim->philosophers[phil]->state = P_THINKING;	/* os filosofos começam a pensar */
		sim->philosophers[phil]->meal = P_NONE;
		sim->philosophers[phil]->cutlery[0] = P_NOTHING;
		sim->philosophers[phil]->cutlery[1] = P_NOTHING;
		pthread_cond_init(&philosophers_cond[phil], NULL);

	}

	sim->waiter->state = W_SLEEP;
	sim->waiter->reqCutlery = W_INACTIVE;
	sim->waiter->reqPizza = W_INACTIVE;
	sim->waiter->reqSpaghetti = W_INACTIVE;

}

void init_pizza(void){
	sim->diningRoom->pizza = sim->params->NUM_PIZZA;
	pthread_mutex_unlock(&accessPizza);
}

void init_spaghetti(void){
	sim->diningRoom->spaghetti = sim->params->NUM_SPAGHETTI;
	pthread_mutex_unlock(&accessSpaghetti);
}

void init_forks(void){
	sim->diningRoom->cleanForks = sim->params->NUM_FORKS;
	sim->diningRoom->dirtyForks = 0;
	sim->diningRoom->dirtyForksInWaiter = 0;
	pthread_mutex_unlock(&accessForks);
	pthread_mutex_unlock(&accessDirtyForks);
}

void init_knives(void){
	sim->diningRoom->cleanKnives = sim->params->NUM_KNIVES;
	sim->diningRoom->dirtyKnives = 0;
	sim->diningRoom->dirtyKnivesInWaiter = 0;
	pthread_mutex_unlock(&accessKnives);
	pthread_mutex_unlock(&accessDirtyKnives);
}

void init_kill(void){
	sim->diningRoom->dead_philosophers = 0;
	pthread_mutex_unlock(&accessKill);
}


void get_pizza(int id){
	pthread_once (&initial, init_dinner); /* inicializa as estruturas de dados, se for o primeiro acesso */

	/*se não houver pizzas disponíveis, pedir ao waiter */
	if(sim->diningRoom->pizza < 1){
		/* fazer pedido ao waiter*/
		request_pizza(id);
		pthread_mutex_lock(&accessPizza);

		/* acordar o waiter */
		signal_waiter();

		/*adormercer filosofo ate o waiter o acordar*/
		wait_philosopher(id, &accessPizza);
		pthread_mutex_unlock(&accessPizza);
		return get_pizza(id);
	}
	pthread_mutex_lock(&accessPizza);

	/*enquanto esperamos que o mutex fique unlock, as pizzas podem esgotar*/
	if(sim->diningRoom->pizza < 1){
		pthread_mutex_unlock(&accessPizza);
		return get_pizza(id);
	}
	sim->diningRoom->pizza-=1;

	/*se não sobrarem pizzas, pedir ao waiter, mas sem ficar a espera */
	if(sim->diningRoom->pizza < 1){
		request_pizza_without_need();
		signal_waiter();
	}

	logger(sim);
	pthread_mutex_unlock(&accessPizza);
}



void get_spaghetti(int id){
	pthread_once (&initial, init_dinner); /* inicializa as estruturas de dados, se for o primeiro acesso */

	/*se não houver esparguete disponível, pedir ao waiter */
	if(sim->diningRoom->spaghetti < 1){
		/* fazer pedido ao waiter*/
		request_spaghetti(id);
		pthread_mutex_lock(&accessSpaghetti);

		/* acordar o waiter */
		signal_waiter();

		/*adormercer filosofo ate o waiter o acordar*/
		wait_philosopher(id, &accessSpaghetti);
		pthread_mutex_unlock(&accessSpaghetti);
		return get_spaghetti(id);
	}
	pthread_mutex_lock(&accessSpaghetti);

	/*enquanto esperamos que o mutex fique unlock, o esparguete pode esgotar*/
	if(sim->diningRoom->spaghetti < 1){
		pthread_mutex_unlock(&accessSpaghetti);
		return get_spaghetti(id);
	}
	sim->diningRoom->spaghetti-=1;

	/*se não sobrar esparguete, pedir ao waiter, mas sem ficar a espera */
	if(sim->diningRoom->spaghetti < 1){
		request_spaghetti_without_need();
		signal_waiter();
	}

	logger(sim);
	pthread_mutex_unlock(&accessSpaghetti);
}

void get_two_forks(int id){
	pthread_once (&initial, init_dinner); /* inicializa as estruturas de dados, se for o primeiro acesso */

	/*se não houver 2 garfos disponíveis, pedir ao waiter */
	if(sim->diningRoom->cleanForks < 2){
		/* fazer pedido ao waiter*/
	   	request_cutlery(id,2,1);
	   	pthread_mutex_lock(&accessForks);

	    /* acordar o waiter */
	    signal_waiter();

	    /*adormercer filosofo ate o waiter o acordar*/
	    wait_philosopher(id, &accessForks);
	    pthread_mutex_unlock(&accessForks);
		return get_two_forks(id);
	}
	pthread_mutex_lock(&accessForks);

	/*enquanto esperamos que o mutex fique unlock, os 2 garfos podem ser retirados*/
	if(sim->diningRoom->cleanForks < 2){
		pthread_mutex_unlock(&accessForks);
		return get_two_forks(id);
	}
	sim->diningRoom->cleanForks-=2;
    sim->philosophers[id]->cutlery[0] = P_FORK;
    sim->philosophers[id]->cutlery[1] = P_FORK;

    /*se não sobrar pelo menos 2 garfos, pedir ao waiter, mas sem ficar a espera */
    if (sim->diningRoom->cleanForks<=1){
    	request_cutlery_without_need();
        signal_waiter();
    }

    logger(sim);
    pthread_mutex_unlock(&accessForks);
}



void get_fork_knife(int id){
	pthread_once (&initial, init_dinner); /* inicializa as estruturas de dados, se for o primeiro acesso */

	/*se não houver 1 garfo disponível, pedir ao waiter */
	if(sim->diningRoom->cleanForks < 1){
		/* fazer pedido ao waiter*/
	   	request_cutlery(id,1,1);
	   	pthread_mutex_lock(&accessForks);

	    /* acordar o waiter */
	    signal_waiter();

	    /*adormercer filosofo ate o waiter o acordar*/
	    wait_philosopher(id, &accessForks);
	    pthread_mutex_unlock(&accessForks);
		return get_fork_knife(id);
	}

	/*se não houver 1 faca disponível, pedir ao waiter */
	if(sim->diningRoom->cleanKnives < 1){
		/* fazer pedido ao waiter*/
	    request_cutlery(id,1,0);
	    pthread_mutex_lock(&accessKnives);
	    
	    /* acordar o waiter */
	    signal_waiter();

	    /*adormercer filosofo ate o waiter o acordar*/
	    wait_philosopher(id, &accessKnives);
	    pthread_mutex_unlock(&accessKnives);
		return get_fork_knife(id);
	}
	pthread_mutex_lock(&accessForks);
	pthread_mutex_lock(&accessKnives);

	/*enquanto esperamos que o mutex fique unlock, o garfo ou/e a faca podem ser retirados*/
	if(sim->diningRoom->cleanForks < 1 || sim->diningRoom->cleanKnives < 1){
		pthread_mutex_unlock(&accessForks);
		pthread_mutex_unlock(&accessKnives);	
		return get_fork_knife(id);
	}
	sim->diningRoom->cleanKnives-=1;
	sim->diningRoom->cleanForks-=1;
    sim->philosophers[id]->cutlery[0] = P_FORK;
    sim->philosophers[id]->cutlery[1] = P_KNIFE;

    /*se não sobrar pelo menos 2 garfos e 1 faca, pedir ao waiter, mas sem ficar a espera */
    if (sim->diningRoom->cleanForks<=1 || sim->diningRoom->cleanKnives==0){
        request_cutlery_without_need();
       	signal_waiter();     
    }

    logger(sim);
    pthread_mutex_unlock(&accessForks);
   	pthread_mutex_unlock(&accessKnives);

}

void drop_two_forks(int id){
	pthread_mutex_lock(&accessDirtyForks);

	sim->diningRoom->dirtyForks+=2;
	sim->philosophers[id]->cutlery[0] = P_PUT_FORK;
	sim->philosophers[id]->cutlery[1] = P_PUT_FORK;
	logger(sim);

	pthread_mutex_unlock(&accessDirtyForks);
}


void drop_fork_knife(int id){
	pthread_mutex_lock(&accessDirtyForks);
	pthread_mutex_lock(&accessDirtyKnives);

	sim->diningRoom->dirtyForks+=1;
	sim->diningRoom->dirtyKnives+=1;
	sim->philosophers[id]->cutlery[0] = P_PUT_FORK;
	sim->philosophers[id]->cutlery[1] = P_PUT_KNIFE;
	logger(sim);

	pthread_mutex_unlock(&accessDirtyForks);
	pthread_mutex_unlock(&accessDirtyKnives);
}


void get_cutlery(){
	pthread_mutex_lock(&accessDirtyForks);
	pthread_mutex_lock(&accessDirtyKnives);

	sim->diningRoom->dirtyForksInWaiter += sim->diningRoom->dirtyForks;
	sim->diningRoom->dirtyForks = 0;
	sim->diningRoom->dirtyKnivesInWaiter += sim->diningRoom->dirtyKnives;
	sim->diningRoom->dirtyKnives = 0;
	logger(sim);

	pthread_mutex_unlock(&accessDirtyForks);
	pthread_mutex_unlock(&accessDirtyKnives);
}

void replenish_cutlery(int *cleans){
	pthread_mutex_lock(&accessForks);
	pthread_mutex_lock(&accessKnives);

	sim->diningRoom->cleanForks += sim->diningRoom->dirtyForksInWaiter;
	sim->diningRoom->dirtyForksInWaiter = 0;
	sim->diningRoom->cleanKnives += sim->diningRoom->dirtyKnivesInWaiter;
	sim->diningRoom->dirtyKnivesInWaiter = 0;
	logger(sim);

	cleans[FORK] = sim->diningRoom->cleanForks;
	cleans[KNIFE] = sim->diningRoom->cleanKnives;

	pthread_mutex_unlock(&accessForks);
	pthread_mutex_unlock(&accessKnives);

}

void add_pizza(){
	pthread_mutex_lock(&accessPizza);
	if(sim->diningRoom->pizza==0){
		sim->diningRoom->pizza += sim->params->NUM_PIZZA;
	}
	logger(sim);
	pthread_mutex_unlock(&accessPizza);
}
	
void add_spaghetti(){
	pthread_mutex_lock(&accessSpaghetti);
	if(sim->diningRoom->spaghetti==0){
		sim->diningRoom->spaghetti += sim->params->NUM_SPAGHETTI;
	}
	logger(sim);
	pthread_mutex_unlock(&accessSpaghetti);
}

void kill_phil(int id){
	pthread_mutex_lock(&accessKill);

	sim->diningRoom->dead_philosophers+=1;

	int t=0;
	/* ver se ainda existem filosofos vivos*/
	if(sim->diningRoom->dead_philosophers==sim->params->NUM_PHILOSOPHERS)
        t=1;
    
    pthread_mutex_unlock(&accessKill);

    /* se não houver filosofos vivos, acordar o waiter para limpar a cozinha */
    if (t==1)
         pthread_cond_signal(&wakeup_waiter);  
}

void signal_waiter(void){
	pthread_cond_signal(&wakeup_waiter);
}

void wait_waiter(void){
	pthread_mutex_lock(&waiter);
	pthread_cond_wait(&wakeup_waiter, &waiter);
	unlock_waiter();
}

void unlock_waiter(void){
	pthread_mutex_unlock(&waiter);
}

void signal_philosopher(int id){
	pthread_cond_signal(&philosophers_cond[id]);

}

void wait_philosopher(int id, pthread_mutex_t* access){
	pthread_cond_wait(&philosophers_cond[id], access);
}

