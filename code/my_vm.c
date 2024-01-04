//Sean Patrick (smp429) & Fulton Wilcox (frw14)
#include "my_vm.h"

#define DEBUG 0

/* Determines if physical memory needs to be initialized */
int initial_call = 1;

/* Simulated physical memory */
char *memory;

/* Page Directory */
pde_t *page_directory;

/* General pthread mutex lock */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* TLB pthread mutex lock */
pthread_mutex_t TLB_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Memory bitmaps */
char *physical_bitmap;
char *virtual_bitmap;

/* Page Table information */
int virtual_pages = 0;
int physical_pages = 0;
int page_directory_entries = 0;
int page_table_entries = 0;
int virtual_page_number_bits = 0;
int page_directory_bits = 0;
int page_table_bits = 0;
int page_offset_bits = 0;

/* TLB variables */
tlb mem_tlb;
int lookups;
int misses;

/*
Function responsible for allocating and setting your physical memory 
*/
void set_physical_mem() {

    //Allocate physical memory using mmap or malloc; this is the total size of
    //your memory you are simulating
    
    //HINT: Also calculate the number of physical and virtual pages and allocate
    //virtual and physical bitmaps and initialize them

    if(!initial_call) return;
    initialize_memory();
    calculate_page_table_info();
    //Initialize page table directory to first page of physical memory
    page_directory = (pde_t*) memory;
    initial_call = 0;
}

/*
 * Part 2: Add a virtual to physical page translation to the TLB.
 * Feel free to extend the function arguments or return type.
 */
int add_TLB(void *va, void *pa)
{
    //every addition of TLB entries is a miss
    pthread_mutex_lock(&TLB_mutex);
    lookups++;
    misses++;
    pthread_mutex_unlock(&TLB_mutex);
    if(DEBUG) printf("adding to tlb virtual address %ld \noffset bits: %d\n", (unsigned long)va, page_offset_bits);
    unsigned long virtual_address = (unsigned long) va;
    unsigned long virt_page_num = calculate_VPN(virtual_address, page_offset_bits);
    //Calculate index of TLB entry array based on TLB_ENTRIES size
    int tag = virt_page_num % TLB_ENTRIES;
    if(DEBUG) printf("tag: %d \n", tag);
    pthread_mutex_lock(&TLB_mutex);
    //save the physical address and VPN for quick translation upon TLB hit
    mem_tlb.entries[tag].valid = 1;
    mem_tlb.entries[tag].physical_address = (unsigned long) pa;
    mem_tlb.entries[tag].virt_page_num = virt_page_num;
    pthread_mutex_unlock(&TLB_mutex);
    return 0;
}

/*
 * Part 2: Check TLB for a valid translation.
 * Returns the physical page address.
 * Feel free to extend this function and change the return type.
 */
pte_t *check_TLB(void *va) {
    /* Part 2: TLB lookup code here */
    if(DEBUG) print_tlb_entries();
    pthread_mutex_lock(&TLB_mutex);
    lookups++;
    pthread_mutex_unlock(&TLB_mutex);
    unsigned long virtual_address = (unsigned long) va;
    unsigned long virt_page_num = calculate_VPN(virtual_address, page_offset_bits);
    //Calculate index of TLB entry array based on TLB_ENTRIES size
    int tag = virt_page_num % TLB_ENTRIES;
    if(mem_tlb.entries[tag].valid && mem_tlb.entries[tag].virt_page_num == virt_page_num) return (pte_t*)mem_tlb.entries[tag].physical_address;
    //If we got this far it's a miss
    pthread_mutex_lock(&TLB_mutex);
    misses++;
    pthread_mutex_unlock(&TLB_mutex);
    return NULL;
    /*This function should return a pte_t pointer*/
}

/*
 * Part 2: Print TLB miss rate.
 * Feel free to extend the function arguments or return type.
 */
void print_TLB_missrate()
{	
    /*Part 2 Code here to calculate and print the TLB miss rate*/
    double miss_rate;
    fprintf(stderr,"lookups: %d misses: %d ", lookups, misses);
    if(lookups != 0) miss_rate = (double)misses/(double)lookups;
    else miss_rate = 0;
    fprintf(stderr, "TLB miss rate: %lf \n", miss_rate);
}

