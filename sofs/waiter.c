/**
 *  \brief Waiter module
 *  
 * \author Miguel Oliveira e Silva - 2016
 		   Eduardo Reis Silva nºmec. 76354
 		   Nuno Filipe Sousa Capela nº mec. 76385
 		   Pedro Marques Ferreira da Silva nºmec. 72645
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include "dining-room.h"
#include "logger.h"

#include <pthread.h>

/* #define sizeofa(array) sizeof array / sizeof array[ 0 ] */

extern Simulation *sim;

static void clean_cutlery(int *cleans);
static void replenish_pizza(void);
static void replenish_spaghetti(void);
static void check_requests(void);

static pthread_mutex_t accessRequests = PTHREAD_MUTEX_INITIALIZER; /* mutex que controla o acesso ao nº de pedidos */
static pthread_mutex_t accessHasRequests = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t accessReqPizza = PTHREAD_MUTEX_INITIALIZER; /* mutex que controla o acesso aos pedidos de pizza */
static pthread_mutex_t accessReqSpaghetti = PTHREAD_MUTEX_INITIALIZER; /* mutex que controla o acesso aos pedidos de spaghetti */
static pthread_mutex_t accessReqCutlery = PTHREAD_MUTEX_INITIALIZER; /* mutex que controla o acesso aos pedidos de limpeza dos talheres */

static pthread_mutex_t pizza_requests_without_need = PTHREAD_MUTEX_INITIALIZER; /* mutex que controla os pedidos de pizza para os filosofos seguintes */
static pthread_mutex_t spaghetti_requests_without_need = PTHREAD_MUTEX_INITIALIZER; /* mutex que controla os pedidos de spaghetti para os filosofos seguintes */
static pthread_mutex_t cutlery_requests_without_need = PTHREAD_MUTEX_INITIALIZER; /* mutex que controla os pedidos de cutlery para os filosofos seguintes */


void init_waiter(void){
	
	sim->waiter->reqCutleryPhilosophers = (Cutlery_Request*)mem_alloc(sizeof(Cutlery_Request)*sim->params->NUM_PHILOSOPHERS);
	sim->waiter->reqPizzaPhilosophers = (int*)mem_alloc(sizeof(int)*sim->params->NUM_PHILOSOPHERS);
	sim->waiter->reqSpaghettiPhilosophers = (int*)mem_alloc(sizeof(int)*sim->params->NUM_PHILOSOPHERS);

	sim->waiter->reqCutleryPhilosophers[0].num = 0;
	sim->waiter->reqPizzaPhilosophers[0] = 0;
	sim->waiter->reqSpaghettiPhilosophers[0] = 0;

	/* Unlock all access */
	pthread_mutex_unlock(&accessRequests);
	pthread_mutex_unlock(&accessHasRequests);
	pthread_mutex_unlock(&accessReqPizza);
	pthread_mutex_unlock(&accessReqSpaghetti);
	pthread_mutex_unlock(&accessReqCutlery);
	pthread_mutex_unlock(&pizza_requests_without_need);
	pthread_mutex_unlock(&spaghetti_requests_without_need);
	pthread_mutex_unlock(&cutlery_requests_without_need);
	
}


