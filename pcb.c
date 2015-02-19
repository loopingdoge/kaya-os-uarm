 /*******************************************************************************
  * Copyright 2014, Devid Farinelli, Erik Minarini, Alberto Nicoletti           *
  * This file is part of kaya2014.                                              *
  *                                                                             *
  * kaya2014 is free software: you can redistribute it and/or modify            *
  * it under the terms of the GNU General Public License as published by        *
  * the Free Software Foundation, either version 3 of the License, org          *
  * (at your option) any later version.                                         *
  *                                                                             *
  * kaya2014 is distributed in the hope that it will be useful,                 *
  * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
  * GNU General Public License for more details.                                *
  *                                                                             *
  * You should have received a copy of the GNU General Public licenses          *
  * along with kaya2014.  If not, see <http://www.gnu.org/licenses/>.           *
  ******************************************************************************/
 
/*********************************** pcb.c **************************************
 *
 *  Questo modulo implementa le strutture dati e le funzioni necessaria alla
 *  gestione dei Process Control Block.
 *
 */

#include <uARMconst.h>
#include <uARMtypes.h>

#include "include/const.h"
#include "include/types10.h"

#include "include/pcb.h"

/** Lista dei pcb inutilizzati */
HIDDEN pcb_t *pcbFree_h;

/**
* 	 Funzioni di gestione della lista pcbFree
*/

/** Inserisce il pcb passato come parametro alla lista pcbFree */
void freePcb(pcb_t *p){
    pcb_t *i = pcbFree_h;
    p -> p_next = NULL;
	/* aggiunge p in coda */
	if(pcbFree_h == NULL){
		pcbFree_h = p;
	} else {
		while(i -> p_next != NULL){
			i = i -> p_next;
		}
		i -> p_next = p;
	}
}

/** Rimuove un pcb dalla lista pcbFree e lo restituisce
*	Restituisce NULL se la lista è vuota 
*/
pcb_t *allocPcb(){
	pcb_t *pcb = NULL;
	int i;
	if(pcbFree_h != NULL){
		/* Rimuove un elemento e inizializza valori dei suoi attributi e lo restituisce */
		pcb = pcbFree_h;
		pcbFree_h = pcbFree_h -> p_next;
		pcb->p_next = NULL;
		pcb->p_prev = NULL;
		pcb->p_prnt = NULL;
		pcb->p_child = NULL;
		pcb->p_sib = NULL;
		/* Inizializzazione state_t */
			pcb->p_s.a1 = 0;
    		pcb->p_s.a2 = 0;
    		pcb->p_s.a3 = 0;
    		pcb->p_s.a4 = 0;
    		pcb->p_s.v1 = 0;
    		pcb->p_s.v2 = 0;
    		pcb->p_s.v3 = 0;
    		pcb->p_s.v4 = 0;
    		pcb->p_s.v5 = 0;
    		pcb->p_s.v6 = 0;
    		pcb->p_s.sl = 0;
    		pcb->p_s.fp = 0;
    		pcb->p_s.ip = 0;
    		pcb->p_s.sp = 0;
    		pcb->p_s.lr = 0;
    		pcb->p_s.pc = 0;
    		pcb->p_s.cpsr = 0;
    		pcb->p_s.CP15_Control = 0;
    		pcb->p_s.CP15_EntryHi = 0;
    		pcb->p_s.CP15_Cause = 0;
    		pcb->p_s.TOD_Hi = 0;
    		pcb->p_s.TOD_Low = 0;
		pcb->p_semAdd = NULL;
		pcb->waitForDev = 0;
		/* Inizializzazione aree per SYS5  */
    	for(i=0; i<6; i++){
    	    pcb->excStVec[i]=NULL;
    	}
    	/* Inizializzazione cpu_time  */
    	pcb->cpu_time=0;
	}
	return pcb;
}

/** Inizializza la lista pcbFree.
*	Questo metodo verrà chiamato solo una volta durante l’inizializzazione della struttura dati. 
*/
void initPcbs(){
	static pcb_t pcbs[MAXPROC];
    int i;
	/* Inizializza il primo elemento dell'array e lo 
	* inserisce in testa alla lista */
	pcbFree_h = &pcbs[0];
	/* Aggiunge gli elementi restanti all'array
	* e poi li mette nella lista */
	for(i = 1; i < MAXPROC; i++){
		pcbs[i-1].p_next = &pcbs[i];
	}
	pcbs[MAXPROC-1].p_next = NULL;
}

/*
*		Funzioni di gestione della Coda
*/

/** Crea una procQ vuota e ne restituisce il puntatore */
pcb_t *mkEmptyProcQ(){
	pcb_t *queue = NULL;
	return queue;
}

