#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/resource.h>
#include <stdint.h>
#include <time.h>
#include <sys/shm.h>
#include "common_file.h"


char fr = 1;

void free_heap(int signo) { 
    //devo liberare lo heap e siccome free non è una syscall signal safe uso un flag
    fr = 0;
}

int main(){
pid_t * atoms;
int sem_init, i = 0, size, id_mex, id_scis, j = 0, id_stats, hit_miss, p = 0;
struct msgbuf_atom atom_pid;
struct shared_stats * my_stats;
struct sigaction sa;


sigset_t mask;
srand(getpid());

sigfillset(&mask);
sigdelset(&mask,SIGTERM);
sigprocmask(SIG_SETMASK, &mask, NULL);

if((sem_init = semget(SEM_KEY, 1, IPC_CREAT | 0600)) == -1){
	TEST_ERROR;
	exit(0);
}
if((id_mex = msgget(ATOM_KEY, IPC_CREAT | 0600)) == -1){
	TEST_ERROR;
	exit(0);
}
if((id_scis = msgget(SCIS_KEY, IPC_CREAT | 0600)) == -1){
	TEST_ERROR;
	exit(0);
}


/*alloco in heap abbastanza spazio per 100 atomi*/
if((atoms = malloc(sizeof(*atoms) * (N_ATOMI_INIT +100))) == NULL){
	TEST_ERROR;
	exit(0);
}

size = N_ATOMI_INIT +100;

if((id_stats = shmget(STATS_KEY, sizeof(*my_stats), 0600)) == -1){
	TEST_ERROR;
	exit(0);
}

if((my_stats= shmat(id_stats, NULL, 0)) == NULL){
	TEST_ERROR;
	exit(0);
}


bzero (&sa , sizeof(sa)); 
sa.sa_handler = free_heap;
sigaction(SIGTERM, &sa, NULL);

UNLOCK(0);
	
/*inizio con solo un atomo, il resto verrà prelevato durante l'esecuzione del ciclo*/
msgrcv(id_mex, &atom_pid, sizeof(atom_pid) - sizeof(long), 0, 0); /*prelevo un atomo iniziale*/
atoms[0] = atom_pid.mtype;
j = 1;
i = 0; 

while(fr){
	/*scelta modulare per atomo da scindere*/
	i = i % j;

	/*scissione*/
	if(kill(atoms[i], 0) == 0){

		atom_pid.mtype = atoms[i];
		
		if(msgsnd(id_scis, &atom_pid, sizeof(atom_pid) -sizeof(long), 0) != -1){
	
		LOCK(2);
    			my_stats->num_attivati += 1;
    		UNLOCK(2);	
 
		}else if(errno == EIDRM){break;}
	}
	
	
	/*se è presente: ricezione nuovo atomo da scindere, in caso contrario non aspetta nuovi atomi e riprova a fare la scissione*/
	if(msgrcv(id_mex, &atom_pid, sizeof(atom_pid) - sizeof(long), 0, IPC_NOWAIT) == 0){
		if(j > size-1){
			
			if((atoms = realloc(atoms, sizeof(pid_t) * (size + 1000)))  == NULL){
				dprintf(2,"\nCRITICAL ERROR: HEAP OVERLOAD\n");
				free(atoms); /*heap rimepito troppo quindi lo libero ed esco*/
				exit(0);
			}

			size = size + 1000;
		}
		atoms[j] = atom_pid.mtype;
		j = j+1;
	}else if(errno == EIDRM){break;}

	i++;
}


free(atoms);

return 0;
}
