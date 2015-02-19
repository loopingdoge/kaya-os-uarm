/*****************************************************************************
 * Copyright 2004, 2005 Michael Goldweber, Davide Brini.                     *
 *                                                                           *
 * This file is part of kaya.                                                *
 *                                                                           *
 * kaya is free software; you can redistribute it and/or modify it under the *
 * terms of the GNU General Public License as published by the Free Software *
 * Foundation; either version 2 of the License, or (at your option) any      *
 * later version.                                                            *
 * This program is distributed in the hope that it will be useful, but       *
 * WITHOUT ANY WARRANTY; without even the implied warranty of                *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General *
 * Public License for more details.                                          *
 * You should have received a copy of the GNU General Public License along   *
 * with this program; if not, write to the Free Software Foundation, Inc.,   *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.                  *
 *****************************************************************************/

/*********************************P2TEST.C*******************************
 *
 *	Test program for the Kaya Kernel: phase 2.
 *
 *	Produces progress messages on Terminal0.
 *	
 *	This is pretty convoluted code, so good luck!
 *
 *		Aborts as soon as an error is detected.
 *
 *      Modified by Michael Goldweber on May 15, 2004
 *      Modified by Davide Brini on Nov 26, 2004
 *      Modified by Renzo Davoli 2010
 * 		Modified by Marco Melletti 2014
 */

#include <uARMconst.h>
#include <uARMtypes.h>
#include <base.h>
#include <libuarm.h>

typedef unsigned int devregtr;
/* if these are not defined */
typedef U32 cpu_t;

/* hardware constants */
#define PRINTCHR	2
#define BYTELEN	8
#define RECVD	5
#define TRANSM 5

#define CLOCKINTERVAL	100000UL	/* interval to V clock semaphore */

#define TERMSTATMASK	0xFF
#define TERMCHARMASK	0xFF00
#define CAUSEMASK		0xFFFFFF
#define VMOFF 			0xF8FFFFFF

#define QPAGE			1024

#define KUPBITON		0x8
#define KUPBITOFF		0xFFFFFFF7

#define MINLOOPTIME		10000
#define LOOPNUM 		10000

#define CLOCKLOOP		10
#define MINCLOCKLOOP	3000	

#define BADADDR			0xFFFFFFFF /* could be 0x00000000 as well */
#define	TERM0ADDR		0x240

/* Software and other constants */
#define PRGOLDVECT		4
#define TLBOLDVECT		2
#define SYSOLDVECT		6

#define CREATENOGOOD	-1

#define TERMINATENOGOOD	-1

#define ON        1
#define OFF       0

/* just to be clear */
#define SEMAPHORE		S32
#define NOLEAVES		4	/* number of leaves of p8 process tree */
#define MAXSEM			20



SEMAPHORE term_mut=1,	/* for mutual exclusion on terminal */
		s[MAXSEM+1],	/* semaphore array */
		testsem=0,		/* for a simple test */
		startp2=0,		/* used to start p2 */
		endp2=0,		/* used to signal p2's demise */
		endp3=0,		/* used to signal p3's demise */
		blkp4=1,		/* used to block second incaration of p4 */
		synp4=0,		/* used to allow p4 incarnations to synhronize */
		endp4=0,		/* to signal demise of p4 */
		endp5=0,		/* to signal demise of p5 */
		endp8=0,		/* to signal demise of p8 */
		endcreate=0,	/* for a p8 leaf to signal its creation */
		blkp8=0;		/* to block p8 */

state_t p2state, p3state, p4state, p5state,	p6state, p7state;
state_t p8rootstate, child1state, child2state;
state_t gchild1state, gchild2state, gchild3state, gchild4state;

/* trap states for p5 */
state_t pstat_n, mstat_n, sstat_n, pstat_o,	mstat_o, sstat_o;

int		p1p2synch = 0;	/* to check on p1/p2 synchronization */

int 	p8inc;			/* p8's incarnation number */ 
int		p4inc=1;		/* p4 incarnation number */

unsigned int p5Stack;	/* so we can allocate new stack for 2nd p5 */

int creation = 0; 				/* return code for SYSCALL invocation */
memaddr *p5MemLocation = (memaddr *) 0x34;		/* To cause a p5 trap */

