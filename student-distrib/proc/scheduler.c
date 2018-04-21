#include "scheduler.h"

#include "signal.h"

static pid_t scheduler_iterator = 0;

int scheduler_on_flag = 0;

void scheduling_start(){
	scheduler_on_flag = 1;
}

void scheduler_event(){
	pid_t prev = task_current_pid();
	task_t *proc;
	int i;

	scheduler_iterator ++;
	while (task_list[scheduler_iterator].status != TASK_ST_RUNNING){
		++scheduler_iterator;
		if (scheduler_iterator >= (TASK_MAX_PROC-1)){
			scheduler_iterator = 0;
		}
	}
	// don't tell there's only one process running!
/*	if (prev == scheduler_iterator){
		// well no need to switch then
		return;
	}*/

	proc = task_list + scheduler_iterator;
	if (proc->signals) {
		for (i = 1; i < SIG_MAX; i++) {
			if (proc->signals & (1<<i)) {
				proc->signals &= ~(1<<i);
				signal_exec(1<<i);
				// Should not return
			}
		}
	}

	// A different running task! switch to it!
	scheduler_switch(&task_list[prev], &task_list[scheduler_iterator]);
}

void scheduler_switch(task_t* from, task_t* to){
	// tear down original paging
	scheduler_page_clear(from->pages);
	// set up new pagin
	scheduler_page_setup(to->pages);

	// set up tss
	tss.ss0 = KERNEL_DS;
	tss.esp0 = to->ks_esp;

	// ---- play with registers & set up IRET ----

	// load register address according to magic number on the stack
	regs_t* original = scheduler_get_magic();

	// save the registers of from process
	memcpy(&from->regs, original, sizeof(regs_t));

	// TODO SET DS

	/*if (to->regs.cs == KERNEL_CS){
		scheduler_kernel_iret(&(to->regs), to->regs.esp_k);
	}else{
		// brutal force iret
		scheduler_user_iret(&(to->regs));
	}*/
	scheduler_iret(&(to->regs));
}

void scheduler_page_clear(task_ptentry_t* pages){
	int i;
	for (i=0; i<TASK_MAX_PAGE_MAPS; ++i){
		// check whether the page is present
		if (pages[i].pt_flags & PAGE_DIR_ENT_PRESENT) {
			// then delete this entry
			if (pages[i].pt_flags & PAGE_DIR_ENT_4MB){
				page_dir_delete_entry(pages[i].vaddr);
			}else{
				page_tab_delete_entry(pages[i].vaddr);
			}
		}
		// else do nothing
	}
}

void scheduler_page_setup(task_ptentry_t* pages){
	int i;
	for (i=0; i<TASK_MAX_PAGE_MAPS; ++i){
		// check whether the page is present
		if (pages[i].pt_flags & PAGE_DIR_ENT_PRESENT) {
			// then setup this entry
			if (pages[i].pt_flags & PAGE_DIR_ENT_4MB){
				page_dir_add_4MB_entry(pages[i].vaddr, pages[i].paddr, pages[i].pt_flags);
			}else{
				page_tab_add_entry(pages[i].vaddr, pages[i].paddr, pages[i].pt_flags);
			}
		}
		// else do nothing
	}
}
