#include <stdint.h>
#include <stdbool.h>

#include <stm32f4xx.h>

#include <led.h>
#include <tusb.h>

void led_blinking_task(void);
void cdc_task(void);
void usb_hal_init(void);
void board_init(void);

int main() {
    board_init();
    /* init usb */
    tusb_init();

    while (1) {
        tud_task();
        led_blinking_task();

        cdc_task();
    }
}

//--------------------------------------------------------------------+
// SYSTICK
//--------------------------------------------------------------------+
volatile uint32_t system_ticks = 0;
void SysTick_Handler(void) { system_ticks++; }
uint32_t board_millis(void) { return system_ticks; }
void HardFault_Handler(void) { asm("bkpt"); }
void OTG_FS_IRQHandler(void) { tud_int_handler(0); }

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
enum { blink_not_mounted = 250, blink_mounted = 1000, blink_suspended = 2500 };

static uint32_t blink_interval_ms = blink_not_mounted;

void led_blinking_task(void) {
    static uint32_t start_ms = 0;
    static bool led_state = false;

    if (board_millis() - start_ms < blink_interval_ms)
        return;
    start_ms += blink_interval_ms;

    led_sys_set(led_state);
    led_state = led_state ? false : true; // toggle
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) { blink_interval_ms = blink_mounted; }

// Invoked when device is unmounted
void tud_umount_cb(void) { blink_interval_ms = blink_not_mounted; }

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
    (void)remote_wakeup_en;
    blink_interval_ms = blink_suspended;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) { blink_interval_ms = blink_mounted; }

//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+
void cdc_task(void) {
    // connected() check for DTR bit
    // Most but not all terminal client set this when making connection
    // if ( tud_cdc_connected() )
    {
        // connected and there are data available
        if (tud_cdc_available()) {
            // read datas
            char buf[64];
            uint32_t count = tud_cdc_read(buf, sizeof(buf));
            (void)count;

            // Echo back
            // Note: Skip echo by commenting out write() and write_flush()
            // for throughput test e.g
            //    $ dd if=/dev/zero of=/dev/ttyACM0 count=10000
            tud_cdc_write(buf, count);
            tud_cdc_write_flush();
        }
    }
}

// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
    (void)itf;
    (void)rts;

    // TODO set some indicator
    if (dtr) {
        // Terminal connected
    } else {
        // Terminal disconnected
    }
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf) { (void)itf; }
void board_led_write(bool state) { GPIO_WriteBit(GPIOC, GPIO_Pin_13, state); }

#define USB_OTG_FS ((USB_OTG_GlobalTypeDef *)USB_OTG_FS_PERIPH_BASE)
static inline void board_vbus_sense_init(void) {
    // Blackpill doens't use VBUS sense (B device) explicitly disable it
    USB_OTG_FS->GCCFG |= USB_OTG_GCCFG_NOVBUSSENS;
    USB_OTG_FS->GCCFG &= ~USB_OTG_GCCFG_VBUSBSEN;
    USB_OTG_FS->GCCFG &= ~USB_OTG_GCCFG_VBUSASEN;
}

void board_init(void) {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    /* The voltage scaling allows optimizing the power consumption when the
    device is clocked below the maximum system frequency, to update the voltage
    scaling value regarding system frequency refer to product datasheet. */
    // this should config finished at SystemInit()
    //  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    // enable clocks for LED, USB
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

    /* config system clock */
    SysTick_Config(SystemCoreClock / 1000);

    GPIO_InitTypeDef GPIO_InitStrucutre;
    GPIO_InitStrucutre.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStrucutre.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStrucutre.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStrucutre.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStrucutre.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(GPIOC, &GPIO_InitStrucutre);

    board_led_write(false);

    GPIO_InitStrucutre.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStrucutre.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStrucutre.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStrucutre.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(GPIOA, &GPIO_InitStrucutre);

    /* init usb gpio */
    GPIO_InitStrucutre.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
    GPIO_InitStrucutre.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStrucutre.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStrucutre.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStrucutre.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStrucutre);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_OTG_FS);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource12, GPIO_AF_OTG_FS);

    /* Enable USB OTG clock */
    RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_OTG_FS, ENABLE);

    board_vbus_sense_init();
}
