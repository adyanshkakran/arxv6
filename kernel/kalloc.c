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
} kmem;

struct {
  struct spinlock lock;
  int references[MAXREF];
} Ref;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&Ref.lock, "Ref");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    kfree(p);
  }
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
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&Ref.lock);
  if(--Ref.references[ref] > 0){
    release(&Ref.lock);
    return;
  }
  release(&Ref.lock);

  memset(pa, 1, PGSIZE);   // Fill with junk to catch dangling refs.
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
  release(&kmem.lock);

  if(r){
    memset((char*)r, 69, PGSIZE); // fill with junk
    Ref.references[PA2REF((uint64)r)] = 1;
  }
  return (void*)r;
}

void addReference(void* pa){
  int ref = PA2REF(pa);
  if(ref < 0 || ref >= MAXREF)
    return;
  acquire(&Ref.lock);
  Ref.references[ref]++;
  release(&Ref.lock);
}

// Copy the page to another memory address and decrease reference count
void* copyndecref(void* pa){
  int ref = PA2REF(pa);
  if(ref < 0 || ref >= MAXREF)
    return 0;

  acquire(&Ref.lock);
  if(Ref.references[ref] <= 1){
    release(&Ref.lock);
    return pa;
  }
  
  Ref.references[ref]--;
  
  uint64 mem = (uint64)kalloc();
  if(mem == 0){
    release(&Ref.lock);
    return 0;
  }
  memmove((void*)mem,(void*)pa, PGSIZE);
  release(&Ref.lock);
  return (void*)mem;
}

int pagefhandler(pagetable_t pagetable,uint64 va){
  if(va >= MAXVA || va <= 0)
    return -1;

  pte_t *pte = walk(pagetable,va,0);
  if(pte == 0){
    return -1;
  }
  if(!(*pte & PTE_V) || !(*pte & PTE_U) || !(*pte & PTE_COW)){
    return -1;
  }

  uint64 pa = PTE2PA(*pte);
  void* mem = copyndecref((void*)pa);

  if(mem == 0)
    return -1;

  uint64 flags = (PTE_FLAGS(*pte) | PTE_W) & ~PTE_COW;
  uvmunmap(pagetable,PGROUNDDOWN(va),1,0);
  if(mappages(pagetable,va,1,(uint64)mem,flags) == -1){
    panic("Pagefhandler mappages");
  }

  return 0;
}