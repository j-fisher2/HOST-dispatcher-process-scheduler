#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
/******************************/
#define  TOTAL_MEM 	  1024	//Total amount of memory (MB) for processes
#define	 RESERVED_MEM 64		// reserved for real-time processes
#define  NUM_PRINTERS 2			//Number of printers
#define  NUM_SCANNERS 1			//Number of scanners
#define  NUM_MODEMS	 1			//Number of modems
#define  NUM_CDS	 2	      //Number of CD drives
/*****************************/

/*************Process statuses*************/
#define NOT_STARTED 0
#define RUNNING 1
#define SUSPENDED 2
#define TERMINATED 3
/****************************************/
#define NEWLINE '\n'

int PID=1;  //starting PID, incremented as each process is added

//input format:  <arrival time>, <priority>, <processor time>, <Mbytes>, <#printers>, <#scanners>, <#modems>, <#CDs>

struct mab {
	int offset;				//Location of memory block	
	int size;				//Size of the memory block		
	int allocated;			//Indicates whether the block is currently allocated to a process - 0 false, 1 true
	struct mab * next;		//Pointer to next block of memory
	struct mab * previous;	//Pointer to previous block of memory
};
typedef struct mab Mab;
typedef Mab * mabptr;

/*Process Information*/
struct PCB {
	int pid;						
	int arrival_time;	
	int remaining_cpu_time;
	int priority;
	int status;						//Status of the process
	int num_printers;
	int num_scanners;
	int num_modems;
	int num_cds;
	int memory_alloc;
	mabptr memory;					//Allocated memory block. NULL no memory has been allocated.	
	struct PCB* next;				//Link for PCB handler
};
typedef struct PCB pcb;
typedef pcb * pcbptr;

/*Queue data strucutre*/
struct Queue {
	unsigned int timer;
	pcbptr front;
	pcbptr back;
};
typedef struct Queue queue;

int mRemainingMem = TOTAL_MEM;	//Amount of free memory in the system
mabptr mFirstBlock = NULL;	//Pointer to the first block in memory

//Initializes an empty queue
void init_queue(queue* q) {
	q->front = NULL;
	q->back = NULL;
	q->timer = 0;
}

//Returns true if queue is empty. False otherwise
int isEmptyQueue(queue q) {
	if(q.front == 0){
	    return 1;
	}
	return 0;
}

/*Adds the PCB into the queue*/
void enqueue(queue* q, pcbptr value) {
	value->next = NULL;
	if(!isEmptyQueue(*q) )
		q->back->next = value;
	else
		q->front = value;
	q->back = value;
}

//Removes element at the front of the queue.
pcbptr dequeue(queue* q) {
	if(q == NULL)
		return NULL;	
		
	pcbptr front = q->front;
	if (front != NULL) {
		q->front = q->front->next;
	}
	return front;
}


mabptr getNewMemBlock() {
	mabptr memBlock = malloc(sizeof(Mab));
	memBlock->next = NULL;
	memBlock->previous = NULL;
	memBlock->allocated = 0;
	memBlock->size = 0;
	memBlock->offset = 0;

	return memBlock;
}

//check memory
int mem_check(int size) {
	int canBeAllocated = 0; //false

	//initialize the memory if first allocation.
	if (mRemainingMem == TOTAL_MEM && mFirstBlock == NULL) {
		mFirstBlock = getNewMemBlock();// memory consists of a single block initially
		mFirstBlock->size = TOTAL_MEM;
		mFirstBlock->offset = 0;
	}
	mabptr head = mFirstBlock;	//Pointer used to traverse the list of mem. blocks

	/*Traverse the list of mem blocks until an available large enough memory block is found
	   or the end of memory is reached.*/
	while(head != NULL && ( (!head->allocated && head->size < size) ||
			head->allocated )){
		/*If the end of memory is reached, return NULL to indicate allocation failure.*/
		if( (head->offset + head->size) > TOTAL_MEM) {
			canBeAllocated = 0;
			break;
		}
		head = head->next;
	}

	if(head != NULL)
		canBeAllocated = 1;
	else
		canBeAllocated = 0;

	return canBeAllocated;
}

//Splits memory block
mabptr mem_split(mabptr memory, int size) {
	if(memory == NULL) {
		printf("\tERROR - Passed NULL pointer to mem_split()\n");
		return NULL;		
	}
		
	int originalSize = memory->size;
	memory->size = size;
	
	mabptr leftover = NULL;	//Pointer to block of memory left over after the allocation

	if (memory->size < originalSize) {
		/*Create a new block for the leftover memory and redirect the pointers*/
		leftover = getNewMemBlock();
		leftover->allocated = 0;
		leftover->size = originalSize - memory->size;
		leftover->offset = memory->offset + memory->size;
		leftover->previous = memory;
		leftover->next = memory->next;
		memory->next = leftover;
	}
	return leftover;
}