/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/
pte_t *translate(pde_t *pgdir, void *va) {
    /* Part 1 HINT: Get the Page directory index (1st level) Then get the
    * 2nd-level-page table index using the virtual address.  Using the page
    * directory index and page table index get the physical address.
    *
    * Part 2 HINT: Check the TLB before performing the translation. If
    * translation exists, then you can return physical address from the TLB.
    */
    
    if(!pgdir || !va) return NULL; //null pointer check
    unsigned long virtual_address = (unsigned long) va;
    unsigned long page_offset = get_low_order_bits(virtual_address, page_offset_bits);
    unsigned long physical_address = (unsigned long) check_TLB(va);
    if(physical_address) //if TLB hit, return the physical address from the TLB translation
    {
        physical_address += page_offset;
        return (pte_t*) physical_address;
    }
    //Calculate virtual page number and page table info
    unsigned long page_directory_index = get_high_order_bits(virtual_address, page_directory_bits);
    unsigned long page_table_index = get_mid_order_bits(virtual_address, page_offset_bits, page_table_bits);
    unsigned long VPN = calculate_VPN(virtual_address, page_offset_bits);
    //Check if a valid virtual page mapping exists
    //If translation not successful, then return NULL
    if(!valid_VPN(VPN) || !virtual_page_is_allocated(VPN, virtual_bitmap))
    {
        printf("Invalid virtual address\n");
        return NULL;
    }
    else
    {
        //Traverse the page directory to find page info and determine physical address
        pde_t page_directory_entry = pgdir[page_directory_index];
        if(!page_directory_entry) 
        {
            if(DEBUG) printf("Invalid page directory entry\n"); 
            return NULL;
        }
        unsigned long page_table_number = get_low_order_bits(page_directory_entry, page_directory_bits + page_table_bits);
        pte_t *page_table = (pte_t*) (memory + page_table_number * PGSIZE);
        pte_t page_table_entry = page_table[page_table_index];
        if(!page_table_entry)
        {
            if(DEBUG) printf("Invalid page table entry\n"); 
            return NULL;
        }
        unsigned long PPN = get_low_order_bits(page_table_entry, page_directory_bits + page_table_bits);
        physical_address = (unsigned long) ((memory + PPN * PGSIZE) + page_offset);
        if(DEBUG) printf("page num: %lu, phys addr: %lu\n", PPN, physical_address);
        return (pte_t*) physical_address;
    }
    printf("Virtual address translation failed\n");
    return NULL; 
}

/*
The function takes a page directory address, virtual address, physical address
as an argument, and sets a page table entry. This function will walk the page
directory to see if there is an existing mapping for a virtual address. If the
virtual address is not present, then a new entry will be added
*/
int page_map(pde_t *pgdir, void *va, void *pa)
{

    /*HINT: Similar to translate(), find the page directory (1st level)
    and page table (2nd-level) indices. If no mapping exists, set the
    virtual to physical mapping */

    if(!pgdir || !va || !pa) return -1; //null pointer check
    //Calculate virtual page number and page table info
    unsigned long virtual_address = (unsigned long) va;
    unsigned long physical_address = (unsigned long) pa;
    if(DEBUG) printf("Attempting to map virtual address %lu to physical address %lu\n", virtual_address, physical_address);
    unsigned long page_directory_index = get_high_order_bits(virtual_address, page_directory_bits);
    unsigned long page_table_index = get_mid_order_bits(virtual_address, page_offset_bits, page_table_bits);
    unsigned long VPN = calculate_VPN(virtual_address, page_offset_bits);
    if(DEBUG) printf("page dir index: %lu, page table index: %lu, vpn: %lu\n", page_directory_index, page_table_index, VPN);
    if(valid_VPN(VPN) && virtual_page_is_allocated(VPN, virtual_bitmap))
    {
        //Traverse the page directory to set the mappings
        pde_t page_directory_entry = pgdir[page_directory_index];
        if(DEBUG) printf("page dir entry: %lu\n", (unsigned long) page_directory_entry);
        page_directory_entry = allocate_page_table(pgdir, page_directory_index, page_directory_entry);
        unsigned long page_table_number = get_low_order_bits(page_directory_entry, page_directory_bits + page_table_bits);
        pte_t *page_table = (pte_t*) (memory + page_table_number * PGSIZE);
        pte_t page_table_entry = page_table[page_table_index];
        unsigned long PPN = ((pte_t) pa - (pte_t) memory) / PGSIZE;
        if(page_table_entry != (pte_t) PPN) page_table[page_table_index] = (pte_t) PPN;
        if(DEBUG) printf("page table num: %lu, page table entry: %lu, page num: %lu\n", page_table_number, (unsigned long) page_table_entry, PPN);
        //Add new entry to TLB based on what we're mapping
        add_TLB(va, pa);
        return 0;
    }
    printf("Invalid virtual page\n");
    return -1;
}

