/*
 * All the contents of this file are overwritten during automated
 * testing. Please consider this before changing anything in this file.
 */

#include <types.h>
#include <lib.h>
#include <clock.h>
#include <thread.h>
#include <synch.h>
#include <test.h>
#include <kern/test161.h>
#include <spinlock.h>

#define N_WRITETHREADS 2
#define N_READTHREADS 10

void wthread(void * unused, unsigned long num);
void rthread(void * unused, unsigned long num);

/*
 * Use these stubs to test your reader-writer locks.
 */
static struct rwlock *testrwl;
static struct semaphore * endsem;

int rwtest(int nargs, char **args) {
	(void) nargs;
	(void) args;
	int result;
	int i;

	// create semaphore to wait for threads to end
	endsem = sem_create("threads_finish", 0);
	testrwl = rwlock_create("readwrite test");

	// create writer thread
	i = 0;
	for (i = 0; i < N_WRITETHREADS; i++) {
		result = thread_fork("writer", NULL, wthread, NULL, i);
		if (result) {
			panic("lt1: thread_fork failed: %s\n", strerror(result));
		}
		kprintf("writer thread created\n");
	}
	for (i = 0; i < N_READTHREADS; i++) {
		result = thread_fork("reader", NULL, rthread, NULL, i);
		if (result) {
			panic("lt1: thread_fork failed: %s\n", strerror(result));
		}
		kprintf("reader thread created\n");
	}

	//wait for thread to finish
	for (i = 0; i < N_READTHREADS + N_WRITETHREADS; i++) {
		P(endsem);
	}
	rwlock_destroy(testrwl);
	kprintf("rwtest end\n");

	//kprintf_n("rwt1 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt1");

	return 0;
}

int rwtest2(int nargs, char **args) {
	(void) nargs;
	(void) args;

	kprintf_n("rwt2 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt2");

	return 0;
}

int rwtest3(int nargs, char **args) {
	(void) nargs;
	(void) args;

	kprintf_n("rwt3 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt3");

	return 0;
}

int rwtest4(int nargs, char **args) {
	(void) nargs;
	(void) args;

	kprintf_n("rwt4 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt4");

	return 0;
}

int rwtest5(int nargs, char **args) {
	(void) nargs;
	(void) args;

	kprintf_n("rwt5 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt5");

	return 0;
}

void wthread(void * unused, unsigned long num) {
	// to prevent compiler error
	(void) unused;

	int i;
	for (i = 0; i < 100; i++) {
		rwlock_acquire_write(testrwl);
		kprintf("Write thread %ld %d\n", num, i);
		rwlock_release_write(testrwl);
	}

	V(endsem);
	return;
}

void rthread(void * unused, unsigned long num) {
	// to prevent compiler error
	(void) unused;

	int i;
	for (i = 0; i < 10; i++) {
		rwlock_acquire_read(testrwl);
		kprintf("Read thread %ld %d\n", num, i);
		rwlock_release_read(testrwl);
	}
	V(endsem);
	return;
}

