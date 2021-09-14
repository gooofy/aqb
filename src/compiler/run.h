#ifndef HAVE_RUN_H
#define HAVE_RUN_H

#ifdef __amigaos__

#include <dos/dosextens.h>

typedef enum {RUN_stateRunning, RUN_stateStopped} RUN_state;

void      RUN_start (const char *binfn);

void      RUN_init (struct MsgPort *debugPort, struct FileHandle *output);

void      RUN_handleMessages(void);
RUN_state RUN_getState(void);

void      RUN_break (void);

#endif

#endif
