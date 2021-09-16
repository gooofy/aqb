#ifndef HAVE_RUN_H
#define HAVE_RUN_H

typedef enum {RUN_stateStopped, RUN_stateRunning} RUN_state;

RUN_state RUN_getState(void);

#ifdef __amigaos__

#include <dos/dosextens.h>

void      RUN_start (const char *binfn);

void      RUN_init (struct MsgPort *debugPort);

void      RUN_handleMessages(void);

void      RUN_break (void);

#endif

#endif
