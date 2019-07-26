/**
 *  \brief Waiter data structures
 *  
 * \author Miguel Oliveira e Silva - 2016
 */

#ifndef WAITER_H
#define WAITER_H

typedef enum {
   W_NONE,                           // waiter initial state
   W_SLEEP,                          // waiter sleeping (waiting for requests)
   W_REQUEST_CUTLERY,                // waiter processing a request for clean cutlery
   W_REQUEST_PIZZA,                  // waiter processing a request for pizza meals
   W_REQUEST_SPAGHETTI,              // waiter processing a request for spaghetti meals
   W_DEAD                            // waiter is dead
} WaiterState;

typedef enum {
   W_INACTIVE,                       // no pending request
   W_ACTIVE                          // a pending request is active
} WaiterPendingState;

typedef enum{
   FORK,
   KNIFE
}Fork_Knife;

typedef struct _Cutlery_Request_{
   int id;
   Fork_Knife request;
   int num;
}Cutlery_Request;


typedef struct _Waiter_ {
   WaiterState state;                // current waiter state
   WaiterPendingState reqCutlery;    // waiter's request cutlery state
   WaiterPendingState reqPizza;      // waiter's request pizza state
   WaiterPendingState reqSpaghetti;  // waiter's request spaghetti state
   Cutlery_Request *reqCutleryPhilosophers;      // Array with philosophers' id of cutlery requests
   int *reqPizzaPhilosophers;        // Array with philosophers' id of pizza requests
   int *reqSpaghettiPhilosophers;    // Array with philosophers' id of spaghetti requests
   int cutlery_number_request_withou_need;
   int pizza_number_request_withou_need;
   int spaghetti_number_request_withou_need;
} Waiter;


void init_waiter(void);

void waiter_life(void);

#endif
