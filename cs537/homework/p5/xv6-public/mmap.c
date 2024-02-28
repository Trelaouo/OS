#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "elf.h"
#include "mmap.h"

static struct map_mem mapped;

// struct map_mem *mapped allocmmap() {
//     mapped = kalloc();
//     return mapp
// }

void *mmap(void *addr, int length, int prot, int flags, int fd, int offset) {
    
    

    struct proc *curproc = myproc();
    
    int private_bit = flags & MAP_PRIVATE;
    int shared_bit = flags & MAP_SHARED;
    int anon_bit = flags & MAP_ANONYMOUS;
    int fixed_bit = flags & MAP_FIXED;
    int growsup_bit = flags & MAP_GROWSUP;

    int read_bit = prot & PROT_READ;
    int write_bit = prot & PROT_WRITE;

    

    if(fixed_bit == MAP_FIXED) {
        // Check if address is multiple
        if(!((uint)addr % PGSIZE)){
            return MAP_FAIL;
        }

        // Check address bounds
        if(!((uint)addr < KERNBASE && (uint)addr >= MMAPBASE)) {
            // seg fault
            kill(curproc->pid);
            return MAP_FAIL;
        }

        // populate struct to reserve memory
        // can also think of this as the 'lazy' part of lazy allocation
        mapped.addr = addr;
        mapped.length = length;
        mapped.offset = offset;
        mapped.prot = prot;
        mapped.flags = flags;
        
        // If there is file descriptor, check its valid
        if(fd < 0 || fd >= NOFILE || (mapped.f=myproc()->ofile[fd]) == 0)
            return MAP_FAIL;
        mapped.fd = fd;

        // Make sure we aren't exceeding max number of mappings
        if(curproc->num_mappings == MAX_MAPS){
            cprintf("max number of mappings have been reached\n");    
        }

        // Find smallest available index and put the struct at that index
        for(int i = 0; i < MAX_MAPS; i++){
            if(curproc -> map[i].addr == 0){
                curproc -> map[i] = mapped;
                curproc->num_mappings++;
                break;
            }
       }


       return addr;

    }

}
int munmap(void* addr, int length) {
    struct proc *curproc = myproc();

    if(!((uint)addr % PGSIZE)){
        return MAP_FAIL;
    }

    for(int i = 0; i < MAX_MAPS; i++){
        if(curproc -> map[i].addr == addr){
            
            uint starting_addr = curproc -> map[i].addr;

            uint anon = curproc->map[i].flags & MAP_ANON;
            uint shared = curproc->map[i].flags & MAP_SHARED;
            // Check for file backing
            if(shared && !anon){
                if(filewrite(curproc -> map[i].f, (char *) curproc -> map[i].addr, curproc -> map[i].length) < 0){
                    return -1; 
                }
            }



            for(uint j = starting_addr; j < starting_addr + length; j+=PGSIZE){
                // Get the pte
                pte_t *pte = walkpgdir(curproc ->pgdir, (char*)j, 1);
                char *v;
                
                // Check if the pte is present in the pgtable
                if(*pte & PTE_P){
                    
                    uint pa = PTE_ADDR(*pte);

                    v = P2V(pa);
                    
                    // Free memory
                    kfree(v);

                    // Indicate the page is not present
                    *pte = *pte & ~PTE_P;

                }
            }

            curproc -> map[i].addr += length;
            curproc -> map[i].length -= length; 
        }
    }


    return 0;
}
