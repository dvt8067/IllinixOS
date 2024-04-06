#include "idt.h"
#include "x86_desc.h"   //need to include so we can modify ESP0 field of TSS -- wyatt
#include "syscalls.h"

#define     VIDEO               0xB8000
#define     KEYBOARD_PORT       0x60       //WYATT ADDED
#define     NUM_COLS            80
#define     NUM_ROWS            25
#define     MAX_BUFF_SIZE       128
#define     SPEC_CHAR_OFFSET    54


#define KERNEL_CS   0x0010
#define KERNEL_DS   0x0018
#define USER_CS     0x0023
#define USER_DS     0x002B
#define KERNEL_TSS  0x0030
#define KERNEL_LDT  0x0038


#define         RESERVED4MASK               0x1F // kill bits 7-5
#define         NUMBER_OF_VECTORS           256
#define         NUMBER_OF_EXCEPTIONS_DEFINING        20      //based on what the CA said, it seems like we only need these 20 exceptions 
#define         NUMBER_OF_SYS_CALLS         10 // for letter check points, for now we just have a simply handle for sys calls -- James


//TABLE for the keyboard handler
const char table_kb[] = {'\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
, '0', '-', '=', '\0', '\0', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o',
'p', '[', ']', '\0', '\0', 'a', 's', 'd', 'f', 'g', 'h' , 'j', 'k' ,'l', ';'
, '\'', '`', '\0', '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/','\0', '\0',
'!', '@', '#', '$', '%', '^', '&', '*', '(' 
, ')', '_', '+', '\0', '\0', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O',
'P', '{', '}', '\0', '\0', 'A', 'S', 'D', 'F', 'G', 'H' , 'J', 'K' ,'L', ':'
, '"', '~', '\0', '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?'};


/*
kb_handler

Description: Handles keyboard interrupts.
    For now, this function will only take in the scancode and print the corresponding character to the screen.
Inputs: None
Outputs: None
Side effects: Handles the exception/interrupt raised by the keyboard. Upon the program receiving an exception/interrupt,
    it will jump to the keyboard handler to deal with the exception/interrupt
*/

// variables that keep track of whether shift, cap, or control is pressed
int shift = 0;
int cap = 0;
int ctrl = 0;
// variables that keep track of the x and y position of the cursor
uint16_t x;
uint16_t y;
uint16_t og_x;
uint16_t og_y;

int setup = 1;