void	p2(),p3(),p4(),p5(),p5a(),p5b(),p6(),p7(),p7a(),p5prog(),p5mm();
void	p5sys(),p8root(),child1(),child2(),p8leaf();

/* a procedure to print on terminal 0 */
void print(char *msg) {

	char * s = msg;
	devregtr * base = (devregtr *) (TERM0ADDR);
	devregtr status;
	
	SYSCALL(PASSEREN, (int)&term_mut, 0, 0);				/* get term_mut lock */
	
	while (*s != '\0') {
	  /* Put "transmit char" command+char in term0 register (3rd word). This 
		   actually starts the operation on the device! */
		*(base + 3) = PRINTCHR | (((devregtr) *s) << BYTELEN);
		
		/* Wait for I/O completion (SYS8) */
		status = SYSCALL(WAITIO, INT_TERMINAL, 0, FALSE);
		
		if ((status & TERMSTATMASK) != TRANSM)
			PANIC();
			
		if (((status & TERMCHARMASK) >> BYTELEN) != *s)
			PANIC();

		s++;	
	}
	
	SYSCALL(VERHOGEN, (int)&term_mut, 0, 0);				/* release term_mut */
}


/*                                                                   */
/*                 p1 -- the root process                            */
/*                                                                   */
void test() {	
	cpu_t		time1, time2;
	
	SYSCALL(VERHOGEN, (int)&testsem, 0, 0);					/* V(testsem)   */

	if (testsem != 1) { print("error: p1 v(testsem) with no effects\n"); PANIC(); }

	print("p1 v(testsem)\n");

	/* set up states of the other processes */

	/* set up p2's state */
	STST(&p2state);			/* create a state area using p1's state    */
	
	/* stack of p2 should sit above ??????  */
	p2state.sp = p2state.sp - QPAGE;
	
	/* p2 starts executing function p2 */
	p2state.pc = (memaddr)p2;
	
	/* p2 has interrupts on and unmasked */
	p2state.cpsr = STATUS_ALL_INT_ENABLE(p2state.cpsr);
		
	
  /* Set up p3's state */
	STST(&p3state);

  /* p3's stack is another 1K below p2's one */
	p3state.sp = p2state.sp - QPAGE;
	p3state.pc = (memaddr)p3;
	p3state.cpsr = STATUS_ALL_INT_ENABLE(p3state.cpsr);
	
	
	/* Set up p4's state */
	STST(&p4state);

  /* p4's stack is another 1k below p3's one */
	p4state.sp = p3state.sp - QPAGE;
	p4state.pc = (memaddr)p4;
	p4state.cpsr = STATUS_ALL_INT_ENABLE(p4state.cpsr);
	
	/* Set up p5's state */
	STST(&p5state);
		
	/* because there will be two p4s running*/
	/* Record the value in p5stack */
	p5Stack = p5state.sp = p4state.sp - (2 * QPAGE);
	p5state.pc = (memaddr)p5;
	p5state.cpsr = STATUS_ALL_INT_ENABLE(p5state.cpsr);

  /* Set up p6's state */
	STST(&p6state);
	
	/* Ther will be two p5s ???? */
	p6state.sp = p5state.sp - (2 * QPAGE);
	p6state.pc = (memaddr)p6;
	p6state.cpsr = STATUS_ALL_INT_ENABLE(p6state.cpsr);
	
	
	/* Set up p7's state */
	STST(&p7state);
	
	/* Only one p6 */
	p7state.sp = p6state.sp - QPAGE;
	p7state.pc = (memaddr)p7;
	p7state.cpsr = STATUS_ALL_INT_ENABLE(p7state.cpsr);

	STST(&p8rootstate);
	p8rootstate.sp = p7state.sp - QPAGE;
	p8rootstate.pc = (memaddr)p8root;
	p8rootstate.cpsr = STATUS_ALL_INT_ENABLE(p8rootstate.cpsr);
    
	STST(&child1state);
	child1state.sp = p8rootstate.sp - QPAGE;
	child1state.pc = (memaddr)child1;
	child1state.cpsr = STATUS_ALL_INT_ENABLE(child1state.cpsr);
	
	STST(&child2state);
	child2state.sp = child1state.sp - QPAGE;
	child2state.pc = (memaddr)child2;
	child2state.cpsr = STATUS_ALL_INT_ENABLE(child2state.cpsr);
	
	STST(&gchild1state);
	gchild1state.sp = child2state.sp - QPAGE;
	gchild1state.pc = (memaddr)p8leaf;
	gchild1state.cpsr = STATUS_ALL_INT_ENABLE(gchild1state.cpsr);

	STST(&gchild2state);
	gchild2state.sp = gchild1state.sp - QPAGE;
	gchild2state.pc = (memaddr)p8leaf;
	gchild2state.cpsr = STATUS_ALL_INT_ENABLE(gchild2state.cpsr);
	
	STST(&gchild3state);
	gchild3state.sp = gchild2state.sp - QPAGE;
	gchild3state.pc = (memaddr)p8leaf;
	gchild3state.cpsr = STATUS_ALL_INT_ENABLE(gchild3state.cpsr);
	
	STST(&gchild4state);
	gchild4state.sp = gchild3state.sp - QPAGE;
	gchild4state.pc = (memaddr)p8leaf;
	gchild4state.cpsr = STATUS_ALL_INT_ENABLE(gchild4state.cpsr);
	
	/* create process p2 */
	SYSCALL(CREATEPROCESS, (int)&p2state, 0, 0);				/* start p2     */

	print("p2 was started\n");

	SYSCALL(VERHOGEN, (int)&startp2, 0, 0);					/* V(startp2)   */

  /* P1 blocks until p2 finishes and Vs endp2 */
	SYSCALL(PASSEREN, (int)&endp2, 0, 0);					/* P(endp2)     */

	/* make sure we really blocked */
	if (p1p2synch == 0)
		print("error: p1/p2 synchronization bad\n");

	SYSCALL(CREATEPROCESS, (int)&p3state, 0, 0);				/* start p3  */

	print("p3 is started\n");

  /* P1 blocks until p3 ends */
	SYSCALL(PASSEREN, (int)&endp3, 0, 0);					/* P(endp3)     */


	SYSCALL(CREATEPROCESS, (int)&p4state, 0, 0);		/* start p4     */

	SYSCALL(CREATEPROCESS, (int)&p5state, 0, 0); 		/* start p5     */

	SYSCALL(CREATEPROCESS, (int)&p6state, 0, 0);		/* start p6		*/

	SYSCALL(CREATEPROCESS, (int)&p7state, 0, 0);		/* start p7		*/

	SYSCALL(PASSEREN, (int)&endp5, 0, 0);
	
	print("p1 knows p5 ended\n");

	SYSCALL(PASSEREN, (int)&blkp4, 0, 0);					/* P(blkp4)		*/
	
	print("p1 knows p4 ended\n");
	
	/* now for a more rigorous check of process termination */
	SYSCALL(VERHOGEN, (int)&blkp8, 0, 0);
	
	for (p8inc = 0; p8inc < 4; p8inc++) {
		SYSCALL(PASSEREN, (int)&blkp8, 0, 0);
		
		creation = SYSCALL(CREATEPROCESS, (int)&p8rootstate, 0, 0);

		if (creation == CREATENOGOOD) {
			print("error in p8 process creation\n");
			PANIC();
		}

		SYSCALL(PASSEREN, (int)&endp8, 0, 0);
		
		/* do some delay to be reasonably sure p8 and its offspring are dead */
		time1 = 0;
		time2 = 0;
		while (time2 - time1 < (CLOCKINTERVAL >> 1))  {
			time1 = getTODLO();
			SYSCALL(WAITCLOCK, 0, 0, 0);
			time2 = getTODLO();
		}
		
		SYSCALL(VERHOGEN, (int)&blkp8, 0, 0);
	}
	
	print("p1 finishes OK -- TTFN\n");
	* ((memaddr *) BADADDR) = 0;				/* terminate p1 */

	/* should not reach this point, since p1 just got a program trap */
	print("error: p1 still alive after progtrap & no trap vector\n");
	PANIC();					/* PANIC !!!     */
}


