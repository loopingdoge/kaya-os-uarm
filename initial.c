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
 
/********************************* initial.c ************************************
 *
 *  Questo modulo implementa main() ed esporta le variabili globali
 *  (es. Process Count, semafori dei device, ecc.).
 *
 */

#include <libuarm.h>
#include <uARMconst.h>
#include <uARMtypes.h>
#include <arch.h>
    
#include "include/base.h"
#include "include/const.h"
#include "include/types10.h"

#include "./include/pcb.h"
#include "./include/asl.h"

#include "./include/scheduler.h"
#include "./include/exceptions.h"
#include "./include/interrupts.h"
#include "./include/p2test.h"
#include "./include/initial.h"

/* Numero di processi nel sistema */
U32 processCount;
/* Numero di processi bloccati in attesa di un interrupt */
U32 softBlockCount;
/* Puntatore (a tail) alla coda di pcb_t dei processi ready */
pcb_t *readyQueue;
/* Puntatore alla pcb_t del processo corrente */
pcb_t *currentProcess;
/* Semaforo per la linea di interrupt 0 */
int semIpi;
/* Semaforo per la linea di interrupt 1 */
int semCpuTimer;
/* Matrice di semafori dei device */
int semDev[N_EXT_IL + 1][N_DEV_PER_IL];
/* Matrice contenente lo status register dei device */
int statusDev[N_EXT_IL + 1][N_DEV_PER_IL];
/* Semaforo del Pseudo-Clock Timer */
int semPseudoClock;

/**
    @param line:    Linea di interrupt del device           (3-8)
    @param dev:     Numero del device sulla interrupt line  (0-7)
    @return:        Restituisce l'indirizzo del semaforo del device
*/
memaddr* getSemDev(int line, int dev){
    if(3 <= line && line <= 8 && 0 <= dev && dev <= 7)
        return (memaddr *) &semDev[line - DEV_IL_START][(N_DEV_PER_IL-1) -dev];
    else
        return NULL;
}

/**
    @param line:    Linea di interrupt del device           (3-8)
    @param dev:     Numero del device sulla interrupt line  (0-7)
    @return:        Restituisce l'indirizzo dello status register del device
                    (Mantenuto dal kernel, non reale)
*/
memaddr* getKernelStatusDev(int line, int dev){
    if(3 <= line  && line <= 8 && 0 <= dev && dev <= 7)
        return (memaddr *) &statusDev[line - DEV_IL_START][(N_DEV_PER_IL-1) -dev];
    else
        return NULL;
}

/**
    @param area:    Indirizzo di inizio dell'area da popolare
    @param handler: Indirizzo dell'handler da assegnare al PC dell'area
                    Popola una nuova area nella ROM Reserved Frame
*/
void initArea(memaddr area, memaddr handler){
    state_t *newArea = (state_t*) area;
    /* Memorizza il contenuto attuale del processore in newArea */
    STST(newArea);
    /* Setta pc alla funzione che gestirà l'eccezione */
    newArea->pc = handler;
    /* Setta sp a RAMTOP */
    newArea->sp = RAM_TOP;
    /* Setta il registro di Stato per mascherare tutti gli interrupt e si mette in kernel-mode. */
    newArea->cpsr = STATUS_ALL_INT_DISABLE((newArea->cpsr) | STATUS_SYS_MODE);
    /* Disabilita la memoria virtuale */
    newArea->CP15_Control = (newArea->CP15_Control) & ~(ENABLE_VM);
}

int main(void){
    /* Iteratori */
    int i,j;
    /* Processo iniziale */
    pcb_t *startingProcess;
    
    /* Popola le 4 nuove Aree nella ROM Reserved Frame. */
    initArea(INT_NEWAREA,       (memaddr) intHandler);
    initArea(TLB_NEWAREA,       (memaddr) tlbHandler);
    initArea(PGMTRAP_NEWAREA,   (memaddr) pgmHandler);
    initArea(SYSBK_NEWAREA,     (memaddr) sysBpHandler);
    
    /* Inizializza la stuttura dati dei pcbs */
    initPcbs();
    /* Inizializza la struttura dati dei semafori */
    initASL();
    
    /* Inizializza le varibili mantenute dal kernel */ 
    processCount = 0;
    softBlockCount = 0;
    readyQueue = mkEmptyProcQ();
    currentProcess = NULL;
    
    /* Inizializza i semafori */
    semPseudoClock = 0;

    /* Inizializza i semafori dei (sub)device */
    for(i = 0; i < N_EXT_IL + 1; i++){
        for(j = 0; j < N_DEV_PER_IL; j++){
            semDev[i][j] = 0;
        }
    }

    /* Istanzio un singolo processo */
    startingProcess = allocPcb();
    if(startingProcess == NULL){
        PANIC();
    }
    
    /* Abilita gli interrupt, il Local Timer e la kernel-mode */
    startingProcess->p_s.cpsr = STATUS_ALL_INT_ENABLE(startingProcess->p_s.cpsr) | STATUS_SYS_MODE;
    /* Disabilita la memoria virtuale */
    startingProcess->p_s.CP15_Control = (startingProcess->p_s.CP15_Control) & ~(ENABLE_VM);
    /* Assegna ad SP il valore RAMTOP - FRAMESIZE */
    startingProcess->p_s.sp = RAM_TOP - FRAME_SIZE;
    /* Assegna a PC l'indirizzo della funzine esterna test() */
    startingProcess->p_s.pc = (memaddr) test;
    
    /* Inserisco nella Ready Queue */
    insertProcQ(&readyQueue, startingProcess);
    processCount++;
    /* Chiama lo scheduler() */
    scheduler();
    
    return 0;
}