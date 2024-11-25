## Debug log:

### MP3_CP1

#### File System

For file system, there was a bug which the reading of `inode_t` might causing stack overflow when doing rubric test, the solution was malloc the the `inode_t` will solve the problem.

There are cases where the file system might have issue reading the last datablock, turns out the offset was calculated wrong.

Was having trouble running the file system along with `shell_main.c` turns out some `ioctl` of file system should pass the results as an argument.

#### Virtio Block Device Driver

1. Pointers were assigned the wrong values
   Files related: `vioblk.c`

**Description**

In vioblk_attach, `dev->blkbuf` was assigned `(void *)(&dev+sizeof(struct vioblk_device))` the operation on pointer was done before casting it to a void pointer.
So the result is actually dev's address + `sizeof(struct vioblk_device)*sizeof(struct vioblk_device)` because dev has a type of `struct vioblk_device *`

**Fix**

From:

```C
dev->blkbuf = (void *)(&(dev))+sizeof(struct vioblk_device);
```

To:

```C
`dev->blkbuf = (void *)(dev)+sizeof(struct vioblk_device);
```

2. Didn't set write flag for data buffer and status buffer descriptor
   Files related: `vioblk.c`

**Description**

In vioblk_attach, in the flag of descriptor describing the status byte (`dev->vq.desc[3].flag`), the bit `VIRTQ_DESC_F_WRITE` is not set so the device cannot write to this status byte.

In vioblk_io_request, in the flag of descriptor pointing to the data buffer (`dev->vq.desc[2].flag`), this bit is also not set when writing to buffer (reading from blk device).

**Fix**

In vioblk_attach:

From

```C
desc_tab[2].flags = 0;
```

To:

```C
desc_tab[2].flags |= VIRTQ_DESC_F_WRITE;
```

In vioblk_io_request:

add

```C
intr_disable();
dev->vq.avail.idx ++;
if(op_type == VIRTIO_BLK_T_IN){
    dev->vq.desc[2].flags |= VIRTQ_DESC_F_WRITE; // the data buffer is device-writable
}else{
    dev->vq.desc[2].flags &= ~VIRTQ_DESC_F_WRITE; // the data buffer is not device-writable in a write operation
}

// notify and condition_wait

intr_enable();

```

3. Didn't update `dev->bufblkno`
   Files related: `vioblk.c`

**Description**

In vioblk_read, we need to update the block number in the device struct buffer after a read is performed

**Fix**

add

```C
// memcpy from dev->blkbuf to buf (from function paramter)
dev->bufblkno = blkn_no;
```

#### Elf Loader

In `elf_load`, we cannot use `Elf_hdr` pointer, which may cause warning. We also notice we should check `p_type` first before checking valid load section address.
They are not parallel conditions.

### MP3_CP2

#### November 24, 2024

Encountered a bug which while invoking `thread_exit` within `thread.c` after `process_exit` called the function in `process.c`, the condition broadcast got a wrong condition variable, the condition waitlist is completely unreachable and a LOAD ACCESS FAULT occured when trying to access the condition variable.

Solution:

It turns out the `tp` was incorrect at `_trap_entry_from_umode`
