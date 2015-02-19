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

#ifndef SYSCALL_H
#define SYSCALL_H

void createProcess(state_t *stato);
void terminateProcess(pcb_t *pcb);
void verhogen(int *sem);
void passeren(int *sem);
void specExStVec(int type, state_t *oldp, state_t *newp);
void getCPUTime();
void waitForClock();
void waitForIO(int line, int dev, int waitForTermRead);

#endif
