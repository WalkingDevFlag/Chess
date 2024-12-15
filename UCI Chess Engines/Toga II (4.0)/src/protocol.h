
// protocol.h
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#ifndef PROTOCOL_H
#define PROTOCOL_H

// includes

#include "util.h"

// variables
#ifdef _WIN32
extern CRITICAL_SECTION CriticalSection; 
#else
extern  pthread_mutex_t CriticalSection;
#endif

// functions

extern void loop  ();
extern void event ();
extern void book_parameter();

extern void get   (char string[], int size);
extern void send  (const char format[], ...);

#endif // !defined PROTOCOL_H

// end of protocol.h

