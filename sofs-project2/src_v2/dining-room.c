/**
 * \brief Dining room
 *  
 * \author Miguel Oliveira e Silva - 2016
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

void init_philosophers(void){
	unsigned int phil;

	for(phil=0; phil < sim->params->NUM_PHILOSOPHERS; phil++){
		sim->philosophers[phil]->state = P_BIRTH;	/* antes de iniciar o dinner primeiro temos de criar os filosofos */
		sim->philosophers[phil]->meal = P_NONE;
		sim->philosophers[phil]->cutlery[0] = P_NOTHING;
		sim->philosophers[phil]->cutlery[1] = P_NOTHING;
	}

}

void init_dinner(void){
	//init_threads();

	init_knives();
   	init_forks();
   	init_spaghetti();
   	init_pizza();

   	sim->threads->philosophers_cond = (pthread_cond_t*)mem_alloc(sizeof(pthread_cond_t)*sim->params->NUM_PHILOSOPHERS);

	diningRoom_init();

	unsigned int phil;

	for(phil=0; phil < sim->params->NUM_PHILOSOPHERS; phil++){
		sim->philosophers[phil]->state = P_THINKING;	/* os filosofos começam a pensar */
		sim->philosophers[phil]->meal = P_NONE;
		sim->philosophers[phil]->cutlery[0] = P_NOTHING;
		sim->philosophers[phil]->cutlery[1] = P_NOTHING;
		pthread_cond_init(&sim->threads->philosophers_cond[phil], NULL);

	}

	sim->waiter->state = W_SLEEP;
	sim->waiter->reqCutlery = W_INACTIVE;
	sim->waiter->reqPizza = W_INACTIVE;
	sim->waiter->reqSpaghetti = W_INACTIVE;
}

void diningRoom_init(void){
	pthread_mutex_unlock(&sim->threads->accessReqPizza);
	pthread_mutex_unlock(&sim->threads->accessReqSpaghetti);
	pthread_mutex_unlock(&sim->threads->accessReqCutlery);
	pthread_mutex_unlock(&sim->threads->pizza_requests_without_need);
	pthread_mutex_unlock(&sim->threads->spaghetti_requests_without_need);
	pthread_mutex_unlock(&sim->threads->cutlery_requests_without_need);
	pthread_mutex_unlock(&sim->threads->accessKill);

}

/*void init_threads(void){
	pthread_cond_t wakeup_waiter = PTHREAD_COND_INITIALIZER;// variavel de condicao que controla a utilizacao do waiter 

	pthread_mutex_t waiter = PTHREAD_MUTEX_INITIALIZER; // mutex que controla a utilizacao do waiter 

	pthread_mutex_t accessPizza = PTHREAD_MUTEX_INITIALIZER; // mutex que controla o acesso a pizza 
	pthread_mutex_t accessSpaghetti = PTHREAD_MUTEX_INITIALIZER; // mutex que controla o acesso ao esparguete 
	pthread_mutex_t accessForks = PTHREAD_MUTEX_INITIALIZER; // mutex que controla o acesso aos garfos 
	pthread_mutex_t accessKnives = PTHREAD_MUTEX_INITIALIZER; // mutex que controla o acesso as facas
	pthread_mutex_t accessDirtyForks = PTHREAD_MUTEX_INITIALIZER; // mutex que controla o acesso aos garfos sujos 
	pthread_mutex_t accessDirtyKnives = PTHREAD_MUTEX_INITIALIZER; // mutex que controla o acesso as facas sujas 
	pthread_mutex_t accessKill = PTHREAD_MUTEX_INITIALIZER; // mutex que controla o acesso a matar o filosofo ahahah 

	pthread_mutex_t accessReqPizza = PTHREAD_MUTEX_INITIALIZER; // mutex que controla o acesso aos pedidos de pizza 
	pthread_mutex_t accessReqSpaghetti = PTHREAD_MUTEX_INITIALIZER; // mutex que controla o acesso aos pedidos de spaghetti 
	pthread_mutex_t accessReqCutlery = PTHREAD_MUTEX_INITIALIZER; // mutex que controla o acesso aos pedidos de limpeza dos talheres 
 	pthread_mutex_t pizza_requests_without_need = PTHREAD_MUTEX_INITIALIZER; // mutex que controla os pedidos de pizza para os filosofos seguintes 
 	pthread_mutex_t spaghetti_requests_without_need = PTHREAD_MUTEX_INITIALIZER; // mutex que controla os pedidos de spaghetti para os filosofos seguintes 
	pthread_mutex_t cutlery_requests_without_need = PTHREAD_MUTEX_INITIALIZER; // mutex que controla os pedidos de cutlery para os filosofos seguintes 
}*/

