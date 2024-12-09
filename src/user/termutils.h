#ifndef TERMUTILS_H
#define TERMUTILS_H


#include <stddef.h>
#include "syscall.h"
#include "termio.h"
extern int cat(char *filename);
extern int ls();
extern int edit(char *filename);
#define assert(c)                                                              \
  do                                                                           \
  {                                                                            \
    if (!(c))                                                                  \
    {                                                                          \
      char message[100];                                                       \
      snprintf(message, 100, "Assertion failed at %s:%d", __FILE__, __LINE__); \
      _msgout(message);                                                        \
      _exit();                                                                 \
    }                                                                          \
  } while (0)

#endif // TERMUTILS_H