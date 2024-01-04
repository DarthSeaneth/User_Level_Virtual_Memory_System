# User_Level_Virtual_Memory_System
Sean Patrick

1)
Implementation of Virtual Memory System:

void set_physical_mem():
    This function is called once upon the first t_malloc() call, ensured by our global variable initial_call.
    First we allocate memory for the simulated physical memory declared as char *memory based on the value of MEMSIZE.
    Then we allocate memory for the physical and virtual bitmaps and set the first bits as in use for each as we want the first physical page
    to be the page directory and we do not want to use the first virtual page as the address 0x00000000 is invalid.
    Then we calculate all of the page table information we need, such as the amount of physical and virtual pages, bits for the page directory, page table,
    and page offset, and the amount of entries in the page directory and inner page table.
    Finally, we store the page directory within the first page of physical memory.

int add_TLB(void *va, void *pa):
    First we enforce a lock to ensure that multiple threads do not attempt to increment the global variables concurrently, and then we increment
    the TLB lookups and misses because every addition of a TLB entry counts as a miss.
    Then we calculate the virtual page number based on the virtual address and calculate the index of the TLB array based on the VPN and TLB_ENTRIES.
    Then we set its validity to true, store the translated physical address, and also the virtual page number in the TLB structure, which is all considered 
    a critical section. Then we return 0 to indicate success.

pte_t *check_TLB(void *va):
    First we increment the TLB lookups which is enforced by a mutex lock, and then we calulcate the virtual page number determined by the virtual address.
    Then we calculate the index of the TLB array based on the VPN and TLB_ENTRIES.
    We then check if the entry is valid and if its VPN matches the calculated VPN. If that is true, we return the TLB stored physical address translation.
    Otherwise, we increment the TLB misses, enforced by a lock, and return a null pointer to indicate failure.

void print_TLB_missrate():
    This function simply calculates the TLB miss rate by dividing the number of misses by the total number of TLB entry lookups.
    Then it prints the amount of TLB lookups, followed by the TLB misses and the miss rate.

pte_t *translate(pde_t *pgdir, void *va):
    First we calculate the specific page offset using the virtual address and the bits for the page offset and then we check the TLB for a valid translation.
    If there is a hit in the TLB, we simply use the TLB physical address translation, add the offset, and then return the physical address based on that.
    If there is a TLB miss, we then calculate the page directory index, page table index, and the virtual page number.
    If the virtual page is valid, we then traverse our page directory to obtain the information needed to calculate the physical page number.
    Then we calculate the physical address based on the PPN and we return the correct address.
    If the VPN, page directory entry, or page table entry is invalid we return a null pointer.

int page_map(pde_t *pgdir, void *va, void *pa):
    First we calculate the page directory index, page table index, and the virtual page number.
    If the virtual page is invalid, we return -1.
    Then we traverse the page directory and allocate a new page table if one does not already exist for the corresponding page directory entry.
    We continue to traverse the page directory and we set the page table entry to contain the physical page number.
    We also add an entry into the TLB based on the specific virtual to physical address mapping.
    The function returns 0 upon success.

void *get_next_avail(int num_pages, unsigned long *physical_page_addresses):
    First we traverse the virtual bitmap to find free contiguous virtual pages, the amount of which is indicated by num_pages.
    A null pointer is returned if not enough pages are available.
    Then we traverse the physical bitmap to find num_pages free pages, which do not need to be contiguous.
    If not enough are available, a null pointer is returned.
    Then we set the bits in the bitmaps corresponding to the pages we found to mark them as in use.
    Then we calculate the first virtual address based on the first page's virtual page number, and we can easily find the other addresses if more than one page
    was requested.
    Then we return that virtual address.

void *t_malloc(unsigned int num_bytes):
    If num_bytes is 0, we return a null pointer as allocating 0 bytes is not valid.
    Then we enforce a lock before checking if memory needs to be initialized to ensure that multiple threads don't attempt to initialize
    memory at the same time.
    Then we calculate the number of pages needed for the allocation.
    Then we enforce a lock to ensure that multiple threads are not allocating the same free pages, and we obtain the information needed regarding
    the free virtual and physical pages.
    If not enough pages were available or a page mapping fails, a null pointer is returned.
    Then we set the mapping for each virtual to physical page translation using page_map().
    Then we return a pointer to the first virtual address that was allocated.