//Allocate memory block
mabptr mem_alloc(int size){
	//If this is the first time allocating memory, initialize the memory.
	if (mRemainingMem == TOTAL_MEM && mFirstBlock == NULL) {
		mFirstBlock = getNewMemBlock();//Initially, the whole memory consists of a single block.
		mFirstBlock->size = TOTAL_MEM;
		mFirstBlock->offset = 0;
	}

	//Make sure there is enough memory
	if (mRemainingMem < size) {
		printf("\tSystem out of memory\n");
		return NULL;
	}
		
	mabptr head = mFirstBlock;	//Pointer used to traverse the list of mem. blocks
	mabptr memory = NULL;		//Pointer to memory being allocated
	
	//Traverse the list of mem blocks until an available large enough memory block is found
	while(head != NULL && ( (!head->allocated && head->size < size) ||
			head->allocated )){
		/*If the end of memory is reached, return NULL*/
		if( (head->offset + head->size) > TOTAL_MEM)
			return NULL;
		head = head->next;
	}

	/*If head == NULL, then the end of memory has been reached.*/
	if (head != NULL) {
		if(!head->allocated && head->size > size) {
			mem_split(head, size); //Reallocate memory block to new process and resize it
			memory = head;
			memory->allocated = 1;
			mRemainingMem -= size;
		} else { //head->size == size
			memory = head;
			memory->allocated = 1;
			mRemainingMem -= memory->size;
		}
	}

	return memory;
}

//Returns true if merge was successful. False otherwise.
int mem_merge(mabptr top, mabptr bottom) {
	if(top == NULL || bottom == NULL) {
		printf("\tERROR - Passed NULL pointer to mem_merge()\n");
		return 0;
	}
	
	/*Redirect pointers*/
	top->next = bottom->next;
	if(bottom->next != NULL)
		bottom->next->previous = top;
	
	/*Free memory*/
	top->size += bottom->size;
	free(bottom);
	bottom = NULL; //Set bottom pointer to NULL since the block it points to no longer exists
	return 1;
}


int mPrinters = NUM_PRINTERS;					//Number of free printers
int mScanners = NUM_SCANNERS;					//Number of free scanners
int mModems = NUM_MODEMS;						//Number of free modems
int mCDs = NUM_CDS;	


const int mQUANTUM = 1;							//CPU time allocated to each process in the feedback queue.
const int mTotalMem = TOTAL_MEM - RESERVED_MEM; //Total memory in the system (excluding the memory reserved for real-time processes).
int mDispatcher_timer;
char *mProcessName[] = {"./process", NULL};		//Process parameters (for execvp())
int status = 0;			

queue mInput;	//Input queue
queue mJobs;	//User job queue

queue mFcfs;	/*First-come-first-serve queue used for real time (priority = 0) processes
				  This queue must be empty before the other queues are activated.*/
				  
queue level1;	//High priority (priority = 1)
queue level2;	//Medium priority (priority = 2)
queue level3;  //Low priority (priority = 3)

pcbptr mActiveProcess = NULL;	//Currently active process. Set to NULL after running process is terminated.

mabptr mReservedMem = NULL;		//Memory reserved for real-time processes.

//Frees data (from actual memory) that the process holds.
void free_process_pointers (pcbptr process) {
	free(mActiveProcess);
}

//Free resources allocated to a process
void rsrc_free(pcbptr process) {
	mPrinters += process->num_printers;
	mScanners += process->num_scanners;
	mModems += process->num_modems;
	mCDs += process->num_cds;

	process->num_printers = 0;
	process->num_scanners = 0;
	process->num_modems = 0;
	process->num_cds = 0;

}
//Returns true if the requested resources are available.
int rsrc_chk(int printers, int scanners, int modems, int cds) {
    if(printers <= mPrinters && scanners <= mScanners &&
			modems <= mModems && cds <= mCDs){
			    return 1;
			}
	return 0;
}

//Allocates resources to process. Returns true if resources were allocated. False otherwise.
int rsrc_alloc (pcbptr process, int printers, int scanners, int modems, int cds) {
	if (!rsrc_chk(printers, scanners, modems, cds))
		return 0;

	/*Allocate resources*/
	process->num_printers = printers;
	process->num_scanners = scanners;
	process->num_modems = modems;
	process->num_cds = cds;

	/*Update resource counts*/
	mPrinters -= printers;
	mScanners -= scanners;
	mModems -= modems;
	mCDs -= cds;

	return 1;
}

void display_process_info(pcbptr process){
    printf("priority %d, processor time remaining %d, memory location %d, block size %d, %d printers %d scanners %d modems %d cds\n",process->priority,process->remaining_cpu_time,process->memory->offset,process->memory_alloc,process->num_printers,process->num_scanners,process->num_modems,process->num_cds);
}

