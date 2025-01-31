#include "rtc.h"

#define     RTC_PORT            0x70
#define     CMOS                0x71
#define     REGA                0x8A
#define     REGB                0x8B


int rtc_int[3]; //global rtc interrupt flag
// int vrtc_int; //global virtualization interrupt flag
int RTC_COUNTER_MAX[3];
// int rtc_test_flag;
/*
* Description: This function initializes the RTC.
* Inputs: NONE
* Outputs: NONE
* Side Effects: RTC is initialized 
*/
 void init_rtc(void){ // initializing the RTC -- Aadhesh
    enable_irq(8);
    //cli() called prior to init_rtc()
    char prev = inb(RTC_PORT) | 0x80; //Disable NMI by setting first bit (0x80 bit) to 1 using 0x80
    outb(prev, RTC_PORT);
    inb(CMOS);

    outb(REGB, RTC_PORT); //select register B, and disable NMI
    prev = inb(CMOS); //read the current value of register B
    outb(REGB, RTC_PORT); //set the index again (a read will reset the index to register D)
    outb(prev | 0x40, CMOS); //write the previous value ORed with 0x40. This turns on bit 6 of register B

    rtc_int[0] = 0; //Initialize RTC interrupt flag
    rtc_int[1] = 0; //Initialize RTC interrupt flag
    rtc_int[2] = 0; //Initialize RTC interrupt flag
    // vrtc_int = 0; //Initialize virtualization interrupt flag
 }

/*
* Description: This function is the RTC Handler. It is used whenever the RTC generates an interrupts
* Inputs: NONE
* Outputs: NONE
* Side Effects: None
*/
void rtc_handler(){
    cli();
    // test_interrupts();
    // putc('1');
    // rtc_int[scheduled_terminal] += 3;
    // rtc_test_flag = 1;
    rtc_int[0] += 1;
    rtc_int[1] += 1;
    rtc_int[2] += 1;
    if(rtc_int[0] >= RTC_COUNTER_MAX[0]){
        // printf("RTC INIT VALUE")
        RTC_FLAG[0] = 1;
    }
    if(rtc_int[1] >= RTC_COUNTER_MAX[1]){
        // printf("RTC INIT VALUE")
        RTC_FLAG[1] = 1;
    }
    if(rtc_int[2] >= RTC_COUNTER_MAX[2]){
        // printf("RTC INIT VALUE")
        RTC_FLAG[2] = 1;
    }
    outb(0x0C, RTC_PORT); //select register C
    inb(CMOS); //just throw away contents
    //clear();
    send_eoi(8);
    sti();
}

/* Description: This is a helper function created to set the rtc to a given frequency. It was created to handle RTC Open and RTC Write
* Inputs: NONE
* Outputs: NONE
* Side Effects: None
*/
int32_t rtc_set_frequency(int32_t frequency){ //created to handle rtc_write and rtc_open -- Aadhesh
    //printf("%d", frequency);
    if(frequency < 2 || frequency > 1024) return -1; //Return failure if frequency exceeds 1024 Hz or drops below 2 Hz
    if(frequency & (frequency - 1)) return -1; //Check that frequency is power of 2 (bitwise-and frequency and frequency -1; if power of 2, bitwise operation returns 0)

    //changing interrupt rates
    int log = 0;
    while(frequency >>= 1) log++; //log2(frequency)
    uint8_t rate = 16 - log; //rate must be above 2 and not over 15

    cli(); //clear interrupts
    char prev = inb(RTC_PORT) | 0x80; //Disable NMI by setting first bit (0x80 bit) to 1 using 0x80
    outb(prev, RTC_PORT);
    inb(CMOS); 

    outb(REGA, RTC_PORT); //set index to register A
    prev = inb(CMOS); //get initial value of register A
    outb(REGA, RTC_PORT); //reset index to A
    outb((prev & 0xF0) | rate, CMOS); //write only our rate to A. Note, rate is the bottom 4 bits.
    
    prev = inb(RTC_PORT) & 0x7F; //Enable NMI by setting 0x80 bit to 0 using 0x7F
    outb(prev,RTC_PORT);
    inb(CMOS);
    sti(); //Enable all other interrupts

    return 0;
}
/* Description: This is RTC Read
* Inputs: File Descriptor, buffer, and number of bytes
* Outputs: Returns 0 for success and -1 for failure
* Side Effects: None
*/
int32_t rtc_read (int32_t fd, void* buf, int32_t nbytes){ //Aadhesh
    // int32_t curr = 0; //Stores current rtc
    // cli();
    rtc_int[scheduled_terminal] = 0;
    // sti();
    while(1){
        if(RTC_FLAG[scheduled_terminal] != 0){
            break;
        }

    }; //Waits until rtc_int value is changed by the rtc_handler
    // cli();
    // printf("\n RTC READ TIME");
    RTC_FLAG[scheduled_terminal] = 0;
    rtc_int[scheduled_terminal] = 0;
    // sti();
    return 0;

    // sti();
    
}

/* Description: This is RTC write
* Inputs: File Descriptor, buffer, and number of bytes
* Outputs: Returns 0 for success and -1 for failure
* Side Effects: None
*/
int32_t rtc_write (int32_t fd, const void * buf, int32_t nbytes){ //Aadhesh
    // rtc_set_frequency(1024);
    // cli();
    int32_t* buffer = (int32_t *) buf; //added for pingpong RTC
    if(nbytes != 4) return -1; //Return failure if more or less than 4 bytes passed in
    int32_t new_freq = *buffer;
    // printf("\n The value of the new frequency is : %d", new_freq);
    // could add some checks to make sure this makes sense
    // RTC_COUNTER_MAX[scheduled_terminal] = (1024/new_freq);
    RTC_COUNTER_MAX[scheduled_terminal] = (512/new_freq);
    // printf("\n The value of the counter is : %d", RTC_COUNTER_MAX[scheduled_terminal]);
    rtc_int[scheduled_terminal] = 0;
    // return rtc_set_frequency(*buffer); //change frequency based on buf value 
    // sti();
    // rtc_test_flag=0;
    // rtc_set_frequency(*buffer);
    return 0;
}

/* Description: This is RTC Open
* Inputs: inputs a file name
* Outputs: Returns 0 for success and -1 for failure
* Side Effects: None
*/
int32_t rtc_open (const uint8_t* filename){ //Aadhesh
    rtc_set_frequency(512); //Initialize RTC Frequency to 512 Hz
    return 0; //Return zero for success 
}

/* Description: This is RTC Close
* Inputs: inputs a file descriptor
* Outputs: Returns 0 for success and -1 for failure
* Side Effects: None
*/
int32_t rtc_close (int32_t fd){ //Aadhesh
    return 0;
}


/* Description: Process for virtualization of the RTC
* Inputs: File Descriptor, buffer, and number of bytes
* Outputs: Returns 0 for success and -1 for failure
* Side Effects: None
*/
//DAVID TOOK OUT THE CONST FROM THE BUF 04/08/2024
// int32_t vrtc_process (int32_t fd, void * buf, int32_t nbytes){ //Aadhesh
//     if(nbytes != 4) return -1; //Return failure if more or less than 4 bytes passed in
//     rtc_set_frequency(1024); //Set to max frequency
//     while(vrtc_int < *(int32_t *) buf){ // x virtual interrupts for 1 1024-Hz interrupt (x = buf)
//         rtc_read(fd,buf,nbytes);
//         vrtc_int++;
//     }
//     vrtc_int = 0; //virtual interrupt reset
//     return 0;
// }
