# ECE391 Group 3

#### Greetings

Hello and welcome to the group repo for ECE 391 Fall 2024 group 3. Here are detailed instructions of how to run tests regarding this final MP:



1. Choose different tests in `src/kern/main.c` and modify 

```c
#define INIT_PROC "*user program that you want*"
```

2. Get the initial `kfs.raw` based on our user programs and text files for testing locks and reference counting by running bash script

```bash
sh get_init_fsimg.sh # Get user and utils cleaned and recompiled
```

3. Run the kernel:

```bash
make run-kernel
```



#### User Programs that we have:

Besides the baseline user programs(like `init_fib_rule30` or such), we also provide `refcnt` for testing reference count under child and parent and `lock_test` for testing concurrency issue prevention accordingly to the tests in `MP3_CP3`.

#### Credits:

This repo is constructed by Rick Xu (rickxu2), Ziheng Qi (zihengq2), and Ziyi Wang (zw67). Thank you all, no matter you are a CA, TA, or a student struggling in this amazing class, hard work will eventually pay off.