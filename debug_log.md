## Debug log:

For file system, there was a bug which the reading of `inode_t` might causing stack overflow when doing rubric test, the solution was malloc the the `inode_t` will solve the problem.

There are cases where the file system might have issue reading the last datablock, turns out the offset was calculated wrong. 

Was having trouble running the file system along with `shell_main.c` turns out some `ioctl` of file system should pass the results as an argument. 

End of File system debug log.