void init_pizza(){
	sim->diningRoom->pizza = sim->params->NUM_PIZZA;
	pthread_mutex_unlock(&sim->threads->accessPizza);
}

void init_spaghetti(){
	sim->diningRoom->spaghetti = sim->params->NUM_SPAGHETTI;
	pthread_mutex_unlock(&sim->threads->accessSpaghetti);
}

void init_forks(){
	sim->diningRoom->cleanForks = sim->params->NUM_FORKS;
	sim->diningRoom->dirtyForks = 0;
	sim->diningRoom->dirtyForksInWaiter = 0;
	pthread_mutex_unlock(&sim->threads->accessForks);
	pthread_mutex_unlock(&sim->threads->accessDirtyForks);
}

void init_knives(){
	sim->diningRoom->cleanKnives = sim->params->NUM_KNIVES;
	sim->diningRoom->dirtyKnives = 0;
	sim->diningRoom->dirtyKnivesInWaiter = 0;
	pthread_mutex_unlock(&sim->threads->accessKnives);
	pthread_mutex_unlock(&sim->threads->accessDirtyKnives);
}



void get_pizza(int id){

	pthread_mutex_lock(&sim->threads->accessPizza);
	pthread_once (&sim->threads->initial, init_dinner); /* inicializa as estruturas de dados, se for o primeiro acesso */

	if(sim->diningRoom->pizza < 1){
		//pthread_once (&sim->threads->initial, init_dinner); /* inicializa as estruturas de dados, se for o primeiro acesso */

		/* Fill request to waiter */
		pthread_mutex_lock(&sim->threads->accessReqPizza);

		sim->waiter->reqPizza = W_ACTIVE;
		sim->waiter->reqPizzaPhilosophers[0] += 1;
		sim->waiter->reqPizzaPhilosophers[sim->waiter->reqPizzaPhilosophers[0]] = id;

		pthread_mutex_unlock(&sim->threads->accessReqPizza);

		printf("Phil %d waiting for pizza\n", id);

		// wake up waiter
		signal_waiter();

		//hunger philosopher waits for waiter
		wait_philosopher(id, &sim->threads->accessPizza);

		return get_pizza(id);
	}

	sim->diningRoom->pizza-=1;
	if(sim->diningRoom->pizza < 1){
		pthread_mutex_lock(&sim->threads->pizza_requests_without_need);
		sim->waiter->pizza_number_request_withou_need+=1;
		pthread_mutex_unlock(&sim->threads->pizza_requests_without_need);
		printf("ZERO PIZZA\n");
		sim->waiter->reqPizza = W_ACTIVE;
		signal_waiter();
	}
	logger(sim);
	pthread_mutex_unlock(&sim->threads->accessPizza);
}



void get_spaghetti(int id){

	pthread_mutex_lock(&sim->threads->accessSpaghetti);
	pthread_once (&sim->threads->initial, init_dinner); /* inicializa as estruturas de dados, se for o primeiro acesso */

	if(sim->diningRoom->spaghetti < 1){
		//pthread_once (&sim->threads->initial, init_dinner); /* inicializa as estruturas de dados, se for o primeiro acesso */

		/* Fill request to waiter */
		pthread_mutex_lock(&sim->threads->accessReqSpaghetti);

		sim->waiter->reqSpaghetti = W_ACTIVE;
		sim->waiter->reqSpaghettiPhilosophers[0] += 1;
		sim->waiter->reqSpaghettiPhilosophers[sim->waiter->reqSpaghettiPhilosophers[0]] = id;

		pthread_mutex_unlock(&sim->threads->accessReqSpaghetti);

		printf("Phil %d waiting for spaghetti\n", id);

		// wake up waiter
		signal_waiter();

		//hunger philosopher waits for waiter
		wait_philosopher(id, &sim->threads->accessSpaghetti);

		return get_spaghetti(id);
	}

	sim->diningRoom->spaghetti-=1;
	if(sim->diningRoom->spaghetti < 1){
		pthread_mutex_lock(&sim->threads->spaghetti_requests_without_need);
		sim->waiter->spaghetti_number_request_withou_need+=1;
		pthread_mutex_unlock(&sim->threads->spaghetti_requests_without_need);
		printf("ZERO SPAGHETTI\n");
		sim->waiter->reqSpaghetti = W_ACTIVE;
		signal_waiter();
	}
	logger(sim);
	pthread_mutex_unlock(&sim->threads->accessSpaghetti);
}

