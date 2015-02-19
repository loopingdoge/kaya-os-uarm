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

#ifndef ASL_H
#define ASL_H
/* Funzioni gestione liste semafori */

EXTERN void initASL(void);
EXTERN int insertBlocked(int *semAdd, pcb_t *p); /*Inserisce il ProcBlk puntato da p*/
EXTERN pcb_t *removeBlocked(int *semAdd); /* rimuove il primo ProcBlk, se trovato */
EXTERN pcb_t *outBlocked(pcb_t *p); /* Rimuove il ProcBlk puntato da p */
EXTERN pcb_t *headBlocked(int *semAdd); /*ritorna un puntatore al ProcBlk */

#endif