void kb_handler() {

    unsigned char key = inb(KEYBOARD_PORT);
    if (setup) {
        uint16_t p = get_cursor_position();
        og_x = p % NUM_COLS;
        og_y = p / NUM_COLS;
        setup = 0;
    }

    // if tab is pressed
    if (key == 0x0F) {
        int i;
        for (i = 0; i < 4; i++) { // i < 4 because tab prints 4 spaces
            if (kb_idx != MAX_BUFF_SIZE - 1) {
                putc(' ');
                kb_buff[kb_idx] = ' ';
                kb_idx++;
            }
        }
        send_eoi(1);
        return;
    }
    // if space is pressed
    if (key == 0x39) {
        if (kb_idx != MAX_BUFF_SIZE - 1) { // if buffer isn't full
            kb_buff[kb_idx] = ' ';
            kb_idx++;
            putc(' ');
        }
    }

    // if backspace is pressed
    if (key == 0x0E) {
        uint16_t pos = get_cursor_position();
        x = pos % NUM_COLS;
        y = pos / NUM_COLS;
        if ((x-1 >= og_x) && (y >= og_y || y-1 >= og_y)) {  //THIS LINE WAS CHANGED AT 6:55 PM ON 4/6/2024 TO REMOVE A COMPILER WARNING -- WE ADDED BRACKETS
            if (x == 0 && y != 0) { // any other row
                update_xy(NUM_COLS - 1, y-1);
                putc(' ');
                if (y-1 >= og_y) { // anything below user_y space we can delete
                    update_xy(NUM_COLS - 1, y-1);
                    update_cursor(NUM_COLS - 1, y-1);
                }
            } else { // just deleting charcter in a row that doesn't go to other rows
                update_xy(x-1, y);
                putc(' ');
                update_xy(x-1, y);
                update_cursor(x-1, y);
            }
            if (kb_idx != 0) { // if buffer isn't empty already
                kb_idx -= 1;
                kb_buff[kb_idx] = '\t'; // code for not print anything
            }
        }
        send_eoi(1);
        return;
    }

    // if enter is pressed
    if (key == 0x1C) {
        uint16_t pos = get_cursor_position();
        x = pos % NUM_COLS;
        y = pos / NUM_COLS;
        if (y != 0) {
            putc('\n'); // prepare a new line to print buf
        }
        if (y == 0 && x != 0) {
            putc('\n');
        }
        kb_buff[kb_idx] = '\n';
        // user_y += 2; // add 2 because we need to print the buffer value but also move to a new line

        kb_idx = 0;
        setup = 1;
        send_eoi(1);
        return;
    }

    // if LEFT or RIGHT ctrl pressed
    if (key == 0x1D) {
        ctrl = 1;
        send_eoi(1);
        return;
    // if LEFT or RIGHT ctrl released
    } else if (key == 0x9D) {
        ctrl = 0;
        send_eoi(1);
        return;
    }

    // if right or left shift is pressed
    if (key == 0x36 || key == 0x2A) {
        shift = 1;
        send_eoi(1);
        return;
    // right or left shift is released
    } else if (key == 0xAA || key == 0xB6) {  
        shift = 0;
        send_eoi(1);
        return;
    }
    // pressing caps lock
    if (key == 0x3A) {
        cap = !cap;
        send_eoi(1);
        return;
    }

    // clear screen operation
    if (ctrl && key == 0x26) {
        // reset everything to top left of screen
        clear();
        update_xy(0, 0);
        update_cursor(0, 0);
        // user_y = 0;
        send_eoi(1);
        // uint8_t string[1];
        // string[0] = '\n';
        // t_write(1, string, 1);
        return;
    }

    // caps open and pressing shift
    if (cap && shift) {
        if (key <= 0x37) { // if it's within our non special character bound
            char p = table_kb[key];
            // check that it's a printable character
            if (p != '\0') {
                // check if it's a number/symbol   
                if (p == '1' || p == '2' || p == '3' || p == '4' || p == '5' || p == '6' || p == '7'
                 || p == '8' || p == '9' || p == '0' || p == '-' || p == '=' || p == '[' || p == ']' || p == '\\'
                 || p == ';' || p == '\'' || p == ',' || p == '.' || p == '/') {
                    p = table_kb[key + SPEC_CHAR_OFFSET];
                }

                if (kb_idx != MAX_BUFF_SIZE - 1) { // if buffer isn't full
                    kb_buff[kb_idx] = p;
                    kb_idx++;
                    putc(p);
                }
            }
        }   
    // words all capped, symbols are normal
    } else if (cap) {
        if (key <= 0x37) { // if it's within our non special character bound
            char p = table_kb[key];
            // check that it's a printable character
            if (p != '\0') {
                // check if it's a number/symbol   
                if (!(p == '1' || p == '2' || p == '3' || p == '4' || p == '5' || p == '6' || p == '7'
                 || p == '8' || p == '9' || p == '0' || p == '-' || p == '=' || p == '[' || p == ']' || p == '\\'
                 || p == ';' || p == '\'' || p == ',' || p == '.' || p == '/')) {
                    p = table_kb[key + SPEC_CHAR_OFFSET];
                }

                if (kb_idx != MAX_BUFF_SIZE - 1) { // if buffer isn't full
                    kb_buff[kb_idx] = p;
                    kb_idx++;
                    putc(p);
                }
            }
        }       
    // words all capped, symbols are diff
    } else if (shift) {
        if (key <= 0x37) { // if it's within our non special character bound
            char p = table_kb[key + SPEC_CHAR_OFFSET];
            if (p != '\0') { // check it's printable character 
                if (kb_idx != MAX_BUFF_SIZE - 1) { // if buffer isn't full
                    kb_buff[kb_idx] = p;
                    kb_idx++;
                    putc(p);
                }
            }
        }
    // normal behav
    } else {
        if (key <= 0x37) { // if it's within our non special character bound
            char p = table_kb[key];
            if (p != '\0') { // check it's printable character
                if (kb_idx != MAX_BUFF_SIZE - 1) { // if buffer isn't full
                    kb_buff[kb_idx] = p;
                    kb_idx++;
                    putc(p);
                }
            }
        }
    }
    send_eoi(1);
}