/* p2 -- semaphore and cputime-SYS test process */
void p2() {
	int		i;				       /* just to waste time  */
	cpu_t	now1,now2;		   /* times of day        */
	cpu_t	cpu_t1, cpu_t2;	 /* cpu time used       */

  /* startp2 is initialized to 0. p1 Vs it then waits for p2 termination */
	SYSCALL(PASSEREN, (int)&startp2, 0, 0);				/* P(startp2)   */

	print("p2 starts\n");

	/* initialize all semaphores in the s[] array */
	for (i = 0; i <= MAXSEM; i++) s[i] = 0;

	/* V, then P, all of the semaphores in the s[] array */
	for (i = 0; i <= MAXSEM; i++)  {
		SYSCALL(VERHOGEN, (int)&s[i], 0, 0);			/* V(S[I]) */
		SYSCALL(PASSEREN, (int)&s[i], 0, 0);			/* P(S[I]) */
		if (s[i] != 0) print("error: p2 bad v/p pairs\n");
	}

	print("p2 v/p pairs successfully\n");

	/* test of SYS6 */

	now1 = getTODLO();                  				/* time of day   */
	cpu_t1 = SYSCALL(GETCPUTIME, 0, 0, 0);			/* CPU time used */

	/* delay for several milliseconds */
	for (i = 1; i < LOOPNUM; i++)
		;

	cpu_t2 = SYSCALL(GETCPUTIME, 0, 0, 0);			/* CPU time used */
	now2 = getTODLO();				/* time of day  */

	if (((now2 - now1) >= (cpu_t2 - cpu_t1)) &&
			((cpu_t2 - cpu_t1) >= MINLOOPTIME)) {
		print("p2 is OK\n");
	} else {
		if ((now2 - now1) < (cpu_t2 - cpu_t1))
			print ("error: more cpu time than real time\n");
		if ((cpu_t2 - cpu_t1) < MINLOOPTIME)
			print ("error: not enough cpu time went by\n");
		print("p2 blew it!\n");
	}

	p1p2synch = 1;				/* p1 will check this */

	SYSCALL(VERHOGEN, (int)&endp2, 0, 0);				/* V(endp2)     */

	SYSCALL(TERMINATEPROCESS, 0, 0, 0);			/* terminate p2 */

	/* just did a SYS2, so should not get to this point */
	print("error: p2 didn't terminate\n");
	PANIC();					/* PANIC! */
}


