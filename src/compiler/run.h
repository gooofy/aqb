#ifndef HAVE_RUN_H
#define HAVE_RUN_H

#include <inttypes.h>

typedef enum {RUN_stateStopped, RUN_stateRunning} RUN_state;

void      RUN_handleEvent (uint16_t key);

RUN_state RUN_getState(void);

#ifdef __amigaos__

#include <dos/dosextens.h>

void      RUN_start (const char *binfn);
void      RUN_help (char *binfn, char *arg1);

void      RUN_init (struct MsgPort *debugPort);

uint16_t  RUN_handleMessages(void);

void      RUN_break (void);

#endif

#endif