/*
Exception Handlers

Description: Each of the handlers below are called whenever an exception arises.
    Every handler corresponds to a specific exception. Some data will be printed to the
    screen regarding the nature of the current exception.
Inputs: None
Outputs: None
Side effects: Handles the exception/interrupt raised by the system. Upon the program receiving an exception/interrupt,
    it will jump to a specific handler to deal with the exception/interrupt

*/
void exec_handler0() {
    //asm volatile("pushal") ;
    //asm volatile("pushfl");
    printf("\n EXCEPTION 0: A DIVIDE BY ZERO EXCEPTION HAS OCCURED \n");
    //asm volatile("popfl") ;
    //asm volatile("popal") ;
    //asm volatile("iret") ;
    asm volatile("movl $256, %eax");
    sys_halt(0);
    // asm volatile ("iret \n\  ");
}
void exec_handler1() {
    // asm("pushal") ;
    // asm("pushfl");
    printf("\n EXCEPTION 1: A DEBUG EXCEPTION HAS OCCURED \n");
    // asm("popfl") ;
    // asm("popal") ;
    //asm("iret") ;
    sys_halt(0);
}
void exec_handler2() {
    // asm("pushal") ;
    // asm("pushfl");
    printf("\n EXCEPTION 2: A NON-MASKABLE INTERRUPT EXCEPTION HAS OCCURED \n");
    // asm("popfl") ;
    // asm("popal") ;
    //asm("iret") ;
    sys_halt(0);
}
void exec_handler3() {
    // asm("pushal") ;
    // asm("pushfl");
    printf("\n EXCEPTION 3: A BREAKPOINT EXCEPTION HAS OCCURED \n");
    // asm("popfl") ;
    // asm("popal") ;
    //asm("iret") ;
    sys_halt(0);
}
void exec_handler4() {
    // asm("pushal") ;
    // asm("pushfl");
    printf("\n EXCEPTION 4: AN OVERFLOW EXCEPTION HAS OCCURED \n");
    // asm("popfl") ;
    // asm("popal") ;
    //asm("iret") ;
    sys_halt(0);
}
void exec_handler5() {
    // asm("pushal") ;
    // asm("pushfl");
    printf("\n EXCEPTION 5: A BOUND RANGE EXCEEDED EXCEPTION HAS OCCURED \n");
    // asm("popfl") ;
    // asm("popal") ;
    //asm("iret") ;
    sys_halt(0);
}
void exec_handler6() {
    // asm("pushal") ;
    // asm("pushfl");
    printf("\n EXCEPTION 6: AN INVALID OPCODE EXCEPTION HAS OCCURED \n");
    // asm("popfl") ;
    // asm("popal") ;
    //asm("iret") ;
    sys_halt(0);
}
void exec_handler7() {
    // asm("pushal") ;
    // asm("pushfl");
    printf("\n EXCEPTION 7: A DEVICE NOT AVAILABLE EXCEPTION HAS OCCURED \n");
    // asm("popfl") ;
    // asm("popal") ;
    //asm("iret") ;
    sys_halt(0);
}
void exec_handler8() {
    // asm("pushal") ;
    // asm("pushfl");
    printf("\n EXCEPTION 8: A DOUBLE FAULT EXCEPTION HAS OCCURED \n");
    // asm("popfl") ;
    // asm("popal") ;
    //asm("iret") ;
    sys_halt(0);
}
void exec_handler9() {
    // asm("pushal") ;
    // asm("pushfl");
    printf("\n EXCEPTION 9: A SUSPICIOUS ERROR HAS OCCURED \n");
    // asm("popfl") ;
    // asm("popal") ;
    //asm("iret") ;
    sys_halt(0);
}
void exec_handler10() {
    // asm("pushal") ;
    // asm("pushfl");
    printf("\n EXCEPTION 10:INVALID TSS EXCEPTION HAS OCCURED \n");
    // asm("popfl") ;
    // asm("popal") ;
    //asm("iret") ;
    sys_halt(0);
}
void exec_handler11() {
    // asm("pushal") ;
    // asm("pushfl");
    printf("\n EXCEPTION 11: SEGMENT NOT PRESENT EXCEPTION HAS OCCURED \n");
    // asm("popfl") ;
    // asm("popal") ;
    //asm("iret") ;
    sys_halt(0);
}
void exec_handler12() {
    // asm("pushal") ;
    // asm("pushfl");
    printf("\n EXCEPTION 12: Stack Segment EXCEPTION HAS OCCURED \n");
    // asm("popfl") ;
    // asm("popal") ;
    //asm("iret") ;
    sys_halt(0);
}
void exec_handler13() {
    // asm("pushal") ;
    // asm("pushfl");
    printf("\n EXCEPTION 13: General Protection Fault EXCEPTION HAS OCCURED \n");
    // asm("popfl") ;
    // asm("popal") ;
    //asm("iret") ;
    sys_halt(0);
}
void exec_handler14() {
    // asm("pushal") ;
    // asm("pushfl");
    printf("\n EXCEPTION 14: Page Fault EXCEPTION HAS OCCURED \n");
    // asm("popfl") ;
    // asm("popal") ;
    //asm("iret") ;
    EXCEPTION_FLAG = 1;
    sys_halt(0);
}
void exec_handler15() {
    // asm("pushal") ;
    // asm("pushfl");
    printf("\n EXCEPTION 15: Reserved EXCEPTION HAS OCCURED \n");
    sys_halt(0);
}
void exec_handler16() {
    // asm("pushal") ;
    // asm("pushfl");
    printf("\n EXCEPTION 16: x87 Floating point EXCEPTION HAS OCCURED \n");
    // asm("popfl") ;
    // asm("popal") ;
    //asm("iret") ;
    sys_halt(0);
}
void exec_handler17() {
    // asm("pushal") ;
    // asm("pushfl");
    printf("\n EXCEPTION 17: Alignment Check EXCEPTION HAS OCCURED \n");
    // asm("popfl") ;
    // asm("popal") ;
    //asm("iret") ;
    sys_halt(0);
}
void exec_handler18() {
    // asm("pushal") ;
    // asm("pushfl");
    printf("\n EXCEPTION 18: Machine Check EXCEPTION HAS OCCURED \n");
    // asm("popfl") ;
    // asm("popal") ;
    //asm("iret") ;
    sys_halt(0);
}
void exec_handler19() {
    // asm("pushal") ;
    // asm("pushfl");
    printf("\n EXCEPTION 19: SIMD Floating Point EXCEPTION HAS OCCURED \n");
    // asm("popfl") ;
    // asm("popal") ;
    //asm("iret") ;
    sys_halt(0);
}



