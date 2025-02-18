/*file preso dagli esercizi di "System V: IPC objects and message queues" e modificato opportunamente
	fonte: https://informatica.i-learn.unito.it/mod/folder/view.php?id=225028
*/

#ifndef COMMON_FILE_H_
#define COMMON_FILE_H_
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>
#include <sys/sem.h>


/*macro progettate in base all'esempio fornito in "System V: shared memory"
fonte: https://informatica.i-learn.unito.it/mod/folder/view.php?id=225044
*/
#define LOCK(x)	my_op.sem_num = (x); \
		my_op.sem_flg = 0; \
   		my_op.sem_op = -1; \
    		while(semop(sem_init, &my_op, 1) == -1 && errno == EINTR); 

#define UNLOCK(x)  my_op.sem_num = x; \
    		my_op.sem_flg = 0; \
    		my_op.sem_op = +1; \
    		semop(sem_init, &my_op, 1);

#define TEST_ERROR    if (errno) {fprintf(stderr, \
					  "%s:%d: PID=%5d: Error %d (%s)\n", \
					  __FILE__,			\
					  __LINE__,			\
					  getpid(),			\
					  errno,			\
					  strerror(errno));}


struct sembuf my_op;
struct my_msgbuf {
	long mtype;             /* message type, must be > 0 */
	int num_atomico;    /* message data */
};
struct msgbuf_atom {
	long mtype;             /* message type, must be > 0 */
};


struct shared_stats {
	/* index where next write will happen */
	unsigned long num_attivati;
	unsigned long num_scissi;
	unsigned long energia_prodotta;
	unsigned long energia_consumata;
	unsigned long scorie; 
	
	/*per inibitore*/
	unsigned long limit;
	unsigned char prob_scis;
	unsigned long energia_assorbita;
	unsigned long atomi_terminati;

};

#define SEM_KEY 0x123456
#define MEX_KEY 0x123459
#define ATOM_KEY 0x123457
#define SCIS_KEY 0x123458
#define STATS_KEY 0x123455


#define N_ATOMO "./atomo"
#define N_ATOM_MAX 118
#define N_ATOMI_INIT 1000
#define MIN_N_ATOMICO 2
#define SIM_DURATION 60
#define ENERGY_DEMAND 1000
#define ENERGY_EXPLODE_THERESHOLD 30000000
#define STEP 999999999
#define N_NUOVI_ATOMI 10


/*Questo è un valore approssimato per avere una aprossimazione del numero massimo di processi dormienti in memoria*/
#define SYS_LIMIT 8000
/*più è alta la precisione più è sicuro l'inibitore per thereshold piccoli*/
#define PRECISION 90000000000.0

#endif   /* COMMON_FILE_H_ */
