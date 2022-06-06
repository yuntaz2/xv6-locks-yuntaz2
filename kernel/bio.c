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

#define NBUCKETS 7

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
} bcache[NBUCKETS];

void
binit(void)
{
  for(int i = 0; i < NBUCKETS; ++i)
  {
    for (struct buf *b = bcache[i].buf; b < bcache[i].buf + NBUF; ++b)
    {
      initsleeplock(&b->lock, "buffer");
    }
    initlock(&bcache[i].lock, "bcache.bucket");
  }
}

unsigned char getIndex(uint blockno)
{
  return blockno % NBUCKETS;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b = 0;

  int idx = getIndex(blockno);
  uint minTimeStamp = -1;
  struct buf *minBuf = 0;

  acquire(&bcache[idx].lock);

  // Is the block already cached?
  for(b = bcache[idx].buf; b < bcache[idx].buf + NBUF; ++b)
  {
    if (b->blockno == blockno && b->dev==dev)
    {
      b->refcnt++;
      release(&bcache[idx].lock);
      acquiresleep(&b->lock);
      return b;
    }
    if (b->timeStamp < minTimeStamp && b->refcnt == 0)
    {
      minTimeStamp = b->timeStamp;
      minBuf = b;
    }
  }

  b = minBuf;
  if (b)
  {
    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;
    release(&bcache[idx].lock);
    acquiresleep(&b->lock);
    return b;
  }
  panic("bget: no buffers");
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
  int idx = getIndex(b->blockno);
  
  acquire(&bcache[idx].lock);
  b->refcnt--;

  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->timeStamp=ticks;
  }
  
  release(&bcache[idx].lock);
}

void
bpin(struct buf *b) {
  int idx = getIndex(b->blockno);
  acquire(&bcache[idx].lock);
  b->refcnt++;
  release(&bcache[idx].lock);
}

void
bunpin(struct buf *b) {
  int idx = getIndex(b->blockno);
  acquire(&bcache[idx].lock);
  b->refcnt--;
  release(&bcache[idx].lock);
}


