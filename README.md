# Hypothetical Operating System Testbed (HOST) Dispatcher

This C program simulates an OS dispatcher for the Hypothetical Operating System Testbed (HOST). HOST is a multiprogramming system with a four-level priority process dispatcher operating within the constraints of finite available resources.

## Overview

The dispatcher operates at four priority levels:
1. Real-Time processes: These must be run immediately on a First Come First Served (FCFS) basis, preempting any other processes running with lower priority.
2. Normal user processes: These are run on a three-level feedback dispatcher, with a basic timing quantum of 1 second.

## Priority Dispatcher

The dispatcher maintains two submission queues - Real-Time and User priority - fed from the job dispatch list. It examines the dispatch list at every tick and transfers jobs that have arrived to the appropriate submission queue. Real-Time jobs are run to completion, preempting any other jobs currently running.

## Resource Constraints

HOST has the following resources:
- 2 Printers
- 1 Scanner
- 1 Modem
- 2 CD Drives
- 1024 Mbyte Memory available for processes

## Processes Lifecycle

The life-cycle of a process on HOST involves submission to the dispatcher, becoming ready-to-run when all required resources are available, execution, and termination.

## Criteria for Dispatcher

1. Real-time processes are prioritized and executed immediately upon arrival.
2. User processes are scheduled based on priority and processor time.
3. Dispatcher displays job parameters when a process starts execution.
4. Processes may be lowered in priority if necessary and re-queued.
5. Highest priority pending processes are started or resumed when no real-time processes are pending.
6. Terminated processes free up resources for other processes.
7. If a User process does not completely executed in its priority level, the user process
should be lowered (if possible) and it is re-queued on the appropriate priority queue.
8. If a User process is suspended by a higher user process, then its priority level should be
lowered (if possible) and it is re-queued on the appropriate priority queue.

## Dispatch List

The Dispatch List is a text/csv file containing process information, including arrival time, priority, processor time, memory block size, and requested resources.


---

**Project by [Your Name]**

*Date: [Date]*
