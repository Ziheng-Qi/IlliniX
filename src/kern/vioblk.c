//           vioblk.c - VirtIO serial port (console)
//          

#include "virtio.h"
#include "intr.h"
#include "halt.h"
#include "heap.h"
#include "io.h"
#include "device.h"
#include "error.h"
#include "string.h"
#include "thread.h"
#include "lock.h"

struct lock vblk_lk;

#define min(a,b) (a < b ? a : b)

//           COMPILE-TIME PARAMETERS
//          

#define VIOBLK_IRQ_PRIO 1

//           INTERNAL CONSTANT DEFINITIONS
//          

//           VirtIO block device feature bits (number, *not* mask)

#define VIRTIO_BLK_F_SIZE_MAX       1
#define VIRTIO_BLK_F_SEG_MAX        2
#define VIRTIO_BLK_F_GEOMETRY       4
#define VIRTIO_BLK_F_RO             5
#define VIRTIO_BLK_F_BLK_SIZE       6
#define VIRTIO_BLK_F_FLUSH          9
#define VIRTIO_BLK_F_TOPOLOGY       10
#define VIRTIO_BLK_F_CONFIG_WCE     11
#define VIRTIO_BLK_F_MQ             12
#define VIRTIO_BLK_F_DISCARD        13
#define VIRTIO_BLK_F_WRITE_ZEROES   14

//           INTERNAL TYPE DEFINITIONS
//          

//           All VirtIO block device requests consist of a request header, defined below,
//           followed by data, followed by a status byte. The header is device-read-only,
//           the data may be device-read-only or device-written (depending on request
//           type), and the status byte is device-written.

struct vioblk_request_header {
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;
};

//           Request type (for vioblk_request_header)

#define VIRTIO_BLK_T_IN             0
#define VIRTIO_BLK_T_OUT            1

//           Status byte values

#define VIRTIO_BLK_S_OK         0
#define VIRTIO_BLK_S_IOERR      1
#define VIRTIO_BLK_S_UNSUPP     2

// Queue size that we use
#define VIOBLK_Q_SIZE 1


//           Main device structure.
//          
//           FIXME You may modify this structure in any way you want. It is given as a
//           hint to help you, but you may have your own (better!) way of doing things.

struct vioblk_device {
    volatile struct virtio_mmio_regs * regs;
    struct io_intf io_intf;
    uint16_t instno;
    uint16_t irqno;
    int8_t opened;
    int8_t readonly;

    //           optimal block size
    uint32_t blksz;
    //           current position
    uint64_t pos;
    //           sizeo of device in bytes
    uint64_t size;
    //           size of device in blksz blocks
    uint64_t blkcnt;

    struct {
        //           signaled from ISR
        struct condition used_updated;

        //           We use a simple scheme of one transaction at a time.

        union {
            struct virtq_avail avail;
            char _avail_filler[VIRTQ_AVAIL_SIZE(VIOBLK_Q_SIZE)];
        };

        union {
            volatile struct virtq_used used;
            char _used_filler[VIRTQ_USED_SIZE(VIOBLK_Q_SIZE)];
        };

        //           The first descriptor is an indirect descriptor and is the one used in
        //           the avail and used rings. The second descriptor points to the header,
        //           the third points to the data, and the fourth to the status byte.

        struct virtq_desc desc[4];
        struct vioblk_request_header req_header;
        uint8_t req_status;
    } vq;

    //           Block currently in block buffer
    uint64_t bufblkno;
    //           Block buffer
    char * blkbuf;
};

#define VIOBLK_ATTEMPT_MAX 10
#define VIOBLK_SECTOR_SIZE 512 // this is the smallest unit of size used by VIRTIO, 512 Bytes

#define VIOBLK_DESC_INDIRECT_ID 0
#define VIOBLK_DESC_INDIRECT_TAB_OFFSET 1 // offset to the descriptor table pointed by the interrupt descriptor
#define VIOBLK_DESC_HEADER_ID 0
#define VIOBLK_DESC_DATA_ID 1
#define VIOBLK_DESC_STATUS_ID 2


//           INTERNAL FUNCTION DECLARATIONS
//          

static int vioblk_open(struct io_intf ** ioptr, void * aux);

static void vioblk_close(struct io_intf * io);

static long vioblk_read (
    struct io_intf * restrict io,
    void * restrict buf,
    unsigned long bufsz);

static long vioblk_write (
    struct io_intf * restrict io,
    const void * restrict buf,
    unsigned long n);

