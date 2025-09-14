#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

//////////////////////////////////////////////////////////////////////////////

// No autotest, but you might want to set this anyway.
const char* username = "username";

// When testing basic TX/RX
#define STEP2
// When connecting UART to printf(), fgets()
// #define STEP3
// When testing UART IRQ for buffering
// #define STEP4
// When testing PCS
// #define STEP5

//////////////////////////////////////////////////////////////////////////////

void init_uart() {
    // fill in
}

#ifdef STEP2
int main() {
    init_uart();
    for (;;) {
        char buf[2];
        uart_read_blocking(uart0, (uint8_t*)buf, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0'; // Ensure null-termination
        uart_puts(uart0, "You said: ");
        uart_puts(uart0, buf);
        uart_puts(uart0, "\n");
    }
}
#endif

//////////////////////////////////////////////////////////////////////////////

#ifdef STEP3

// 3.3
int _read(__unused int handle, char *buffer, int length) {
    // Your code here to read from the UART and fill the buffer.
    // DO NOT USE THE STDIO_* FUNCTIONS FROM ABOVE.  Only UART ones.

    // The argument "handle" is unused.  This is meant for use with 
    // files, which are not very different from text streams.  However, 
    // we read from the UART, not the file specified by the handle.

    // handle is irrelevant since these functions will only ever be called 
    // by the correct functions.  No need for an if statement.

    // Instructions: Given the buffer and a specific length to read, read 1 
    // character at a time from the UART until the buffer is 
    // filled or the length is reached. 
}

int _write(__unused int handle, char *buffer, int length) {
    // Your code here to write to the UART from the buffer.
    // DO NOT USE THE STDIO_* FUNCTIONS FROM ABOVE.  Only UART ones.

    // The argument "handle" is unused.  This is meant for use with 
    // files, which are not very different from text streams.  However, 
    // we write to the UART, not the file specified by the handle.

    // handle is irrelevant since these functions will only ever be called 
    // by the correct functions.  No need for an if statement.

    // Instructions: Given the buffer and a specific length to write, write 1
    // character at a time to the UART until the length is reached. 
}

int main()
{
    init_uart();

    // insert any setbuf lines below...

    for(;;) {
        putchar(getchar()); 
    }
}
#endif

//////////////////////////////////////////////////////////////////////////////

#ifdef STEP4

#define BUFSIZE 32
char serbuf[BUFSIZE];
int seridx = 0;
int newline_seen = 0;

// add this here so that compiler does not complain about implicit function
void uart_rx_handler();

void init_uart_irq() {
    // fill in.
}

void uart_rx_handler() {
    // fill in.
}

int _read(__unused int handle, char *buffer, int length) {
    // fill in.
}

int _write(__unused int handle, char *buffer, int length) {
    // fill in.
}

int main() {
    // fill in.
}

#endif

//////////////////////////////////////////////////////////////////////////////

#ifdef STEP5

// Copy global variables, init_uart_irq, uart_rx_handler, _read, and _write from STEP4.

void cmd_gpio(int argc, char **argv) {
    // This is the main command handler for the "gpio" command.
    // It will call either cmd_gpio_out or cmd_gpio_set based on the arguments.
    
    // Ensure that argc is at least 2, otherwise print an example use case and return.

    // If the second argument is "out":
    //      Ensure that argc is exactly 3, otherwise print an example use case and return.
    //      Convert the third argument to an integer pin number using atoi.
    //      Check if the pin number is valid (0-47), otherwise print an error and return.
    //      Set the pin to output using gpio_init and gpio_set_dir.
    //      Print a success message.
    
    // If the second argument is "set":
    //      Ensure that argc is exactly 4, otherwise print an example use case and return.
    //      Convert the third argument to an integer pin number using atoi.
    //      Check if the pin number is valid (0-47), otherwise print an error and return.
    //      Check if the pin has been initialized as a GPIO output, if not, return.
    //      Convert the fourth argument to an integer value (0 or 1) using atoi.
    //      Check if the value is valid (0 or 1), otherwise print an error and return.
    //      Set the pin to the specified value using gpio_put.
    //      Print a success message.
    
    // Else, print an unknown command error.
}

int main() {
    // See lab for instructions.
}

#endif

//////////////////////////////////////////////////////////////////////////////