//Starts process
void start_process(pcbptr process) {
	process->status = RUNNING;

	printf("\tProcess ID %d started.\n", process->pid);
	display_process_info(process);
}

//Adds process to appropriate queue based on its priority level.
void placeInQueue(pcbptr process) {
	switch(process->priority) {
		case 0:
			enqueue(&mFcfs, process);
			break;
		case 1:
			enqueue(&level1, process);
			break;
		case 2:
			enqueue(&level2, process);
			break;
		case 3:
			enqueue(&level3, process);
			break;
		default:
			printf("Invalid input in placeInQueue()\n");
			break;			
	}			
}

//Free memory block
void mem_free(mabptr memory){	
	if(memory == NULL)
		return;

	memory->allocated = 0;
	mRemainingMem += memory->size;
	
	/*If previous block is free, merge them and set the pointer to point to
	  the merged block*/
	if(memory->previous != NULL && !memory->previous->allocated) {
		mem_merge(memory->previous, memory);
		memory = memory->previous;
	}
	
	/*If next block is free, merge them*/
	if(memory->next != NULL && !memory->next->allocated) {
		mem_merge(memory, memory->next);
	}
}

//kill process
void kill_process(pcbptr process) {
	//*Free the resources for user job* would normally send kill signal for running process here
	if(process->priority != 0) {
		mem_free(process->memory);
		rsrc_free(process);
	}
	process->status = TERMINATED;
}

//Suspends process
void suspend_process(pcbptr process) {
	process->status = SUSPENDED; 
}

int areEmptyQueues() {
	if(isEmptyQueue(mFcfs) && isEmptyQueue(level1) &&
		isEmptyQueue(level2) && isEmptyQueue(level3)){
		    return 1;
	}
	return 0;
}
//Restarts process
void restart_process(pcbptr process) {
	process->status = RUNNING;
	printf("\tProcess ID %d started.\n", process->pid);
	display_process_info(process);
}

void start_dispatcher() {
	do {
		mabptr allocatedMem = NULL; //Pointer to memory that will be allocated to a process.

		/*Unload pending processes from the input queue*/
		while (mInput.front != NULL && mInput.front->arrival_time <= mDispatcher_timer) {

			/*Remove process from input queue and add it to the user job queue if it is a user job.
			  Otherwise, add it to the real-time processes queue.*/
			pcbptr newProcess = dequeue(&mInput);
			/*If the process is larger than the total available memory, it is not admitted into the system*/
			if(newProcess->priority != 0) {
				/*If job requires more memory than the system has in total, display appropriate message and
				  do not admit the process.*/
				if (newProcess->memory_alloc > mTotalMem) {
					printf("\nERROR - Job memory request(%dMB) exceeds total memory(%dMB) - job deleted\n\n",
							newProcess->memory_alloc, mTotalMem);
				} else if (newProcess->num_printers > NUM_PRINTERS ||
							newProcess->num_scanners > NUM_SCANNERS ||
							newProcess->num_modems > NUM_MODEMS ||
							newProcess->num_cds > NUM_CDS) {
					printf("\nERROR - Job demands too many resources - job deleted\n\n");
				}
				else {
					enqueue(&mJobs, newProcess);
				}
			}
			else {
				/*If job requires more memory than the system has in total, display appropriate message and
				  do not admit the process.*/
				if (newProcess->memory_alloc > RESERVED_MEM) {
					printf("\nERROR - Real-time memory request(%dMB) exceeds reserved memory(%dMB) - job deleted\n\n",
							newProcess->memory_alloc, RESERVED_MEM);
				} else if (newProcess->num_printers > 0 ||
						newProcess->num_scanners > 0 ||
						newProcess->num_modems > 0 ||
						newProcess->num_cds > 0) {
					printf("\nERROR - Real-time job not allowed I/O resources - job deleted\n\n");
				}
				else
					enqueue(&mFcfs, newProcess);
			}
		}

		/*Unload pending processes from user jobs queue while the memory can be allocated to them.*/
		while(mJobs.front != NULL &&
				mem_check(mJobs.front->memory_alloc) &&
				 rsrc_chk(mJobs.front->num_printers, mJobs.front->num_scanners,
						  mJobs.front->num_modems, mJobs.front->num_cds)) {
			/*Allocate memory to process and place it in appropriate queue based on its priority level.*/
			allocatedMem = mem_alloc( mJobs.front->memory_alloc);
			pcbptr process = dequeue(&mJobs);
			process->memory = allocatedMem;
			rsrc_alloc(process, process->num_printers, process->num_scanners,
					process->num_modems, process->num_cds);
			placeInQueue(process);
		}

		/*If a process is running*/
		if (mActiveProcess != NULL) {
			mActiveProcess->remaining_cpu_time--;
			printf("Process ID %d -",mActiveProcess->pid);
			display_process_info(mActiveProcess);
			/*If process is done executing, terminate it and free its resources.*/
			if(mActiveProcess->remaining_cpu_time == 0) {
				kill_process(mActiveProcess);
				free_process_pointers(mActiveProcess);
				mActiveProcess = NULL;		//Set active process to NULL to indicate there is no currently running process
			} 
			else if(mActiveProcess->priority > 0 && !areEmptyQueues()) {
				/*If active process is not a real-time process and there are processes in other queues, 
				  then suspend the active process*/
				suspend_process(mActiveProcess);
				/*If the process's priority is < 3, reduce its priority (higher # = lower priority)
				  and send it to its appropriate queue. */
				if(mActiveProcess->priority < 3)
					mActiveProcess->priority++;
				placeInQueue(mActiveProcess);
				mActiveProcess = NULL;	//Set active process to NULL to indicate there is no currently running process
			}
		}	//End if
		if(mActiveProcess == NULL && !areEmptyQueues()) {
			/*Run the a process from the highest priority queue that isn't empty*/
			if (!isEmptyQueue(mFcfs))
				mActiveProcess = dequeue(&mFcfs);
			else if (!isEmptyQueue(level1))
				mActiveProcess = dequeue(&level1);
			else if (!isEmptyQueue(level2))
				mActiveProcess = dequeue(&level2);
			else if (!isEmptyQueue(level3))
				mActiveProcess = dequeue(&level3);

			if(mActiveProcess->priority == 0)
				mActiveProcess->memory = mReservedMem;

			/*If process is suspended, restart it*/
			if(mActiveProcess->status == SUSPENDED)
				restart_process(mActiveProcess);
			else
				start_process(mActiveProcess);
		} //End if		
		mDispatcher_timer += mQUANTUM;
	} while (!areEmptyQueues() || mActiveProcess != NULL ||		/*Loop continues until all queues are empty and there is no process running*/
				!isEmptyQueue(mJobs) || !isEmptyQueue(mInput));	//End while
}


