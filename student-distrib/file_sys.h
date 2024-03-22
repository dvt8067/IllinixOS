#include "types.h"

void file_system_init();

typedef struct inode_struct_t { // page directory entry
        uint32_t length_in_bytes;
        uint32_t zero_th_data_block;
        uint32_t first_th_data_block;
        uint32_t second_th_data_block;      //I'm CHOOSING to make this struct in right side up order because I think if we aren't messing with individual bits then it will be in order
} inode_struct_t;

typedef struct dir_entries_struct {
    char file_name[32]; //Allocating 32 bytes for file Name //might need to be uint32
    uint32_t  file_type;
    uint32_t  inode_number;
    uint8_t reserved[24];  //made this uint8 because 1 byte, but idk

} dir_entries_struct;

typedef struct boot_struct {
    uint32_t number_of_dir_entries; //Allocating 32 bytes for file Name //might need to be uint32
    uint32_t number_of_inodes;
    uint32_t number_of_data_blocks;
    uint8_t reserved[52];  //made this char, but idk
    dir_entries_struct dir_entries;

} boot_struct;
//Questions:

//do we need to implement syscalls for this?
    //I see we need to do read/write, but i thought syscalls was cp 3
    //it looks like the inode is variable length, how do I initialize that struct
    // Where are the data blocks going to be stored

    //MOD ADDRESS IS GOING TO CORRESPOND TO THE BOOT BLOCK

    //How does the boot block go at the 0th inode, but
    // inode struct as well???