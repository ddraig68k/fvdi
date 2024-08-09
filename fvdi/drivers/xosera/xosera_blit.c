/*
 * BLIT routines for Xosera, 4 bits per pixel.
 *
 * Implements FVDI driver API functions c_expand_area() and c_blit_area().
 */

#include "fvdi.h"
#include <stdint.h>
#include "xosera.h"
#include "driver.h"

#undef FVDI_DEBUG

#define OPER_NOT_D 10

static uint16_t
screen_to_screen(volatile xmreg_t *const xosera_ptr, uint16_t src_addr, uint16_t dst_addr, uint16_t src_x,
                 uint16_t w, uint16_t h, long operation, uint16_t pf_words_per_line)
{
    uint16_t num_words = compute_word_width(src_x, w);
    uint16_t mod = pf_words_per_line - num_words;

    BlitParameters p = {
            .control = 0,
            .andc = 0,
            .xor = operation == OPER_NOT_D ? 0xFFFF : 0,
            .src_s = src_addr,
            .mod_s = mod,
            .dst_d = dst_addr,
            .mod_d = mod,
            .shift = compute_lr_masks_4bpp(src_x, w),
            .lines = h,
            .words = num_words,
    };
    do_blit(xosera_ptr, &p);
    return h * num_words;
}

static uint16_t
screen_to_screen_reverse(volatile xmreg_t *const xosera_ptr, uint16_t dst_x, uint16_t dst_y, uint16_t src_x,
                         uint16_t src_y, uint16_t w, uint16_t h, long operation, uint16_t pf_words_per_line)
{
    // Start the transfer at the highest addressed Y.
    uint16_t src_num_words = compute_word_width(src_x, w);
    uint16_t dst_num_words = compute_word_width(dst_x, w);

    BlitParameters p = {
            .control = 0,
            .andc = 0,
            .xor = operation == OPER_NOT_D ? 0xFFFF : 0,
            .src_s = (src_y + h - 1) * pf_words_per_line + (src_x >> 2),
            .mod_s = -(pf_words_per_line + src_num_words),
            .dst_d = (dst_y + h - 1) * pf_words_per_line + (dst_x >> 2),
            .mod_d = -(pf_words_per_line + dst_num_words),
            .shift = compute_lr_masks_4bpp(src_x, w),
            .lines = h,
            .words = src_num_words
    };
    do_blit(xosera_ptr, &p);
    return h * dst_num_words;
}

static uint16_t save_to_vram(volatile xmreg_t *const xosera_ptr, uint16_t vram_save_address, uint16_t src_x,
                             uint16_t src_y, uint16_t w, uint16_t h, uint16_t pf_words_per_line)
{
    uint16_t num_words = compute_word_width(src_x, w);
    BlitParameters p = {
            .control = 0,
            .andc = 0x0000,
            .xor = 0x0000,
            .src_s = src_y * pf_words_per_line + (src_x >> 2),
            .mod_s = pf_words_per_line - num_words,
            .dst_d = vram_save_address,
            .mod_d = 0, /* Save to contiguous VRAM save area. */
            .shift = compute_lr_masks_4bpp(src_x, w),
            .lines = h, .words = num_words
    };
    do_blit(xosera_ptr, &p);
    return h * num_words;
}

static void restore_to_screen(volatile xmreg_t *const xosera_ptr, uint16_t vram_save_address, uint16_t dst_x,
                              uint16_t dst_y, uint16_t w, uint16_t h, uint16_t src_wdwidth,
                              uint16_t pf_words_per_line)
{
    uint16_t num_words = compute_word_width(dst_x, w);
    BlitParameters p = {
            .control = 0,
            .andc = 0x0000,
            .xor = 0x0000,
            .dst_d = dst_y * pf_words_per_line + (dst_x >> 2),
            .mod_d = pf_words_per_line - num_words,
            .src_s = vram_save_address,
            .mod_s = src_wdwidth - num_words,
            .shift = compute_lr_masks_4bpp(dst_x, w),
            .lines = h, .words = num_words
    };
    do_blit(xosera_ptr, &p);

}

static void expand_blit(volatile xmreg_t *const xosera_ptr, uint16_t source_addr, uint16_t dst_x, uint16_t dst_y,
                        uint16_t width, uint16_t height, uint16_t exp_color, uint16_t src_wdwidth, enum Mode mode,
                        uint16_t pf_words_per_line)
{
    uint16_t wdwidth = compute_word_width(dst_x, width);

    // If the incoming color is zero, but the mode is transparent, then we need to do some special handling.
    uint16_t control = 0, andc = ~exp_color, xor = 0;
    if (mode == MODE_TRANSPARENT) {
        control |= 0x0010;
        if (exp_color == 0) {
            xor = 0xFFFF;
            andc = 0x0;
        }
    }
    BlitParameters p = {
            .control = control,
            .andc = andc,
            .xor = xor,
            .src_s = source_addr,
            .mod_s = src_wdwidth - wdwidth,
            .dst_d = dst_y * pf_words_per_line + (dst_x >> 2),
            .mod_d = pf_words_per_line - wdwidth,
            .shift = compute_shift(dst_x, width),
            .words = wdwidth,
            .lines = height,
    };
    do_blit(xosera_ptr, &p);
}

