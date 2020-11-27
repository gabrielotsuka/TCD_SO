#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>

#define gettid() syscall(SYS_gettid)
#define MEM_SZ   140
#define QUEUE_SZ 10

struct shared_area{ 
	int num;
	sem_t mutex;
	sem_t sync;
	int pids[8];
	int queue[QUEUE_SZ];
};

struct shared_area *shared_area_ptr;

int criaFilhos();
struct shared_area * criaMemoriaCompartilhada();
void  inicializaMemoriaCompartilhada();
void  inicializaSemaforo(sem_t * semaforo);
void  p1p2p3Produtor(int id);
void* p4Consumidor(void * arg);
void  p4CriaThread();

int main(){
	criaMemoriaCompartilhada(9827);
	
	inicializaSemaforo((sem_t *)&shared_area_ptr->mutex);
	inicializaSemaforo((sem_t *)&shared_area_ptr->sync);

	int id = criaFilhos();	

	if ( id <= 3 ){
		p1p2p3Produtor(id);
	}
	else if ( id == 4 ){
		while(1) {
			signal(SIGUSR1, p4CriaThread);
		}
	}
	else if ( id == 5 ){
	}
	else if ( id == 6 ){
	}
	else if ( id == 7 ){
	}
	else if ( id == 8 ){
	}
	
	exit(0); /* Executado por todos os processos ao finalizarem */
}

struct shared_area * criaMemoriaCompartilhada(int keySM) {
	key_t key=keySM;
	void *shared_memory = (void *)0;
	int shmid;

	shmid = shmget(key,MEM_SZ,0666|IPC_CREAT);
	if ( shmid == -1 ) {
		printf("shmget falhou\n");
		exit(-1);
	}

	printf("shmid=%d\n",shmid);
  
	shared_memory = shmat(shmid,(void*)0,0);
  
	if (shared_memory == (void *) -1 ) {
		printf("shmat falhou\n");
		exit(-1);
  	}
	
	printf("Endereco de F1: %p\n", shared_memory);

	shared_area_ptr = (struct shared_area *) shared_memory;

	inicializaMemoriaCompartilhada();
}

void  inicializaMemoriaCompartilhada() {
	shared_area_ptr->pids[0] = getpid();
	int i;
	for (i = 0; i < MEM_SZ; i++)
		shared_area_ptr->queue[i] = 0;
	for (i = 0; i < 8; i++)
		shared_area_ptr->pids[i] = 0; 
	shared_area_ptr->num=0;
}

void  inicializaSemaforo(sem_t * semaforo) {
	if ( sem_init(semaforo,1,1) != 0 ) {
		printf("sem_init mutex falhou\n");
		exit(-1);
	}
}

int criaFilhos() {
	pid_t p;
	int id=0;
	sem_wait((sem_t*)&shared_area_ptr->sync);
	for(id=1; id<=7; id++){
		p = fork();
		if ( p < 0 ) {
			printf("Erro no fork()\n");
			exit(-1);
		}
		if ( p == 0 ){
			sem_wait((sem_t*)&shared_area_ptr->sync);
			break;
		}
		shared_area_ptr->pids[id] = p; 
	}

	if(p > 0) {
		for (int i = 0; i < 8; ++i) 
			sem_post((sem_t*)&shared_area_ptr->sync);
		wait(NULL);
	} 
		
	return id;
}

void p1p2p3Produtor(int id) {
	srand(getpid());
	while (shared_area_ptr->num <= 9) {
		sem_wait((sem_t*)&shared_area_ptr->mutex);
		if (shared_area_ptr->num < 9) {
			shared_area_ptr->queue[shared_area_ptr->num] = rand()%1000;
			shared_area_ptr->num++;
			sem_post((sem_t*)&shared_area_ptr->mutex);
		} 
		else if (shared_area_ptr->num == 9){
			shared_area_ptr->queue[shared_area_ptr->num] = rand()%1000;
			shared_area_ptr->num++;
			printf("Signal de p%d para p4!\n", id);
			while(kill(shared_area_ptr->pids[4], SIGUSR1) == -1);
			sem_post((sem_t*)&shared_area_ptr->mutex);
			break;
		} else {
			sem_post((sem_t*)&shared_area_ptr->mutex);
			break; 
		}
	}
}

void p4CriaThread() {
	shared_area_ptr->num = 0;
	int i;
	for(i=0; i<10; i++) {
		printf("%d ", shared_area_ptr->queue[i]);
	}
	printf("\n\n");
	pthread_t thread2;
	pthread_create(&thread2, NULL, p4Consumidor, NULL);
	p4Consumidor(NULL);
	pthread_join(thread2, NULL);
	printf("finalizamo\n");
	exit(0);
}

void* p4Consumidor(void * arg) {
	while (shared_area_ptr->num <= 9) {
		sem_wait((sem_t*)&shared_area_ptr->mutex);
		if (shared_area_ptr->num < 9) {
			shared_area_ptr->num++;
			sem_post((sem_t*)&shared_area_ptr->mutex);
		} 
		else if (shared_area_ptr->num == 9){
			shared_area_ptr->num++;
			sem_post((sem_t*)&shared_area_ptr->mutex);
			break;
		} else {
			sem_post((sem_t*)&shared_area_ptr->mutex);
			break; 
		}
	}
}