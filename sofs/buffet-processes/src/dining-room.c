/**
 * \brief Dining room
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
#include <getopt.h>
#include <assert.h>
#include <string.h>
#include <locale.h>
#include "parameters.h"
#include "dining-room.h"
#include "waiter.h"
#include "logger.h"

#include <sys/shm.h>
#include <sys/sem.h>
#include <pthread.h>

static const long key = 0x0001L;
static int *shmid;
Simulation * sim = NULL;

static void * get_shared_memory(int n, int size);
static void* connect_key(int n);

static void * get_shared_memory(int n, int size){
    void *mem;
    shmid[n] = shmget(key+n, size, 0600 | IPC_CREAT | IPC_EXCL);

    if (shmid[n] == -1)
    {
        perror("Fail creating shared data");
        exit(EXIT_FAILURE);
    }

    /* attach shared memory to process addressing space */ 
    mem = shmat(shmid[n], NULL, 0);
    if (mem == (void*)-1)
    {
        perror("Fail connecting to shared data");
        exit(EXIT_FAILURE);
    }
    return mem;
}

static void* connect_key(int n){
    void *c;
    /* get access to the shared memory */
    shmid[n] = shmget(key+n, 0, 0);
    if (shmid[n] == -1)
    {
        perror("Fail connecting to shared data");
        exit(EXIT_FAILURE);
    }

    /* attach shared memory to process addressing space */ 
    c = shmat(shmid[n], NULL, 0);
    if (c == (void*)-1)
    {
        perror("Fail connecting to shared data");
        exit(EXIT_FAILURE);
    }

    return c;
}

void up(int n_sem, int n_ups){
    struct sembuf up = {n_sem, n_ups, 0};
    if (semop(sim->semid, &up, 1) == -1)
    {
        perror("unlock");
        exit(EXIT_FAILURE);
    }
}

void down(int n_sem, int n_down){
    struct sembuf down_s = {n_sem, -n_down, 0};
    if (semop(sim->semid, &down_s, 1) == -1)
    {
        perror("lock");
        exit(EXIT_FAILURE);
    }
}

void create(Simulation *s){
	shmid = (int*)mem_alloc(sizeof(int)*(8+s->params->NUM_PHILOSOPHERS));
    sim = get_shared_memory(SHM_SIMULATION, sizeof(Simulation));
    sim->waiter = get_shared_memory(SHM_WAITER, sizeof(Waiter));
    sim->diningRoom = get_shared_memory(SHM_DININGROOM, sizeof(DiningRoom));
    sim->params = get_shared_memory(SHM_PARAMS, sizeof(Parameters));
    sim->philosophers = get_shared_memory(SHM_PHIL, sizeof(Philosopher*));
    int i;
    for (i=0; i<s->params->NUM_PHILOSOPHERS; i++){
        sim->philosophers[i] = get_shared_memory(SHM_NUM_PHIL+i, sizeof(Philosopher));
        sim->philosophers[i]->state = P_BIRTH;
        sim->philosophers[i]->meal = P_NONE;
        sim->philosophers[i]->cutlery[0] = P_NOTHING;
        sim->philosophers[i]->cutlery[1] = P_NOTHING;
    }

    /* create access locker */
    sim->semid = semget(key+SHM_SIMULATION, 20+s->params->NUM_PHILOSOPHERS, 0600 | IPC_CREAT | IPC_EXCL);
    if (sim->semid == -1)
    {
        perror("Fail creating locker semaphore");
        exit(EXIT_FAILURE);
    }

    memcpy(sim->params,s->params,sizeof(Parameters));
    memcpy(sim->diningRoom, s->diningRoom, sizeof(DiningRoom));
    memcpy(sim->waiter, s->waiter, sizeof(Waiter));

    sim->waiter->reqCutleryPhilosophers = get_shared_memory(SHM_REQUEST_CUTLERY, sizeof(Cutlery_Request)*(1+s->params->NUM_PHILOSOPHERS));
    sim->waiter->reqPizzaPhilosophers = get_shared_memory(SHM_REQUEST_PIZZA, sizeof(int)*(1+s->params->NUM_PHILOSOPHERS));
    sim->waiter->reqSpaghettiPhilosophers = get_shared_memory(SHM_REQUEST_SPAGHETTI, sizeof(int)*(1+s->params->NUM_PHILOSOPHERS));

    /* detach shared memory from process addressing space */
    shmdt(sim);
    sim = NULL;
}

