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
 
/******************************** scheduler.c ***********************************
 *
 *  Questo modulo implementa lo scheduler di Kaya e la rilevazione di
 *  stati di deadlock.
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

#include "include/initial.h"
#include "include/scheduler.h"

/* TOD di inizio dell'ultimo time slice */
unsigned int slice_TOD = 0;
/* TOD di inizio dell'ultimo intervallo dello pseudo-clock */
unsigned int clock_TOD = 0;
/* TOD in cui il processo corrente è stato messo in esecuzione */
unsigned int process_TOD = 0;

/**
 * @param   CLOCK_TYPE: SCHED_TIME_SLICE o SCHED_PSEUDO_CLOCK
 * @return  TRUE  (1)   se bisogna impostare l'IT con il tipo di timer passato per parametro
            FALSE (0)   altrimenti
*/
int isTimer(unsigned int TIMER_TYPE){
    int time_until_timer;
    /* Calcola il tempo che manca allo scadere del timer di tipo TIMER_TYPE */
    if(TIMER_TYPE == SCHED_TIME_SLICE){
        time_until_timer= TIMER_TYPE - (getTODLO() - slice_TOD);
    } else if(TIMER_TYPE == SCHED_PSEUDO_CLOCK){
        time_until_timer= TIMER_TYPE - (getTODLO() - clock_TOD);
    }
    /* Se è scaduto ritorna true, altrimenti false */
    if(time_until_timer <= 0){
        return TRUE;
    } else { 
        return FALSE;
    }
}

/**
    Setta il timer all'evento più prossimo fra la fine del time slice e la fine
    del ciclo corrente dello pseudo clock
*/    
void setNextTimer(){
    unsigned int TODLO = getTODLO();
    /* Calcola il tempo trascorso dall'inizio del time slice corrente */
    int time_until_slice = SCHED_TIME_SLICE - (TODLO - slice_TOD);

    /* Se il time slice è appena teminato setta il prossimo */
    if(time_until_slice<=0){
        slice_TOD = TODLO;
        time_until_slice= SCHED_TIME_SLICE;
    }
    
    /* Calcola il tempo trascorso dall'inizio del ciclo di pseudo clock corrente */
    int time_until_clock = SCHED_PSEUDO_CLOCK - (TODLO - clock_TOD);
    /* Se il ciclo di pseudo clock è appena teminato setta il prossimo */
    if(time_until_clock <= 0){
        clock_TOD = TODLO;
        time_until_clock = SCHED_PSEUDO_CLOCK;
    }
    /* Setta il prossimo timer */
    if(time_until_slice <= time_until_clock) {
        setTIMER(time_until_slice);
    } else {
        setTIMER(time_until_clock);
    }
}

void scheduler(){
    /* Setta l'interval timer al prossimo evento */
    setNextTimer();
    /* Se non c'è un processo in esecuzione */
    if(currentProcess == NULL) {
        
        /* Se la readyQueue è vuota */
        if(emptyProcQ(readyQueue)) {
            /* Se processCount è zero chiamo HALT */
            if(processCount==0){
                HALT();
            /* Se processCount > 0 e softBlockCount vale 0. Si è verificato un deadlock, invoco PANIC() */
            }else if(processCount>0 && softBlockCount==0) {
                PANIC();
            /* Se processCount>0 e softBlockCount>0 mi metto in stato di attesa, invoco WAIT() */
            }else if(processCount>0 && softBlockCount>0) {
                setSTATUS(STATUS_ALL_INT_ENABLE(getSTATUS()));
                WAIT();
            }
        } else {
            currentProcess = (pcb_t*) removeProcQ(&readyQueue);
            
            /* Caso anomalo */
            if(currentProcess == NULL){
                PANIC();
            }
            
        }
    }
    
    process_TOD = getTODLO();
    /* Carica lo stato del processo corrente */
    LDST(&(currentProcess->p_s));
}