void waiter_life(void){

	int n_phil, wake_without_request;
	int *phil_id, *cleans;
	int i;

	/* Inicializar número de pedidos efectuados ao waiter */
	sim->waiter->cutlery_number_request_without_need = 0;
	sim->waiter->pizza_number_request_without_need = 0;
	sim->waiter->spaghetti_number_request_without_need = 0;
	sim->waiter->numRequests = 0;
	sim->waiter->hasRequestWithoutNeed = 0;

	sim->waiter->state = W_SLEEP;
	logger(sim);

	// filosofo esta a dormir ate haver um pedido (signal) do waiter
	wait_waiter();

	cleans = (int*)mem_alloc(sizeof(int)*2);

	/* Enquanto houver filosofos vivos */
	while(sim->diningRoom->dead_philosophers != sim->params->NUM_PHILOSOPHERS){
		n_phil = 0;
		wake_without_request = 0;
		check_requests();

		/*Verificar quais os pedidos a serem efectuados numa fila;
		  Responder a pedidos e apagar os pedidos dos filosofos */
		if(sim->waiter->reqPizza == W_ACTIVE){
			sim->waiter->state = W_REQUEST_PIZZA;
			logger(sim);

			pthread_mutex_lock(&accessReqPizza);

			/*Repor pizzas */
			replenish_pizza();

			
			/* criar um array para guardar os id dos filosofos que vao ser acordados */
			phil_id = (int*)mem_alloc(sizeof(int)*sim->params->NUM_PIZZA);

			/* se o numero de pizzas novas for maior que o nº de filosofos então vamos acordar todos os filosofos */
			if(sim->waiter->reqPizzaPhilosophers[0] < sim->params->NUM_PIZZA){
				n_phil = sim->waiter->reqPizzaPhilosophers[0];
				/* copiar os id's dos filosofos */
				memcpy(phil_id,&sim->waiter->reqPizzaPhilosophers[1], sizeof(int)*sim->waiter->reqPizzaPhilosophers[0]);
				/* limpar a fila de pedidos */
				sim->waiter->reqPizzaPhilosophers[0] = 0;
				sim->waiter->reqPizza = W_INACTIVE;
			}

			/* se o numero de pizzas novas nao for maior que o nº de filosofos, acordar apenas n filosofos sendo n o nº de pizzas 'novas' */
			else{
				n_phil = sim->params->NUM_PIZZA;
				/* copiar os id's dos filosofos que podem ser acordados */
				memcpy(phil_id,&sim->waiter->reqPizzaPhilosophers[1], sizeof(int)*sim->params->NUM_PIZZA);
				/* alterar a fila de pedidos, aumentando a prioridade dos filosofos que ainda nao foram atendidos */
				sim->waiter->reqPizzaPhilosophers[0] -= sim->params->NUM_PIZZA;
				memcpy(&sim->waiter->reqPizzaPhilosophers[1], &sim->waiter->reqPizzaPhilosophers[1+sim->params->NUM_PIZZA], sizeof(int)*sim->waiter->reqPizzaPhilosophers[0]);
			}
			pthread_mutex_unlock(&accessReqPizza);

			/* Guardar o nº de wake's sem request para mais tarde ver se ainda há pedidos por responder*/
			pthread_mutex_lock(&pizza_requests_without_need);
			wake_without_request = sim->waiter->pizza_number_request_without_need;
			sim->waiter->pizza_number_request_without_need = 0;
			pthread_mutex_unlock(&pizza_requests_without_need);

		}

		else if(sim->waiter->reqSpaghetti == W_ACTIVE){
			sim->waiter->state = W_REQUEST_SPAGHETTI;
			logger(sim);

			pthread_mutex_lock(&accessReqSpaghetti);

			/* Repor o esparguete */
			replenish_spaghetti();

			/* criar um array para guardar os id's dos filosofos que vao ser acordados */
			phil_id = (int*)mem_alloc(sizeof(int)*sim->params->NUM_SPAGHETTI);
			/* se o numero de doses de esparguete for maior que o nº de filosofos entao vamos acordar todos os filosofos */
			if(sim->waiter->reqSpaghettiPhilosophers[0] < sim->params->NUM_SPAGHETTI){
				n_phil = sim->waiter->reqSpaghettiPhilosophers[0];
				/* copiar os id's dos filosofos*/
				memcpy(phil_id,&sim->waiter->reqSpaghettiPhilosophers[1], sizeof(int)*sim->waiter->reqSpaghettiPhilosophers[0]);
				/*limpar a fila de pedidos */
				sim->waiter->reqSpaghettiPhilosophers[0] = 0;
				sim->waiter->reqSpaghetti = W_INACTIVE;
			}
			/* se o numero de doses de esparguete nao for maior que o nº de filosofos, acordar apenas n filosofos, sendo n o nº de doses de spaghetti novas */

			else{
				n_phil = sim->params->NUM_SPAGHETTI;
				/* copiar os id's dos filosofos que podem ser acordados */
				memcpy(phil_id,&sim->waiter->reqSpaghettiPhilosophers[1], sizeof(int)*sim->params->NUM_SPAGHETTI);
				sim->waiter->reqSpaghettiPhilosophers[0] -= sim->params->NUM_SPAGHETTI;
				/* alterar fila de pedidos, aumentando a prioridade dos filosofos que ainda nao foram atendidos */
				memcpy(&sim->waiter->reqSpaghettiPhilosophers[1], &sim->waiter->reqSpaghettiPhilosophers[1+sim->params->NUM_SPAGHETTI], sizeof(int)*sim->waiter->reqSpaghettiPhilosophers[0]);
			}
			pthread_mutex_unlock(&accessReqSpaghetti);

			/* Guardar o nº de wake's sem request para mais tarde ver se ainda há pedidos por responder*/
			pthread_mutex_lock(&spaghetti_requests_without_need);
			wake_without_request = sim->waiter->spaghetti_number_request_without_need;
			sim->waiter->spaghetti_number_request_without_need = 0;
			pthread_mutex_unlock(&spaghetti_requests_without_need);

		}

		else if(sim->waiter->reqCutlery == W_ACTIVE){
			sim->waiter->state = W_REQUEST_CUTLERY;
			logger(sim);

			pthread_mutex_lock(&accessReqCutlery);

			/*Limpar os talheres*/
			clean_cutlery(cleans);

			/* Para todos os filosofos na fila, calcular quantos filosofos podem ser acordados de acordo com o nº de talheres disponiveis */
			for(i=0; i<sim->waiter->reqCutleryPhilosophers[0].num;i++){
				if(sim->waiter->reqCutleryPhilosophers[i+1].request==FORK){
					if(sim->waiter->reqCutleryPhilosophers[i+1].num <= cleans[FORK]){
						cleans[FORK] -= sim->waiter->reqCutleryPhilosophers[i+1].num;
					}
					else{
						break;
					}
				}
				else{
					if(sim->waiter->reqCutleryPhilosophers[i+1].num <= cleans[KNIFE]){
						cleans[KNIFE] -= sim->waiter->reqCutleryPhilosophers[i+1].num;
					}
					else{
						break;
					}
				}
			}

			n_phil = i;
			phil_id = (int*)mem_alloc(sizeof(int)*n_phil);
			
			/* obter os id's dos filosofos a acordar */
			for(i=0;i<n_phil;i++){
				phil_id[i] = sim->waiter->reqCutleryPhilosophers[i+1].id;
			}

			sim->waiter->reqCutleryPhilosophers[0].num -= n_phil;
			/* alterar a fila de pedidos, aumentando a prioridade dos filosofos que ainda nao foram atendidos */
			memcpy(&sim->waiter->reqCutleryPhilosophers[1],&sim->waiter->reqCutleryPhilosophers[1+n_phil],sizeof(Cutlery_Request)*sim->waiter->reqCutleryPhilosophers[0].num);
			
			if(sim->waiter->reqCutleryPhilosophers[0].num == 0){
				sim->waiter->reqCutlery = W_INACTIVE;
			}
			pthread_mutex_unlock(&accessReqCutlery);

			/* Guardar o nº de wake's sem request para mais tarde ver se ainda há pedidos por responder*/
			pthread_mutex_lock(&cutlery_requests_without_need);
			wake_without_request = sim->waiter->cutlery_number_request_without_need;
			sim->waiter->cutlery_number_request_without_need = 0;
			pthread_mutex_unlock(&cutlery_requests_without_need);
		}

		logger(sim);

		// Acordar filosofos e atualizar o nº de pedidos por responder
		if(n_phil != 0){
			for(i=0;i<n_phil;i++){
				signal_philosopher(phil_id[i]);
			}
			pthread_mutex_lock(&accessRequests);
			sim->waiter->numRequests -= n_phil;
			pthread_mutex_unlock(&accessRequests);
			free(phil_id);
		}

		/* atualizar o nº de pedidos por responder */
		if(wake_without_request > 0){
			pthread_mutex_lock(&accessRequests);
			sim->waiter->hasRequestWithoutNeed = 0;
			pthread_mutex_unlock(&accessRequests);
		}

		/*se ainda houver filosofos vivos e não houver pedidos, adormecer o waiter ate que haja novos pedidos*/
		if(!sim->waiter->hasRequestWithoutNeed && sim->waiter->numRequests == 0 && sim->diningRoom->dead_philosophers != sim->params->NUM_PHILOSOPHERS){
			sim->waiter->state = W_SLEEP;
			logger(sim);
			wait_waiter();;

		}

		/*se não existir filosofos vivos, limpar o nº de pedidos */
		if(sim->diningRoom->dead_philosophers == sim->params->NUM_PHILOSOPHERS){
			sim->waiter->numRequests = 0;
			sim->waiter->reqPizza = W_INACTIVE;
			sim->waiter->reqSpaghetti = W_INACTIVE;
			sim->waiter->reqCutlery = W_INACTIVE;
		}
		
	}
	/* Com os filosofos todos mortos, o waiter vai limpar a cozinha */
	if(sim->diningRoom->dirtyForks > 0 || sim->diningRoom->dirtyKnives > 0){
		sim->waiter->state = W_REQUEST_CUTLERY;
		logger(sim);
		clean_cutlery(cleans);
	}
	/* waiter so pode morrer quando nao houver mais talheres para lavar e se os filosofos ja tiverem todos mortos */
	if(sim->diningRoom->dirtyForks == 0 && sim->diningRoom->dirtyKnives == 0 && sim->diningRoom->dead_philosophers == sim->params->NUM_PHILOSOPHERS){
		sim->waiter->state = W_DEAD;
		logger(sim);
	}

}


