# /*******************************************************************************
# * Copyright 2014, Devid Farinelli, Erik Minarini, Alberto Nicoletti           *
# * This file is part of kaya2014.                                              *
# *                                                                             *
# * kaya2014 is free software: you can redistribute it and/or modify            *
# * it under the terms of the GNU General Public License as published by        *
# * the Free Software Foundation, either version 3 of the License, org          *
# * (at your option) any later version.                                         *
# *                                                                             *
# * kaya2014 is distributed in the hope that it will be useful,                 *
# * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
# * GNU General Public License for more details.                                *
# *                                                                             *
# * You should have received a copy of the GNU General Public licenses          *
# * along with kaya2014.  If not, see <http://www.gnu.org/licenses/>.           *
# ******************************************************************************/

UARM_H		= include/base.h include/types10.h /usr/include/uarm/libuarm.h /usr/include/uarm/uARMconst.h /usr/include/uarm/uARMtypes.h /usr/include/uarm/arch.h
PHASE1_H 	= include/asl.h include/pcb.h
CONST_H		= include/const.h
OBJECTS		= pcb.o asl.o initial.o scheduler.o syscall.o interrupts.o exceptions.o p2test.o
UARM_LIBS	= /usr/include/uarm/ldscripts/elf32ltsarm.h.uarmcore.x -o kaya /usr/include/uarm/crtso.o /usr/include/uarm/libuarm.o

all: kaya

p1: p1test.o pcb.o asl.o
	arm-none-eabi-ld -T $(UARM_LIBS)
	elf2uarm -k kaya

kaya: $(OBJECTS)
	arm-none-eabi-ld -T $(UARM_LIBS) $(OBJECTS)
	elf2uarm -k kaya

pcb.o: pcb.c include/types10.h include/pcb.h
	arm-none-eabi-gcc -mcpu=arm7tdmi -c -Wall -I "/usr/include/uarm" -o pcb.o pcb.c

asl.o: asl.c $(PHASE1_H)
	arm-none-eabi-gcc -mcpu=arm7tdmi -c -Wall -I "/usr/include/uarm" -o asl.o asl.c
	
initial.o: initial.c include/p2test.h include/initial.h include/scheduler.h $(UARM_H) $(CONST_H) $(PHASE1_H)
	arm-none-eabi-gcc -mcpu=arm7tdmi -c -Wall -I "/usr/include/uarm" -o initial.o initial.c

scheduler.o: scheduler.c include/initial.h include/scheduler.h $(UARM_H) $(CONST_H)
	arm-none-eabi-gcc -mcpu=arm7tdmi -c -Wall -I "/usr/include/uarm" -o scheduler.o scheduler.c

syscall.o: syscall.c include/initial.h include/scheduler.h $(UARM_H) $(CONST_H) $(PHASE1_H)
	arm-none-eabi-gcc -mcpu=arm7tdmi -c -Wall -I "/usr/include/uarm" -o syscall.o syscall.c

interrupts.o: interrupts.c include/scheduler.h include/interrupts.h include/syscall.h $(UARM_H) $(CONST_H)
	arm-none-eabi-gcc -mcpu=arm7tdmi -c -Wall -I "/usr/include/uarm" -o interrupts.o interrupts.c

exceptions.o: exceptions.c include/exceptions.h $(UARM_H) $(CONST_H)
	arm-none-eabi-gcc -mcpu=arm7tdmi -c -Wall -I "/usr/include/uarm" -o exceptions.o exceptions.c

p1test.o: p1test.c $(UARM_H) $(PHASE1_H)
	arm-none-eabi-gcc -mcpu=arm7tdmi -c -Wall -I "/usr/include/uarm" -o p1test.o p1test.c

p2test.o: p2test.c include/p2test.h $(UARM_H)
	arm-none-eabi-gcc -mcpu=arm7tdmi -c -Wall -I "/usr/include/uarm" -I "./include" -o p2test.o p2test.c

clean:
	rm -rf *o kaya

cleanall:
	rm -rf *o kaya kaya.core.uarm kaya.stab.uarm