static int vioblk_ioctl (
    struct io_intf * restrict io, int cmd, void * restrict arg);

static void vioblk_isr(int irqno, void * aux);

//           IOCTLs

static int vioblk_getlen(const struct vioblk_device * dev, uint64_t * lenptr);
static int vioblk_getpos(const struct vioblk_device * dev, uint64_t * posptr);
static int vioblk_setpos(struct vioblk_device * dev, const uint64_t * posptr);
static int vioblk_getblksz (
    const struct vioblk_device * dev, uint32_t * blkszptr);

//           EXPORTED FUNCTION DEFINITIONS
//          

//           Attaches a VirtIO block device. Declared and called directly from virtio.c.

static const struct io_ops vio_ops = {
    .close = vioblk_close,
    .read = vioblk_read,
    .write = vioblk_write,
    .ctl = vioblk_ioctl,
};

/**
 * @brief 
 * This attaches a virtio block device with the provided MMIO register and register its interrupt to the irqno provided
 * @param regs the address of the MMIO register of the virtio block device
 * @param irqno the interrupt request number that you want this block device to be attached to
 * @return no return
 */
void vioblk_attach(volatile struct virtio_mmio_regs * regs, int irqno) {
    //           FIXME add additional declarations here if needed

    // this has to be static because io_intf in main_shell.c is not initilaized
    // this is similar to uart_ops in uart.c


    virtio_featset_t enabled_features, wanted_features, needed_features;
    struct vioblk_device * dev;
    uint_fast32_t blksz;
    int result;

    assert (regs->device_id == VIRTIO_ID_BLOCK);

    //           Signal device that we found a driver

    regs->status |= VIRTIO_STAT_DRIVER;
    //           fence o,io
    __sync_synchronize();

    //           Negotiate features. We need:
    //            - VIRTIO_F_RING_RESET and
    //            - VIRTIO_F_INDIRECT_DESC
    //           We want:
    //            - VIRTIO_BLK_F_BLK_SIZE and
    //            - VIRTIO_BLK_F_TOPOLOGY.

    virtio_featset_init(needed_features);
    virtio_featset_add(needed_features, VIRTIO_F_RING_RESET);
    virtio_featset_add(needed_features, VIRTIO_F_INDIRECT_DESC);
    virtio_featset_init(wanted_features);
    virtio_featset_add(wanted_features, VIRTIO_BLK_F_BLK_SIZE);
    virtio_featset_add(wanted_features, VIRTIO_BLK_F_TOPOLOGY);
    result = virtio_negotiate_features(regs,
        enabled_features, wanted_features, needed_features);

    if (result != 0) {
        kprintf("%p: virtio feature negotiation failed\n", regs);
        return;
    }

    //           If the device provides a block size, use it. Otherwise, use 512.

    if (virtio_featset_test(enabled_features, VIRTIO_BLK_F_BLK_SIZE))
        blksz = regs->config.blk.blk_size;
    else
        blksz = VIOBLK_SECTOR_SIZE;

    // the block size must be a multiple of 512 bytes
    assert(blksz % VIOBLK_SECTOR_SIZE == 0);
    debug("%p: virtio block device block size is %lu", regs, (long)blksz);

    //           Allocate initialize device struct

    dev = kmalloc(sizeof(struct vioblk_device) + blksz);
    memset(dev, 0, sizeof(struct vioblk_device));

    lock_init(&vblk_lk, "vioblk_lock");

    //           FIXME Finish initialization of vioblk device here
    dev->regs = regs;
    dev->instno = 0; // not needed (piazza post @567)
    dev->io_intf.ops = &vio_ops; //pointer to io_ops
    dev->irqno = irqno;
    dev->opened = 0;
    dev->readonly = 0; // not needed
    dev->blksz = blksz;
    dev->pos = 0; 
    dev->size = regs->config.blk.capacity * VIOBLK_SECTOR_SIZE; 
    dev->blkcnt = dev->size / blksz;
    dev->bufblkno = UINT64_MAX; // the data in the buffer is nothing, the block number cannot reach -1 in unsigned because INT64_MAX * blksz is so large
    dev->blkbuf = (void *)(dev)+sizeof(struct vioblk_device); // the block buffer is after the dev struct

    condition_init(&(dev->vq.used_updated), "used ring updated");



    // fills out the descriptors in the virtq struct

    // indirect descriptor
    struct virtq_desc* indirect_desc = &(dev->vq.desc[0]);  
    indirect_desc->addr = (uint64_t)(void *)(dev->vq.desc)+sizeof(struct virtq_desc); // points to the second entry in the desc[] array
    indirect_desc->flags |= VIRTQ_DESC_F_INDIRECT;
    indirect_desc->len = 3*sizeof(struct virtq_desc); // 3 because 1 descriptor for each of request header, data, and status
    indirect_desc->next = 0; // doesn't matter because the NEXT flag is not set

    struct virtq_desc* desc_tab = (void *)(dev->vq.desc)+sizeof(struct virtq_desc);
    // descriptor to the header
    desc_tab[VIOBLK_DESC_HEADER_ID].addr = (uint64_t)(void *) (&(dev->vq.req_header));
    desc_tab[VIOBLK_DESC_HEADER_ID].len = sizeof(struct vioblk_request_header); // section 2.7.5.3
    desc_tab[VIOBLK_DESC_HEADER_ID].flags |= VIRTQ_DESC_F_NEXT;
    desc_tab[VIOBLK_DESC_HEADER_ID].next = VIOBLK_DESC_DATA_ID; // pointing to the descriptor of the data buffer
    
    // descriptor to the data buffer (block buffer?)
    desc_tab[VIOBLK_DESC_DATA_ID].addr = (uint64_t)(void *) (dev->blkbuf);
    desc_tab[VIOBLK_DESC_DATA_ID].flags |= VIRTQ_DESC_F_NEXT; // we should change whether this is device-writable in the before IO operation
    desc_tab[VIOBLK_DESC_DATA_ID].len = blksz; // so that the device know the size of the buffer
    desc_tab[VIOBLK_DESC_DATA_ID].next = VIOBLK_DESC_STATUS_ID; // pointing to the descriptor of the status byte

    //descriptor to the status BYTE;
    desc_tab[VIOBLK_DESC_STATUS_ID].addr = (uint64_t)(void *)(&(dev->vq.req_status));
    desc_tab[VIOBLK_DESC_STATUS_ID].flags |= VIRTQ_DESC_F_WRITE; // the status byte is always device writable
    desc_tab[VIOBLK_DESC_STATUS_ID].len = sizeof(uint8_t);
    desc_tab[VIOBLK_DESC_STATUS_ID].next = 0; // doesn't matter because NEXT flag is not set

    // attaches virtq_avail and virtq_used structs using the virtio_attach_virtq function

    // the size of queue is 1
    // There's only one queue so the qid is 0
    virtio_attach_virtq(dev->regs, 0, VIOBLK_Q_SIZE, (uint64_t)(void *)(&(dev->vq.desc)), (uint64_t)(void *)(&(dev->vq.used)), (uint64_t)(void *)(&(dev->vq.avail)));
    
    // Finally, the isr and dev are registered
    intr_register_isr(irqno, VIOBLK_IRQ_PRIO, vioblk_isr, dev);
    device_register("blk", &vioblk_open, dev);

    regs->status |= VIRTIO_STAT_DRIVER_OK;    
    //           fence o,oi
    __sync_synchronize();
}