//THIS IS FOR NMI interrpt
void intr_handler() {
    // asm("pushal") ;
    // asm("pushfl");
    printf("This is an intr handler moment");
    // asm("popfl") ;
    // asm("popal") ;
    //asm("iret") ;
    while(1){}
}
/*
*   Description:    This function initializes the external array idt that is defined in x86_desc.h and fills in the idt with what it needs to be filled in as
*   Inputs:         None
*   Outputs:        None
*   Side Effects:   The idt array will be set and ready to be imported into the idt table that is defined in x86_desc.S
* 
*/
void initialize_idt(){ // need to set all 256 to something, zero everything out and then specify the ones we care about

    //printf("\n THE INITIALIZE IDT FUNCTION IS BEING CALLED \n");
    int i;
    //int j;
    for(i=0; i< NUMBER_OF_VECTORS; i++){ // Initially zero out every single vector in the idt
        idt[i].val[0] = 0x00000000;
        idt[i].val[1] = 0x00000000;
    }
    for(i=0;i <= NUMBER_OF_EXCEPTIONS_DEFINING; i++){
        // 15th exception is reserved so set present to 0
        if (i == 15 || i == 20) {
           (&(idt[i]))->present = 0;
        } else {
            set_exception_params(&idt[i], i);
        }
    }
    // 0x80 for Sys_Call
    idt_desc_t * idt_array_index = &(idt[0x80]);
    idt_array_index->seg_selector = KERNEL_CS; //This represents the kernel CS <- i think this is defined in x86_desc?
    idt_array_index->reserved4 = 0;
    idt_array_index->reserved3 = 1; // 0 corresponds to interrupt, 1 is trap
    idt_array_index->reserved2 = 1; // RESERVED BITS 0-2 are specified on intel's x86 documentation
    idt_array_index->reserved1 = 1;
    idt_array_index->size = 1; // Means we are in 32 bit mode
    idt_array_index->reserved0 = 0;
    
    idt_array_index->dpl = 3; // this one is also going to depend on syscall vs trap/interrupt
    idt_array_index->present = 1; // 90% sure this bit needs to be 1 or else it won't like the address
    SET_IDT_ENTRY((*idt_array_index), sys_call);


    //SETTING UP THE KEYBOARD FOR DEVICES
    idt_array_index = &(idt[0x21]);
    idt_array_index->seg_selector = KERNEL_CS; //This represents the kernel CS <- i think this is defined in x86_desc?
    idt_array_index->reserved4 = 0;
    idt_array_index->reserved3 = 0; // 0 corresponds to interrupt, 1 is trap
    idt_array_index->reserved2 = 1; // RESERVED BITS 0-2 are specified on intel's x86 documentation
    idt_array_index->reserved1 = 1;
    idt_array_index->size = 1; // Means we are in 32 bit mode
    idt_array_index->reserved0 = 0;
    
    idt_array_index->dpl = 3; // this one is also going to depend on syscall vs trap/interrupt
    idt_array_index->present = 1; // 90% sure this bit needs to be 1 or else it won't like the address
    SET_IDT_ENTRY((*idt_array_index), keyboard_call);
    //SETTING UP THE RTC FOR THE HANDLER
    idt_array_index = &(idt[0x28]);
    idt_array_index->seg_selector = KERNEL_CS; //This represents the kernel CS <- i think this is defined in x86_desc?
    idt_array_index->reserved4 = 0;
    idt_array_index->reserved3 = 0; // 0 corresponds to interrupt, 1 is trap
    idt_array_index->reserved2 = 1; // RESERVED BITS 0-2 are specified on intel's x86 documentation
    idt_array_index->reserved1 = 1;
    idt_array_index->size = 1; // Means we are in 32 bit mode
    idt_array_index->reserved0 = 0;
    
    idt_array_index->dpl = 0; // this one is also going to depend on syscall vs trap/interrupt
    idt_array_index->present = 1; // 90% sure this bit needs to be 1 or else it won't like the address
    SET_IDT_ENTRY((*idt_array_index), rtc_call);
}

