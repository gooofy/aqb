#ifndef HAVE_RUN_H
#define HAVE_RUN_H

#ifdef __amigaos__

#include <dos/dosextens.h>

void RUN_start (const char *binfn);

void RUN_init (int termSignalBit, struct FileHandle *output);
#endif

#endif
