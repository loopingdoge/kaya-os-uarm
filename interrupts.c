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
 
/******************************** interrupts.c **********************************
 *
 *	Questo modulo implementa la gestione delle eccezioni interrupt.
 *	Il modulo gestirà  tutti gli interrupt dei device, dell' Interval
 *  Timer e dello Pseudo Clock, convertendo gli interrupt dei device
 *	in V sui semafori appropriati.
 *
 */

#include <libuarm.h>
#include <uARMconst.h>
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
#include "include/interrupts.h"

state_t *int_old 	 = (state_t*) INT_OLDAREA;

/**
	Handler per gli interrupt.
	L'indirizzo di questa funzione è salvato nella ROM Reserved Frame
	come gestore degli interrupt.
*/
void intHandler(){
    int cause;
    (*int_old).pc -= 4;
    if(currentProcess != NULL){
		saveStateIn(int_old, &currentProcess->p_s);
    }
	/* prendo il contenuto del registro cause */
	cause = getCAUSE();
	/* Se la causa dell'interrupt è la linea 0 */
	if(CAUSE_IP_GET(cause, IL_IPI)){
		lineOneTwoHandler(IL_IPI);
	} else 
	/* linea 1 */
	if(CAUSE_IP_GET(cause, IL_CPUTIMER)){
		lineOneTwoHandler(IL_CPUTIMER);
	} else
	/*  linea 2 timer */
	if (CAUSE_IP_GET(cause, IL_TIMER)){
	    timerHandler();
	} else
    /* linea 3 disk */
    if (CAUSE_IP_GET(cause, IL_DISK)){
        genericDevHandler(IL_DISK);
    } else
    /* linea 4 tape*/
    if (CAUSE_IP_GET(cause, IL_TAPE)){
        genericDevHandler(IL_TAPE);
    } else
    /* linea 5 network */
    if (CAUSE_IP_GET(cause, IL_ETHERNET)){
        genericDevHandler(IL_ETHERNET);
    } else
    /* linea 6 printer*/
    if (CAUSE_IP_GET(cause, INT_PRINTER)){
        genericDevHandler(IL_PRINTER);
    } else
    /* linea 7 terminal */
	if (CAUSE_IP_GET(cause, INT_TERMINAL)){
	    terminalHandler();
	}
	scheduler();
}

/**
	Esegue una verhogen
*/
void interruptVerhogen(int *sem, int statusRegister, memaddr* kernelStatusDev){
    /* Prendo il primo processo in coda */
    pcb_t* first = removeBlocked(sem);
    /* Incremento il valore del semaforo */
    (*sem)++;
    /* Se la coda non era vuota */
    if(first!=NULL){
        /* Inserisco il processo nella readyqueue */
        insertProcQ(&readyQueue, first);
        /* Aggiorno il puntatore al semaforo in pcb_t */
        first-> p_semAdd = NULL;
        first-> waitForDev = FALSE;
        first-> p_s.a1 = statusRegister;
        softBlockCount--;
    } else {
		(*kernelStatusDev) = statusRegister;
    }
}

/**
	@param line: Linea di Interrupt di un tipo di device
	
	Data la maschera di bit di una linea di interrupt restituisce 
	l'indice del device attivo con priorità maggiore
*/
int getHighestPriorityDev(memaddr* line){
	int activeBit = 0x00000001;
	int i;
	/* Usando una maschera (activeBit) ad ogni iterazione isolo i singoli
	bit dei device della linea. Quando ne trovo uno settato ne restituisco
	l'indice */
	for(i = 0; i < 8; i++){
		if(((*line)&activeBit) == activeBit){
			return i;
		}
		activeBit = activeBit << 1;
	}
	return -1;
}

/**
	Manda un ACK di conferma al device ed esegue una V sul suo semaforo
*/
void ackAndVerhogen(int intLine, int device, int statusReg, memaddr *commandReg){
	/* Ottengo il semaforo corrispondente al device */
	memaddr *semDev 		 = getSemDev(intLine, device);
	memaddr *kernelStatusDev = getKernelStatusDev(intLine, device);
	/* Mando al device un ACK */
	(*commandReg) = DEV_C_ACK;
	/* Eseguo una operazione di V su quel semaforo */
	interruptVerhogen((int *) semDev, statusReg, kernelStatusDev);
}