/*
*   Description:    This function initializes the external array idt[0->20] that is defined in x86_desc.h and fills in these exceptions with what it needs to be filled in as
*   Inputs:         None
*   Outputs:        None
*   Side Effects:   The idt[0->20] array will be set and ready to be imported into the idt table that is defined in x86_desc.S
*   https://wiki.osdev.org/Exceptions // FILL THESE VALUES IN WITH THE INFO FROM THIS
*/      
void set_exception_params(idt_desc_t * idt_array_index, int vec){
    switch(vec) {
    case 0:
        SET_IDT_ENTRY((*idt_array_index), de);
        break;
    case 1:
        SET_IDT_ENTRY((*idt_array_index), db);
        break;
    case 2:
        SET_IDT_ENTRY((*idt_array_index), nmi);
        break;
    case 3:
        SET_IDT_ENTRY((*idt_array_index), bp);
        break;
    case 4:
        SET_IDT_ENTRY((*idt_array_index), of);
        break;
    case 5:
        SET_IDT_ENTRY((*idt_array_index), br);
        break;
    case 6:
        SET_IDT_ENTRY((*idt_array_index), ud);
        break;
    case 7:
        SET_IDT_ENTRY((*idt_array_index), nm);
        break;
    case 8:
        SET_IDT_ENTRY((*idt_array_index), df);
        break;
    case 9:
        SET_IDT_ENTRY((*idt_array_index), cso);
        break;
    case 10:
        SET_IDT_ENTRY((*idt_array_index), ts);
        break;
    case 11:
        SET_IDT_ENTRY((*idt_array_index), np);
        break;
    case 12:
        SET_IDT_ENTRY((*idt_array_index), ss);
        break;
    case 13:
        SET_IDT_ENTRY((*idt_array_index), gp);
        break;
    case 14:
        SET_IDT_ENTRY((*idt_array_index), pf);
        break;
    case 15:
        SET_IDT_ENTRY((*idt_array_index), mf);
        break;
    case 16:
        SET_IDT_ENTRY((*idt_array_index), mf);
        break;
    case 17:
        SET_IDT_ENTRY((*idt_array_index), ac);
        break;
    case 18:
        SET_IDT_ENTRY((*idt_array_index), mc);
        break;
    case 19:
        SET_IDT_ENTRY((*idt_array_index), xf);
        break;
    default:
        // Nothing here for default case
        break;
}


    idt_array_index->seg_selector = KERNEL_CS; //This represents the kernel CS <- i think this is defined in x86_desc?
    
    idt_array_index->reserved4 = 0;
    idt_array_index->reserved3 = 1; // 1 corresponds to trap gate
    idt_array_index->reserved2 = 1; // RESERVED BITS 0-2 are specified on intel's x86 documentation
    idt_array_index->reserved1 = 1;
    idt_array_index->size = 1; // Means we are in 32 bit mode
    idt_array_index->reserved0 = 0;
    
    idt_array_index->dpl = 0; // this one is also going to depend on syscall vs trap/interrupt
    idt_array_index->present = 1; // 90% sure this bit needs to be 1 or else it won't like the address
    
}