void connect(Simulation *s){
    sim = connect_key(SHM_SIMULATION);
    sim->waiter = connect_key(SHM_WAITER);
    sim->diningRoom = connect_key(SHM_DININGROOM);
    sim->params = connect_key(SHM_PARAMS);
    sim->philosophers = connect_key(SHM_PHIL);
    int i;
    for (i=0; i<s->params->NUM_PHILOSOPHERS; i++){
        sim->philosophers[i] = connect_key(SHM_NUM_PHIL+i);
    }
    sim->waiter->reqCutleryPhilosophers = connect_key(SHM_REQUEST_CUTLERY);
    sim->waiter->reqPizzaPhilosophers = connect_key(SHM_REQUEST_PIZZA);
    sim->waiter->reqSpaghettiPhilosophers = connect_key(SHM_REQUEST_SPAGHETTI);
}

void destroy(Simulation *s){
    
	/* destroy the locker semaphore */
    semctl(sim->semid, 0, IPC_RMID, NULL);

    /* detach shared memory from process addressing space */
    shmdt(sim);
    sim = NULL;

    /* ask OS to destroy the shared memory */
    int shm_keys[] = {SHM_SIMULATION, SHM_WAITER, SHM_DININGROOM, SHM_PARAMS, SHM_PHIL, SHM_REQUEST_CUTLERY, SHM_REQUEST_PIZZA, SHM_REQUEST_SPAGHETTI};
    int i;
    for (i=0;i<8; i++){
        shmctl(shmid[shm_keys[i]], IPC_RMID, NULL);
        shmid[shm_keys[i]] = -1;
    }

    for (i=0; i<s->params->NUM_PHILOSOPHERS; i++){
        shmctl(shmid[SHM_NUM_PHIL+i], IPC_RMID, NULL);
        shmid[SHM_NUM_PHIL+i] = -1;
    }


    free(shmid);

}

void init_forks(){
    up(S_FORKS_COUNT, sim->params->NUM_FORKS);
    up(S_FORKS_ACCESS, 1);
    up(S_DIRTY_FORKS_ACCESS, 1);
}

void init_knives(){
    up(S_KNIVES_COUNT, sim->params->NUM_KNIVES);
    up(S_KNIVES_ACCESS, 1);
    up(S_DIRTY_KNIVES_ACCESS, 1);
}

void init_spaghetti(){
    up(S_SPAGHETTI_COUNT, sim->params->NUM_SPAGHETTI);
    up(S_SPAGHETTI_ACCESS, 1);
}

void init_pizza(){
    up(S_PIZZA_COUNT, sim->params->NUM_PIZZA);
    up(S_PIZZA_ACCESS, 1);
}

void diningRoom_init(){
    up(S_KILL_ACCESS, 1);
    up(S_REQUEST_ACCESS_CUTLERY, 1);
    up(S_REQUEST_ACCESS_PIZZA, 1);
    up(S_REQUEST_ACCESS_SPAGHETTI, 1);
    up(S_NUMBER_UPS_SPAGHETTI, 1);
    up(S_NUMBER_UPS_PIZZA, 1);
    up(S_NUMBER_UPS_CUTLERY, 1);
	up(S_LOGGER, 1);
}

void get_two_forks(int id){
	struct sembuf down_s = {S_FORKS_COUNT, -2, IPC_NOWAIT};

    /* While there are no forks available,
    request waiter */
    if (semop(sim->semid, &down_s, 1) == -1)
    {   
        /* Fill Request to waiter */
        down(S_REQUEST_ACCESS_CUTLERY, 1);
        

        /* Se já existem garfos disponíveis, não efetuar pedido e voltar a tentar */
        if(semctl(sim->semid, S_FORKS_COUNT, GETVAL)>=2){
            up(S_REQUEST_ACCESS_CUTLERY, 1);
            return get_two_forks(id);
        }

        /* Adicionar o filósofo à fila de espera para bloqueio */
        sim->waiter->reqCutlery = W_ACTIVE;
        sim->waiter->reqCutleryPhilosophers[0].num+=1;
        sim->waiter->reqCutleryPhilosophers[sim->waiter->reqCutleryPhilosophers[0].num].id=id;
        sim->waiter->reqCutleryPhilosophers[sim->waiter->reqCutleryPhilosophers[0].num].request=FORK;
        sim->waiter->reqCutleryPhilosophers[sim->waiter->reqCutleryPhilosophers[0].num].num=2;

        up(S_REQUEST_ACCESS_CUTLERY, 1);
        
        /* Wake up waiter */
        up_waiter();
        
        /* Hungry Philosopher waits for waiter */
        down_phil(id);

        return get_two_forks(id);

    }

    /* If there are no forks, wake waiter, but don't wait for request fulfilled */
    down(S_FORKS_ACCESS, 1);
	sim->diningRoom->cleanForks-=2;
    sim->philosophers[id]->cutlery[0] = P_FORK;
    sim->philosophers[id]->cutlery[1] = P_FORK;
    if (sim->diningRoom->cleanForks<=1){
        down(S_NUMBER_UPS_CUTLERY, 1);
        sim->waiter->cutlery_number_ups_without_request+=1;
        up(S_NUMBER_UPS_CUTLERY, 1);
        sim->waiter->reqCutlery=W_ACTIVE;
        up_waiter();
    }
    logger(sim);
    up(S_FORKS_ACCESS, 1);

}

