#include <stdbool.h>
#include <stdint.h>
#include <stm32f4xx.h>

#include <led.h>
#include <tusb.h>

#define USB_OTG_FS ((USB_OTG_GlobalTypeDef *)USB_OTG_FS_PERIPH_BASE)
static inline void board_vbus_sense_init(void) {
    // Blackpill doens't use VBUS sense (B device) explicitly disable it
    USB_OTG_FS->GCCFG |= USB_OTG_GCCFG_NOVBUSSENS;
    USB_OTG_FS->GCCFG &= ~USB_OTG_GCCFG_VBUSBSEN;
    USB_OTG_FS->GCCFG &= ~USB_OTG_GCCFG_VBUSASEN;
}

void usb_init(void) {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    /* The voltage scaling allows optimizing the power consumption when the
    device is clocked below the maximum system frequency, to update the voltage
    scaling value regarding system frequency refer to product datasheet. */
    // this should config finished at SystemInit()
    //  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    // enable clocks for LED, USB
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    /* init usb gpio */
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_OTG_FS);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource12, GPIO_AF_OTG_FS);

    /* Enable USB OTG clock */
    RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_OTG_FS, ENABLE);

    board_vbus_sense_init();

    tusb_init();
}

void OTG_FS_IRQHandler(void) { tud_int_handler(0); }

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) { led_blinking_timeset(blink_mounted); }

// Invoked when device is unmounted
void tud_umount_cb(void) { led_blinking_timeset(blink_not_mounted); }

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
    (void)remote_wakeup_en;
    led_blinking_timeset(blink_suspended);
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) { led_blinking_timeset(blink_mounted); }

//--------------------------------------------------------------------+
// USB CDC callbacks 
//--------------------------------------------------------------------+
void cdc_task(void) {
    // connected() check for DTR bit
    // Most but not all terminal client set this when making connection
    // if ( tud_cdc_connected() )
    {
        // connected and there are data available
        if (tud_cdc_available()) {
            // TODO: make write and read fifo here
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

void usb_task(void) {
    tud_task();
    cdc_task();
}

//--------------------------------------------------------------------+
// USB HID callbacks 
//--------------------------------------------------------------------+

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen) {
    // TODO not Implemented
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize) {
    // TODO set LED based on CAPLOCK, NUMLOCK etc...
    (void)itf;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}
