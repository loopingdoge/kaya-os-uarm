#ifndef TYPES10_H
#define TYPES10_H

#include <uARMtypes.h>

/* process control block type */
typedef struct pcb_t {
	/* Campi coda processo */
	struct pcb_t *p_next,               /* puntatore a next */
                             *p_prev,   /* puntatore a prev */
							 /* campi albero processo */
							 * p_prnt,  /* puntatore a parent */
							 * p_child, /* puntatore al primo child */
							 * p_sib;   /* puntatore al fratello*/
	state_t p_s,                        /* stato processo */
	        *excStVec[6];               /* Array di Exception State Vector */
	int *p_semAdd; /* puntatore al semaforo sul quale è bloccato il processo */
	int waitForDev; /* indica se processo è in coda sul semaforo di un device */
	unsigned int cpu_time;         /* Tempo trascorso dal processo sulla CPU */ 
} pcb_t;

/*  definizione struttura del semaforo */
typedef struct semd_t {
	struct semd_t		*s_next; 	/* prossimo elemento della ASL */
	int 				*s_semAdd; 	/* puntatore al semaforo */
	pcb_t 				*s_procQ; 	/* puntatore di coda ad una process queue */
} semd_t;

#endif