void get_fork_knive(int id){
	struct sembuf down_s = {S_FORKS_COUNT, -1, IPC_NOWAIT};
    if (semop(sim->semid, &down_s, 1) == -1)
    {

        /* Fill Request to waiter */
        down(S_REQUEST_ACCESS_CUTLERY, 1);

        /* Se já existem garfos disponíveis, não efetuar pedido e voltar a tentar */
        if(semctl(sim->semid, S_FORKS_COUNT, GETVAL)>=1){
            up(S_REQUEST_ACCESS_CUTLERY, 1);
            return get_fork_knive(id);
        }

        /* Adicionar o filósofo à fila de espera para bloqueio */
        sim->waiter->reqCutlery = W_ACTIVE;
        sim->waiter->reqCutleryPhilosophers[0].num+=1;
        sim->waiter->reqCutleryPhilosophers[sim->waiter->reqCutleryPhilosophers[0].num].id=id;
        sim->waiter->reqCutleryPhilosophers[sim->waiter->reqCutleryPhilosophers[0].num].request=FORK;
        sim->waiter->reqCutleryPhilosophers[sim->waiter->reqCutleryPhilosophers[0].num].num=1;

        up(S_REQUEST_ACCESS_CUTLERY, 1);

        /* Wake up waiter */
        up_waiter();
        /* Hungry Philosopher waits for waiter */
        down_phil(id);

        return get_fork_knive(id);

    }
    struct sembuf down2 = {S_KNIVES_COUNT, -1, IPC_NOWAIT};
    if (semop(sim->semid, &down2, 1) == -1)
    {
        /* Fill Request to waiter */
        down(S_REQUEST_ACCESS_CUTLERY, 1);

        /* Se já existem facas disponíveis, não efetuar pedido e voltar a tentar */
        if(semctl(sim->semid, S_KNIVES_COUNT, GETVAL)>=1){
            up(S_FORKS_COUNT, 1);
            up(S_REQUEST_ACCESS_CUTLERY, 1);
            return get_fork_knive(id);
        }

        /* Adicionar o filósofo à fila de espera para bloqueio */
        sim->waiter->reqCutlery = W_ACTIVE;
        sim->waiter->reqCutleryPhilosophers[0].num+=1;
        sim->waiter->reqCutleryPhilosophers[sim->waiter->reqCutleryPhilosophers[0].num].id=id;
        sim->waiter->reqCutleryPhilosophers[sim->waiter->reqCutleryPhilosophers[0].num].request=KNIFE;
        sim->waiter->reqCutleryPhilosophers[sim->waiter->reqCutleryPhilosophers[0].num].num=1;

        up(S_REQUEST_ACCESS_CUTLERY, 1);

        /* Wake up waiter */
        up_waiter();

        /* Hungry Philosopher waits for waiter */
        down_phil(id);

        up(S_FORKS_COUNT, 1);
        return get_fork_knive(id);

    }

    down(S_FORKS_ACCESS, 1);
    down(S_KNIVES_ACCESS, 1);
    
    /* If there are no forks or knives, wake waiter, but don't wait for request fulfilled */
	sim->diningRoom->cleanForks-=1;
	sim->diningRoom->cleanKnives-=1;
    sim->philosophers[id]->cutlery[0] = P_FORK;
    sim->philosophers[id]->cutlery[1] = P_KNIFE;
    if (sim->diningRoom->cleanForks<=1 || sim->diningRoom->cleanKnives==0){
        down(S_NUMBER_UPS_CUTLERY, 1);
        sim->waiter->cutlery_number_ups_without_request+=1;
        up(S_NUMBER_UPS_CUTLERY, 1);
        sim->waiter->reqCutlery=W_ACTIVE;
        up_waiter();
    }
    logger(sim);

    up(S_KNIVES_ACCESS, 1);
	up(S_FORKS_ACCESS, 1);
}

