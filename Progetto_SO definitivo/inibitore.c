#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <time.h>
#include <math.h>
#include "common_file.h"

    		
double max;
struct shared_stats * my_stats;

void stop_inib(int sig){

    	my_stats->prob_scis = 100;
    	my_stats->limit = max;
    	raise(SIGSTOP);
}


int main(){
struct sigaction sa;

struct shmid_ds shm_info;
int sem_init, id_mqueue, id_mex,id_scis, id_stats;
double lim, level, proc_level, prob_s;
unsigned long energia;
double offset = 0, scis_offset;

sigset_t mask;
if((id_stats = shmget(STATS_KEY, sizeof(*my_stats), 0600)) == -1){
	TEST_ERROR;
	exit(0);
}
if((my_stats= shmat(id_stats, NULL, 0)) == NULL){
	TEST_ERROR;
	exit(0);
}
if((sem_init = semget(SEM_KEY, 2, 0600)) == -1){
	TEST_ERROR;
	exit(0);
}
if((id_mqueue = msgget(MEX_KEY, 0600)) == -1){
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

if(shmctl(id_stats, IPC_STAT, &shm_info) == -1){
	TEST_ERROR;
	exit(0);
}

bzero (&sa , sizeof(sa)); 
sa.sa_handler = stop_inib;
sigaction(SIGUSR1, &sa, NULL);

sigfillset(&mask);
sigdelset(&mask,SIGCONT);
sigdelset(&mask,SIGUSR1);
sigprocmask(SIG_SETMASK, &mask, NULL);


/*gestione offset per l'energia x * e^(-(x*x)/k), formula concepita con l'aiuto di Perplexity ai (fonte: https://www.perplexity.ai/) */
/*PRECISION è una costante di sicurezza, la quantità di energia che l'inbitore assorbe per valori piccoli di THERESHOLD aumenta*/
max = (double)my_stats->limit; 
offset = ENERGY_EXPLODE_THERESHOLD * exp(- (((unsigned long)ENERGY_EXPLODE_THERESHOLD * (unsigned long)ENERGY_EXPLODE_THERESHOLD) / PRECISION));
if(offset < 10000) offset = 10000; /*offset minimo*/

/*gestione offset per la scissione*/
if(N_NUOVI_ATOMI >= 1000){ 
	scis_offset = 100;
}else{
/*piu secondi metto minore percentuale ho
 piu atomi immetto piu percentuale ho*/
	scis_offset = ((100 -((double)STEP / 999999999) *100)+ (((double) N_NUOVI_ATOMI / (double) SYS_LIMIT) *100));
}



my_stats->limit = max - (max * ((float)offset / (float)ENERGY_EXPLODE_THERESHOLD));
my_stats->prob_scis = 100 - scis_offset;

/*sblocco il semaforo cosi gli atomi non saranno arrivati a leggere che non hanno limiti*/
my_op.sem_num = 4;
my_op.sem_flg = 0;
my_op.sem_op = -1;
semop(sem_init, &my_op, 1);


UNLOCK(0);

sigemptyset(&mask);
sigaddset(&mask,SIGUSR1);

while(1){

    	/*assorbimento energia*/
    	energia = my_stats->energia_prodotta - my_stats->energia_consumata + offset;

		/*se il livello scende allora il limite aumenta se il livello aumenta allora il limite diminuisce assorbendo piu energia*/
	level = (float)(my_stats->energia_prodotta - my_stats->energia_consumata + offset)/ (float)ENERGY_EXPLODE_THERESHOLD;
	lim = max - (max*level);
	
	/*limitazione scissione*/
	shmctl(id_stats, IPC_STAT, &shm_info);
	proc_level = (float) shm_info.shm_nattch/ (float)SYS_LIMIT;

	prob_s = (100 - proc_level*100) - scis_offset;
	
	sigprocmask(SIG_BLOCK, &mask, NULL);
	
	LOCK(3);
	
	(lim > 0) ? (my_stats->limit = lim) : (my_stats->limit = 0);
	
	(prob_s >=0) ? (my_stats->prob_scis = prob_s) : (my_stats->prob_scis = 0);
	
	UNLOCK(3);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
	
}
}

