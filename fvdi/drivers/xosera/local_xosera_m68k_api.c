/*
 * A copy of some functions from xosera_m68k_api.c. I'll delete this if and when the Xosera API
 * supports setting xosera_ptr dynamically.
 */

#include <stdbool.h>
#include "xosera_m68k_api.h"

bool local_xosera_init(volatile xmreg_t *const xosera_ptr, xosera_mode_t init_mode);

#define SYNC_RETRIES 250        // ~1/4 second

// TODO: This is less than ideal (tuned for ~10MHz)
__attribute__((noinline)) static void my_cpu_delay(int ms)
{
    __asm__ __volatile__(
            "    lsl.l   #8,%[temp]\n"
            "    add.l   %[temp],%[temp]\n"
            "0:  sub.l   #1,%[temp]\n"
            "    tst.l   %[temp]\n"
            "    bne.s   0b\n"
            : [temp] "+d"(ms));
}

static bool local_xosera_sync(volatile xmreg_t *const xosera_ptr)
{
    uint16_t rd_incr = xm_getw(RD_INCR);
    uint16_t test_incr = rd_incr ^ 0xF5FA;
    xm_setw(RD_INCR, test_incr);
    if (xm_getw(RD_INCR) != test_incr) {
        return false;        // not detected
    }
    xm_setw(RD_INCR, rd_incr);

    return true;
}

// wait for Xosera to respond after reconfigure
static bool local_xosera_wait_sync(volatile xmreg_t *const xosera_ptr)
{
    // check for Xosera presense (retry in case it is reconfiguring)
    for (uint16_t r = SYNC_RETRIES; r != 0; --r) {
        if (local_xosera_sync(xosera_ptr)) {
            return true;
        }
        my_cpu_delay(10);
    }
    return false;
}


// reconfigure or sync Xosera and return true if it is responsive
// NOTE: May BUS ERROR if no hardware present
bool local_xosera_init(volatile xmreg_t *const xosera_ptr, xosera_mode_t init_mode)
{
    bool detected = local_xosera_wait_sync(xosera_ptr);

    if (detected) {
        xwait_not_vblank();
        xwait_vblank();
        if (init_mode >= XINIT_CONFIG_640x480) {
            xm_setbh(INT_CTRL, 0x80 | (init_mode & 0x03));        // reconfig FPGA to init_mode 0-3
            detected = local_xosera_wait_sync(xosera_ptr);                        // wait for detect
            if (detected) {
                // wait for initial copper program to disable itself (or timeout)
                uint16_t timeout = 100;
                do {
                    my_cpu_delay(1);
                } while ((xreg_getw(COPP_CTRL) & COPP_CTRL_COPP_EN_F) && --timeout);
            }
        }
    }

    return detected;
}
