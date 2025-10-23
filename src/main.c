#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

//////////////////////////////////////////////////////////////////////////////

// No autotest, but you might want to set this anyway.
const char* username = "rajago11";

// When testing basic TX/RX
// #define STEP2
// When connecting UART to printf(), fgets()
#define STEP3
// When testing UART IRQ for buffering
// #define STEP4
// When testing PCS
// #define STEP5

//////////////////////////////////////////////////////////////////////////////

void init_uart() 
{
    uart_inst_t *UART_ID = uart0;
    const uint BAUD = 115200;
    const uint TX_PIN = 0;
    const uint RX_PIN = 1;

    uart_init(UART_ID, BAUD);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);

    gpio_set_function(TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(RX_PIN, GPIO_FUNC_UART);
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

    int count = 0;
    while (count < length) 
    {
        uint8_t ch = 0;
        uart_read_blocking(uart0, &ch, 1);

        if (ch == 8) 
        {
            uart_putc_raw(uart0, 8);
            uart_putc_raw(uart0, ' ');
            uart_putc_raw(uart0, 8);

            if (count > 0) 
            {
                count--;
            }
            continue;
        }

    // Echo the received character so the user sees what they type
    uart_putc_raw(uart0, ch);
    buffer[count++] = (char)ch;
        return count;
    }
    return count;
}

int _write(__unused int handle, char *buffer, int length) 
{
    // Your code here to write to the UART from the buffer.
    // DO NOT USE THE STDIO_* FUNCTIONS FROM ABOVE.  Only UART ones.

    // The argument "handle" is unused.  This is meant for use with 
    // files, which are not very different from text streams.  However, 
    // we write to the UART, not the file specified by the handle.

    // handle is irrelevant since these functions will only ever be called 
    // by the correct functions.  No need for an if statement.

    // Instructions: Given the buffer and a specific length to write, write 1
    // character at a time to the UART until the length is reached.

    for (int i = 0; i < length; ++i) 
    {
        uart_putc_raw(uart0, buffer[i]);
    }
    return length;
}

