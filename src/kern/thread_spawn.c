// This thread_spawn function should replace youre existing thread_spawn function in thread.c

int thread_spawn(const char * name, void (*start)(void *), void * arg) {
    struct thread_stack_anchor * stack_anchor;
    void * stack_page;
    struct thread * child;
    int saved_intr_state;
    int tid;

    trace("%s(name=\"%s\") in %s", __func__, name, CURTHR->name);

    // Find a free thread slot.

    tid = 0;
    while (++tid < NTHR)
        if (thrtab[tid] == NULL)
            break;
    
    if (tid == NTHR)
        panic("Too many threads");
    
    // Allocate a struct thread and a stack

    child = kmalloc(sizeof(struct thread));

    stack_page = memory_alloc_page();
    stack_anchor = stack_page + PAGE_SIZE;
    stack_anchor -= 1;
    stack_anchor->thread = child;
    stack_anchor->reserved = 0;


    thrtab[tid] = child;

    child->id = tid;
    child->name = name;
    child->parent = CURTHR;
    child->proc = CURTHR->proc;
    child->stack_base = stack_anchor;
    child->stack_size = child->stack_base - stack_page;
    set_thread_state(child, THREAD_READY);

    saved_intr_state = intr_disable();
    tlinsert(&ready_list, child);
    intr_restore(saved_intr_state);

    _thread_setup(child, child->stack_base, start, arg);
    
    return tid;
}