/*Function that gets the next available page
*/
void *get_next_avail(int num_pages, unsigned long *physical_page_addresses) {
 
    //Use virtual address bitmap to find the next free page

    if(DEBUG) printf("Attempting to find free pages to allocate\n");
    //Need to find the first (num_pages) of unallocated virtual pages
    unsigned long first_VPN = 0;
    int i = 0;
    int free_pages_found = find_free_virtual_pages(i, num_pages, &first_VPN);
    if(free_pages_found < num_pages)
    {
        printf("Not enough contiguous free virtual pages\n");
        return NULL;
    }
    //Find num_pages free physical pages
    int physical_page_indices[num_pages];
    int physical_pages_found = get_free_physical_pages(i, num_pages, physical_page_addresses, physical_page_indices);
    if(physical_pages_found < num_pages)
    {
        printf("Not enough free physical pages\n");
        return NULL;
    }
    //Save the physical page addresses corresponding to the free pages found
    for(i = 0; i < num_pages; i++) physical_page_addresses[i] = (unsigned long) (memory + (physical_page_indices[i] * PGSIZE));
    //Set bits in virtual & physical bitmaps corresponding to the pages found to 1 to mark that they are in use
    for(i = first_VPN; i < first_VPN + num_pages; i++)
    { 
        if(DEBUG) printf("virt bitmap index: %d\n", i);
        set_bit_at_index(virtual_bitmap, i);
    }
    for(i = 0; i < num_pages; i++) 
    {
        if(DEBUG) printf("phys bitmap index: %d\n", physical_page_indices[i]);
        set_bit_at_index(physical_bitmap, physical_page_indices[i]);
    }
    //Calculate the virtual address based on the index of the first free virtual page found (the first vpn)
    unsigned long virtual_address = calculate_virtual_address(first_VPN);
    if(DEBUG)
    {
        printf("1st found free page virtual address: %lu\n", virtual_address);
        for(i = 0; i < num_pages; i++)
        {
            printf("Page %d physical address: %lu\n", physical_page_indices[i], physical_page_addresses[i]);
        }
    }
    return (void*) virtual_address;
}

