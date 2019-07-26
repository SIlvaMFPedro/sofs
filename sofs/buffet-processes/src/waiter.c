/**
 *  \brief Waiter module
 *  
 * \author Miguel Oliveira e Silva - 2016
 *         Ana Patrícia Gomes da Cruz
 *         Diogo Daniel Soares Ferreira
 *         João Pedro de Almeida Maia
 *         
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include "dining-room.h"
#include "logger.h"

#include <sys/shm.h>
#include <sys/sem.h>
#include <pthread.h>

extern Simulation *sim;


static void clean_cutlery(int* cleans);
static void replenish_pizza(void);
static void replenish_spaghetti(void);


void init_waiter(void){
	sim->waiter->reqCutleryPhilosophers[0].num = 0;
	sim->waiter->reqPizzaPhilosophers[0] = 0;
	sim->waiter->reqSpaghettiPhilosophers[0] = 0;
	diningRoom_init();
}

void waiter_life(void){

	int *cleans;
	int n_phil, wake_without_request;
	int *phil_id;
	int i;

	/* Inicializar número de pedidos efetuados ao waiter */
	sim->waiter->cutlery_number_ups_without_request = 0;
	sim->waiter->pizza_number_ups_without_request = 0;
	sim->waiter->spaghetti_number_ups_without_request = 0;

	sim->waiter->state = W_SLEEP;

	logger(sim);

	/* Down to waiter */
	down_waiter(1);

	cleans = (int*)mem_alloc(sizeof(int)*2);

	/* Enquanto houver filósofos */
	while (sim->diningRoom->dead_philosophers!=sim->params->NUM_PHILOSOPHERS){
		n_phil = 0;
		wake_without_request = 0;
		
		
		/* Verificar quais os pedidos a serem efetuados numa fila,
		   Responder a pedidos e apagar pedidos de filósofos */

		if (sim->waiter->reqPizza==W_ACTIVE){
			sim->waiter->state = W_REQUEST_PIZZA;
			logger(sim);
			
			/* Operação de ir buscar pizzas */
			replenish_pizza();
			
			down(S_REQUEST_ACCESS_PIZZA, 1);
			/* Criar um array para guardar o id dos filósofos que irão ser acordados */
			phil_id = (int*)mem_alloc(sizeof(int)*sim->params->NUM_PIZZA);
			
			/* Se o número de pizzas novas for maior que o nº de filósofos, acordar todos os filósofos */
			if (sim->waiter->reqPizzaPhilosophers[0]<sim->params->NUM_PIZZA){
				n_phil = sim->waiter->reqPizzaPhilosophers[0];
				/* Copiar id's dos filósofos */
				memcpy(phil_id, &sim->waiter->reqPizzaPhilosophers[1], sizeof(int)*sim->waiter->reqPizzaPhilosophers[0]);
				/* Limpar fila de pedidos */
				sim->waiter->reqPizzaPhilosophers[0] = 0;
				sim->waiter->reqPizza = W_INACTIVE;
			}
			/* Se o número de pizzas novas não for maior que o nº de filósofos, acordar apenas n filósofos, sendo n o nº de pizzas 'novas' */
			else{
				n_phil = sim->params->NUM_PIZZA;
				/* Copiar id's dos filósofos que podem ser acordados */
				memcpy(phil_id, &sim->waiter->reqPizzaPhilosophers[1], sizeof(int)*sim->params->NUM_PIZZA);
				/* Alterar fila de pedidos, aumentando a prioridade dos filósofos que não foram atentidos */
				sim->waiter->reqPizzaPhilosophers[0] -= sim->params->NUM_PIZZA;
				memcpy(&sim->waiter->reqPizzaPhilosophers[1], &sim->waiter->reqPizzaPhilosophers[1+sim->params->NUM_PIZZA], sizeof(int)*sim->waiter->reqPizzaPhilosophers[0]);
			}

			
			up(S_REQUEST_ACCESS_PIZZA, 1);

			/* Guardar o nº de wake's sem request, para mais tarde fazer up no waiter. */
			down(S_NUMBER_UPS_PIZZA, 1);
			wake_without_request = sim->waiter->pizza_number_ups_without_request;
			sim->waiter->pizza_number_ups_without_request = 0;
			up(S_NUMBER_UPS_PIZZA, 1);
		}
		else if (sim->waiter->reqSpaghetti==W_ACTIVE){
			sim->waiter->state = W_REQUEST_SPAGHETTI;
			logger(sim);

			/* Operação de ir buscar spaghetti */
			replenish_spaghetti();

			down(S_REQUEST_ACCESS_SPAGHETTI, 1);
			/* Criar um array para guardar o id dos filósofos que irão ser acordados */
			phil_id = (int*)mem_alloc(sizeof(int)*sim->params->NUM_SPAGHETTI);
			
			/* Se o número de doses de spaghetti for maior que o nº de filósofos, acordar todos os filósofos */
			if (sim->waiter->reqSpaghettiPhilosophers[0]<sim->params->NUM_SPAGHETTI){
				n_phil = sim->waiter->reqSpaghettiPhilosophers[0];
				/* Copiar id's dos filósofos */
				memcpy(phil_id, &sim->waiter->reqSpaghettiPhilosophers[1], sizeof(int)*sim->waiter->reqSpaghettiPhilosophers[0]);
				/* Limpar fila de pedidos */
				sim->waiter->reqSpaghettiPhilosophers[0] = 0;
				sim->waiter->reqSpaghetti = W_INACTIVE;
			}
			
			/* Se o número de doses de spaghetii não for maior que o nº de filósofos, acordar apenas n filósofos, sendo n o nº de doses de spaghetti 'novas' */
			else{
				n_phil = sim->params->NUM_SPAGHETTI;
				/* Copiar id's dos filósofos que podem ser acordados */
				memcpy(phil_id, &sim->waiter->reqSpaghettiPhilosophers[1], sizeof(int)*sim->params->NUM_SPAGHETTI);
				sim->waiter->reqSpaghettiPhilosophers[0] -= sim->params->NUM_SPAGHETTI;
				/* Alterar fila de pedidos, aumentando a prioridade dos filósofos que não foram atentidos */
				memcpy(&sim->waiter->reqSpaghettiPhilosophers[1], &sim->waiter->reqSpaghettiPhilosophers[1+sim->params->NUM_SPAGHETTI], sizeof(int)*sim->waiter->reqSpaghettiPhilosophers[0]);
			}

			up(S_REQUEST_ACCESS_SPAGHETTI, 1);

			/* Guardar o nº de wake's sem request, para mais tarde fazer up no waiter. */
			down(S_NUMBER_UPS_SPAGHETTI, 1);
			wake_without_request = sim->waiter->spaghetti_number_ups_without_request;
			sim->waiter->spaghetti_number_ups_without_request = 0;
			up(S_NUMBER_UPS_SPAGHETTI, 1);
		}
		else if (sim->waiter->reqCutlery==W_ACTIVE){
			sim->waiter->state = W_REQUEST_CUTLERY;
			logger(sim);

			/* Operação de limpar talheres */
			clean_cutlery(cleans);
			down(S_REQUEST_ACCESS_CUTLERY, 1);

			/* Para todos os filósofos na fila, calcular quantos podem ser acordados, de acordo com os talheres disponíveis */
			for(i=0; i<sim->waiter->reqCutleryPhilosophers[0].num; i++){
				if (sim->waiter->reqCutleryPhilosophers[i+1].request==FORK){
					if (sim->waiter->reqCutleryPhilosophers[i+1].num<=cleans[FORK]){
						cleans[FORK] -= sim->waiter->reqCutleryPhilosophers[i+1].num;
					}
					else{
						break;
					}
				}
				else{
					if (sim->waiter->reqCutleryPhilosophers[i+1].num<=cleans[KNIFE]){
						cleans[KNIFE] -= sim->waiter->reqCutleryPhilosophers[i+1].num;
					}
					else{
						break;
					}

				}
			}
			
			
			n_phil = i;
			phil_id = (int*)mem_alloc(sizeof(int)*n_phil);

			/* Obter id dos filósofos a acordar */
			for (i=0; i<n_phil; i++){
				phil_id[i] = sim->waiter->reqCutleryPhilosophers[1+i].id;
			}


			sim->waiter->reqCutleryPhilosophers[0].num -= n_phil;
			/* Aumentar prioridade dos filósofos não atendidos na fila de espera */
			memcpy(&sim->waiter->reqCutleryPhilosophers[1], &sim->waiter->reqCutleryPhilosophers[1+n_phil], sizeof(Cutlery_Request)*sim->waiter->reqCutleryPhilosophers[0].num);
			if (sim->waiter->reqCutleryPhilosophers[0].num==0)
				sim->waiter->reqCutlery = W_INACTIVE;


			up(S_REQUEST_ACCESS_CUTLERY, 1);

			down(S_NUMBER_UPS_CUTLERY, 1);
			wake_without_request = sim->waiter->cutlery_number_ups_without_request;
			sim->waiter->cutlery_number_ups_without_request = 0;
			up(S_NUMBER_UPS_CUTLERY, 1);

		}
		
		logger(sim);

		int i;
        /* Efetuar up's de filósofos que efetuaram pedidos */
        if(n_phil!=0){
        	for (i=0; i<n_phil; i++){
        		up_phil(phil_id[i]);
        	}
        	free(phil_id);
        }

        /* up do waiter nº de vezes dos filósofos acordados */
		if (n_phil+wake_without_request>1)
			down_waiter(n_phil+wake_without_request-1);
		sim->waiter->state = W_SLEEP;
		logger(sim);
		down_waiter(1);

	}
	/* Limpar mesa no final */
	/* Não necessita de semáforos */
	/* Apenas 1 waiter e nenhum filósofo */
	if (sim->diningRoom->dirtyForks>0 || sim->diningRoom->dirtyKnives>0){
		sim->waiter->state = W_REQUEST_CUTLERY;
		logger(sim);
		clean_cutlery(cleans);
	}

	sim->waiter->state = W_DEAD;
	logger(sim);
}

static void clean_cutlery(int *cleans){
	get_cutlery();
	int wash_time = rand() % sim->params->WASH_TIME+1;
	usleep(wash_time*1000);
	replenish_cutlery(cleans);
}

static void replenish_pizza(void){
	add_pizza();
}

static void replenish_spaghetti(void){
	add_spaghetti();
}