/** Restituisce TRUE se tp è una coda vuota
    Restituisce FALSE altrimenti */
int emptyProcQ(pcb_t *tp){
	if(tp == NULL){
		return TRUE;
	} else {
		return FALSE;
	}
}

/** Inserisce un pcb in coda alla process queue tail-puntata da tp */
void insertProcQ(pcb_t **tp, pcb_t *p){
    if(p == NULL){
        return;
    } else if(emptyProcQ((*tp))){
        p -> p_next = p;
		p -> p_prev = p;
        (*tp) = p;
    } else {
        p -> p_next = (*tp) -> p_next;
		p -> p_prev = (*tp);
		(*tp) -> p_next -> p_prev = p;
        (*tp) -> p_next = p;
        (*tp) = p;
    }
}

/** Rimuove il primo elemento dalla process queue tail-puntata da tp e lo restituisce
*	Restituisce NULL se la coda è vuota
*/
pcb_t *removeProcQ(pcb_t **tp){
	pcb_t *removed=NULL;
    /* Se la coda non è vuota */
	if((*tp) != NULL){
		removed = (*tp) -> p_next;
        /* Se la lista ha un solo elemento */
		if(removed -> p_next == removed ){
        	(*tp) = NULL;
        }else{
            /* aggiorna il puntatore */
			(*tp) -> p_next = removed -> p_next;
			removed -> p_next -> p_prev = (*tp);
		}
	}
	return removed;
}

/** Rimuove il pcb puntato da p dalla process queue tail-puntata da tp e lo restituisce
*	Restituisce NULL se la coda è vuota o se p ha valore NULL
*/
pcb_t *outProcQ(pcb_t **tp, pcb_t *p){
    pcb_t *iterator = (*tp);
    pcb_t *removed = NULL;
    if(p != NULL && !emptyProcQ((*tp))){
		while(iterator -> p_next != p && iterator -> p_next != (*tp)){
			/* Scorro in avanti */
			iterator = iterator -> p_next;
		}
		if( iterator -> p_next == p ){
			removed = iterator -> p_next;
            /* Se la coda ha un solo elemento */
			if(iterator -> p_next == iterator){
				*(tp) = NULL;
			} else {
				iterator -> p_next = removed -> p_next;
				removed -> p_next -> p_prev = iterator;
			}
		}
	}
    return removed;
}

/** Restituisce un puntatore al primo pcb dalla coda 
*	Restituisc NULL se la coda è vuota 
*/
pcb_t *headProcQ(pcb_t *tp){
    /* Se la coda è vuota */
    if(emptyProcQ(tp)){
		return NULL; 
	} else {
        return (tp -> p_next);
    }
}

/*
* 		Funzioni di gestione dell'albero
*/

/** Restituisce TRUE se il pcb puntato da p non ha figli 
*	Restituisce FALSE altrimenti
*/
int emptyChild(pcb_t *p){
    if(p -> p_child == NULL){
        return TRUE;
    } else {
        return FALSE;
    }
}

/** Inserisce il pcb puntato da p come figlio del pcb puntato da prnt */
void insertChild(pcb_t *prnt, pcb_t *p){
    p->p_prnt=prnt;
    if (emptyChild(prnt)) {
        prnt -> p_child = p;
    } else {
        pcb_t *sib = prnt -> p_child;
		while(sib -> p_sib != NULL){
			sib = sib -> p_sib;
		}
        sib -> p_sib = p;
		p -> p_sib = NULL;
    }
}

/** Rimuove il primo figlio del pcb puntato da p e lo restituisce
*	Restituisce NULL se il nodo non ha figli
*/
pcb_t *removeChild(pcb_t *p){
	pcb_t *child = NULL;
    if(p -> p_child != NULL){
        child = p -> p_child;
        if(child -> p_sib != NULL){
            pcb_t *sib = child -> p_sib;
            p -> p_child = sib;
        } else {
            p -> p_child = NULL;
        }
    }
	return child;
}

/** Rimuove il pcb puntato da p dai figli del proprio parent e lo restituisce
*	Restituisce NULL se p non ha parent
*/
pcb_t *outChild(pcb_t *p){
	pcb_t *pcb = NULL;
    if(p -> p_prnt != NULL){
        pcb_t *prnt = p -> p_prnt;
        p -> p_prnt = NULL;
        if(prnt -> p_child == p){
            removeChild(prnt);
        } else {
            pcb_t *i = prnt -> p_child;
            while(i -> p_sib != p){
                i = i -> p_sib;
            }
            i -> p_sib = (i -> p_sib) -> p_sib;
        }
        pcb = p;
    }
	return pcb;
}
