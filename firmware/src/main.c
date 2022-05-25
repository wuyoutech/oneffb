#include <stdbool.h>
#include <stdint.h>

#include <stm32f4xx.h>

#include <led.h>
#include <tusb.h>

#include <class/net/net_device.h>
#include <dhserver.h>
#include <dnserver.h>
#include <lwip/apps/httpd.h>
#include <lwip/init.h>
#include <lwip/ip4_addr.h>
#include <lwip/timeouts.h>

#define INIT_IP4(a, b, c, d)                                                   \
    { PP_HTONL(LWIP_MAKEU32(a, b, c, d)) }

/* lwip context */
static struct netif netif_data;

/* shared between tud_network_recv_cb() and service_traffic() */
static struct pbuf *received_frame;

/* this is used by this code, ./class/net/net_driver.c, and usb_descriptors.c */
/* ideally speaking, this should be generated from the hardware's unique ID (if
 * available) */
/* it is suggested that the first byte is 0x02 to indicate a link-local address
 */
const uint8_t tud_network_mac_address[6] = {0x02, 0x02, 0x84, 0x6A, 0x96, 0x00};

/* network parameters of this MCU */
static const ip4_addr_t ipaddr = INIT_IP4(192, 168, 7, 1);
static const ip4_addr_t netmask = INIT_IP4(255, 255, 255, 0);
static const ip4_addr_t gateway = INIT_IP4(0, 0, 0, 0);

/* database IP addresses that can be offered to the host; this must be in RAM to
 * store assigned MAC addresses */
static dhcp_entry_t entries[] = {
    /* mac ip address                          lease time */
    {{0}, INIT_IP4(192, 168, 7, 2), 24 * 60 * 60},
    {{0}, INIT_IP4(192, 168, 7, 3), 24 * 60 * 60},
    {{0}, INIT_IP4(192, 168, 7, 4), 24 * 60 * 60},
};

static const dhcp_config_t dhcp_config = {
    .router = INIT_IP4(0, 0, 0, 0),  /* router address (if any) */
    .port = 67,                      /* listen port */
    .dns = INIT_IP4(192, 168, 7, 1), /* dns server (if any) */
    "usb",                           /* dns suffix */
    TU_ARRAY_SIZE(entries),          /* num entry */
    entries                          /* entries */
};

static err_t linkoutput_fn(struct netif *netif, struct pbuf *p) {
    (void)netif;

    for (;;) {
        /* if TinyUSB isn't ready, we must signal back to lwip that there is
         * nothing we can do */
        if (!tud_ready())
            return ERR_USE;

        /* if the network driver can accept another packet, we make it happen */
        if (tud_network_can_xmit(p->tot_len)) {
            tud_network_xmit(p, 0 /* unused for this example */);
            return ERR_OK;
        }

        /* transfer execution to TinyUSB in the hopes that it will finish
         * transmitting the prior packet */
        tud_task();
    }
}

static err_t ip4_output_fn(struct netif *netif, struct pbuf *p,
                           const ip4_addr_t *addr) {
    return etharp_output(netif, p, addr);
}

static err_t netif_init_cb(struct netif *netif) {
    LWIP_ASSERT("netif != NULL", (netif != NULL));
    netif->mtu = CFG_TUD_NET_MTU;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP |
                   NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    netif->state = NULL;
    netif->name[0] = 'E';
    netif->name[1] = 'X';
    netif->linkoutput = linkoutput_fn;
    netif->output = ip4_output_fn;
    return ERR_OK;
}

static void init_lwip(void) {
    struct netif *netif = &netif_data;

    lwip_init();

    /* the lwip virtual MAC address must be different from the host's; to ensure
     * this, we toggle the LSbit */
    netif->hwaddr_len = sizeof(tud_network_mac_address);
    memcpy(netif->hwaddr, tud_network_mac_address,
           sizeof(tud_network_mac_address));
    netif->hwaddr[5] ^= 0x01;

    netif = netif_add(netif, &ipaddr, &netmask, &gateway, NULL, netif_init_cb,
                      ip_input);
    netif_set_default(netif);
}

/* handle any DNS requests from dns-server */
bool dns_query_proc(const char *name, ip4_addr_t *addr) {
    if (0 == strcmp(name, "tiny.usb")) {
        *addr = ipaddr;
        return true;
    }
    return false;
}

bool tud_network_recv_cb(const uint8_t *src, uint16_t size) {
    /* this shouldn't happen, but if we get another packet before
    parsing the previous, we must signal our inability to accept it */
    if (received_frame)
        return false;

    if (size) {
        struct pbuf *p = pbuf_alloc(PBUF_RAW, size, PBUF_POOL);

        if (p) {
            /* pbuf_alloc() has already initialized struct; all we need to do is
             * copy the data */
            memcpy(p->payload, src, size);

            /* store away the pointer for service_traffic() to later handle */
            received_frame = p;
        }
    }

    return true;
}