/**
 * @brief 
 * Opens a virtio block device specificed by aux, returns an io_intf through pointer parameter
 * @param ioptr this pointer will point to io_intf that is setup by this function, act like a return value
 * @param aux the pointer to the device struct that needs to be opened
 * @return 0 if open is successful, negative error code if not successful
 */
int vioblk_open(struct io_intf ** ioptr, void * aux) {
    //           FIXME your code here

    struct vioblk_device * const dev = aux;
    // int size = sizeof(dev->vq._avail_filler);

    assert (ioptr != NULL);

    if (dev->opened)
        return -EBUSY;

    // sets the virtq_avail and virtq_used queues such that they are available for use.
    virtio_enable_virtq(dev->regs, 0);

    dev->vq.avail.flags = 0; // we need notification, so NO_NOTIF flag should not be set
    dev->vq.avail.idx = 0; // the index of the indirect descriptor
    // dev->vq._avail_filler[] = (uint64_t)(void *)(&(dev->vq.avail)) + sizeof(struct virtq_avail);
    dev->vq.avail.ring[0] = 0; // we would only have one request at a time, so index will always be 0

    // dev->vq.used.ring = (uint64_t)(void *)(&(dev->vq.used)) + sizeof(struct virtq_used);

    // enable interrupt
    intr_enable_irq(dev->irqno);

    //sets necessary flags in vioblk_device (opened?)
    dev->opened = 1;

    *ioptr = &dev->io_intf;

    return 0;
}