void drop_two_forks(int id){
	down(S_DIRTY_FORKS_ACCESS, 1);

	sim->diningRoom->dirtyForks+=2;
    sim->philosophers[id]->cutlery[0] = P_PUT_FORK;
    sim->philosophers[id]->cutlery[1] = P_PUT_FORK;
    logger(sim);

	up(S_DIRTY_FORKS_ACCESS, 1);
    up(S_DIRTY_CUTLERY_COUNT, 2);
 
}

void drop_fork_knive(int id){
	down(S_DIRTY_FORKS_ACCESS, 1);
    down(S_DIRTY_KNIVES_ACCESS, 1);

	sim->diningRoom->dirtyForks+=1;
	sim->diningRoom->dirtyKnives+=1;
    sim->philosophers[id]->cutlery[0] = P_PUT_FORK;
    sim->philosophers[id]->cutlery[1] = P_PUT_KNIFE;
    logger(sim);

	up(S_DIRTY_KNIVES_ACCESS, 1);
    up(S_DIRTY_FORKS_ACCESS, 1);
    up(S_DIRTY_CUTLERY_COUNT, 2);
    
}

void get_spaghetti(int id){
	
    struct sembuf down_s = {S_SPAGHETTI_COUNT, -1, IPC_NOWAIT};
    if (semop(sim->semid, &down_s, 1) == -1)
    {
        /* Fill Request to waiter */
        down(S_REQUEST_ACCESS_SPAGHETTI, 1);

        /* Se já existe esparguete, não efetuar pedido e voltar a tentar */
        if(semctl(sim->semid, S_SPAGHETTI_COUNT, GETVAL)>=1){
            up(S_REQUEST_ACCESS_SPAGHETTI, 1);
            return get_spaghetti(id);
        }

        /* Adicionar o filósofo à fila de espera para bloqueio */
        sim->waiter->reqSpaghetti = W_ACTIVE;
        sim->waiter->reqSpaghettiPhilosophers[0]+=1;
        sim->waiter->reqSpaghettiPhilosophers[sim->waiter->reqSpaghettiPhilosophers[0]]=id;

        up(S_REQUEST_ACCESS_SPAGHETTI, 1);

        /* Wake up waiter */
        up_waiter();

        /* Hungry Philosopher waits for waiter */
        down_phil(id);

        return get_spaghetti(id);
    }

    /* If there is no spaghetti, wake waiter, but don't wait for request fulfilled */
    down(S_SPAGHETTI_ACCESS, 1);
	sim->diningRoom->spaghetti-=1;
    if (sim->diningRoom->spaghetti==0){
        down(S_NUMBER_UPS_SPAGHETTI, 1);
        sim->waiter->spaghetti_number_ups_without_request+=1;
        up(S_NUMBER_UPS_SPAGHETTI, 1);
        sim->waiter->reqSpaghetti=W_ACTIVE;
        up_waiter();
    }
	up(S_SPAGHETTI_ACCESS, 1);
}

void get_pizza(int id){
	struct sembuf down_s = {S_PIZZA_COUNT, -1, IPC_NOWAIT};
    if (semop(sim->semid, &down_s, 1) == -1)
    {
        /* Fill Request to waiter */
        down(S_REQUEST_ACCESS_PIZZA, 1);

        
        /* Se já existe pizza, não efetuar pedido e voltar a tentar */
        if(semctl(sim->semid, S_PIZZA_COUNT, GETVAL)>=1){
            up(S_REQUEST_ACCESS_PIZZA, 1);
            return get_pizza(id);
        }
        
        /* Adicionar o filósofo à fila de espera para bloqueio */
        sim->waiter->reqPizza = W_ACTIVE;
        sim->waiter->reqPizzaPhilosophers[0]+=1;
        sim->waiter->reqPizzaPhilosophers[sim->waiter->reqPizzaPhilosophers[0]]=id;

        up(S_REQUEST_ACCESS_PIZZA, 1);

        /* Wake up waiter */
        up_waiter();

        /* Hungry Philosopher waits for waiter */
        down_phil(id);

        return get_pizza(id);
    }

    /* If there is no pizza, wake waiter, but don't wait for request fulfilled */
    down(S_PIZZA_ACCESS, 1);
	sim->diningRoom->pizza-=1;
    if (sim->diningRoom->pizza==0){
        down(S_NUMBER_UPS_PIZZA, 1);
        sim->waiter->pizza_number_ups_without_request+=1;
        up(S_NUMBER_UPS_PIZZA, 1);
        sim->waiter->reqPizza=W_ACTIVE;
        up_waiter();
    }
	up(S_PIZZA_ACCESS, 1);
}