int * readInfo(FILE *file) {
	const int SIZE = 8;
	int * array = malloc(sizeof(int) * SIZE);
	int index = 0;
	//Read line
	while(!feof(file) && index < SIZE) {
		char number[] = "\0\0\0\0";
		char digit = '\0';
		int i = 0;
		while (!feof(file) && isdigit(digit = fgetc(file) ) )
			number[i++] = digit;			
					
		if(strlen(number) > 0)
			array[index++] = atoi(number);
			
		if(digit == NEWLINE)
			break;
	}	
	/*Single newline encountered. Set array to NULL to indicate the read failed*/
	if(index < SIZE) {
		free(array);
		array = NULL;
	}
	
	return array;
}

//Returns a pointer to an empty PCB
pcbptr create_pcb() {
	pcbptr control_block = malloc(sizeof(pcb));
	control_block->pid = 0;
	control_block->arrival_time = 0;
	control_block->remaining_cpu_time = 0;
	control_block->next = NULL;
	
	return control_block;
}


//Initializes process block
void init_process(pcbptr process, int * processInfo) {
    process->pid=PID+1;
    PID++;
	process->arrival_time = processInfo[0];
	process->remaining_cpu_time = processInfo[2];
	process->priority = processInfo[1];
	process->num_printers=processInfo[4];
	process->num_cds=processInfo[7];
	process->memory_alloc=processInfo[3];
	process->num_scanners=processInfo[5];
	process->num_modems=processInfo[6];
}

//Initializes dispatcher
void init_dispatcher(FILE *file) {
	if(mReservedMem == NULL) {
		mReservedMem = mem_alloc(RESERVED_MEM);
		mReservedMem->allocated = 1;
	}

	mDispatcher_timer = 0;
	mActiveProcess = NULL;
	/*Initialize queues*/
	init_queue(&mInput);
	init_queue(&mJobs);
	init_queue(&mFcfs);
	init_queue(&level1);
	init_queue(&level2);
	init_queue(&level3);

	/*Populate input queue.*/
	while (!feof(file)) {
		int *processInfo = readInfo(file);	
		if (processInfo == NULL)
			continue;			
		pcbptr process = create_pcb();
		init_process(process, processInfo);		
		enqueue(&mInput, process);
	}
}


int main(int argc, char* argv[]){
    char * fileName = "process.txt";
	//If an argument (file name) is passed, then set the file name
	
	if(argc > 1)
		fileName = argv[1];
		
	FILE * file = fopen(fileName, "r");

	if (file == NULL) {
		printf("ERROR - Could not open file \"%s\"\n", fileName);
		exit(EXIT_FAILURE);
	}

	init_dispatcher(file);
	fclose(file);
	start_dispatcher(); //Run the dispatcher
	printf("\nAll jobs finished");
	
	return 0;
}