//           Must be called with interrupts enabled to ensure there are no pending
//           interrupts (ISR will not execute after closing).

/**
 * @brief close the virtio block device with the device that the io interface specified is in
 * @param io the io interface that is in the device struct that is about to close (make sure this is actually in a device's struct)
 * @return no return value
 */
void vioblk_close(struct io_intf * io) {
    //           FIXME your code here
    struct vioblk_device * const dev = (void *) io - offsetof(struct vioblk_device, io_intf);

    trace("%s()", __func__);
    assert(io != NULL);
    assert(dev->opened);

    // resets the virtq_avail and virtq_used queues
    virtio_reset_virtq(dev->regs, 0);

    intr_disable_irq(dev->irqno);
    dev->opened = 0;
}

/**
 * @brief performs a single block io request (read/write to a signle block) with the provided device struct, block number and op_type
 * @param dev the pointer to the device that is performing this io
 * @param blk_no the block number that this io request will access
 * @param op_type read or write, can be VIRTIO_BLK_T_IN or VIRTIO_BLK_T_OUT
 * @return 0 if the read/write is success, -1 if not success
 */
int vioblk_io_request(struct vioblk_device * const dev, uint64_t blk_no, uint32_t op_type){
    assert(dev->opened);

    if(op_type == VIRTIO_BLK_T_OUT && dev->bufblkno != blk_no){
        // for write operation, check if the blk_no is the same as the number of the block in the dev buffer
        kprintf("The block number requested is not the same as the number of the block in the buffer!\n");
        return -1;
    }
    
    // VIRTIO_BLK_T_IN or VIRTIO_BLK_T_OUT 
    dev->vq.req_header.type = op_type; 
    // the sector size is always 512 as defined by the virtio protocol, but we want to read/write by aligning to block size
    uint64_t sector_no = blk_no * dev->blksz / VIOBLK_SECTOR_SIZE; 

    assert(sector_no < dev->regs->config.blk.capacity);

    dev->vq.req_header.sector = sector_no;


    for(int i = 0; i < VIOBLK_ATTEMPT_MAX; i++) {
        int next_idx = dev->vq.used.idx;
        intr_disable(); // we don't want interrupt to trigger before entering condition_wait
        if(op_type == VIRTIO_BLK_T_IN){
            dev->vq.desc[2].flags |= VIRTQ_DESC_F_WRITE; // the data buffer is device-writable
        }else{
            dev->vq.desc[2].flags &= ~VIRTQ_DESC_F_WRITE; // the data buffer is not device-writable in a write operation
        }
        dev->vq.avail.idx ++;
        virtio_notify_avail(dev->regs, 0);
        // kprintf("notifying the block device a read/write op.\n");
        condition_wait(&(dev->vq.used_updated)); //wait for a read/write to complete
        intr_enable();

        // if there's a used buffer notification, then the idx will be updated by plus 1.
        assert(next_idx != dev->vq.used.idx);
        // kprintf("the idx of the used ring has update!\n");

        if(op_type == VIRTIO_BLK_T_IN){ // if this is a read operation
            // now blkbuf contains the block data, update the bufblkno
            dev->bufblkno = blk_no;
        }

        // check the id and the len
        // dev->vq.used.flags does not matter because we don't use VIRTQ_USED_F_NO_NOTIFY
        // should use next_idx as index byt it's always 0 because modulo QUEUE_SIZE(1) will always be 0
        if(dev->vq.used.ring[0].id != 0) {
            // we only have one descriptor chain, so id should always be 0
            // 
            kprintf("the used ring is not returning id of 0.\n");
        }

        // if(op_type == VIRTIO_BLK_T_IN && dev->vq.used.ring[0].len != dev->blksz){
        //     kprintf("For a block read request, the number of bytes read is not the same as block size.\n");
        // }

        // if(op_type == VIRTIO_BLK_T_OUT && dev->vq.used.ring[0].len != 0){
        //     kprintf("For a block write request, the number of bytes read is not 0\n");
        // }

        if (dev->vq.req_status == VIRTIO_BLK_S_OK)
        {
            // kprintf("read/write request ok!\n");
            return 0; 
        }else if(dev->vq.req_status == VIRTIO_BLK_S_IOERR){
            kprintf("read/write request IO Error!\n");
        }else if(dev->vq.req_status == VIRTIO_BLK_S_UNSUPP){
            kprintf("read/write request un supported\n");
        }
    }

    return -1;
}

