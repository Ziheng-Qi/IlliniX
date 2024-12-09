#include "syscall.h"
#include "string.h"
#include "stdlib.h"

void main() {
    int result;
    result = _pipe(0);

    if (result < 0)
    {
        _msgout("_pipe failed");
        _exit();
    }

    if (_fork() == 0)
    {   
        result = _devopen(1, "ser", 1);
    
        const char * const child_read = "child reads line:";
        const char * const child_write = "child writes line:";

        while(1){
            _write(1, child_write, strlen(child_write));


            // child write until enter is pressed
            char c = '\0';
            while(1){
                _read(1, &c, 1); // read from terminal
                _write(1, &c, 1); // write to terminal
                _write(0, &c, 1); // write to pipe
                if(c == '\r'){
                    // _write(0,"\n",1);
                    break;
                }
            }

            _write(1,"\n",1);
            _write(1, child_read, strlen(child_read));
            _ioctl(0, 8, NULL); // wait until buffer empty
            c = '\0';
            // child read until enter
            while(1){
                _read(0, &c, 1); // read from pipe
                if(c == '\0'){
                    continue;
                }
                _write(1, &c, 1); // write to terminal
                if(c == '\r'){ // read change line
                    break;
                }
            }

            _write(1,"\n",1);
        }

        _exit();
    }
    else
    {
        result = _devopen(1, "ser", 2);

        const char * const parent_read = "Parent reads line:";
        const char * const parent_write = "Parent writes line:";
        while(1){
            _write(1, parent_read, strlen(parent_read));

            // parent read until enter is pressed
            char c = '\0';
            while(1){
                _read(0, &c, 1); // read from pipe // non_blocking
                if(c == '\0'){
                    continue;
                }
                _write(1, &c, 1); // write to terminal
                if(c == '\r'){
                    break;
                }
            }

            _write(1,"\n",1);
            _write(1, parent_write, strlen(parent_write));

            // parent write until enter
            c = '\0';
            while(1){
                _read(1, &c, 1); // read from terminal
                _write(1, &c, 1); // write to terminal
                _write(0, &c, 1); // write to pipe
                if(c == '\r'){
                    // change line
                    // _write(0,"\n",1); 
                    break;
                }
            }
            _write(1,"\n",1); 
            _ioctl(0, 8, NULL); // wait until buffer empty
        }
        
        _exit();
    }
}