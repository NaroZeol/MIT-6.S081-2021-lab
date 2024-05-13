// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
} bcache;

struct {
  struct spinlock lock;
  struct buf head;
} buckets[NBUCKET];

static uint hash(uint dev, uint blockno) {
  return (dev + blockno) % NBUCKET;
}

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");
  for (int i = 0; i != NBUCKET; ++i) {
    initlock(&buckets[i].lock, "bcache");
    buckets[i].head.next = &buckets[i].head;
    buckets[i].head.prev = &buckets[i].head;
  }

  uint32 num = 0;
  uint32 index = 0;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = buckets[index].head.next;
    b->prev = &buckets[index].head;
    initsleeplock(&b->lock, "buffer");
    buckets[index].head.next->prev = b;
    buckets[index].head.next = b;
    if (++num == (NBUF / NBUCKET) && index != NBUCKET - 1) {
      index += 1;
      num = 0;
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int curr = hash(dev, blockno); // Is there any race condition?
  acquire(&buckets[curr].lock);

  // Is the block already cached?
  for(b = buckets[curr].head.next; b != &buckets[curr].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&buckets[curr].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = buckets[curr].head.prev; b != &buckets[curr].head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&buckets[curr].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // No buffer in bucket[curr], try to get it from other buckets
  // Let i start from curr + 1 can avoid deadlock!
  for (int i = (curr + 1) % NBUCKET; i != curr; i = (i + 1) % NBUCKET) { 
    if (i == curr)
      continue;
    acquire(&buckets[i].lock);
    for(b = buckets[i].head.prev; b != &buckets[i].head; b = b->prev){
      if(b->refcnt == 0) {
        // detach from other buckets
        b->next->prev = b->prev;
        b->prev->next = b->next;

        // steal
        b->next = buckets[curr].head.next;
        buckets[curr].head.next->prev = b;
        b->prev = &buckets[curr].head;
        buckets[curr].head.next = b;

        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;

        release(&buckets[i].lock);
        release(&buckets[curr].lock);
        acquiresleep(&b->lock);

        return b;
      }
    }
    release(&buckets[i].lock);
  }

  panic("bget: no buffers");
  return 0;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");


  releasesleep(&b->lock);

  int index = hash(b->dev, b->blockno);
  acquire(&buckets[index].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = buckets[index].head.next;
    b->prev = &buckets[index].head;
    buckets[index].head.next->prev = b;
    buckets[index].head.next = b;
  }
  release(&buckets[index].lock);
}

void
bpin(struct buf *b) {
  int index = hash(b->dev, b->blockno);
  acquire(&buckets[index].lock);
  b->refcnt++;
  release(&buckets[index].lock);
}

void
bunpin(struct buf *b) {
  int index = hash(b->dev, b->blockno);
  acquire(&buckets[index].lock);
  b->refcnt--;
  release(&buckets[index].lock);
}