int main()
{
    init_uart();

    // insert any setbuf lines below...
    setbuf(stdout, NULL);  // Disable buffering for stdout
    setbuf(stdin, NULL);   // Disable buffering for stdin
    char name[8];
    int age = 0;
    for(;;) {
        printf("Enter your name and age: ");
        scanf("%s %d", name, &age);
        printf("Hello, %s! You are %d years old.\n", name, age);
        sleep_ms(100);  // in case the output loops and is too fast
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

void init_uart_irq() 
{
    uart_set_fifo_enabled(uart0, false);
    uart_set_irq_enables(uart0, true, false); // rx=true, tx=false

    irq_set_exclusive_handler(UART0_IRQ, uart_rx_handler);
    irq_set_enabled(UART0_IRQ, true);
}

void uart_rx_handler() 
{
    int c = uart_getc(uart0);

    if (c < 0) 
    {
        return;
    }

    if (seridx >= BUFSIZE) 
    {
        return;
    }

    if (c == '\n') 
    {
        serbuf[seridx] = '\0';
        newline_seen = 1;
        // Echo newline to host
        uart_putc_raw(uart0, (char)c);
        return;
    }

    if (c == 8) 
    {
        if (seridx > 0) 
        {
            uart_putc_raw(uart0, 8);
            uart_putc_raw(uart0, ' ');
            uart_putc_raw(uart0, 8);
            seridx--;
            serbuf[seridx] = '\0';
        }
        return;
    }

    uart_putc_raw(uart0, (char)c);
    serbuf[seridx++] = (char)c;
}

int _read(__unused int handle, char *buffer, int length) 
{
    while (!newline_seen) 
    {
        sleep_ms(5);
    }

    newline_seen = 0;

    int to_copy = seridx;
    if (to_copy > length) to_copy = length;

    for (int i = 0; i < to_copy; ++i) 
    {
        buffer[i] = serbuf[i];
    }

    seridx = 0;

    return length;
}

int _write(__unused int handle, char *buffer, int length) 
{
    for (int i = 0; i < length; ++i) 
    {
        uart_putc_raw(uart0, buffer[i]);
    }
    return length;
}

int main() 
{
    init_uart();
    init_uart_irq();

    setbuf(stdout, NULL);  

    char name[8];
    int age = 0;
    for(;;) 
    {
        printf("Enter your name and age: ");
        scanf("%s %d", name, &age);
        fflush(stdin);
        printf("Hello, %s! You are %d years old.\r\n", name, age);
        sleep_ms(100);
    }
}

#endif

//////////////////////////////////////////////////////////////////////////////

#ifdef STEP5

// Copy global variables, init_uart_irq, uart_rx_handler, _read, and _write from STEP4.

#define BUFSIZE 32
char serbuf[BUFSIZE];
int seridx = 0;
int newline_seen = 0;

// Track which pins have been initialized as outputs
static int gpio_out_init[48] = {0};

// add this here so that compiler does not complain about implicit function
void uart_rx_handler();

void init_uart_irq() 
{
    uart_set_fifo_enabled(uart0, false);
    uart_set_irq_enables(uart0, true, false); // rx=true, tx=false

    irq_set_exclusive_handler(UART0_IRQ, uart_rx_handler);
    irq_set_enabled(UART0_IRQ, true);
}

void uart_rx_handler() 
{
    int c = uart_getc(uart0);

    if (c < 0) 
    {
        return;
    }

    if (seridx >= BUFSIZE) 
    {
        return;
    }

    if (c == '\n') 
    {
        serbuf[seridx] = '\0';
        newline_seen = 1;
        uart_putc_raw(uart0, (char)c);
        return;
    }

    if (c == 8) 
    {
        if (seridx > 0) 
        {
            uart_putc_raw(uart0, 8);
            uart_putc_raw(uart0, ' ');
            uart_putc_raw(uart0, 8);
            seridx--;
            serbuf[seridx] = '\0';
        }
        return;
    }

    uart_putc_raw(uart0, (char)c);
    serbuf[seridx++] = (char)c;
}

int _read(__unused int handle, char *buffer, int length) 
{
    while (!newline_seen) 
    {
        sleep_ms(5);
    }

    newline_seen = 0;

    int to_copy = seridx;
    if (to_copy > length) to_copy = length;

    for (int i = 0; i < to_copy; ++i) 
    {
        buffer[i] = serbuf[i];
    }

    seridx = 0;
    return length;
}

int _write(__unused int handle, char *buffer, int length) 
{
    for (int i = 0; i < length; ++i) 
    {
        uart_putc_raw(uart0, buffer[i]);
    }
    return length;
}

void cmd_gpio(int argc, char **argv) 
{
    if (argc < 2) 
    {
        printf("Usage: gpio <out|set> ...\n");
        return;
    }

    if (strcmp(argv[1], "out") == 0) 
    {
        if (argc != 3) 
        {
            printf("Usage: gpio out <pin>\n");
            return;
        }

        int pin = atoi(argv[2]);

        if (pin < 0 || pin > 47) 
        {
            printf("Invalid pin number: %d. Must be between 0 and 47.\n", pin);
            return;
        }

        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);
        gpio_out_init[pin] = 1;
        printf("Initialized pin %d as output.\n", pin);
        return;
    }

    if (strcmp(argv[1], "set") == 0) 
    {
        if (argc != 4) 
        {
            printf("Usage: gpio set <pin> <0|1>\n");
            return;
        }

        int pin = atoi(argv[2]);

        if (pin < 0 || pin > 47) 
        {
            printf("Invalid pin number: %d. Must be between 0 and 47.\n", pin);
            return;
        }

        if (!gpio_out_init[pin]) 
        {
            printf("Pin %d is not initialized as an output.\n", pin);
            return;
        }

        int val = atoi(argv[3]);

        if (!(val == 0 || val == 1)) 
        {
            printf("Invalid value: %d. Must be 0 or 1.\n", val);
            return;
        }

        gpio_put(pin, val);

        if (val == 1) 
        {
            if (pin == 23) {
                printf("Set pin %d to %d.   (LED on pin %d should now be on.)\n", pin, val, pin);
            } else {
                printf("Set pin %d to %d.   (This should turn on the LED on pin %d.)\n", pin, val, pin);
            }
        } 
        else 
        {
            printf("Set pin %d to %d.   (LED on pin %d should already be off.)\n", pin, val, pin);
        }

        return;
    }

    printf("Unknown command: %s\n", argv[1]);
}

int main() 
{
    init_uart();
    init_uart_irq();

    setbuf(stdout, NULL);  // Disable buffering for stdout

    printf("%s's Peripheral Command Shell (PCS)\n", username);
    printf("Enter a command below.\n\n");

    int argc = 0;
    char *argv[10];
    char input[100];

    for (;;) 
    {
        printf("\r\n> ");
        if (fgets(input, sizeof(input), stdin) == NULL) 
        {
            continue;
        }

        // Flush any pending stdin (lab guidance)
        fflush(stdin);

        // Remove newline if present
        input[strcspn(input, "\n")] = '\0';

        // Tokenize
        argc = 0;
        char *token = strtok(input, " ");
        while (token != NULL && argc < 10) 
        {
            argv[argc++] = token;
            token = strtok(NULL, " ");
        }

        if (argc == 0) 
            continue;

        if (strcmp(argv[0], "gpio") == 0) 
        {
            cmd_gpio(argc, argv);
        } 
        else 
        {
            printf("Unknown command: %s\n", argv[0]);
        }
    }
}

#endif

//////////////////////////////////////////////////////////////////////////////