void get_two_forks(int id){
	
	pthread_mutex_lock(&sim->threads->accessForks);
	pthread_once (&sim->threads->initial, init_dinner); /* inicializa as estruturas de dados, se for o primeiro acesso */

	if(sim->diningRoom->cleanForks < 2){
		//pthread_once (&sim->threads->initial, init_dinner); /* inicializa as estruturas de dados, se for o primeiro acesso */

		/* Fill Request to waiter */
	    pthread_mutex_lock(&sim->threads->accessReqCutlery);

	    /* Adicionar o filósofo à fila de espera para bloqueio */
	    sim->waiter->reqCutlery = W_ACTIVE;
	    sim->waiter->reqCutleryPhilosophers[0].num++;
	    sim->waiter->reqCutleryPhilosophers[sim->waiter->reqCutleryPhilosophers[0].num].id=id;
	    sim->waiter->reqCutleryPhilosophers[sim->waiter->reqCutleryPhilosophers[0].num].request=FORK;
	    sim->waiter->reqCutleryPhilosophers[sim->waiter->reqCutleryPhilosophers[0].num].num=2;

	    pthread_mutex_unlock(&sim->threads->accessReqCutlery);
	    printf("Phil %d is waiting for two forks\n", id);
	    /*wake up the waiter*/
	    signal_waiter();

	    /*hungry philosopher waits for the waiter */
	    wait_philosopher(id, &sim->threads->accessForks);

		return get_two_forks(id);
	}
	

	sim->diningRoom->cleanForks-=2;
    sim->philosophers[id]->cutlery[0] = P_FORK;
    sim->philosophers[id]->cutlery[1] = P_FORK;
    if (sim->diningRoom->cleanForks<=1){
    	pthread_mutex_lock(&sim->threads->cutlery_requests_without_need);
        sim->waiter->cutlery_number_request_withou_need+=1;
        pthread_mutex_unlock(&sim->threads->cutlery_requests_without_need);
        printf("ZERO Cutlery\n");
        sim->waiter->reqCutlery=W_ACTIVE;
        signal_waiter();
    }
    logger(sim);
    pthread_mutex_unlock(&sim->threads->accessForks);
}



void get_fork_knife(int id){

	pthread_mutex_lock(&sim->threads->accessForks);
	pthread_once (&sim->threads->initial, init_dinner); /* inicializa as estruturas de dados, se for o primeiro acesso */

	if(sim->diningRoom->cleanForks < 1){
		//pthread_once (&sim->threads->initial, init_dinner); /* inicializa as estruturas de dados, se for o primeiro acesso */

		/* Fill Request to waiter */
	    pthread_mutex_lock(&sim->threads->accessReqCutlery);

	    /* Adicionar o filósofo à fila de espera para bloqueio */
	    sim->waiter->reqCutlery = W_ACTIVE;
	    sim->waiter->reqCutleryPhilosophers[0].num++;
	    sim->waiter->reqCutleryPhilosophers[sim->waiter->reqCutleryPhilosophers[0].num].id=id;
	    sim->waiter->reqCutleryPhilosophers[sim->waiter->reqCutleryPhilosophers[0].num].request=FORK;
	    sim->waiter->reqCutleryPhilosophers[sim->waiter->reqCutleryPhilosophers[0].num].num=1;

	    pthread_mutex_unlock(&sim->threads->accessReqCutlery);
	    printf("Phil %d is waiting for one fork\n", id);
	    /*wake up the waiter*/
	    signal_waiter();

	    /*hungry philosopher waits for the waiter */
	    wait_philosopher(id, &sim->threads->accessForks);

		return get_fork_knife(id);
	}

	pthread_mutex_lock(&sim->threads->accessKnives);

	if(sim->diningRoom->cleanKnives < 1){
		//pthread_once (&sim->threads->initial, init_dinner); /* inicializa as estruturas de dados, se for o primeiro acesso */

		/* Fill Request to waiter */
	    pthread_mutex_lock(&sim->threads->accessReqCutlery);

	     /* Adicionar o filósofo à fila de espera para bloqueio */
	    sim->waiter->reqCutlery = W_ACTIVE;
	    sim->waiter->reqCutleryPhilosophers[0].num++;
	    sim->waiter->reqCutleryPhilosophers[sim->waiter->reqCutleryPhilosophers[0].num].id=id;
	    sim->waiter->reqCutleryPhilosophers[sim->waiter->reqCutleryPhilosophers[0].num].request=KNIFE;
	    sim->waiter->reqCutleryPhilosophers[sim->waiter->reqCutleryPhilosophers[0].num].num=1;

	    pthread_mutex_unlock(&sim->threads->accessReqCutlery);
	    printf("Phil %d is waiting for one knife\n", id);
	    /*wake up the waiter*/
	    signal_waiter();

	    /*hungry philosopher waits for the waiter */
	    wait_philosopher(id, &sim->threads->accessKnives);

		return get_fork_knife(id);

	}

	sim->diningRoom->cleanKnives-=1;
	sim->diningRoom->cleanForks-=1;
    sim->philosophers[id]->cutlery[0] = P_FORK;
    sim->philosophers[id]->cutlery[1] = P_KNIFE;
    if (sim->diningRoom->cleanForks==0 || sim->diningRoom->cleanKnives==0){
        pthread_mutex_lock(&sim->threads->cutlery_requests_without_need);
        sim->waiter->cutlery_number_request_withou_need+=1;
        pthread_mutex_unlock(&sim->threads->cutlery_requests_without_need);
        printf("ZERO Cutlery\n");
        sim->waiter->reqCutlery=W_ACTIVE;
       	signal_waiter();     
    }
    logger(sim);
    pthread_mutex_unlock(&sim->threads->accessForks);
   	pthread_mutex_unlock(&sim->threads->accessKnives);
}

