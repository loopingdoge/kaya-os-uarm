 /*******************************************************************************
  * Copyright 2014, Devid Farinelli, Erik Minarini, Alberto Nicoletti          	*
  * This file is part of kaya2014.       									    *
  *																				*
  * kaya2014 is free software: you can redistribute it and/or modify			*
  * it under the terms of the GNU General Public License as published by		*
  * the Free Software Foundation, either version 3 of the License, org          *
  * (at your option) any later version.											*
  *																				*
  * kaya2014 is distributed in the hope that it will be useful,					*
  * but WITHOUT ANY WARRANTY; without even the implied warranty of				*
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 				*
  * GNU General Public License for more details.								*
  *																				*
  * You should have received a copy of the GNU General Public licenses 			*
  * along with kaya2014.  If not, see <http://www.gnu.org/licenses/>.			*
  ******************************************************************************/
 
/*********************************** asl.c **************************************
 *
 *	Questo modulo implementa le strutture dati e le funzioni necessarie
 *	alla gestione della Active Semaphore List.
 *
 */

#include <uARMconst.h>
#include <uARMtypes.h>

#include "include/const.h"
#include "include/types10.h"

#include "include/pcb.h"
#include "include/asl.h"

/** semd_h 		- Puntatore alla testa della ASL 
*   semdFree_h  - Puntatore alla testa dei semafori non allocati 
*/
HIDDEN semd_t *semd_h, *semdFree_h;

/**	Cerca nella ASL il semd_t passato come parametro
*	e lo restituisce se lo trova
*	altrimenti restituisce NULL
*/
semd_t *findSemd(int *semAdd){
	semd_t *i = semd_h;
	semd_t *ret = NULL;
	if(semd_h != NULL){
		while(i -> s_semAdd < semAdd && i -> s_next != NULL){
	        i = i -> s_next;
	    }
	    if(i->s_semAdd == semAdd){
	    	ret = i;
	    }
	}
    return ret;
}

/**	Rimuove dalla ASL il semd_t passato come parametro
*	e lo inserisce in testa alla semdFree
*/
void freeSemd(semd_t *current){
	semd_t *i = semd_h;
	/* Toglie dalla ASL */
	if(i != current){
		while(i -> s_next != current){
			i = i -> s_next;
		}
		i -> s_next = current -> s_next;
	} else {
		semd_h = semd_h -> s_next;
	}
	/* Aggiunge in testa alla semdFree */
	current -> s_next = semdFree_h;
	semdFree_h = current;
}

/** Rimuove un semaforo da semdFree_h e lo alloca, inserendolo nella ASL.
*   Restituisce TRUE se semdFree è vuota.
*   Restituisce FALSE se ha successo.
*/
HIDDEN int allocFree(int *semAdd, pcb_t *p){
	/* Alloco un nuovo semaforo per metterlo nell'ASL */
	int result = FALSE;
	semd_t *i = NULL;
	if(semdFree_h != NULL){
		semd_t *alloc = findSemd(semAdd);
		if(alloc==NULL && semdFree_h != NULL){
			alloc = semdFree_h;
        	semdFree_h = semdFree_h -> s_next;
       		alloc -> s_next = NULL;
        	alloc -> s_semAdd = semAdd;
        	alloc -> s_procQ = mkEmptyProcQ();
        	/* Coda vuota  */
        	if(semd_h == NULL){
        		semd_h = alloc;
        	} else {
        		if(alloc->s_semAdd < semd_h->s_semAdd){
					/* Inserimento in testa */
					alloc -> s_next = semd_h;
					semd_h = alloc;
				} else {
					/* Scorro la ASL */
					i = semd_h;
					while(i -> s_next != NULL && alloc->s_semAdd > (i -> s_next)->s_semAdd){
						i = i -> s_next;
					}
					if(i -> s_next == NULL){
						/* Inserimento in coda */
						i -> s_next = alloc;
					} else {
						/* Inserimento prima del semAdd maggiore di alloc */
						alloc -> s_next = i -> s_next;
						i -> s_next = alloc;
					}
				}
        	}
		}
        p -> p_semAdd = semAdd;
        insertProcQ(&(alloc -> s_procQ), p);
    } else {
    	result = TRUE;
    }
    return result;
}

/** Inserisce il pcb puntato da p come ultimo elemento della coda associata al semaforo di indirizzo semAdd
*	Restituisce TRUE se un nuovo semAdd deve essere allocato e la semdFree è vuota
*	Restituisce FALSE in tutti gli altri casi 
*/
int insertBlocked(int *semAdd, pcb_t *p){
    int result = FALSE;
    semd_t *i = findSemd(semAdd);
    if(i == NULL){
		/* ASL vuota, alloco un semaforo */
        result = allocFree(semAdd, p);
    } else {
		/* Il parametro è nella ASL */
		p -> p_semAdd = semAdd;
		insertProcQ(&(i -> s_procQ), p);
    }
    return result;
}
/** Rimuove il primo pcb dalla coda associata al semaforo di indirizzo semAdd
*	Restituisce NULL se non esiste un semaforo di indirizzo semAdd */
pcb_t *removeBlocked(int *semAdd){
    pcb_t *result = NULL;
   	semd_t *i = findSemd(semAdd);
	if(i != NULL){
		result = removeProcQ(&(i -> s_procQ));
		/* Se la sua coda diventa vuota */
		if(emptyProcQ(i ->s_procQ)){
			freeSemd(i);
		}
	}
	return result;
}

/** Rimuove il pcb puntato da p dalla process queue associata al suo semAdd e lo restituisce
*	Restituisce NULL se il p non si trova nella coda associata 
*/
pcb_t *outBlocked(pcb_t *p){
    pcb_t *result = NULL;
    semd_t *i = findSemd(p->p_semAdd);
    if(i != NULL){
		result = outProcQ(&(i->s_procQ), p);
		/* Se la sua coda diventa vuota */
		if(emptyProcQ(i ->s_procQ)){
			freeSemd(i);
		}
	}
    return result;
}

/** Restituisce un puntatore al pcb in testa alla coda associata al semaforo semAdd 
*	Restituisce NULL se il semaforo di indirizzo semAdd non esiste 
*/
pcb_t *headBlocked(int *semAdd){
    pcb_t *result = NULL;
    semd_t *i = findSemd(semAdd);
    if(i != NULL){
        result = headProcQ(i -> s_procQ);
    }
    return result;
}

/** Crea la lista semdFree di pcb allocati staticamente */
void initASL(){
    static semd_t semdTable[MAXPROC];
    int i;
    semd_h = NULL;
    semdFree_h = &semdTable[0];
    for(i=1; i < MAXPROC; i++){
        semdTable[i-1].s_next = &semdTable[i];
    }
    semdTable[MAXPROC-1].s_next = NULL;
}
