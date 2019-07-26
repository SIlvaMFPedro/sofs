/**
 * \brief Dining room data structures
 *  
 * \author Miguel Oliveira e Silva - 2016
 *         Ana Patrícia Gomes da Cruz
 *         Diogo Daniel Soares Ferreira
 *         João Pedro de Almeida Maia
 *         
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
   int dead_philosophers;      // Number of dead philosophers
} DiningRoom;


typedef enum {
   S_LOGGER,
   S_FORKS_COUNT,
   S_FORKS_ACCESS,
   S_DIRTY_FORKS_ACCESS,
   S_KNIVES_COUNT,
   S_KNIVES_ACCESS,
   S_DIRTY_KNIVES_ACCESS,
   S_DIRTY_CUTLERY_COUNT,
   S_PIZZA_COUNT,
   S_PIZZA_ACCESS,
   S_SPAGHETTI_COUNT,
   S_SPAGHETTI_ACCESS,
   S_WAITER,
   S_KILL_ACCESS,
   S_REQUEST_ACCESS_CUTLERY,
   S_REQUEST_ACCESS_PIZZA,
   S_REQUEST_ACCESS_SPAGHETTI,
   S_NUMBER_UPS_CUTLERY,
   S_NUMBER_UPS_PIZZA,
   S_NUMBER_UPS_SPAGHETTI,
   S_PHILOSOPHERS
} semaphores;

typedef enum {
   SHM_SIMULATION,
   SHM_WAITER,
   SHM_DININGROOM,
   SHM_PARAMS,
   SHM_REQUEST_CUTLERY,
   SHM_REQUEST_PIZZA,
   SHM_REQUEST_SPAGHETTI,
   SHM_PHIL,
   SHM_NUM_PHIL,
} SharedData_Enum;

void create(Simulation *s);

void connect(Simulation *s);

void destroy(Simulation *s);

void init_forks(void);

void init_knives(void);

void init_spaghetti(void);

void init_pizza(void);

void diningRoom_init(void);

/* Philospher actions */

void get_two_forks(int id);

void get_fork_knive(int id);

void drop_two_forks(int id);

void drop_fork_knive(int id);

void get_spaghetti(int id);

void get_pizza(int id);


/* Waiter Actions */

void get_cutlery(void);

void replenish_cutlery(int * cleans);

void add_pizza(void);

void add_spaghetti(void);

void down_waiter(int n);

void up_waiter(void);

/* Actions to be performed on philosophers */

void kill_phil(int id);

void down_phil(int id);

void up_phil(int id);

void down(int n_sem, int n_down);

void up(int n_sem, int n_ups);


#endif

