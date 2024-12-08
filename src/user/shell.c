#include "syscall.h"
#include "string.h"
#include "stdlib.h"
#include "termio.h"
#include "error.h"
#include "termutils.h"

int main()
{

  char cmdbuf[50];
  int tid;
  int result;

  result = _devopen(0, "ser", 1);
  if (result < 0)
  {
    _msgout("Failed to open ser1");
    _exit();
  }
  wfent();
  puts("Welcome to the ECE391 shell!");
  for (;;)
  {
    printf("ece391> ");
    if (getsn(cmdbuf, sizeof(cmdbuf)) == NULL)
    {
      puts("Failed to read input");
      _exit();
    }

    if (cmdbuf[0] == '\0')
      continue; // Do nothing for empty input

    // continually parse the input:
    char *cmd = strtok(cmdbuf, " ");
    char *args[10]; // Array to hold arguments
    int arg_count = 0;

    while (cmd != NULL && arg_count < 10)
    {
      args[arg_count++] = cmd;
      cmd = strtok(NULL, " ");
    }

    if (strcmp("exit", args[0]) == 0)
      _exit(); // Exit shell
    // Otherwise, perform the operations in cmdbuf

    if (strcmp(args[0], "cat") == 0)
    {
      result = cat(args[1]);
      if (result < 0)
        continue;
    }
    else if (strcmp(args[0], "edit") == 0)
    {
      result = edit(args[1]);
      if (result < 0)
        continue;
    }
    else
    {
      result = _fsopen(1, args[0]);
      if (result < 0)
      { // Print error code
        if (result == -ENOENT)
          printf("%s: File not found\n", args[0]);
        else
          printf("%s: Error %d\n", args[0], -result);
        continue; // Continue the infinite loop for next cmd
      }

      tid = _fork();

      if (tid < 0)
      {
        puts("_fork() failed");
        _exit();
      }
      // Child execs program
      if (tid == 0)
      {

        _msgout("execute");
        _exec(1);

      } // 0 is child process. This process runs cmd
      // Parent waits for child to finish
      _close(1);

      if (tid)
      {
        _msgout("wait for thread: ");
        char str[10];
        itoa(tid, str, 10);
        _msgout(str);
        result = _wait(tid);
        if (result < 0)
        {
          puts("_wait() failed");
          _exit();
        }
      }
    }
  }
}
