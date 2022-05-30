#include <hal_types.h>
#include <ioCC254x_bitdef.h>

#include <ioCC2544.h>

#include <system.h>

// Baudrate = 57.6 kbps (U0BAUD.BAUD_M = 216, U0GCR.BAUD_E = 10), given 32 MHz system clock.
// Baudrate = ((256+BAUD_M)*2^BAUD_E)/2^28 * system_clock
#define UART_BAUD_M  216
#define UART_BAUD_E  11

static uint8 buffer[] = {'H','e','l','l','o'};

void uart0Send(uint8* uartTxBuf, uint16 uartTxBufLength)
{
    uint16 uartTxIndex;

    // Clear any pending TX interrupt request (set U0CSR.TX_BYTE = 0).
    U0CSR &= ~U0CSR_TX_BYTE;

    // Loop: send each UART0 sample on the UART0 TX line.
    for (uartTxIndex = 0; uartTxIndex < uartTxBufLength; uartTxIndex++)
    {
        U0DBUF = uartTxBuf[uartTxIndex];
        while(! (U0CSR & U0CSR_TX_BYTE) );
        U0CSR &= ~U0CSR_TX_BYTE;
    }
}



void main(void){
    system_init();

    // Set pins 1, 2 and 3 as peripheral I/O and pin 0 as GPIO output.
    // P0SEL0 = 0x11;        // Map P0_0 and P0_1 as UASRT0. 
    P0SEL1 = 0x11;        // Map P0_3 and P0_2 as UASRT0. 

    // // Initialize P0_1 for SRF05EB S1 button.
    // P0SEL0 &= ~P0SEL0_SELP0_1;// Function as General Purpose I/O.
    // PDIR &= ~PDIR_DIRP0_1;    // Input.

    // Initialise bitrate = 57.6 kbps.
    U0BAUD = UART_BAUD_M;
    U0GCR = (U0GCR & ~U0GCR_BAUD_E) | UART_BAUD_E;

    // Initialise UART protocol (start/stop bit, data bits, parity, etc.):
    // USART mode = UART (U0CSR.MODE = 1)
    U0CSR |= U0CSR_MODE;

    // Start bit level = low => Idle level = high  (U0UCR.START = 0).
    U0UCR &= ~U0UCR_START;

    // Stop bit level = high (U0UCR.STOP = 1).
    U0UCR |= U0UCR_STOP;

    // Number of stop bits = 1 (U0UCR.SPB = 0).
    U0UCR &= ~U0UCR_SPB;

    // Parity = disabled (U0UCR.PARITY = 0).
    U0UCR &= ~U0UCR_PARITY;

    // 9-bit data enable = 8 bits transfer (U0UCR.BIT9 = 0).
    U0UCR &= ~U0UCR_BIT9;

    // Level of bit 9 = 0 (U0UCR.D9 = 0), used when U0UCR.BIT9 = 1.
    // Level of bit 9 = 1 (U0UCR.D9 = 1), used when U0UCR.BIT9 = 1.
    // Parity = Even (U0UCR.D9 = 0), used when U0UCR.PARITY = 1.
    // Parity = Odd (U0UCR.D9 = 1), used when U0UCR.PARITY = 1.
    U0UCR &= ~U0UCR_D9;

    // Flow control = disabled (U0UCR.FLOW = 0).
    U0UCR &= ~U0UCR_FLOW;

    // Bit order = LSB first (U0GCR.ORDER = 0).
    U0GCR &= ~U0GCR_ORDER;

    uart0Send(buffer, sizeof(buffer));

    while(1);

}