/* p3 -- clock semaphore test process */
void p3() {
	cpu_t	time1, time2;
	cpu_t	cpu_t1,cpu_t2;	/* cpu time used */
	int	i;

	time1 = 0;
	time2 = 0;

	/* loop until we are delayed at least half of clock V interval */
	while ((time2 - time1) < (CLOCKINTERVAL >> 1) )  {
		time1 = getTODLO();		/* time of day     */
		SYSCALL(WAITCLOCK, 0, 0, 0);
		time2 = getTODLO();			/* new time of day */
	}

	print("p3 - WAITCLOCK OK\n");

	/* now let's check to see if we're really charged for CPU
	   time correctly */
	cpu_t1 = SYSCALL(GETCPUTIME, 0, 0, 0);

	for (i = 0; i < CLOCKLOOP; i++)
		SYSCALL(WAITCLOCK, 0, 0, 0);
		
	cpu_t2 = SYSCALL(GETCPUTIME, 0, 0, 0);

	if ((cpu_t2 - cpu_t1) < MINCLOCKLOOP)
		print("error: p3 - CPU time incorrectly maintained\n");
	else
		print("p3 - CPU time correctly maintained\n");

	SYSCALL(VERHOGEN, (int)&endp3, 0, 0);				/* V(endp3)        */

	SYSCALL(TERMINATEPROCESS, 0, 0, 0);			/* terminate p3    */

	/* just did a SYS2, so should not get to this point */
	print("error: p3 didn't terminate\n");
	PANIC();					/* PANIC  */
}


/* p4 -- termination test process */
void p4() {
	
	switch (p4inc) {
		case 1:
			print("first incarnation of p4 starts\n");
			break;
		case 2:
			print("second incarnation of p4 starts\n");
			break;
	}
	p4inc++;

	SYSCALL(VERHOGEN, (int)&synp4, 0, 0);				/* V(synp4)     */

	/* first incarnation made blkp4=0, the second is blocked (blkp4 become -1) */
	SYSCALL(PASSEREN, (int)&blkp4, 0, 0);				/* P(blkp4)     */

	if(p4inc == 3){
		print("error: second incarnation of p4 did not terminate with original p4\n");
		PANIC();
	}

	SYSCALL(PASSEREN, (int)&synp4, 0, 0);				/* P(synp4)     */

	/* start another incarnation of p4 running, and wait for  */
	/* a V(synp4). the new process will block at the P(blkp4),*/
	/* and eventually, the parent p4 will terminate, killing  */
	/* off both p4's.                                         */

	p4state.sp -= QPAGE;		/* give another page  */

	print("p4 create a new p4\n");
	SYSCALL(CREATEPROCESS, (int)&p4state, 0, 0);			/* start a new p4    */

	SYSCALL(PASSEREN, (int)&synp4, 0, 0);				/* wait for it       */

	SYSCALL(VERHOGEN, (int)&endp4, 0, 0);				/* V(endp4)          */

	print("p4 termination with the child\n");
	
	SYSCALL(TERMINATEPROCESS, 0, 0, 0);			/* terminate p4      */

	/* just did a SYS2, so should not get to this point */
	print("error: p4 didn't terminate\n");
	PANIC();					/* PANIC            */
}