/**
	@param: Numero della linea di interrupt (0 o 1)

	Handler per le prime due linee di interrupt.
*/
void lineOneTwoHandler(int interruptLineNum){
	int *semaphore = NULL;
	if(interruptLineNum == IL_IPI){
		semaphore = &semIpi;
	} else if(interruptLineNum == IL_CPUTIMER){
		semaphore = &semCpuTimer;
	}
	if(semaphore != NULL){
		verhogen(semaphore);
	}
}

/**
	@param interruptLineNum: Numero della linea di interrupt

	Handler per gli interrupt di un tipo di device generico (disk, tape, network, printer)
	Manda un ACK al device ed esegue una V sul semaforo associato al device.
*/
void genericDevHandler(int interruptLineNum){
	/* Uso la MACRO per ottenere la linea di interrupt */
	memaddr *intLine = (memaddr*) CDEV_BITMAP_ADDR(interruptLineNum);
	/* Ottengo il device a priorità più alta */
	int device = getHighestPriorityDev(intLine);
	/* Ottengo il command register del device */
	memaddr *commandReg = (memaddr*) (DEV_REG_ADDR(interruptLineNum, device) + COMMAND_REG_OFFSET);
	/* Ottengo lo status register del device */
	memaddr *statusReg 	= (memaddr*) (DEV_REG_ADDR(interruptLineNum, device));
	ackAndVerhogen(interruptLineNum, device, (*statusReg), commandReg);
}

/**
	Handler per gli interrupt generati dal timer.
	Compie un'operazione di V su tutti i processi bloccati dalla SYS7 e fa ripartire il timer
*/
void timerHandler(){
	/* Controllo che a generare l'interrupt sia stato lo pseudo-clock */
	if(isTimer(SCHED_PSEUDO_CLOCK)){
		/* Eseguo una V su tutti i processi bloccati */
		while(semPseudoClock < 0){
			verhogen(&semPseudoClock);
		}
	} 
	if(isTimer(SCHED_TIME_SLICE)){
		/* Aggiorno il tempo passato dal processo sulla cpu */
		if(currentProcess != NULL){
			currentProcess->cpu_time += getTODLO() - process_TOD;
			insertProcQ(&readyQueue, currentProcess);
	    	currentProcess = NULL;
		}
	}
}

/**
	Handler per gli interrupt generati dai terminali.
*/
void terminalHandler(){
	/* Uso la MACRO per ottenere la linea di interrupt */
	memaddr *intLine = (memaddr*) CDEV_BITMAP_ADDR(IL_TERMINAL);
	/* Ottengo il device a priorità più alta */
	int device = getHighestPriorityDev(intLine);
	/* Controllo il registro di stato del terminale per sapere se è stata effettuata una 
	  lettura o una scrittura */
	memaddr  terminalRegister = (memaddr)  (DEV_REG_ADDR(IL_TERMINAL, device));
	memaddr* statusRegRead    = (memaddr*) (terminalRegister + TERM_STATUS_READ);
	memaddr* commandRegRead   = (memaddr*) (terminalRegister + TERM_COMMAND_READ);
	memaddr* statusRegWrite	  = (memaddr*) (terminalRegister + TERM_STATUS_WRITE);
	memaddr* commandRegWrite  = (memaddr*) (terminalRegister + TERM_COMMAND_WRITE);
	/* Se è una scrittura (priorità più alta) */
	if(((*statusRegWrite) & 0x0F) == DEV_TTRS_S_CHARTRSM){
		ackAndVerhogen((IL_TERMINAL + 1), device, ((*statusRegWrite)), commandRegWrite);
	}
	/* Altrimenti se è una lettura */
	else if(((*statusRegRead) & 0x0F) == DEV_TRCV_S_CHARRECV){
		ackAndVerhogen(IL_TERMINAL, device, ((*statusRegRead)), commandRegRead);
	}
}