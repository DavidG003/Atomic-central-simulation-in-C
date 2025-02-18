#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/shm.h>
#include <string.h>
#include "common_file.h"


int main(){
int sem_init,id_mqueue, id_mex, id_scis, id_stats, max, num_atom, new_atom, term = 0;
unsigned long increase, absorbed = 0, inib, temp = 0;
unsigned char my_prob, death;
char * args[] = {N_ATOMO, NULL};
pid_t son_pid;
sigset_t mask;
struct my_msgbuf rs_num;
struct msgbuf_atom atom_pid;
struct sigaction sa;
struct shared_stats * my_stats;



/*controlli degli id*/
if((id_stats  = shmget(STATS_KEY, sizeof(*my_stats), 0600)) == -1) exit(-1);
if((my_stats= shmat(id_stats, NULL, 0)) == (void *) -1) exit(-1);
if((sem_init  = semget(SEM_KEY, 1, 0600)) == -1) exit(-1);
if((id_mqueue = msgget(MEX_KEY , 0600)) == -1) exit(-1);
if((id_mex    = msgget(ATOM_KEY, 0600)) == -1) exit(-1);
if((id_scis   = msgget(SCIS_KEY, 0600)) == -1) exit(-1);  

srand(getpid());

sigfillset(&mask);
sigdelset(&mask,SIGCHLD);
sigprocmask(SIG_SETMASK, &mask, NULL);

if(msgrcv(id_mqueue, &rs_num, sizeof(rs_num) - sizeof(long), getpid(), 0) == -1 && (errno == EIDRM || errno == EINVAL)){
	exit(0);
}
num_atom = rs_num.num_atomico;

/*gestione zombie: atomo padre non aspetta atomo figlio*/
bzero(&sa , sizeof(sa)); 
sa.sa_flags = SA_NOCLDWAIT;
sigaction(SIGCHLD, &sa, NULL);


/*invio la diponibilità al attivatore per la scissione*/
atom_pid.mtype = getpid(); /*uso il suo stesso pid per il tipo*/
msgsnd(id_mex, &atom_pid, sizeof(atom_pid) -sizeof(long), 0);
if(errno == EINVAL || errno == EIDRM) exit(0);

UNLOCK(0);

inib = ((N_ATOM_MAX/2) * (N_ATOM_MAX/2)) - (N_ATOM_MAX/2);
while(1){


    while(msgrcv(id_scis, &atom_pid, sizeof(atom_pid) - sizeof(long), getpid(), 0) == -1 && errno == EINTR);
   // if(errno == EIDRM || errno == EINVAL) exit(-1);
    
    death = rand() % 100;
    
    if(death >= my_stats->prob_scis) term = 1; /*la scissione puo produrre una scoria anche con un criterio probabilistico grazie al inibitore*/

    if(num_atom <= MIN_N_ATOMICO || term == 1){

		LOCK(1);
    		my_stats->atomi_terminati += term; /*term puo valere 1 solo se inibitore è attivo*/
    		my_stats->scorie  = my_stats->scorie + 1;

    		UNLOCK(1);
    		
    		while(msgrcv(id_scis, &atom_pid, sizeof(atom_pid) - sizeof(long), getpid(), IPC_NOWAIT) != -1); //consumo tutti i mex di scissione visto che non avviene piu alcuna scissione per questo atomo
    		shmdt(my_stats);
		exit(0);
	}else{ /*scindo atomi*/
    	switch(son_pid = fork()){
	case -1:
		
		/*uso il tipo 1 per ricevere il pid del padre che è sempre stato in coda fin dall'inizio*/
		
		if(msgrcv(id_mqueue, &rs_num, sizeof(rs_num) - sizeof(long), 1, 0) == -1 && (errno == EIDRM || errno == EINVAL || errno == EINTR)){
			//solo il primo processo che non riesce a creare preleva il pid e invia il segnale di sigusr1 al padre 
			exit(0);
		}
		kill(rs_num.num_atomico, SIGUSR1);/*segnale di meltdown per il padre*/
		exit(0);
	case 0:
		//figli atomi
		
		execve(N_ATOMO, args, NULL);
		TEST_ERROR;/*solo se execve fallisce*/
		exit(0);
	default:
 		LOCK(1);

    		my_stats->num_scissi++;
    		
    		UNLOCK(1);
    		temp = num_atom;
   	 	rs_num.num_atomico = 1 + (rand() % (num_atom - 1));
    		num_atom = num_atom - rs_num.num_atomico;
    		rs_num.mtype = son_pid; 
    		while(msgsnd(id_mqueue, &rs_num, sizeof(rs_num) -sizeof(long), 0) == -1 && errno == EINTR);
    		if(errno == EIDRM|| errno == EINVAL) exit(0);
	} 
     }
    max = (num_atom > rs_num.num_atomico) ? num_atom : rs_num.num_atomico;
    increase = (num_atom * rs_num.num_atomico) - max;
    
    if(my_stats->limit < inib){
    /*se è presente un limite assorbo energia*/
	LOCK(3);
         /*un modo per troncare l'energia liberata*/
	 if(increase > my_stats->limit){/*condizione possibilmente vera solo con inibitore attivo*/
		absorbed = increase - my_stats->limit;
    		increase = my_stats->limit;
    	}
    	
   	LOCK(1);
       
    	my_stats->energia_prodotta += increase;
    	my_stats->energia_assorbita+= absorbed;
    
   	UNLOCK(1);
   	UNLOCK(3);
   	absorbed = 0;	
    } else {
    /*altrimenti non devo assorbire niente*/
    	LOCK(1);
       
    	my_stats->energia_prodotta += increase;
    
   	UNLOCK(1);
    }
}

return 0;
}