void t_free(void *va, int size):
    First we calculate the number of pages that are requested to be freed and we calculate the virtual page number.
    Then we iterate through each virtual address that was requested to be freed, and we first use translate() to obtain the physical address translation.
    Then we calculate the physical page number.
    Then we enforce a lock to ensure multiple threads don't attempt to free the same pages or clear the same bitmap entries.
    Then we clear the bits in the bitmaps corresponding to the pages being freed, and we also invalidate the TLB entry associated with the pages
    if one exists.
    If any of the pages are not already allocated, we perform an early return as we do not want to free already free pages.

int put_value(void *va, void *val, int size):
    First we check if we are writing data into more than one page.
    If only one page is being written to, we simply use translate() to obtain the physical address and then copy the data into the page.
    If multiple pages are being written to, we first calculate how many pages we need.
    We then iterate through each specific physical address corresponding to a page and copy the data until we have copied all bytes of the data into the
    corresponding pages.

void get_value(void *va, void *val, int size):
    This function is extremely similar to put_value().
    The only difference is that we are copying the data from the physical pages to a specified location instead of copying data
    from a specific location and writing that into the pages.

Note: We also implemented various helper functions to modularize our code and make it more readable.

2)
Benchmark Program Results:
    test.c:
    Allocating three arrays of 400 bytes
    Addresses of the allocations: 1000, 2000, 3000
    Storing integers to generate a SIZExSIZE matrix
    Fetching matrix elements stored in the arrays
    1 1 1 1 1 
    1 1 1 1 1 
    1 1 1 1 1 
    1 1 1 1 1 
    1 1 1 1 1 
    Performing matrix multiplication with itself!
    5 5 5 5 5 
    5 5 5 5 5 
    5 5 5 5 5 
    5 5 5 5 5 
    5 5 5 5 5 
    Freeing the allocations!
    Checking if allocations were freed!
    free function works
    lookups: 407 misses: 4 TLB miss rate: 0.009828

    multi_test.c:
    Allocated Pointers: 
    2b000 28000 25000 22000 1f000 d000 1000 4000 7000 a000 10000 13000 16000 19000 1c000 
    initializing some of the memory by in multiple threads
    Randomly checking a thread allocation to see if everything worked correctly!
    1 1 1 1 1 
    1 1 1 1 1 
    1 1 1 1 1 
    1 1 1 1 1 
    1 1 1 1 1 
    Performing matrix multiplications in multiple threads threads!
    Randomly checking a thread allocation to see if everything worked correctly!
    5 5 5 5 5 
    5 5 5 5 5 
    5 5 5 5 5 
    5 5 5 5 5 
    5 5 5 5 5 
    Gonna free everything in multiple threads!
    Free Worked!
    lookups: 1933 misses: 88 TLB miss rate: 0.045525  

    Observation: We tested our implementation with various different values for PGSIZE and TLB_ENTRIES.
    The TLB miss rate is directly related to the value for TLB_ENTRIES, which is intuitive.
    As TLB_ENTRIES increases, the miss rate decreases.
    As TLB_ENTRIES decreases, the miss rate increases.

3)
Support for Different Page Sizes:
    Our virtual memory management library supports different page sizes in a relatively simple way.
    The amount of bits for the page directory, page table, and page offset are determined by the value for PGSIZE.
    The number of bits for the page offset will always be equal to log_2(PGSIZE), which for a PGSIZE of 4096 will be 12 bits.
    The number of bits for the page directory is equal to (32 - page_offset_bits) / 2.
    The number of bits for the inner page table is equal to (32 - page_directory_bits - page_offset_bits).
    Therefore, if the page offset bits is odd, then there will be an odd number of bits to split between the page directory and inner page table.
    With our implementation, the extra bits will go to the inner page table and the page directory will have less bits.
    This ensures validity and correctness for any page size.

4)
Possible Issues:
    There was one main possible issue that we encountered.
    The issue is internal fragmentation within the physical memory, as pages are allocated non-contiguously when possible.
    This could potentially cause some issues for certain patterns and amounts of memory allocations.
    We did not find any other potential issues, although there may be some potential issues that we overlooked.
    However, upon extensively testing with various different combinations of different page sizes and TLB sizes, we did not see any other issues.
