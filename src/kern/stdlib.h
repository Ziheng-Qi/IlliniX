#ifndef STDLIB_H
#define STDLIB_H

#include <stddef.h>

/* Function prototypes for standard library functions */
int atoi(const char *str);
char *itoa(int num, char *str, int base);
char *strtok(char *str, const char *delim);

#endif /* STDLIB_H */