/**
 * @brief performs a read from a block device indicated by the io_intf, result will be copied to the buf specified.
 * Will only perform read from a single block (if used with ioread())
 * This function is compatible with ioread_full() to perform arbitrary length data reads (from multiple blocks).
 * Will read no more than bufsz
 * @param io the pointer to the io_intf contained in the device struct
 * @param buf the pointer to the buf that the result will be in
 * @param bufsz the maximum length of data that a single call will read
 * @return the number of bytes read into the buf, as required by io_ops
 * 
 */
long vioblk_read (
    struct io_intf * restrict io,
    void * restrict buf,
    unsigned long bufsz)
{
    struct vioblk_device * const dev = (void *) io - offsetof(struct vioblk_device, io_intf);

    trace("%s(buf=%p, bufsz=%ld)", __func__, buf, bufsz);
    assert(io != NULL);
    assert(dev->opened); 
    //           FIXME your code here

    if(dev->pos + bufsz > dev->regs->config.blk.capacity * VIOBLK_SECTOR_SIZE){
        kprintf("read exceeds block device capacity");
        return 0;
    }


    // if data in the block buffer is already the block that we need
    // we can directly copy data from the buffer in dev struct to the output buffer

    int blk_no = (dev->pos) / (dev->blksz);
    int pos_in_blk = (dev->pos) % (dev->blksz);  // the offset of current "cursor" position in block
    int start_pos = pos_in_blk; // the index that we are start reading from 
    int end_pos = min(dev->blksz, start_pos + bufsz); // read until the end of block unless we are reading enough before that, this position is  (we read until end_pos - 1)

    // if the buffer does not contain the block that we want, read it from the device
    if(dev->bufblkno != blk_no){
        // request data from vioblk device, data should be in dev->blkbuf after this.
        vioblk_io_request(dev, blk_no, VIRTIO_BLK_T_IN);
    }

    // now we need to copy data from the buffer we read from the device to the output buffer
    lock_acquire(&vblk_lk);
    memcpy(buf, dev->blkbuf + pos_in_blk, end_pos - start_pos); // copy to the end of the block
    lock_release(&vblk_lk);
    dev->pos += end_pos - start_pos;

    return end_pos - start_pos;
}

/**
 * @brief performs a write to a block device indicated by the io_intf, using data in buf.
 * Will only perform write to a single block.
 * This function is compatible with iowrite() to perform arbitrary length data writes (to multiple blocks).
 * Will write no more than bufsz
 * @param io the pointer to the io_intf contained in the device struct
 * @param buf the pointer to the buffer in which the data writing to the block device is from
 * @param n the requested length of data to write, might not write all in a single call, to write all, used iowrite()
 */
long vioblk_write (
    struct io_intf * restrict io,
    const void * restrict buf,
    unsigned long n)
{
    // FIXME your code here

    struct vioblk_device * const dev = (void *) io - offsetof(struct vioblk_device, io_intf);

    trace("%s(buf=%p, bufsz=%ld)", __func__, buf, n);
    assert(io != NULL);
    assert(dev->opened); 

    if(dev->pos + n > dev->regs->config.blk.capacity * VIOBLK_SECTOR_SIZE){
        kprintf("write exceeds block device capacity");
        return 0;
    }

    int blk_no = (dev->pos) / (dev->blksz);
    int pos_in_blk = (dev->pos) % (dev->blksz); // the offset of the current cursor in the block
    int start_pos = pos_in_blk;
    int end_pos = min(dev->blksz, start_pos+n); // we write until the end of the block unless we are writing enough data, does not include this position
    // n - bytes_written is the number of bytes that still need to be written

    // if the write is not a full block 
    // and the block in the buffer is not the block that we want to write to,
    // we need to read the block first
    if((end_pos != dev->blksz || start_pos != 0) && dev->bufblkno != blk_no){
        vioblk_io_request(dev, blk_no, VIRTIO_BLK_T_IN);
        // now we have a full block in the block buffer of the device struct
        assert(dev->bufblkno == blk_no);
    }

    // the block buffer is already the block that we want to write to
    // we can directly modify the part of the data that we want to modify and then request a write

    // if we are writing a full block, makesure to update the bufblkno as writing into dev->buf
    if(start_pos == 0 || end_pos == dev->blksz){
        dev->bufblkno = blk_no;
    }

    // copy date from buf to the block buffer
    lock_acquire(&vblk_lk);
    memcpy(dev->blkbuf + start_pos, buf, end_pos - start_pos);
    lock_release(&vblk_lk);

    // request a write operation
    vioblk_io_request(dev, dev->bufblkno, VIRTIO_BLK_T_OUT);

    // write to device is done, update bytes_written to device
    dev->pos += end_pos - start_pos;
    return end_pos - start_pos;
}

