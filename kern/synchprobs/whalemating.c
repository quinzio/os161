/*
 * Copyright (c) 2001, 2002, 2009
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
 * Driver code is in kern/tests/synchprobs.c We will
 * replace that file. This file is yours to modify as you see fit.
 *
 * You should implement your solution to the whalemating problem below.
 */

#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>

struct cv *cv_male;
struct cv *cv_female;
struct cv *cv_matchmaker;
struct lock *lk;
struct population_t {
  int males;
  int females;
  int matchmakers; 
} population;

/*
 * Called by the driver during initialization.
 */

void whalemating_init() {
  cv_male =  cv_create("male cv");
  cv_female = cv_create("female cv");
  cv_matchmaker = cv_create("matchmaker cv");
  lk = lock_create("lock");
  lock_acquire(lk);
  population.males = 0;
  population.females = 0;
  population.matchmakers = 0;
  lock_release(lk);
  return;
}

/*
 * Called by the driver during teardown.
 */

void
whalemating_cleanup() {
  cv_destroy(cv_male);
  cv_destroy(cv_female);
  cv_destroy(cv_matchmaker);
  lock_destroy(lk);
  return;
}

void
male(uint32_t index)
{
  (void)index;
  /*
   * Implement this function by calling male_start and male_end when
   * appropriate.
   */
  male_start(index);

  lock_acquire(lk);
  population.males++;
  if(population.females > 0 && population.matchmakers > 0) {
    cv_signal(cv_female, lk);
    cv_signal(cv_matchmaker, lk);
  }
  else {
    cv_wait(cv_male, lk);	
  }

  population.males--;
  lock_release(lk);

  male_end(index);
  return;
}

void
female(uint32_t index)
{
  (void)index;
  /*
   * Implement this function by calling female_start and female_end when
   * appropriate.
   */
  female_start(index);

  lock_acquire(lk);
  population.females++;
  if(population.males > 0 && population.matchmakers > 0) {
    cv_signal(cv_male, lk);
    cv_signal(cv_matchmaker, lk);
  }
  else {
    cv_wait(cv_female, lk);	
  }

  population.females--;
  lock_release(lk);

  female_end(index);
  return;
}

void
matchmaker(uint32_t index)
{
  (void)index;
  /*
   * Implement this function by calling matchmaker_start and matchmaker_end
   * when appropriate.
   */
  matchmaker_start(index);

  lock_acquire(lk);
  population.matchmakers++;
  if(population.females > 0 && population.males > 0) {
    cv_signal(cv_female, lk);
    cv_signal(cv_male, lk);
  }
  else {
    cv_wait(cv_matchmaker, lk);	
  }

  population.matchmakers--;
  lock_release(lk);

  matchmaker_end(index);
  return;
}
