#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "psched.h"
#include "spinlock.h"

extern struct{
	struct spinlock lock;
	struct proc proc[NPROC];
}ptable;


int
sys_nice(void){
	//cprintf("sysproc.c\n");
	int n;
	int pre;
	struct proc *currP = myproc();
	//check currP NULL?
	if(currP == 0){
		return -1;
	}
	pre = currP->nice;
	if(argint(0, &n) < 0){
		return -1;
	}
	if(n < 0 || n > 20){
		return -1;
	}
	currP->nice = n;
	return pre;
}

int
sys_getschedstate(void){
	struct pschedinfo *psinfo;
	if(argptr(0, (void*)&psinfo, sizeof(*psinfo) < 0)){
		return -1;
	}
	//check psinfo NULL?
	if(psinfo == 0){
		return -1;
	}
	for(int i = 0; i < NPROC; i++) {
		//cprintf("%d\n", i);
		psinfo->inuse[i] = (ptable.proc[i].state != UNUSED);
		psinfo->priority[i] = ptable.proc[i].priority;
		psinfo->nice[i] = ptable.proc[i].nice;
		psinfo->pid[i] = ptable.proc[i].pid;
		psinfo->ticks[i] = ptable.proc[i].totalticks;
	}
	return 0;
}


	int
sys_fork(void)
{
	return fork();
}

	int
sys_exit(void)
{
	exit();
	return 0;  // not reached
}

	int
sys_wait(void)
{
	return wait();
}

	int
sys_kill(void)
{
	int pid;

	if(argint(0, &pid) < 0)
		return -1;
	return kill(pid);
}

	int
sys_getpid(void)
{
	return myproc()->pid;
}

	int
sys_sbrk(void)
{
	int addr;
	int n;

	if(argint(0, &n) < 0)
		return -1;
	addr = myproc()->sz;
	if(growproc(n) < 0)
		return -1;
	return addr;
}

	int
sys_sleep(void)
{
	int n;
	uint ticks0;

	if(argint(0, &n) < 0)
		return -1;
	//set p->wakeup_tick = n ?
	myproc()->sleepticks = n + ticks;
	acquire(&tickslock);
	ticks0 = ticks;
	while(ticks - ticks0 < n){
		if(myproc()->killed){
			release(&tickslock);
			return -1;
		}
		sleep(&ticks, &tickslock);
	}
	release(&tickslock);
	return 0;
}

// return how many clock tick interrupts have occurred
// since start.
	int
sys_uptime(void)
{
	uint xticks;

	acquire(&tickslock);
	xticks = ticks;
	release(&tickslock);
	return xticks;
}