long CDECL
c_expand_area(Virtual *vwk, MFDB *src, long src_x, long src_y, MFDB *dst, long dst_x, long dst_y, long w, long h,
              long operation, long colour)
{
    int src_is_screen = is_screen(vwk->real_address, src);
    int dst_is_screen = is_screen(vwk->real_address, dst);

#ifdef FVDI_DEBUG
    static uint16_t exp_seq = 0;
    long fg = colour & 0xFFFF;
    long bg = (colour >> 16) & 0xFFFF;
    kprintf("c_expand_area()[%d]: src: is_screen? %s x: %ld, y: %ld\n", exp_seq++, src_is_screen ? "yes" : "no", src_x, src_y);
    kprintf("               dst: is_screen? %s x: %ld, y: %ld\n", dst_is_screen ? "yes" : "no", dst_x, dst_y);
    kprintf("               w: %ld, h: %ld, operation: %ld (%s), colour = 0x%08lx, fg = %ld, bg = %ld\n", w, h, operation,
            operation2string(operation), colour, fg, bg);
    kprintf("               dst_x: %ld, dst_t: %ld\n", dst_x, dst_y);
#endif

    const uint16_t vram_save_address = 0xD000; // FIXME: Do proper VRAM management here.
    if (!src_is_screen && dst_is_screen) {
        volatile xmreg_t *const xosera_ptr = xv_prep_dyn(me->device);
        uint16_t buffer[4];

        if (src_x != 0 || src_y != 0) return 0; /* Expect src pos to be 0,0; fallback if not. */
        xm_setw(WR_INCR, 1);
        xm_setw(WR_ADDR, vram_save_address);
        short *p = src->address;

        // Expand the incoming monochrome bitmap to 4 bits per pixel.
        for (short y = 0; y < src->height; y++) {
            for (short word = 0; word < src->wdwidth; word++) {
                uint16_t word_val = *p++;
                expand_word(buffer, word_val, 0xF, 0x0, 0);
                for (int i = 0; i < 4; i++) vram_setw_next_wait(buffer[i]);
            }
        }
        // Blit the newly created off-screen pixmap to the screen
        long hw_color = c_get_colour(vwk, colour & 0xFFFF) & 0xF;
        expand_blit(xosera_ptr, vram_save_address, dst_x, dst_y, w, h, expanded_color[hw_color],
                    src->wdwidth * 4, operation, vwk->real_address->screen.mfdb.wdwidth);
        return 1;
    }
    return 0;
}

long CDECL
c_blit_area(Virtual *vwk, MFDB *src, long src_x, long src_y, MFDB *dst, long dst_x, long dst_y, long w, long h,
            long operation)
{
    int vram_save_address = 0xA000; /* FIXME: Don't hardcode this. */
    int src_is_screen = is_screen(vwk->real_address, src);
    int dst_is_screen = is_screen(vwk->real_address, dst);
#ifdef FVDI_DEBUG
    static uint16_t blit_seq = 0;
    kprintf("c_blit_area()[%d]: src: is_screen? %s x: %ld, y: %ld\n", blit_seq++, src_is_screen ? "yes" : "no", src_x, src_y);
    kprintf("               dst: is_screen? %s x: %ld, y: %ld\n", dst_is_screen ? "yes" : "no", dst_x, dst_y);
    kprintf("               w: %ld, h: %ld, operation: %ld (%s)\n", w, h, operation, operation2string(operation));
#endif
    volatile xmreg_t *const xosera_ptr = xv_prep_dyn(me->device);
    uint16_t pf_words_per_line = vwk->real_address->screen.mfdb.wdwidth;

    if (src_is_screen && !dst_is_screen) {
        save_to_vram(xosera_ptr, vram_save_address, src_x, src_y, w, h, pf_words_per_line);
    }
    if (src_is_screen && dst_is_screen) {
        uint16_t src_addr = src_y * pf_words_per_line + (src_x >> 2);
        uint16_t dst_addr = dst_y * pf_words_per_line + (dst_x >> 2);
        if ((dst_addr < src_addr) || operation == OPER_NOT_D) {
            screen_to_screen(xosera_ptr, src_addr, dst_addr, src_x, w, h, operation, pf_words_per_line);
        } else if (src_y != dst_y) {
            screen_to_screen_reverse(xosera_ptr, dst_x, dst_y, src_x, src_y, w, h, operation, pf_words_per_line);
        } else {
            save_to_vram(xosera_ptr, vram_save_address, src_x, src_y, w, h, pf_words_per_line);
            restore_to_screen(xosera_ptr, vram_save_address, dst_x, dst_y, w, h,
                              compute_word_width(dst_x, w), pf_words_per_line);
        }
    }
    if (!src_is_screen && dst_is_screen) {
        restore_to_screen(xosera_ptr, vram_save_address + (src_x >> 2), dst_x, dst_y, w, h,
                          src->wdwidth * 4, pf_words_per_line);
    }
    if (!src_is_screen && !dst_is_screen) {
        return 0; /* fallback for now */
    }

    return 1;
}
