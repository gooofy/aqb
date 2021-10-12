#ifndef HAVE_RUN_H
#define HAVE_RUN_H

typedef enum {RUN_stateStopped, RUN_stateRunning} RUN_state;

RUN_state RUN_getState(void);

#ifdef __amigaos__

#include <dos/dosextens.h>

void      RUN_start (const char *binfn);
void      RUN_help (char *binfn, char *arg1);

void      RUN_init (struct MsgPort *debugPort);

uint16_t  RUN_handleMessages(void);

ULONG     RUN_getERRCode(void);

void      RUN_break (void);

#endif

#endif
