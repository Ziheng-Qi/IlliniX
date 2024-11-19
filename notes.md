One way to solve the problem: At the base of stack, there's an anchor pointing to the thread struct (which we can use to access the stack of that thread)

Another way: add the thread structs in a designated page

trap: stval