void clean_cutlery(int *cleans)
{
	get_cutlery();
	int wash_time = rand() % sim->params->WASH_TIME+1;
	usleep(wash_time*1000);
	replenish_cutlery(cleans);
}

void replenish_pizza(void)
{
	add_pizza();
}

void replenish_spaghetti(void)
{
	add_spaghetti();
}

void request_pizza(int id)
{	
	/* atualizar o nº de pedidos */
	pthread_mutex_lock(&accessRequests);
	sim->waiter->numRequests += 1;
	pthread_mutex_unlock(&accessRequests);

	/* fazer pedido ao waiter e adicionar o waiter a fila de espera*/
	pthread_mutex_lock(&accessReqPizza);
	sim->waiter->reqPizza = W_ACTIVE;
	sim->waiter->reqPizzaPhilosophers[0] += 1;
	sim->waiter->reqPizzaPhilosophers[sim->waiter->reqPizzaPhilosophers[0]] = id;
	pthread_mutex_unlock(&accessReqPizza);
}


void request_spaghetti(int id)
{	
	/* atualizar o nº de pedidos */
	pthread_mutex_lock(&accessRequests);
	sim->waiter->numRequests += 1;
	pthread_mutex_unlock(&accessRequests);

	/* fazer pedido ao waiter e adicionar o waiter a fila de espera*/
	pthread_mutex_lock(&accessReqSpaghetti);
	sim->waiter->reqSpaghetti = W_ACTIVE;
	sim->waiter->reqSpaghettiPhilosophers[0] += 1;
	sim->waiter->reqSpaghettiPhilosophers[sim->waiter->reqSpaghettiPhilosophers[0]] = id;
	pthread_mutex_unlock(&accessReqSpaghetti);
}


