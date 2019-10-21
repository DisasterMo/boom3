/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#include "rthreads/rthreads.h"
#include "features/features_cpu.h"
#include "retro_timers.h"

#include "sys/platform.h"
#include "framework/Common.h"

#include "sys/sys_public.h"

static slock_t	*mutex[MAX_CRITICAL_SECTIONS] = { };
static scond_t		*cond[MAX_TRIGGER_EVENTS] = { };
static bool			signaled[MAX_TRIGGER_EVENTS] = { };
static bool			waiting[MAX_TRIGGER_EVENTS] = { };

static xthreadInfo	*thread[MAX_THREADS] = { };
static size_t		thread_count = 0;

/*
==============
Sys_Sleep
==============
*/
void Sys_Sleep(int msec) {
	retro_sleep(msec);
}

/*
================
Sys_Milliseconds
================
*/
unsigned int Sys_Milliseconds() {
	static uint64_t	base;

	uint64_t time = cpu_features_get_time_usec() / 1000;
	
    if (!base) {
		base = time;
    }

    unsigned int curtime = (unsigned int)(time - base);

    return curtime;
}

/*
==================
Sys_InitThreads
==================
*/
void Sys_InitThreads() {
	// critical sections
	for (int i = 0; i < MAX_CRITICAL_SECTIONS; i++) {
		mutex[i] = slock_new();

		if (!mutex[i]) {
			Sys_Printf("ERROR: SDL_CreateMutex failed\n");
			return;
		}
	}

	// events
	for (int i = 0; i < MAX_TRIGGER_EVENTS; i++) {
		cond[i] = scond_new();

		if (!cond[i]) {
			Sys_Printf("ERROR: SDL_CreateCond failed\n");
			return;
		}

		signaled[i] = false;
		waiting[i] = false;
	}

	// threads
	for (int i = 0; i < MAX_THREADS; i++)
		thread[i] = NULL;

	thread_count = 0;
}

/*
==================
Sys_ShutdownThreads
==================
*/
void Sys_ShutdownThreads() {
	// threads
	for (int i = 0; i < MAX_THREADS; i++) {
		if (!thread[i])
			continue;

		Sys_Printf("WARNING: Thread '%s' still running\n", thread[i]->name);
		thread[i] = NULL;
	}

	// events
	for (int i = 0; i < MAX_TRIGGER_EVENTS; i++) {
		scond_free(cond[i]);
		cond[i] = NULL;
		signaled[i] = false;
		waiting[i] = false;
	}

	// critical sections
	for (int i = 0; i < MAX_CRITICAL_SECTIONS; i++) {
		slock_free(mutex[i]);
		mutex[i] = NULL;
	}
}

/*
==================
Sys_EnterCriticalSection
==================
*/
void Sys_EnterCriticalSection(int index) {
	assert(index >= 0 && index < MAX_CRITICAL_SECTIONS);

	slock_lock(mutex[index]);
}

/*
==================
Sys_LeaveCriticalSection
==================
*/
void Sys_LeaveCriticalSection(int index) {
	assert(index >= 0 && index < MAX_CRITICAL_SECTIONS);

	slock_unlock(mutex[index]);
}

/*
======================================================
wait and trigger events
we use a single lock to manipulate the conditions, CRITICAL_SECTION_SYS

the semantics match the win32 version. signals raised while no one is waiting stay raised until a wait happens (which then does a simple pass-through)

NOTE: we use the same mutex for all the events. I don't think this would become much of a problem
cond_wait unlocks atomically with setting the wait condition, and locks it back before exiting the function
the potential for time wasting lock waits is very low
======================================================
*/

/*
==================
Sys_WaitForEvent
==================
*/
void Sys_WaitForEvent(int index) {
	assert(index >= 0 && index < MAX_TRIGGER_EVENTS);

	Sys_EnterCriticalSection(CRITICAL_SECTION_SYS);

	assert(!waiting[index]);	// WaitForEvent from multiple threads? that wouldn't be good
	if (signaled[index]) {
		// emulate windows behaviour: signal has been raised already. clear and keep going
		signaled[index] = false;
	} else {
		waiting[index] = true;
		scond_wait(cond[index], mutex[CRITICAL_SECTION_SYS]);
		waiting[index] = false;
	}

	Sys_LeaveCriticalSection(CRITICAL_SECTION_SYS);
}

/*
==================
Sys_TriggerEvent
==================
*/
void Sys_TriggerEvent(int index) {
	assert(index >= 0 && index < MAX_TRIGGER_EVENTS);

	Sys_EnterCriticalSection(CRITICAL_SECTION_SYS);

	if (waiting[index]) {
		scond_signal(cond[index]);
	} else {
		// emulate windows behaviour: if no thread is waiting, leave the signal on so next wait keeps going
		signaled[index] = true;
	}

	Sys_LeaveCriticalSection(CRITICAL_SECTION_SYS);
}

/*
==================
Sys_CreateThread
==================
*/
int thid = 1;
void Sys_CreateThread(xthread_t function, void *parms, xthreadInfo& info, const char *name) {
	Sys_EnterCriticalSection();

	sthread_t *t = sthread_create(function, parms);

	if (!t) {
		common->Error("ERROR: SDL_thread for '%s' failed\n", name);
		Sys_LeaveCriticalSection();
		return;
	}

	info.name = name;
	info.threadHandle = t;
	info.threadId = sthread_get_thread_id(t);

	if (thread_count < MAX_THREADS)
		thread[thread_count++] = &info;
	else
		common->DPrintf("WARNING: MAX_THREADS reached\n");

	Sys_LeaveCriticalSection();
}

/*
==================
Sys_DestroyThread
==================
*/
void Sys_DestroyThread(xthreadInfo& info) {
	assert(info.threadHandle);

	sthread_join(info.threadHandle);
	
	info.name = NULL;
	info.threadHandle = NULL;
	info.threadId = 0;

	Sys_EnterCriticalSection();

	for (int i = 0; i < thread_count; i++) {
		if (&info == thread[i]) {
			thread[i] = NULL;

			int j;
			for (j = i + 1; j < thread_count; j++)
				thread[j - 1] = thread[j];

			thread[j - 1] = NULL;
			thread_count--;

			break;
		}
	}

	Sys_LeaveCriticalSection( );
}

/*
==================
Sys_GetThreadName
find the name of the calling thread
==================
*/
const char *Sys_GetThreadName(int *index) {
	const char *name;

	Sys_EnterCriticalSection();

	unsigned int id = sthread_get_current_thread_id();

	for (int i = 0; i < thread_count; i++) {
		if (id == thread[i]->threadId) {
			if (index)
				*index = i;

			name = thread[i]->name;

			Sys_LeaveCriticalSection();

			return name;
		}
	}

	if (index)
		*index = -1;

	Sys_LeaveCriticalSection();

	return "main";
}