/* Function responsible for allocating pages
and used by the benchmark
*/
void *t_malloc(unsigned int num_bytes) {

    /* 
     * HINT: If the physical memory is not yet initialized, then allocate and initialize.
     */

   /* 
    * HINT: If the page directory is not initialized, then initialize the
    * page directory. Next, using get_next_avail(), check if there are free pages. If
    * free pages are available, set the bitmaps and map a new page. Note, you will 
    * have to mark which physical pages are used. 
    */

    if(num_bytes == 0) return NULL; //doesn't make sense to allocate 0 bytes
    pthread_mutex_lock(&mutex); //Needs to be thread safe
    if(initial_call) set_physical_mem();
    pthread_mutex_unlock(&mutex);
    //How many pages do we need
    int num_pages = num_bytes / PGSIZE;
    int remainder = num_bytes % PGSIZE;
    if(remainder) num_pages++;
    unsigned long physical_page_addresses[num_pages];
    pthread_mutex_lock(&mutex);
    //get the first num_bytes free contiguous pages
    unsigned long virtual_address = (unsigned long) get_next_avail(num_pages, physical_page_addresses);
    //Check if virtual address is valid
    if(virtual_address)
    {
        //Map the virtual addresses to the physical addresses
        for(int i = 0; i < num_pages; i++)
        {
            if(page_map(page_directory, (void*) (virtual_address + (i*PGSIZE)), (void*) (physical_page_addresses[i])) == -1)
            {
                printf("Page mapping failed\n");
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
        }
        pthread_mutex_unlock(&mutex);
        return (void*) virtual_address;
    }
    //Not enough free contiguous physical or virtual pages
    pthread_mutex_unlock(&mutex);
    if(DEBUG) printf("Not enough free pages\n");
    return NULL;
}

/* Responsible for releasing one or more memory pages using virtual address (va)
*/
void t_free(void *va, int size) {

    /* Part 1: Free the page table entries starting from this virtual address
     * (va). Also mark the pages free in the bitmap. Perform free only if the 
     * memory from "va" to va+size is valid.
     *
     * Part 2: Also, remove the translation from the TLB
     */
    
    if(!va || size <= 0) return; //null pointer check
    pte_t *physical_address;
    int temp = PGSIZE;
    unsigned long virtual_address = (unsigned long) va;
    //How many pages are we freeing
    int num_pages = size / PGSIZE;
    int remainder = size % PGSIZE;
    if(remainder) num_pages++;
    unsigned long PPN = 0;
    unsigned long VPN = calculate_VPN(virtual_address, page_offset_bits);
    for(int i = 0; i < num_pages; i++, VPN++)
    {
        //Obtain the physical address translation
        physical_address = translate(page_directory, (void*) virtual_address);
        if(!physical_address) return;
        PPN = ((pte_t) physical_address - (pte_t) memory) / PGSIZE;
        if(DEBUG) printf("vpn: %lu, ppn: %lu\n", VPN, PPN);
        pthread_mutex_lock(&mutex);
        if(!get_bit_at_index(virtual_bitmap, VPN) || !get_bit_at_index(physical_bitmap, PPN)) 
        {
            //don't do anything if pages are not already allocated
            if(DEBUG) printf("Invalid virtual or physical page\n");
            pthread_mutex_unlock(&mutex);
            return;
        }
        //Clear bits of pages in bitmaps
        clear_bit_at_index(virtual_bitmap, VPN);
        clear_bit_at_index(physical_bitmap, PPN);
        pthread_mutex_unlock(&mutex);
        //invalidating tlb entries associated with this virtual address
        int tag = VPN % TLB_ENTRIES;
        if(mem_tlb.entries[tag].valid && mem_tlb.entries[tag].physical_address == (unsigned long) physical_address) mem_tlb.entries[tag].valid = 0;
        if(remainder) temp = (i+1 == num_pages) ? remainder : PGSIZE;
        virtual_address += temp;
    }
}

/* The function copies data pointed by "val" to physical
 * memory pages using virtual address (va)
 * The function returns 0 if the put is successfull and -1 otherwise.
*/
int put_value(void *va, void *val, int size) {

    /* HINT: Using the virtual address and translate(), find the physical page. Copy
     * the contents of "val" to a physical page. NOTE: The "size" value can be larger 
     * than one page. Therefore, you may have to find multiple pages using translate()
     * function.
     */
    
    if(!va || !val || size <= 0) return -1;
    pte_t *physical_address;
    if(size <= PGSIZE)
    {
        physical_address = translate(page_directory, va);
        if(!physical_address) return -1;
        memcpy((void*) physical_address, val, size);
    }
    else
    {
        char *temp_val = val;
        int temp = PGSIZE;
        int num_pages = size / PGSIZE;
        int remainder = size % PGSIZE;
        if(remainder) num_pages++;
        for(int i = 0; i < num_pages; i++)
        {
            physical_address = translate(page_directory, va);
            if(!physical_address) return -1;
            if(remainder) temp = (i+1 == num_pages) ? remainder : PGSIZE;
            memcpy((void*) physical_address, temp_val, temp);
            temp_val += temp;
        }
    }
    return 0;
    /*return -1 if put_value failed and 0 if put is successfull*/
}

/*Given a virtual address, this function copies the contents of the page to val*/
void get_value(void *va, void *val, int size) {

    /* HINT: put the values pointed to by "va" inside the physical memory at given
    * "val" address. Assume you can access "val" directly by derefencing them.
    */

    if(!va || !val || size <= 0) return;
    pte_t *physical_address;
    if(size <= PGSIZE)
    {
        physical_address = translate(page_directory, va);
        if(!physical_address) return;
        memcpy(val, (void*) physical_address, size);
    }
    else
    {
        char *temp_val = val;
        int temp = PGSIZE;
        int num_pages = size / PGSIZE;
        int remainder = size % PGSIZE;
        if(remainder) num_pages++;
        for(int i = 0; i < num_pages; i++)
        {
            physical_address = translate(page_directory, va);
            if(!physical_address) return;
            if(remainder) temp = (i+1 == num_pages) ? remainder : PGSIZE;
            memcpy(temp_val, (void*) physical_address, temp);
            temp_val += temp;
        }
    }
}

/*
This function receives two matrices mat1 and mat2 as an argument with size
argument representing the number of rows and columns. After performing matrix
multiplication, copy the result to answer.
*/
void mat_mult(void *mat1, void *mat2, int size, void *answer) {

    /* Hint: You will index as [i * size + j] where  "i, j" are the indices of the
     * matrix accessed. Similar to the code in test.c, you will use get_value() to
     * load each element and perform multiplication. Take a look at test.c! In addition to 
     * getting the values from two matrices, you will perform multiplication and 
     * store the result to the "answer array"
     */
    int x, y, val_size = sizeof(int);
    int i, j, k;
    for (i = 0; i < size; i++) {
        for(j = 0; j < size; j++) {
            unsigned int a, b, c = 0;
            for (k = 0; k < size; k++) {
                int address_a = (unsigned int)mat1 + ((i * size * sizeof(int))) + (k * sizeof(int));
                int address_b = (unsigned int)mat2 + ((k * size * sizeof(int))) + (j * sizeof(int));
                get_value( (void *)address_a, &a, sizeof(int));
                get_value( (void *)address_b, &b, sizeof(int));
                if(DEBUG) printf("Values at the index: %d, %d, %d, %d, %d\n", 
                                 a, b, size, (i * size + k), (k * size + j));
                c += (a * b);
            }
            int address_c = (unsigned int)answer + ((i * size * sizeof(int))) + (j * sizeof(int));
            if(DEBUG) printf("This is the c: %d, address: %x!\n", c, address_c);
            put_value((void *)address_c, (void *)&c, sizeof(int));
        }
    }
}

unsigned long get_high_order_bits(unsigned long value, int mask_bits)
{
    return value >> (ADDR_SPACE - mask_bits);
}

unsigned long get_mid_order_bits(unsigned long value, int mask_bits, int mid_bits)
{
    value >>= mask_bits;
    return value & ((1 << mid_bits) - 1);
}

unsigned long get_low_order_bits(unsigned long value, int low_bits)
{
    return value & ((1 << low_bits) - 1);
}

unsigned long calculate_VPN(unsigned long virtual_address, int offset_bits)
{
    return virtual_address >> offset_bits;
}

void set_bit_at_index(char *bitmap, int index)
{
    int map_index = (index/8);
    index -= (map_index)*8; 
    unsigned long value_to_change = bitmap[map_index];
    unsigned long num = 1 << index;
    bitmap[map_index] = value_to_change | num;
}

void clear_bit_at_index(char *bitmap, int index)
{
    int map_index = (index/8);
    index -= (map_index)*8;
    unsigned long value_to_change = bitmap[map_index];
    unsigned long num = ~(1 << index);
    bitmap[map_index] = value_to_change & num;
}

int get_bit_at_index(char *bitmap, int index)
{
    int map_index = (index/8);
    index -= (map_index)*8;
    unsigned long val = bitmap[map_index];
    return (val >> index) & 1;
}

void initialize_memory()
{
    //Initialize memory and bitmaps
    memory = malloc(sizeof(char) * MEMSIZE); //1GB physical memory
    virtual_pages = MAX_MEMSIZE / PGSIZE;
    physical_pages = MEMSIZE / PGSIZE;
    //bitmap->each bit represents a page, char holds 8 bits
    int physical_bitmap_size = physical_pages / 8;
    int virtual_bitmap_size = virtual_pages / 8;
    physical_bitmap = malloc(physical_bitmap_size);
    virtual_bitmap = malloc(virtual_bitmap_size);
    memset(physical_bitmap, 0, physical_bitmap_size);
    memset(virtual_bitmap, 0, virtual_bitmap_size);
    set_bit_at_index(virtual_bitmap, 0);
    set_bit_at_index(physical_bitmap, 0);
    if(DEBUG)
    {
        printf("virtual pages: %d, physical pages: %d\n", virtual_pages, physical_pages);
        printf("virtual bitmap entries: %d, physical bitmap entries: %d\n", virtual_bitmap_size, physical_bitmap_size);
    }
}

void calculate_page_table_info()
{
    //Calculate page table info
    //offset bits = lg(PGSIZE) -> 2^offsetbits = PGSIZE
    int temp = PGSIZE;
    page_offset_bits = log_2(temp);
    virtual_page_number_bits = ADDR_SPACE - page_offset_bits;
    page_directory_bits = virtual_page_number_bits / 2;
    page_table_bits = ADDR_SPACE - page_directory_bits - page_offset_bits;
    page_directory_entries = 1 << page_directory_bits; //page directory entries = 2^page directory bits
    page_table_entries = 1 << page_table_bits; //page table entries = 2^page table bits
    if(DEBUG)
    {
        printf("page size: %d, offset bits: %d, page dir bits: %d, page table bits: %d\npage dir entries: %d, page table entries: %d\n", 
        PGSIZE, page_offset_bits, page_directory_bits, page_table_bits, page_directory_entries, page_table_entries);
    }
}

int log_2(int x)
{
    int result = 0;
    while(x > 1)
    {
        x >>= 1;
        result++;
    }
    return result;
}

int valid_VPN(unsigned long VPN)
{
    if(VPN < 0 || VPN > (virtual_pages - 1)) return 0;
    return 1;
}

int virtual_page_is_allocated(unsigned long VPN, char *bitmap)
{
    if(get_bit_at_index(bitmap, VPN)) return 1;
    return 0;
}

int find_free_virtual_pages(int i, int num_pages, unsigned long *index)
{
    //Traverse virtual bitmap to find free contiguous pages
    int free_pages = 0;
    for(i = 1; i < virtual_pages, free_pages < num_pages; i++)
    {
        if(!get_bit_at_index(virtual_bitmap, i)) 
        {
            if(!(*index)) *index = i;
            free_pages++;
        }
        else
        {
            *index = 0;
            free_pages = 0;
        }
    }
    return free_pages;
}

int get_free_physical_pages(int i, int num_pages, unsigned long *physical_page_addresses, int *physical_page_indices)
{
    //Traverse physical bitmap to find free pages
    memset(physical_page_addresses, 0, num_pages);
    memset(physical_page_indices, 0, num_pages);
    int physical_pages_found = 0;
    for(i = 0; i < physical_pages, physical_pages_found < num_pages; i++)
    {
        if(!get_bit_at_index(physical_bitmap, i))
        {
            physical_page_indices[physical_pages_found] = i;
            physical_pages_found++;
        }
    }
    return physical_pages_found;
}

pde_t allocate_page_table(pde_t *pgdir, unsigned long page_directory_index, pde_t page_directory_entry)
{
    //Allocate appropriate page table if it does not already exist
    if(!page_directory_entry)
    {
        for(int i = 1; i < physical_pages; i++)
        {
            if(!get_bit_at_index(physical_bitmap, i)) 
            {
                set_bit_at_index(physical_bitmap, i);
                page_directory_entry = (pde_t) i;
                pgdir[page_directory_index] = page_directory_entry;
                if(DEBUG)
                {
                    printf("Allocating new page table for page dir entry\n");
                    printf("page dir entry: %lu\n", (unsigned long) page_directory_entry);
                }
                break;
            }
        }
    }
    return page_directory_entry;
}

unsigned long calculate_virtual_address(unsigned long VPN)
{
    //bit manipulation to calculate virtual address based on the VPN
    unsigned long page_directory_index = VPN / page_table_entries;
    unsigned long page_table_index = VPN % page_table_entries;
    unsigned long page_directory_address = page_directory_index << (ADDR_SPACE - page_directory_bits);
    unsigned long page_table_address = page_table_index << page_offset_bits;
    return page_directory_address | page_table_address;
}

static void print_tlb_entries() {
    for(int i=0; i<TLB_ENTRIES; i++) {
        printf("\n%d: %ld", i, mem_tlb.entries[i].physical_address);
    }
}