/* p5's program trap handler */
void p5prog() {
	unsigned int exeCode = pstat_o.CP15_Cause;
	exeCode = exeCode & CAUSEMASK;
	
	switch (exeCode) {
	case EXC_BUSINVFETCH:
		print("pgmTrapHandler - Access non-existent memory\n");
		pstat_o.pc = (memaddr)p5a;   /* Continue with p5a() */
		break;
		
	case EXC_RESERVEDINSTR:
		print("pgmTrapHandler - privileged instruction - set kernel mode on\n");
		print("p5 - try to call sys13 in kernel mode\n");
		/* return in kernel mode */
		pstat_o.cpsr = pstat_o.cpsr | 0xF;
		pstat_o.pc = (memaddr)p5b;
		break;
		
	case EXC_ADDRINVLOAD:
		print("pgmTrapHandler - Address Error: KSegOS w/KU=1\n");
		/* return in kernel mode */
		pstat_o.cpsr = pstat_o.cpsr | 0xF;
		pstat_o.pc = (memaddr)p5b;
		break;
		
	default:
		print("pgmTrapHandler - other program trap\n");
	}
	
	LDST(&pstat_o);  /* "return" to old area (that changed meanwhile) */
}

/* p5's memory management (tlb) trap handler */
/* void p5mm(unsigned int cause) { */
void p5mm() {
	print("memory management (tlb) trap - set user mode on\n");
	mstat_o.cpsr = mstat_o.cpsr & 0xFFFFFFF0;  /* user mode on */
	mstat_o.CP15_Control &= ~(0x1); /* disable VM */
	mstat_o.pc = (memaddr)p5b;  /* return to p5b */
	mstat_o.sp = p5Stack-QPAGE;				/* Start with a fresh stack */

	/* this should be done by p5b(). print() is called here because p5b() is executed in user mode */
	print("p5 - try to call sys13 in user mode\n");
	
	LDST(&mstat_o);
}

/* p5's SYS/BK trap handler */
/* void p5sys(unsigned int cause) { */
void p5sys() {
	unsigned int p5status = sstat_o.cpsr;
	p5status = p5status & 0xF; 
	if(p5status){
			print("High level SYS call from kernel mode process\n");
	} else {
		print("High level SYS call from user mode process\n");
		print("p5 - try to execute P() in user mode\n");
	}
	LDST(&sstat_o);
}

/* p5 -- SYS5 test process */
void p5() {
	print("p5 starts\n");

	/* set up higher level TRAP handlers (new areas) */
	STST(&pstat_n);  /* pgmtrap new area */
	pstat_n.pc = (memaddr)p5prog; /* pgmtrap exceptions */
	
	STST(&mstat_n);  /* tlb new area */
	mstat_n.pc = (memaddr)p5mm;   /* tlb exceptions */
	
	STST(&sstat_n);  /* sys/bk new area */
	sstat_n.pc = (memaddr)p5sys;  /* sys/bk exceptions */

	/* trap handlers should operate in complete mutex: no interrupts on */
	/* this because they must restart using some BIOS area */
	/* thus, IEP bit is not set for them (see test() for an example of it) */


	/* specify trap vectors */
	SYSCALL(SPECTRAPVEC, SPECPGMT, (int)&pstat_o, (int)&pstat_n);

	SYSCALL(SPECTRAPVEC, SPECTLB, (int)&mstat_o, (int)&mstat_n);

	SYSCALL(SPECTRAPVEC, SPECSYSBP, (int)&sstat_o, (int)&sstat_n);

	print("p5 - try to cause a pgm trap accessing some non-existent memory\n");
	/* to cause a pgm trap access some non-existent memory */	
	*p5MemLocation = *p5MemLocation + 1;		 /* Should cause a program trap */
}

