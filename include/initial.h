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

#ifndef INITIAL_H
#define INITIAL_H

void initArea(memaddr area, memaddr handler);

memaddr* getSemDev(int line, int dev);

memaddr* getKernelStatusDev(int line, int dev);

/* Numero di processi nel sistema */
extern U32 processCount;
/* Numero di processi bloccati in attesa di un interrupt */
extern U32 softBlockCount;
/* Puntatore (a tail) alla coda di pcb_t dei processi ready */
extern pcb_t *readyQueue;
/* Puntatore alla pcb_t del processo corrente */
extern pcb_t *currentProcess;
/* Semaforo per la linea di interrupt 0 */
extern int semIpi;
/* Semaforo per la linea di interrupt 1 */
extern int semCpuTimer;
/* Matrice di semafori dei device */
extern int semDev[N_EXT_IL + 1][N_DEV_PER_IL];
/* Matrice contenente lo status register dei device */
extern int statusDev[N_EXT_IL + 1][N_DEV_PER_IL];
/* Semaforo del Pseudo-Clock Timer */
extern int semPseudoClock;

#endif
