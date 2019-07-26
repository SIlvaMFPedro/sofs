	/**
 *  \brief Waiter module
 *  
 * \author Miguel Oliveira e Silva - 2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include "dining-room.h"
#include "logger.h"

#include <pthread.h>

extern Simulation *sim;

static void clean_cutlery(int *cleans);
static void replenish_pizza(void);
static void replenish_spaghetti(void);


void init_waiter(void){
	
	sim->waiter->reqCutleryPhilosophers = (Cutlery_Request*)mem_alloc(sizeof(Cutlery_Request)*sim->params->NUM_PHILOSOPHERS);
	sim->waiter->reqPizzaPhilosophers = (int*)mem_alloc(sizeof(int)*sim->params->NUM_PHILOSOPHERS);
	sim->waiter->reqSpaghettiPhilosophers = (int*)mem_alloc(sizeof(int)*sim->params->NUM_PHILOSOPHERS);

	sim->waiter->reqCutleryPhilosophers[0].num = 0;
	sim->waiter->reqPizzaPhilosophers[0] = 0;
	sim->waiter->reqSpaghettiPhilosophers[0] = 0;

	diningRoom_init();
	
}


void waiter_life(void){

	int n_phil, wake_without_request;
	int *phil_id, *cleans;
	int i;

	/* Inicializar número de pedidos efectuados ao waiter */
	sim->waiter->cutlery_number_request_withou_need = 0;
	sim->waiter->pizza_number_request_withou_need = 0;
	sim->waiter->spaghetti_number_request_withou_need = 0;

	sim->waiter->state = W_SLEEP;
	logger(sim);

	// filosofo esta a dormir ate haver um pedido (signal) do waiter
	wait_waiter();


	cleans = (int*)mem_alloc(sizeof(int)*2);

	/* Enquanto houver filosofos */
	while(sim->diningRoom->dead_philosophers != sim->params->NUM_PHILOSOPHERS){
		n_phil = 0;
		wake_without_request = 0;

		/*Verificar quais os pedidos a serem efectuados numa fila;
		  Responder a pedidos e apagar os pedidos dos filosofos */
		if(sim->waiter->reqPizza == W_ACTIVE){
			sim->waiter->state = W_REQUEST_PIZZA;
			logger(sim);

			/* go get some pizza */
			replenish_pizza();

			pthread_mutex_lock(&sim->threads->accessReqPizza);
			
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
			pthread_mutex_unlock(&sim->threads->accessReqPizza);

			/* Guardar o nº de wake's sem request, para mais tarde fazer signal ao waiter */
			pthread_mutex_lock(&sim->threads->pizza_requests_without_need);
			wake_without_request = sim->waiter->pizza_number_request_withou_need;
			sim->waiter->pizza_number_request_withou_need = 0;
			pthread_mutex_unlock(&sim->threads->pizza_requests_without_need);


		}

		else if(sim->waiter->reqSpaghetti == W_ACTIVE){
			sim->waiter->state = W_REQUEST_SPAGHETTI;
			logger(sim);

			/* go get some spaghetti */
			replenish_spaghetti();

			pthread_mutex_lock(&sim->threads->accessReqSpaghetti);
			
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
			pthread_mutex_unlock(&sim->threads->accessReqSpaghetti);

			/* Guardar o nº de wake's sem request, para mais tarde fazer signal ao waiter */
			pthread_mutex_lock(&sim->threads->spaghetti_requests_without_need);
			wake_without_request = sim->waiter->spaghetti_number_request_withou_need;
			sim->waiter->spaghetti_number_request_withou_need = 0;
			pthread_mutex_unlock(&sim->threads->spaghetti_requests_without_need);

		}

		else if(sim->waiter->reqCutlery == W_ACTIVE){
			sim->waiter->state = W_REQUEST_CUTLERY;
			logger(sim);

			/* go clean cutlery */
			clean_cutlery(cleans);

			pthread_mutex_lock(&sim->threads->accessReqCutlery);

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

			pthread_mutex_unlock(&sim->threads->accessReqCutlery);

			pthread_mutex_lock(&sim->threads->cutlery_requests_without_need);
			wake_without_request = sim->waiter->cutlery_number_request_withou_need;
			sim->waiter->cutlery_number_request_withou_need = 0;
			pthread_mutex_unlock(&sim->threads->cutlery_requests_without_need);

		}

		logger(sim);

		// Tentativa de acordar filosofos
		if(n_phil != 0){
			for(i=0;i<n_phil;i++){
				printf("Wake up philosopher %d\n",phil_id[i]);
				pthread_cond_signal(&sim->threads->philosophers_cond[phil_id[i]]);
			}
			free(phil_id);
		}
		/* signal the waiter nº de vezes dos filosofos acordados */
		if(n_phil+wake_without_request > 1){
			for(int i = 0; i < n_phil+wake_without_request-1; i++){
				wait_waiter();
				pthread_mutex_unlock(&sim->threads->waiter);
			}
		}
		sim->waiter->state = W_SLEEP;
		logger(sim);
		wait_waiter();
		pthread_mutex_unlock(&sim->threads->waiter);

		
	}
	/* limpar mesa no final -> so necessita do waiter */
	if(sim->diningRoom->dirtyForks>0 || sim->diningRoom->dirtyKnives > 0){
		sim->waiter->state = W_REQUEST_CUTLERY;
		logger(sim);
		clean_cutlery(cleans);
	}

	sim->waiter->state = W_DEAD;
	logger(sim);

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