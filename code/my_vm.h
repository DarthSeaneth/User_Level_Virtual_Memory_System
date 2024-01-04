//Sean Patrick (smp429) & Fulton Wilcox (frw14)
#ifndef MY_VM_H_INCLUDED
#define MY_VM_H_INCLUDED
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

//Assume the address space is 32 bits, so the max memory size is 4GB
//Page size is 4KB

//Add any important includes here which you may need
#include <pthread.h>
#include <string.h>

#define PGSIZE 4096

// Maximum size of virtual memory
#define MAX_MEMSIZE 4ULL*1024*1024*1024

// Size of "physcial memory"
#define MEMSIZE 1024*1024*1024

// Represents a page table entry
typedef unsigned long pte_t;

// Represents a page directory entry
typedef unsigned long pde_t;

#define TLB_ENTRIES 512

/* Macro for address space size */
#define ADDR_SPACE 32

typedef struct entry {
    int valid;
    unsigned long physical_address;
    unsigned long virt_page_num;
} entry;

//Structure to represents TLB
typedef struct tlb {
    /*Assume your TLB is a direct mapped TLB with number of entries as TLB_ENTRIES
    * Think about the size of each TLB entry that performs virtual to physical
    * address translation.
    */
    entry entries[TLB_ENTRIES];
} tlb;
//struct tlb tlb_store;   //Need to figure out why this is causing compiler error

void set_physical_mem();
pte_t* translate(pde_t *pgdir, void *va);
int page_map(pde_t *pgdir, void *va, void* pa);
void *get_next_avail(int num_pages, unsigned long *physical_page_addresses);
bool check_in_tlb(void *va);
void put_in_tlb(void *va, void *pa);
void *t_malloc(unsigned int num_bytes);
void t_free(void *va, int size);
int put_value(void *va, void *val, int size);
void get_value(void *va, void *val, int size);
void mat_mult(void *mat1, void *mat2, int size, void *answer);
void print_TLB_missrate();

/* Helper functions */

/* Gets the high order (32 - mask_bits) bits of value */
unsigned long get_high_order_bits(unsigned long value, int mask_bits);

/* Removes the last (mask_bits) bits of value and then gets the last (mid_bits) bits of value */
unsigned long get_mid_order_bits(unsigned long value, int mask_bits, int mid_bits);

/* Gets the last (low_bits) bits of value */
unsigned long get_low_order_bits(unsigned long value, int low_bits);

/* Calculates virtual page number based on offset bits and virtual address */
unsigned long calculate_VPN(unsigned long virtual_address, int offset_bits);

/* Function to set a bit at "index" bitmap */
void set_bit_at_index(char *bitmap, int index);

/* Calculates log(base_2) of x */
int log_2(int x);

/* Function to clear a bit at "index" of bitmap */
void clear_bit_at_index(char *bitmap, int index);

/* Function to get a bit at "index" */
int get_bit_at_index(char *bitmap, int index);

/* Initializes memory and bitmaps */
void initialize_memory();

/* Calculates important page table information*/
void calculate_page_table_info();

/* Checks if a VPN is valid */
int valid_VPN(unsigned long VPN);

/* Checks virtual bitmap to see if page is allocated */
int virtual_page_is_allocated(unsigned long VPN, char *bitmap);

/* Searches for the first num_pages free contiguous virtual pages */
int find_free_virtual_pages(int i, int num_pages, unsigned long *index);

/* Searches for free physical pages and stores their corresponding bitmap indices and physical addresses */
int get_free_physical_pages(int i, int num_pages, unsigned long *physical_page_addresses, int *physical_page_indices);

/* Allocates a page table for a specific page directory entry if one does not exist */
pde_t allocate_page_table(pde_t *pgdir, unsigned long page_directory_index, pde_t page_directory_entry);

/* Calculates a virtual address based on the virtual page number */
unsigned long calculate_virtual_address(unsigned long VPN);

/* Prints TLB entries */
static void print_tlb_entries();

#endif