void get_cutlery(){
    down(S_DIRTY_CUTLERY_COUNT,1);
	down(S_DIRTY_FORKS_ACCESS, 1);
    down(S_DIRTY_KNIVES_ACCESS, 1);
    int df = sim->diningRoom->dirtyForks;
    int dk = sim->diningRoom->dirtyKnives;
	sim->diningRoom->dirtyForksInWaiter = sim->diningRoom->dirtyForks;
	sim->diningRoom->dirtyKnivesInWaiter = sim->diningRoom->dirtyKnives;
	sim->diningRoom->dirtyForks = 0;
	sim->diningRoom->dirtyKnives = 0;

	up(S_DIRTY_KNIVES_ACCESS, 1);
    up(S_DIRTY_FORKS_ACCESS, 1);

    if (df+dk>1){
        down(S_DIRTY_CUTLERY_COUNT,df+dk-1);
    }
}

void replenish_cutlery(int *cleans){

	down(S_FORKS_ACCESS, 1);
    down(S_KNIVES_ACCESS, 1);

    int clean_forks = sim->diningRoom->dirtyForksInWaiter;
    int clean_knives = sim->diningRoom->dirtyKnivesInWaiter;

	sim->diningRoom->cleanForks += sim->diningRoom->dirtyForksInWaiter;
	sim->diningRoom->cleanKnives += sim->diningRoom->dirtyKnivesInWaiter;
	sim->diningRoom->dirtyForksInWaiter = 0;
	sim->diningRoom->dirtyKnivesInWaiter = 0;

    cleans[FORK] = sim->diningRoom->cleanForks;
    cleans[KNIFE] = sim->diningRoom->cleanKnives;

	up(S_KNIVES_ACCESS, 1);
    up(S_FORKS_ACCESS, 1);

    if (clean_forks>0)
        up(S_FORKS_COUNT, clean_forks);
    if (clean_knives>0)
        up(S_KNIVES_COUNT, clean_knives);

}

void add_pizza(){
    int c = 0;
    down(S_PIZZA_ACCESS, 1);
    if(sim->diningRoom->pizza==0){
        c = 1;
        sim->diningRoom->pizza+=sim->params->NUM_PIZZA;
    }
	up(S_PIZZA_ACCESS, 1);
    if (c==1)
        up(S_PIZZA_COUNT, sim->params->NUM_PIZZA);
}

void add_spaghetti(){
    int c = 0;
	down(S_SPAGHETTI_ACCESS, 1);
    if(sim->diningRoom->spaghetti==0){
        c = 1;
        sim->diningRoom->spaghetti+=sim->params->NUM_SPAGHETTI;
    }
	up(S_SPAGHETTI_ACCESS, 1);
    if (c==1)
	   up(S_SPAGHETTI_COUNT, sim->params->NUM_SPAGHETTI);
}

void down_waiter(int n){
  
    down(S_WAITER, n);
}

void up_waiter(){
    up(S_WAITER, 1);
}

void kill_phil(int id){
    int t = 0;
    down(S_KILL_ACCESS, 1);

    sim->diningRoom->dead_philosophers+=1;
    /* Check if there are still alive philosophers */
    if(sim->diningRoom->dead_philosophers==sim->params->NUM_PHILOSOPHERS)
        t=1;
    
    up(S_KILL_ACCESS, 1);

    /* If there are not alive philosopers, signal waiter */
    if (t==1)
        up_waiter();
}

void down_phil(int id){
    down(S_PHILOSOPHERS+id, 1);
}

void up_phil(int id){
    up(S_PHILOSOPHERS+id, 1);
}
