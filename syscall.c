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
 
/********************************* syscall.c ************************************
 *
 *  Questo modulo implementa le System Call.
 *
 */

#include <libuarm.h>
#include <uARMconst.h>
#include <uARMtypes.h>
#include <arch.h>

#include "include/base.h"
#include "include/const.h"
#include "include/types10.h"

#include "include/pcb.h"
#include "include/asl.h"
    
#include "include/initial.h"
#include "include/scheduler.h"
#include "include/exceptions.h"
#include "include/syscall.h"

/*
    -Create_Process (SYS1)
    Crea un processo figlio del processo che richiama la syscall
    e restituisce in a1, -1 in caso di errore, 0 altrimenti
    @param stato:   area di memoria contenente lo stato del nuovo
                    processo
*/
void createProcess(state_t *stato){
    /* Alloca un nuovo processo */
    pcb_t* newPcb = allocPcb();
    /* Se la PcbFree è vuota */
    if(newPcb==NULL){
        currentProcess->p_s.a1=-1;
    }else{
        /* Imposta lo stato al valore passato come parametro */
        newPcb->p_s.a1              = (*stato).a1;
        newPcb->p_s.a2              = (*stato).a2;
        newPcb->p_s.a3              = (*stato).a3;
        newPcb->p_s.a4              = (*stato).a4;
        newPcb->p_s.v1              = (*stato).v1;
        newPcb->p_s.v2              = (*stato).v2;
        newPcb->p_s.v3              = (*stato).v3;
        newPcb->p_s.v4              = (*stato).v4;
        newPcb->p_s.v5              = (*stato).v5;
        newPcb->p_s.v6              = (*stato).v6;
        newPcb->p_s.sl              = (*stato).sl;
        newPcb->p_s.fp              = (*stato).fp;
        newPcb->p_s.ip              = (*stato).ip;
        newPcb->p_s.sp              = (*stato).sp;
        newPcb->p_s.lr              = (*stato).lr;
        newPcb->p_s.pc              = (*stato).pc;
        newPcb->p_s.cpsr            = (*stato).cpsr;
        newPcb->p_s.CP15_Control    = (*stato).CP15_Control;
        newPcb->p_s.CP15_EntryHi    = (*stato).CP15_EntryHi;
        newPcb->p_s.CP15_Cause      = (*stato).CP15_Cause;
        newPcb->p_s.TOD_Hi          = (*stato).TOD_Hi;
        newPcb->p_s.TOD_Low         = (*stato).TOD_Low;
        /* Inserisce il processo fra i figli del processo chiamante */
        insertChild(currentProcess, newPcb);
        /* Aumenta il contatore dei processi */
        processCount++;
        /* Inserisce il processo nella ready queue */
        insertProcQ(&readyQueue, newPcb);
        currentProcess->p_s.a1=0;
    }
}

/*  
    -Terminate Process (SYS2)
    Termina un processo e tutta la progenie
    @param *pcb:    processo da terminare
*/
void terminateProcess(pcb_t *pcb){
    if(pcb!=NULL){
        if(pcb->p_semAdd != NULL){
            if(pcb-> waitForDev == FALSE){
                if((*pcb->p_semAdd) < 0){
                    (*pcb->p_semAdd)++;
                }
            } else {
                softBlockCount--;
            }
            pcb = outBlocked(pcb);
        }
        while(!emptyChild(pcb)){
            terminateProcess(pcb->p_child);
        }
        outChild(pcb);
        if(currentProcess == pcb){
            currentProcess = NULL;
        }
        outProcQ(&readyQueue, pcb);
        freePcb(pcb);
        processCount--;
    }
}

/*
    -Verhogen (SYS3)
    Esegue una operazione V(signal) sul un semaforo passato come parametro
    Quindi "rilascia" la risorsa
    @param sem: puntatore al semaforo
*/
void verhogen(int *sem){
    /* Incremento il valore del semaforo */
    (*sem)++;
    /* Prendo il primo processo in coda */
    pcb_t* first = removeBlocked(sem);
    /* Se la coda non era vuota */
    if(first!=NULL){
        /* Aggiorno il puntatore al semaforo in pcb_t */
        first->p_semAdd=NULL;
        /* Inserisco il processo nella readyqueue */
        insertProcQ(&readyQueue, first);
        /* Aggiorno il contatore dei processi bloccati */
        if(first->waitForDev == TRUE){
            first->waitForDev = FALSE;
            softBlockCount--;
        }
    }
}

/*
    -Passeren (SYS4)
    Esegue una operazione P sul semaforo passato come parametro
    Quindi "attende" una risorsa
    @param sem: puntatore al semaforo
*/
void passeren(int *sem){
    /* Decrementa il valore del semaforo */
    (*sem)--;
    /* Se il valore del semaforo è negativo */
    if((*sem)<0){
        /* Aggiorno il puntatore al semaforo in pcb_t */
        currentProcess->p_semAdd = sem;
        /* Aggiorno il tempo passato sulla cpu dal processo */
        currentProcess->cpu_time += getTODLO() - process_TOD;
        /* Mette il processo in coda al semaforo */
        insertBlocked(sem, currentProcess);
        /* Il processo non è più in esecuzione */
        currentProcess=NULL;
    }
}

/*
    -Specify Exception State Vector (SYS5)
    Specifica un'area nella quale salvare lo stato del processore (oldp)
    e un nuovo stato da caricare (newp) nel caso si verifichi un eccezione
    di un certo tipo (type).
    Possibili valori di type
    0: TLB exception
    1: PgmTrap exception
    2: SYS/Bp exception
    @param type:    tipo di eccezione
    @param oldp:    old area
    @param newp:    new area
*/
void specExStVec(int type, state_t *oldp, state_t *newp){
    /* Se una delle aree relative all'eccezione è già inizializzata */
    if(currentProcess->excStVec[(type*2)]!=NULL){
        /* Gestisco come una SYS2 */ 
        terminateProcess(currentProcess);
    }else{
        /* Salva oldp e newp */
        currentProcess->excStVec[(type*2)]=oldp;
        currentProcess->excStVec[((type*2)+1)]=newp;  
    }
}

/*
    -Get CPU Time (SYS6)
    Restituisce in a1 del chiamante il tempo trascorso dal processo
    nella cpu
*/
void getCPUTime(){
    /* Aggiorno il tempo passato sulla cpu dal processo */
    currentProcess->p_s.a1 = currentProcess->cpu_time + (getTODLO() - process_TOD);
}

/*
    -Wait For CLock (SYS7)
    Mette il processo corrente in attesa sul semaforo dello pseudoclock
*/
void waitForClock(){
    /* Si mette in attesa del semaforo dello pseudo clock*/
    softBlockCount++;
    currentProcess->waitForDev = TRUE;
    passeren(&semPseudoClock);
}

/*
    -Wait For IO (SYS8)
    Mette il processo in attesa sul semaforo di un device passato come parametro
*/
void waitForIO(int line, int dev, int waitForTermRead){
    /* Se voglio scrivere su terminale */
    if(line==INT_TERMINAL && !waitForTermRead){
        /* Aumento la riga di 1, poichè la 7 è termread e la 8 termwrite */
        line++;
    }
    memaddr *semaphoreDevice = getSemDev(line, dev);
    memaddr *statusReg = getKernelStatusDev(line, dev);
    if((*statusReg) != 0){
        currentProcess->p_s.a1 = (*statusReg);
        (*statusReg) = 0;
    } else {
        softBlockCount++;
        currentProcess->waitForDev = TRUE;
        passeren((int *) semaphoreDevice);
    }
}