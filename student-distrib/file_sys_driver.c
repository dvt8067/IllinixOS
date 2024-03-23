
#include "file_sys_driver.h"

//Note that this doesn't require paging to be working

boot_struct * booting_info_block;

extern void get_bootblock_address(unsigned long addr){
    booting_info_block = (boot_struct*) addr;
}


void file_system_init() {
    // need to map the struct to the mod address


}


//calls dentry_by_name. this function will search for a specified directory
int32_t file_open()    {
    return 0;
}

int32_t file_read()    {
    return 0;
}


/* This is our write function, it isn't functional since our file system is read only right now 
*
*
*/
int32_t file_write()   {
    return -1;
}

int32_t file_close()   {
    return 0;
}



int32_t directory_open()    {
    return 0;
}

int32_t directory_close()   {
    return 0;
}

int32_t directory_read()    {
    return 0;
}

int32_t directory_write()   {
    return 0;
}
void print_number_of_inodes(){
	printf("This is the number of inodes: %d" , (int) booting_info_block->number_of_inodes);
}

//for read_data: we're given the inode, the offset from the start of data (beginning at the first data block, but potentially extending to other blocks),
//buf is what we're putting the data into, and length is (MAYBE) the amount of bytes you want to write into the buffer