uint16_t tud_network_xmit_cb(uint8_t *dst, void *ref, uint16_t arg) {
    struct pbuf *p = (struct pbuf *)ref;

    (void)arg; /* unused for this example */

    return pbuf_copy_partial(p, dst, p->tot_len, 0);
}

static void service_traffic(void) {
    /* handle any packet received by tud_network_recv_cb() */
    if (received_frame) {
        ethernet_input(received_frame, &netif_data);
        pbuf_free(received_frame);
        received_frame = NULL;
        tud_network_recv_renew();
    }

    sys_check_timeouts();
}

void tud_network_init_cb(void) {
    /* if the network is re-initializing and we have a leftover packet, we must
     * do a cleanup */
    if (received_frame) {
        pbuf_free(received_frame);
        received_frame = NULL;
    }
}

void led_blinking_task(void);
void cdc_task(void);
void usb_hal_init(void);
void board_init(void);
void hid_task(void);

int main() {
    board_init();
    /* init usb */
    tusb_init();
    init_lwip();
    while (!netif_is_up(&netif_data))
        ;
    while (dhserv_init(&dhcp_config) != ERR_OK)
        ;
    while (dnserv_init(IP_ADDR_ANY, 53, dns_query_proc) != ERR_OK)
        ;

    httpd_init();

    while (1) {
        tud_task();
        led_blinking_task();

        // cdc_task();
        service_traffic();
        hid_task();
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
    blink_interval_ms = blink_not_mounted;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) { blink_interval_ms = blink_mounted; }

// //--------------------------------------------------------------------+
// // USB CDC
// //--------------------------------------------------------------------+
// void cdc_task(void) {
//     // connected() check for DTR bit
//     // Most but not all terminal client set this when making connection
//     // if ( tud_cdc_connected() )
//     {
//         // connected and there are data available
//         if (tud_cdc_available()) {
//             // read datas
//             char buf[64];
//             uint32_t count = tud_cdc_read(buf, sizeof(buf));
//             (void)count;

//             // Echo back
//             // Note: Skip echo by commenting out write() and write_flush()
//             // for throughput test e.g
//             //    $ dd if=/dev/zero of=/dev/ttyACM0 count=10000
//             tud_cdc_write(buf, count);
//             tud_cdc_write_flush();
//         }
//     }
// }

// // Invoked when cdc when line state changed e.g connected/disconnected
// void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
//     (void)itf;
//     (void)rts;

//     // TODO set some indicator
//     if (dtr) {
//         // Terminal connected
//     } else {
//         // Terminal disconnected
//     }
// }

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

/* lwip has provision for using a mutex, when applicable */
sys_prot_t sys_arch_protect(void) { return 0; }
void sys_arch_unprotect(sys_prot_t pval) { (void)pval; }

/* lwip needs a millisecond time source, and the TinyUSB board support code has
 * one available */
uint32_t sys_now(void) { return board_millis(); }

/* add callback function to avoid link error */
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen) {
    return 0;
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize) {}

/// HID Gamepad Protocol Report.
typedef struct TU_ATTR_PACKED {
    int16_t x;        ///< Delta x  movement of left analog-stick
    int16_t y;        ///< Delta y  movement of left analog-stick
    int16_t z;        ///< Delta z  movement of right analog-joystick
    int16_t rz;       ///< Delta Rz movement of right analog-joystick
    uint32_t buttons; ///< Buttons mask for currently pressed buttons
} hid_custom_report_t;

static void send_hid_report(uint8_t report_id, uint32_t btn) {
    // skip if hid is not ready yet
    if (!tud_hid_ready())
        return;

    // use to avoid send multiple consecutive zero report for keyboard
    static bool has_gamepad_key = false;

    hid_custom_report_t report = {
        .x = 0, .y = 0, .z = 0, .rz = 0, .buttons = 0};

    if (btn) {
        report.buttons = GAMEPAD_BUTTON_A;
        tud_hid_report(1, &report, sizeof(report));

        has_gamepad_key = true;
    } else {
        // report.hat = GAMEPAD_HAT_CENTERED;
        report.buttons = 0;
        if (has_gamepad_key)
            tud_hid_report(1, &report, sizeof(report));
        has_gamepad_key = false;
    }
}

void hid_task(void) {
    // Poll every 10ms
    const uint32_t interval_ms = 1;
    static uint32_t start_ms = 0;

    if (board_millis() - start_ms < interval_ms)
        return;
    start_ms += interval_ms;

    uint32_t const btn = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) > 0 ? 0 : 1;

    if (tud_suspended() && btn) {
        tud_remote_wakeup();
    } else {
        send_hid_report(1, btn);
    }
}