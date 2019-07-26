/**
 * \brief Simulation data structure
 *  
 * \author Miguel Oliveira e Silva - 2016
 *         Ana Patrícia Gomes da Cruz
 *         Diogo Daniel Soares Ferreira
 *         João Pedro de Almeida Maia
 *         
 */

#ifndef SIMULATION_H
#define SIMULATION_H

struct _Waiter_;
struct _Parameters_;
struct _DiningRoom_;
struct _Philosopher_;

typedef struct _Simulation_ {
   struct _Parameters_* params;
   struct _DiningRoom_* diningRoom;
   struct _Philosopher_** philosophers;
   struct _Waiter_* waiter;
   int idWaiter;
   int semid;
   int *idPhil;
} Simulation;

#include "parameters.h"
#include "dining-room.h"
#include "philosopher.h"
#include "waiter.h"

Simulation* initSimulation(Simulation* sim, Parameters* params);
void* mem_alloc(int size);

#endif