void drop_two_forks(int id){
	pthread_mutex_lock(&sim->threads->accessDirtyForks);

	sim->diningRoom->dirtyForks+=2;
	sim->philosophers[id]->cutlery[0] = P_PUT_FORK;
	sim->philosophers[id]->cutlery[1] = P_PUT_FORK;
	logger(sim);

	pthread_mutex_unlock(&sim->threads->accessDirtyForks);
}


void drop_fork_knife(int id){
	pthread_mutex_lock(&sim->threads->accessDirtyForks);
	pthread_mutex_lock(&sim->threads->accessDirtyKnives);

	sim->diningRoom->dirtyForks+=1;
	sim->diningRoom->dirtyKnives+=1;
	sim->philosophers[id]->cutlery[0] = P_PUT_FORK;
	sim->philosophers[id]->cutlery[1] = P_PUT_KNIFE;
	logger(sim);

	pthread_mutex_unlock(&sim->threads->accessDirtyForks);
	pthread_mutex_unlock(&sim->threads->accessDirtyKnives);
}


void get_cutlery(){
	pthread_mutex_lock(&sim->threads->accessDirtyForks);
	pthread_mutex_lock(&sim->threads->accessDirtyKnives);

	sim->diningRoom->dirtyForksInWaiter += sim->diningRoom->dirtyForks;
	sim->diningRoom->dirtyForks = 0;
	sim->diningRoom->dirtyKnivesInWaiter += sim->diningRoom->dirtyKnives;
	sim->diningRoom->dirtyKnives = 0;

	pthread_mutex_unlock(&sim->threads->accessDirtyForks);
	pthread_mutex_unlock(&sim->threads->accessDirtyKnives);
}

void replenish_cutlery(int *cleans){
	pthread_mutex_lock(&sim->threads->accessForks);
	pthread_mutex_lock(&sim->threads->accessKnives);

	sim->diningRoom->cleanForks += sim->diningRoom->dirtyForksInWaiter;
	sim->diningRoom->dirtyForksInWaiter = 0;
	sim->diningRoom->cleanKnives += sim->diningRoom->dirtyKnivesInWaiter;
	sim->diningRoom->dirtyKnivesInWaiter = 0;

	cleans[FORK] = sim->diningRoom->cleanForks;
	cleans[KNIFE] = sim->diningRoom->cleanKnives;

	pthread_mutex_unlock(&sim->threads->accessForks);
	pthread_mutex_unlock(&sim->threads->accessKnives);

}

void add_pizza(){
	pthread_mutex_lock(&sim->threads->accessPizza);
	if(sim->diningRoom->pizza==0){
		sim->diningRoom->pizza += sim->params->NUM_PIZZA;
	}

	pthread_mutex_unlock(&sim->threads->accessPizza);
}
	
void add_spaghetti(){
	pthread_mutex_lock(&sim->threads->accessSpaghetti);
	if(sim->diningRoom->spaghetti==0){
		sim->diningRoom->spaghetti += sim->params->NUM_SPAGHETTI;
	}

	pthread_mutex_unlock(&sim->threads->accessSpaghetti);
}

void kill_phil(int id){
	pthread_mutex_lock(&sim->threads->accessKill);

	sim->diningRoom->dead_philosophers+=1;

	int t=0;
	if(sim->diningRoom->dead_philosophers==sim->params->NUM_PHILOSOPHERS)
        t=1;
    
    pthread_mutex_unlock(&sim->threads->accessKill);

    /* If there are not alive philosopers, signal waiter */
    if (t==1)
         pthread_cond_signal(&sim->threads->wakeup_waiter);  
}

void signal_waiter(void){
	pthread_cond_signal(&sim->threads->wakeup_waiter);
}

void wait_waiter(void){
	pthread_mutex_lock(&sim->threads->waiter);
	pthread_cond_wait(&sim->threads->wakeup_waiter, &sim->threads->waiter);
}

void signal_philosopher(int id){
	pthread_cond_signal(&sim->threads->philosophers_cond[id]);

}

void wait_philosopher(int id, pthread_mutex_t* access){
	pthread_cond_wait(&sim->threads->philosophers_cond[id], access);
}

