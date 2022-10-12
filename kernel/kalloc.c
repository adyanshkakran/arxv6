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

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  int references[MAXREF];
} kmem;


void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  acquire(&kmem.lock);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    kmem.references[PA2REF(p)] = 1;
    release(&kmem.lock);
    kfree(p);
    acquire(&kmem.lock);
  }
  release(&kmem.lock);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  int ref = PA2REF(pa);
  acquire(&kmem.lock);
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP || kmem.references[ref] < 1)
    panic("kfree");

  kmem.references[ref]--;
  if(kmem.references[ref] > 0){
    release(&kmem.lock);
    return;
  }

  memset(pa, 1, PGSIZE);   // Fill with junk to catch dangling refs.

  r = (struct run*)pa;

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
  release(&kmem.lock);

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    acquire(&kmem.lock);
    if(kmem.references[PA2REF((uint64)r)] != 0)
      panic("kalloc");
    kmem.references[PA2REF((uint64)r)] = 1;
    release(&kmem.lock);
  }
  return (void*)r;
}

void addReference(void* pa){
  int ref = PA2REF(pa);
  if(ref < 0 || ref >= MAXREF)
    return;
  acquire(&kmem.lock);
  kmem.references[ref]++;
  release(&kmem.lock);
}

void decReference(void* pa){
  int ref = PA2REF(pa);
  if(ref < 0 || ref >= MAXREF)
    return;
  acquire(&kmem.lock);
  if(kmem.references[ref] <= 0){
    panic("References");
  }
  kmem.references[ref]--;
  
  // If all references to page is removed
  if(kmem.references[ref] == 0){
    release(&kmem.lock);
    kfree(pa);
    return;
  }
  release(&kmem.lock);
}

int pagefhandler(pagetable_t pagetable,uint64 va){
  if(va >= MAXVA || va <= 0)
    return -1;

  pte_t *pte = walk(pagetable,va,0);
  if(pte == 0){
    printf("Page not Found: Page Fault Exception\n");
    return -1;
  }
  if(!(*pte & PTE_V) || !(*pte & PTE_U)){
    printf("Permissions not found\n");
    return -1;
  }

  uint64 pa = (uint64)PTE2PA(*pte);

  char *mem = kalloc(); // Make copy of page and map calling process(parent or child) to it
  if(mem == 0)
    return -1;
  memmove(mem,(char*)pa,PGSIZE);

  kfree((void*)pa);
  *pte = PA2PTE(mem) | PTE_V | PTE_U | PTE_R | PTE_W | PTE_X;

  return 0;
}