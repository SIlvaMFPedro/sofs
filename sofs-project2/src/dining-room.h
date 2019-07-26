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

void init_philosophers(void);

void init_dinner(void);

void diningRoom_init(void);

void init_pizza(void);

void init_spaghetti(void);

void init_forks(void);

void init_knives(void);

void init_kill(void);

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

void unlock_waiter(void);

#endif

