/**
 *  \brief Philosopher module
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
#include "philosopher.h"
#include "dining-room.h"
#include "simulation.h"
#include "logger.h"

#include <pthread.h>

extern Simulation *sim;

static void eat_pizza(unsigned int id);
static void eat_spaghetti(unsigned int id);
static void thinking(unsigned int id);
static void kill_philosopher(unsigned int id);


void philosopher_life(unsigned int id){
	
	int meal_prob;
   	srand(getpid());

	int lifetime = sim->params->PHILOSOPHER_MIN_LIVE + rand() % (sim->params->PHILOSOPHER_MAX_LIVE+1 - sim->params->PHILOSOPHER_MIN_LIVE);

	int cycle;
   	for(cycle=0; cycle<lifetime; cycle++){
		thinking(id);
	  	meal_prob = rand() % 100;
	  	if(meal_prob<sim->params->CHOOSE_PIZZA_PROB){
			 eat_pizza(id);
	  	}
	  	else{
		 	eat_spaghetti(id);
	  	}
   	}

   	kill_philosopher(id);


}


void eat_pizza(unsigned int id){
	sim->philosophers[id]->state = P_HUNGRY;
	sim->philosophers[id]->meal = P_GET_PIZZA;
	sim->philosophers[id]->cutlery[0] = P_GET_FORK;
	sim->philosophers[id]->cutlery[1] = P_GET_KNIFE;
	logger(sim);

	get_pizza(id);
	sim->philosophers[id]->meal = P_EAT_PIZZA;
	logger(sim);

	get_fork_knife(id);
	sim->philosophers[id]->state = P_EATING;
	
	int eat_time = rand() % sim->params->EAT_TIME+1;
	usleep(eat_time*1000);

	sim->philosophers[id]->state = P_FULL;
	sim->philosophers[id]->meal = P_NONE;
	logger(sim);

	drop_fork_knife(id);
	sim->philosophers[id]->cutlery[0] = P_NOTHING;
	sim->philosophers[id]->cutlery[1] = P_NOTHING;
	logger(sim);

}

void eat_spaghetti(unsigned int id){
	sim->philosophers[id]->state = P_HUNGRY;
	sim->philosophers[id]->meal = P_GET_SPAGHETTI;
	sim->philosophers[id]->cutlery[0] = P_GET_FORK;
	sim->philosophers[id]->cutlery[1] = P_GET_FORK;
	logger(sim);

	get_spaghetti(id);
	sim->philosophers[id]->meal = P_EAT_SPAGHETTI;
	logger(sim);
	
	get_two_forks(id);
	sim->philosophers[id]->state = P_EATING;

	int eat_time = rand() % sim->params->EAT_TIME+1;
	usleep(eat_time*1000);

	sim->philosophers[id]->state = P_FULL;
	sim->philosophers[id]->meal = P_NONE;
	logger(sim);

	drop_two_forks(id);
	sim->philosophers[id]->cutlery[0] = P_NOTHING;
	sim->philosophers[id]->cutlery[1] = P_NOTHING;
	logger(sim);
}

void thinking(unsigned int id){
	sim->philosophers[id]->state = P_THINKING;
	logger(sim);

	int think_time = rand() % sim->params->THINK_TIME+1;
	usleep(think_time*1000);
}

void kill_philosopher(unsigned int id){
	kill_phil(id);
	sim->philosophers[id]->state = P_DEAD;
	logger(sim);
}