void p5a() {
	unsigned int p5Status;

	print("p5 - try to generate a TLB exception\n");
	
	/* generate a TLB exception by turning on VM without setting up the 
	   seg tables */
	p5Status = getCONTROL();
	p5Status = p5Status | 0x00000001;
	setCONTROL(p5Status);
}

/* second part of p5 - should be entered in user mode */
void p5b() {
	cpu_t		time1, time2;
	
	SYSCALL(13, 0, 0, 0);
	
	/* the first time through, we are in user mode */
	/* and the P should generate a program trap */
	SYSCALL(PASSEREN, (int)&endp4, 0, 0);			/* P(endp4)*/

	/* do some delay to be reasonably sure p4 and its offspring are dead */
	time1 = 0;
	time2 = 0;
	while (time2 - time1 < (CLOCKINTERVAL >> 1))  {
		time1 = getTODLO();
		SYSCALL(WAITCLOCK, 0, 0, 0);
		time2 = getTODLO();
	}

	/* if p4 and offspring are really dead, this will increment blkp4 */

	SYSCALL(VERHOGEN, (int)&blkp4, 0, 0);			/* V(blkp4) */

	SYSCALL(VERHOGEN, (int)&endp5, 0, 0);			/* V(endp5) */

	print("p5 - try to redefine PGMVECT, this will cause p5 termination\n");
	/* should cause a termination       */
	/* since this has already been      */
	/* done for PROGTRAPs               */
	SYSCALL(SPECTRAPVEC, 1, (int)&pstat_o, (int)&pstat_n);

	/* should have terminated, so should not get to this point */
	print("error: p5 didn't terminate\n");
	PANIC();				/* PANIC            */
}


/*p6 -- high level syscall without initializing trap vector*/
void p6() {
	print("p6 starts\n");

	SYSCALL(13, 0, 0, 0);		/* should cause termination because p6 has no 
			  trap vector */

	print("error: p6 alive after SYS13() with no trap vector\n");

	PANIC();
}

/*p7 -- program trap without initializing passup vector*/
void p7() {
	print("p7 starts\n");

	* ((memaddr *) BADADDR) = 0;
		
	print("error: p7 alive after program trap with no trap vector\n");
	PANIC();
}


/* p8root -- test of termination of subtree of processes              */
/* create a subtree of processes, wait for the leaves to block, signal*/
/* the root process, and then terminate                               */
void p8root() {
	int		grandchild;

	print("p8root starts\n");

	SYSCALL(CREATEPROCESS, (int)&child1state, 0, 0);

	SYSCALL(CREATEPROCESS, (int)&child2state, 0, 0);

	for (grandchild=0; grandchild < NOLEAVES; grandchild++) {
		SYSCALL(PASSEREN, (int)&endcreate, 0, 0);
	}
	
	SYSCALL(VERHOGEN, (int)&endp8, 0, 0);

	SYSCALL(TERMINATEPROCESS, 0, 0, 0);
}

/*child1 & child2 -- create two sub-processes each*/

void child1() {
	print("child1 starts\n");
	
	SYSCALL(CREATEPROCESS, (int)&gchild1state, 0, 0);
	
	SYSCALL(CREATEPROCESS, (int)&gchild2state, 0, 0);

	SYSCALL(PASSEREN, (int)&blkp8, 0, 0);
	
	print("error: p8 child was not killed with father\n");
	PANIC();
}

void child2() {
	print("child2 starts\n");

	SYSCALL(CREATEPROCESS, (int)&gchild3state, 0, 0);
	
	SYSCALL(CREATEPROCESS, (int)&gchild4state, 0, 0);

	SYSCALL(PASSEREN, (int)&blkp8, 0, 0);
	
	print("error: p8 child was not killed with father\n");
	PANIC();
}

/*p8leaf -- code for leaf processes*/

void p8leaf() {
	print("leaf process starts\n");

	SYSCALL(VERHOGEN, (int)&endcreate, 0, 0);

	SYSCALL(PASSEREN, (int)&blkp8, 0, 0);
	
	print("error: p8 grandchild was not killed with father\n");
	PANIC();
}