/**
 * @brief virtio block device io control function, as specified by io_ops.
 * can perform getlen, getpos, setpos, and getblksz functions as specified by cmd.
 * Arguments to these functions are passed through arg
 * @param io the pointer to the io_intf contained in the device struct
 * @param cmd the type of the specific io control function that you want to execute
 * @param arg the argument to pass into the io control functions (including pointers to return values)
 * @return 0 if successful, negative if error
 */
int vioblk_ioctl(struct io_intf * restrict io, int cmd, void * restrict arg) {
    struct vioblk_device * const dev = (void*)io -
        offsetof(struct vioblk_device, io_intf);
    
    trace("%s(cmd=%d,arg=%p)", __func__, cmd, arg);
    
    switch (cmd) {
    case IOCTL_GETLEN:
        return vioblk_getlen(dev, arg);
    case IOCTL_GETPOS:
        return vioblk_getpos(dev, arg);
    case IOCTL_SETPOS:
        return vioblk_setpos(dev, arg);
    case IOCTL_GETBLKSZ:
        return vioblk_getblksz(dev, arg);
    default:
        return -ENOTSUP;
    }
}

/**
 * @brief the interrupt service routine for virtio block device, aux points to the device triggering this isr.
 * If there's a used buffer notification from the block device, it will broadcast the condition used_updated in the device
 * to continue execute a read/write operation.
 * @param irqno the interrupt request number of the device that triggered this isr
 * @param aux the pointer to the device struct triggered this isr
 * @return no return 
 */
void vioblk_isr(int irqno, void * aux) {
    //           FIXME your code here
    struct vioblk_device * const dev = aux;
    const uint32_t USED_BUFFER_NOTIF = (1 << 0); 

    if(dev->regs->interrupt_status & USED_BUFFER_NOTIF){
        // There's a new used buffer, signal the condition to let driver continue.
        condition_broadcast(&(dev->vq.used_updated));

        // acknolwedge the interrupt is handled to the device
        dev->regs->interrupt_ack |= USED_BUFFER_NOTIF;
        // fence 
        __sync_synchronize();
    }
}

/**
 * @brief Get the total length in bytes of the block device, value is returned through the lenptr
 * @param dev the device that you want to ask about, 
 * @param lenptr the pointer to the value that you want to obtain, result will be put here
 * @return 0 if success, negative if error
 */
int vioblk_getlen(const struct vioblk_device * dev, uint64_t * lenptr) {
    //           FIXME your code here
    if (lenptr == NULL)
    {
        return -EINVAL;
    }
    *lenptr = dev->size;
    return 0;
}

/**
 * @brief Get the current cursor position in disk which is reading from/writing to
 * @param dev the device that you want to access
 * @param posptr the pointer to the value that you want to obtain, result will be put here
 * @return 0 if success, negative if error
 */
int vioblk_getpos(const struct vioblk_device * dev, uint64_t * posptr) {
    //           FIXME your code here
    if (posptr == NULL)
    {
        return -EINVAL;
    }
    *posptr = dev->pos;
    return 0;
}

/**
 * @brief Set the current cursor position in disk which is reading from/writing to
 * @param dev the device that you want to access
 * @param posptr the pointer to the value that you want to set
 * @return 0 if success, negative if error
 */
int vioblk_setpos(struct vioblk_device * dev, const uint64_t * posptr) {
    //           FIXME your code here
    
    if(*posptr >= dev->size){
        kprintf("request vioblk_setpos with a position out of device bound.\n");
        return -1;
    }
    dev->pos = *posptr;
    return 0;
}

/**
 * @brief Gets the block size of the block device
 * @param dev the device that you want to access
 * @param posptr the pointer to the value that you want to get, result will be put here
 * @return 0 if success, negative if error
 */
int vioblk_getblksz (
    const struct vioblk_device * dev, uint32_t * blkszptr)
{
    //           FIXME your code here
    *blkszptr = dev->blksz;
    return 0;
}
