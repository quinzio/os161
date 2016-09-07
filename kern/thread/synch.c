/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Synchronization primitives.
 * The specifications of the functions are in synch.h.
 */

#include <types.h>
#include <lib.h>
#include <spinlock.h>
#include <wchan.h>
#include <thread.h>
#include <current.h>
#include <synch.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *name, unsigned initial_count)
{
	struct semaphore *sem;

	sem = kmalloc(sizeof(*sem));
	if (sem == NULL) {
		return NULL;
	}

	sem->sem_name = kstrdup(name);
	if (sem->sem_name == NULL) {
		kfree(sem);
		return NULL;
	}

	sem->sem_wchan = wchan_create(sem->sem_name);
	if (sem->sem_wchan == NULL) {
		kfree(sem->sem_name);
		kfree(sem);
		return NULL;
	}

	spinlock_init(&sem->sem_lock);
	sem->sem_count = initial_count;

	return sem;
}

void
sem_destroy(struct semaphore *sem)
{
	KASSERT(sem != NULL);

	/* wchan_cleanup will assert if anyone's waiting on it */
	spinlock_cleanup(&sem->sem_lock);
	wchan_destroy(sem->sem_wchan);
	kfree(sem->sem_name);
	kfree(sem);
}

void
P(struct semaphore *sem)
{
	KASSERT(sem != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete the P without blocking.
	 */
	KASSERT(curthread->t_in_interrupt == false);

	/* Use the semaphore spinlock to protect the wchan as well. */
	spinlock_acquire(&sem->sem_lock);
	while (sem->sem_count == 0) {
		/*
		 *
		 * Note that we don't maintain strict FIFO ordering of
		 * threads going through the semaphore; that is, we
		 * might "get" it on the first try even if other
		 * threads are waiting. Apparently according to some
		 * textbooks semaphores must for some reason have
		 * strict ordering. Too bad. :-)
		 *
		 * Exercise: how would you implement strict FIFO
		 * ordering?
		 */
		wchan_sleep(sem->sem_wchan, &sem->sem_lock);
	}
	KASSERT(sem->sem_count > 0);
	sem->sem_count--;
	spinlock_release(&sem->sem_lock);
}

void
V(struct semaphore *sem)
{
	KASSERT(sem != NULL);

	spinlock_acquire(&sem->sem_lock);

	sem->sem_count++;
	KASSERT(sem->sem_count > 0);
	wchan_wakeone(sem->sem_wchan, &sem->sem_lock);

	spinlock_release(&sem->sem_lock);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name)
{
	struct lock *lock;

	lock = kmalloc(sizeof(*lock));
	if (lock == NULL) {
		return NULL;
	}

	lock->lk_name = kstrdup(name);
	if (lock->lk_name == NULL) {
		kfree(lock);
		return NULL;
	}

	lock->lock_wchan = wchan_create(lock->lk_name);
	lock->is_locked = false; // not locked
	lock->stack_addr = NULL;
	spinlock_init(&lock->lock_spinlock);

	return lock;
}

void
lock_destroy(struct lock *lock)
{
	KASSERT(lock != NULL);

	// add stuff here as needed
	spinlock_cleanup(&lock->lock_spinlock);
	wchan_destroy(lock->lock_wchan);

	kfree(lock->lk_name);
	kfree(lock);
}

void
lock_acquire(struct lock *lock)
{
	spinlock_acquire(&lock->lock_spinlock);
	while (lock->is_locked == true)
	{
		wchan_sleep(lock->lock_wchan, &lock->lock_spinlock);
	}
	lock->stack_addr =  curthread->t_stack;
	lock->is_locked = true;
	//kprintf("lock_acquire\n");
	spinlock_release(&lock->lock_spinlock);
}

void
lock_release(struct lock *lock)
{
	spinlock_acquire(&lock->lock_spinlock);
	KASSERT(lock->is_locked == true); // don't change order
	KASSERT(lock->stack_addr == curthread->t_stack);
	lock->is_locked = false;
	lock->stack_addr = NULL;
	wchan_wakeone(lock->lock_wchan, &lock->lock_spinlock);
	//kprintf("lock_release\n");
	spinlock_release(&lock->lock_spinlock);

}

bool
lock_do_i_hold(struct lock *lock)
{
	// Write this
	if(lock->is_locked)
		return lock->stack_addr == curthread->t_stack;
	return false; // dummy until code gets written
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
	struct cv *cv;

	cv = kmalloc(sizeof(*cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->cv_name = kstrdup(name);
	if (cv->cv_name==NULL) {
		kfree(cv);
		return NULL;
	}

	spinlock_init(&cv->spinlock);
	cv->wchan = wchan_create(cv->cv_name);
	// add stuff here as needed

	return cv;
}

void
cv_destroy(struct cv *cv)
{
	KASSERT(cv != NULL);
	// add stuff here as needed
	spinlock_acquire(&cv->spinlock);
	wchan_wakeall(cv->wchan, &cv->spinlock);
	wchan_destroy(cv->wchan);
	spinlock_release(&cv->spinlock);
	spinlock_cleanup(&cv->spinlock);

	kfree(cv->cv_name);
	kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	//static int count=0;
	// Write this
	spinlock_acquire(&cv->spinlock);
	lock_release(lock);
	//kprintf("cvwait %d %d\n", spinlock_do_i_hold(&lock->lock_spinlock), count++);
	wchan_sleep(cv->wchan, &cv->spinlock);
	spinlock_release(&cv->spinlock);
	lock_acquire(lock);
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	// Write this
	spinlock_acquire(&cv->spinlock);
	KASSERT(lock_do_i_hold(lock) == 1);
	wchan_wakeone(cv->wchan, &cv->spinlock);
	spinlock_release(&cv->spinlock);
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	// Write this
	spinlock_acquire(&cv->spinlock);
	KASSERT(lock_do_i_hold(lock) == 1);
	wchan_wakeall(cv->wchan, &cv->spinlock);
	spinlock_release(&cv->spinlock);
}

////////////////////////////////////////////////////////////
//
// RW


struct rwlock * rwlock_create(const char *name)
{
	struct rwlock *rwl;
	rwl = kmalloc(sizeof(*rwl));
	if (rwl == NULL) {
		return NULL;
	}

	rwl->rwlock_name = kstrdup(name);
	if (rwl->rwlock_name == NULL) {
		kfree(rwl);
		return NULL;
	}

	rwl->rd_lk = lock_create(rwl->rwlock_name);
	if (rwl->rd_lk == NULL) {
		kfree(rwl->rwlock_name);
		kfree(rwl);
		return NULL;
	}

	rwl->wr_lk = lock_create(rwl->rwlock_name);
	if (rwl->wr_lk == NULL) {
		lock_destroy(rwl->rd_lk);
		kfree(rwl->rwlock_name);
		kfree(rwl);
		return NULL;
	}

	rwl->reader_count = 0; // no readers

	return rwl;

}

void rwlock_destroy(struct rwlock *rwl)
{
	KASSERT(rwl->reader_count == 0);
	lock_destroy(rwl->rd_lk);
	lock_destroy(rwl->wr_lk);
	kfree(rwl->rwlock_name);
	kfree(rwl);
}

void rwlock_acquire_read(struct rwlock *rwl)
{
	lock_acquire(rwl->rd_lk);
	if (rwl->reader_count == 0)
		lock_acquire(rwl->wr_lk);
	rwl->reader_count++;
	lock_release(rwl->rd_lk);
}

void rwlock_release_read(struct rwlock *rwl)
{
	lock_acquire(rwl->rd_lk);
	if (rwl->reader_count == 1)
		lock_release(rwl->wr_lk);
	rwl->reader_count--;
	lock_release(rwl->rd_lk);
}

void rwlock_acquire_write(struct rwlock *rwl)
{
	lock_acquire(rwl->wr_lk);
}

void rwlock_release_write(struct rwlock *rwl)
{
	lock_release(rwl->wr_lk);
}


