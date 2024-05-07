// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

uint16 pgrefcount[REFSIZE];

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
initpgrefcount()
{
  // memset(kmem.pgrefcount, 128, PGSIZE);
  for (int i = 0; i != REFSIZE; ++i)
    pgrefcount[i] = 0;
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
  initpgrefcount();
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// increase a page reference count use physical address
// pa should align to page size
// return new value when success, -1 when failed
int
pgrefcountinc(uint64 pa)
{
  if (pa % PGSIZE != 0) {
    return -1;
  }

  uint16 ret;
  acquire(&kmem.lock);
  // uint64 relativeaddr = pa - (PGROUNDUP((uint64)end));
  uint16 *num = &pgrefcount[(uint64)pa / PGSIZE];
  *num += 1;
  ret = *num;
  release(&kmem.lock);

  return ret;
}

// decrease a page reference count use physical address
// pa should align to page size
// return new value when success, -1 when failed
int
pgrefcountdec(uint64 pa)
{
  if (pa % PGSIZE != 0) {
    return -1;
  }

  uint16 ret;
  acquire(&kmem.lock);
  // uint64 relativeaddr = pa - (PGROUNDUP((uint64)end));
  uint16 *num = &pgrefcount[(uint64)pa / PGSIZE];
  *num -= 1;
  ret = *num;
  release(&kmem.lock);

  return ret;  
}

// return page reference count by physical address, pa should align to page size
int
pgrefcnt(uint64 pa)
{
  if (pa % PGSIZE != 0) {
    return -1;
  }

  uint16 ret;
  acquire(&kmem.lock);
  // uint64 relativeaddr = pa - (PGROUNDUP((uint64)end));
  uint16 *num = &pgrefcount[(uint64)pa / PGSIZE];
  ret = *num;
  release(&kmem.lock);

  return ret;    
}

// no lock, only for kfree and kalloc
int
pgrefcountset(uint64 pa, uint16 val)
{
  if (pa % PGSIZE != 0) {
    return -1;
  }

  // uint64 relativeaddr = pa - (PGROUNDUP((uint64)end));
  uint16 *num = &pgrefcount[(uint64)pa / PGSIZE];
  *num = val;
  return 0;   
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  pgrefcountset((uint64)r, 1);
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