void request_cutlery(int id, int num, int fork)
{	
	/* atualizar o nº de pedidos */
	pthread_mutex_lock(&accessRequests);
	sim->waiter->numRequests += 1;
	pthread_mutex_unlock(&accessRequests);

	/* fazer pedido ao waiter e adicionar o waiter a fila de espera*/
   	pthread_mutex_lock(&accessReqCutlery);
    sim->waiter->reqCutlery = W_ACTIVE;
    sim->waiter->reqCutleryPhilosophers[0].num++;
    sim->waiter->reqCutleryPhilosophers[sim->waiter->reqCutleryPhilosophers[0].num].id=id;
	   
	if(fork){
		sim->waiter->reqCutleryPhilosophers[sim->waiter->reqCutleryPhilosophers[0].num].request=FORK;
    	sim->waiter->reqCutleryPhilosophers[sim->waiter->reqCutleryPhilosophers[0].num].num=num;	
	}

	else{
		sim->waiter->reqCutleryPhilosophers[sim->waiter->reqCutleryPhilosophers[0].num].request=KNIFE;
	    sim->waiter->reqCutleryPhilosophers[sim->waiter->reqCutleryPhilosophers[0].num].num=num;
	}
	pthread_mutex_unlock(&accessReqCutlery);
}


void request_pizza_without_need(void)
{
	/* atualizar o nº de pedidos */
	pthread_mutex_lock(&accessHasRequests);
	sim->waiter->hasRequestWithoutNeed = 1;
	pthread_mutex_unlock(&accessHasRequests);

	/* fazer pedido ao waiter*/
	pthread_mutex_lock(&pizza_requests_without_need);
	sim->waiter->pizza_number_request_without_need+=1;
	pthread_mutex_unlock(&pizza_requests_without_need);
	sim->waiter->reqPizza = W_ACTIVE;

}

void request_spaghetti_without_need(void)
{	
	/* atualizar o nº de pedidos */
	pthread_mutex_lock(&accessHasRequests);
	sim->waiter->hasRequestWithoutNeed = 1;
	pthread_mutex_unlock(&accessHasRequests);

	/* fazer pedido ao waiter*/
	pthread_mutex_lock(&spaghetti_requests_without_need);
	sim->waiter->spaghetti_number_request_without_need+=1;
	pthread_mutex_unlock(&spaghetti_requests_without_need);
	sim->waiter->reqSpaghetti = W_ACTIVE;
}


void request_cutlery_without_need(void)
{	
	/* atualizar o nº de pedidos */
	pthread_mutex_lock(&accessHasRequests);
	sim->waiter->hasRequestWithoutNeed = 1;
	pthread_mutex_unlock(&accessHasRequests);

	/* fazer pedido ao waiter*/
	pthread_mutex_lock(&cutlery_requests_without_need);
    sim->waiter->cutlery_number_request_without_need+=1;
    pthread_mutex_unlock(&cutlery_requests_without_need);
    sim->waiter->reqCutlery=W_ACTIVE;

}

void check_requests(void){

	if(sim->waiter->reqPizza == W_INACTIVE){
		sim->waiter->reqPizzaPhilosophers[0] = 0;
	}

	if(sim->waiter->reqSpaghetti == W_INACTIVE){
		sim->waiter->reqSpaghettiPhilosophers[0] = 0;
	}

	if(sim->waiter->reqCutlery == W_INACTIVE){
		sim->waiter->reqCutleryPhilosophers[0].num = 0;
	}
	
}