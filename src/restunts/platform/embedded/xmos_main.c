#include "stubs.h"

#include <xcore/channel.h>
#include <xcore/parallel.h>
#include <xcore/hwtimer.h>
#include <netmain.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <xscope.h>
#if defined(RESTUNTS_LEGACY_MEMORY_IN_SOC_RAM)
#include <vx4b_canonical_defines.h>
#endif
#include "lcd.h"

#ifndef RESTUNTS_EMBEDDED_DEBUG
#define RESTUNTS_EMBEDDED_DEBUG 0
#endif
#ifndef RESTUNTS_EMBEDDED_SAVE_FRAMES
#define RESTUNTS_EMBEDDED_SAVE_FRAMES 0
#endif

#define XMOS_BMP_HEADER_BYTES (14U + 40U + 256U * 4U)
#define XMOS_FRAME_BYTES (320U * 200U)
#define XMOS_PALETTE_BYTES (256U * 3U)

chanend_t g_c_graphics_side = 0;
volatile legacy_u32 g_graphics_busy = 0;

static legacy_u8 xmos_bmp_header[XMOS_BMP_HEADER_BYTES];

static void xmos_store_u16(legacy_u8 *destination, uint16_t value)
{
	destination[0] = (legacy_u8)value;
	destination[1] = (legacy_u8)(value >> 8);
}

static void xmos_store_u32(legacy_u8 *destination, uint32_t value)
{
	destination[0] = (legacy_u8)value;
	destination[1] = (legacy_u8)(value >> 8);
	destination[2] = (legacy_u8)(value >> 16);
	destination[3] = (legacy_u8)(value >> 24);
}

static void graphics_save_frame(const legacy_u8 *framebuffer, const legacy_u8 *palette,
								uint32_t frame_index)
{
	char digits[10];
	uint32_t digit_count = 0;
	do {
		digits[digit_count++] = (char)('0' + frame_index % 10U);
		frame_index /= 10U;
	} while (frame_index != 0U);
	while (digit_count < 4U) {
		digits[digit_count++] = '0';
	}
	char filename[16] = {'f', 'r'};
	for (uint32_t index = 0; index < digit_count; index++) {
		filename[2U + index] = digits[digit_count - index - 1U];
	}
	filename[2U + digit_count] = '.';
	filename[3U + digit_count] = 'b';
	filename[4U + digit_count] = 'm';
	filename[5U + digit_count] = 'p';
	filename[6U + digit_count] = '\0';

	xmos_bmp_header[0] = 'B';
	xmos_bmp_header[1] = 'M';
	xmos_store_u32(xmos_bmp_header + 2U, XMOS_BMP_HEADER_BYTES + XMOS_FRAME_BYTES);
	xmos_store_u32(xmos_bmp_header + 10U, XMOS_BMP_HEADER_BYTES);
	xmos_store_u32(xmos_bmp_header + 14U, 40U);
	xmos_store_u32(xmos_bmp_header + 18U, 320U);
	xmos_store_u32(xmos_bmp_header + 22U, (uint32_t)-200);
	xmos_store_u16(xmos_bmp_header + 26U, 1U);
	xmos_store_u16(xmos_bmp_header + 28U, 8U);
	xmos_store_u32(xmos_bmp_header + 34U, XMOS_FRAME_BYTES);
	xmos_store_u32(xmos_bmp_header + 46U, 256U);
	for (uint32_t index = 0; index < 256U; index++) {
		uint32_t palette_offset = 54U + index * 4U;
		uint32_t color_offset = index * 3U;
		for (uint32_t channel = 0; channel < 3U; channel++) {
			uint32_t value = palette[color_offset + 2U - channel] & 63U;
			xmos_bmp_header[palette_offset + channel] =
				(legacy_u8)((value * 255U + 31U) / 63U);
		}
	}
	FILE *file = fopen(filename, "wb");
	if (file == NULL) {
		return;
	}
	(void)fwrite(xmos_bmp_header, 1, sizeof(xmos_bmp_header), file);
	(void)fwrite(framebuffer, 1, XMOS_FRAME_BYTES, file);
	(void)fclose(file);
}

DECLARE_JOB(game_thread, (chanend_t, legacy_s16, legacy_s8 **));
void game_thread(chanend_t c_graphics, legacy_s16 argc, legacy_s8 **argv)
{
	g_c_graphics_side = c_graphics;
	(void)restunts_embedded_run(argc, argv);
}

DECLARE_JOB(timer_thread, (void));
void timer_thread(void)
{
	hwtimer_t timer = hwtimer_alloc();
	while (1) {
		const int milliseconds = 100000;
		hwtimer_delay(timer, milliseconds * 10);
		putchar('.');
		// printf("%lu\n", (unsigned long)hwtimer_get_time(timer));
		restunts_embedded_tick_10ms();
	}
}

DECLARE_JOB(graphics_thread, (chanend_t, chanend_t));
#pragma stackfunction 16896
void graphics_thread(chanend_t channel, chanend_t c_lcd)
{
	legacy_u8 framebuffer_snapshot[XMOS_FRAME_BYTES];
	legacy_u8 palette_snapshot[XMOS_PALETTE_BYTES];
	uint16_t lcd_palette[256];
	xscope_mode_lossless();
#if RESTUNTS_EMBEDDED_SAVE_FRAMES
	uint32_t file_index = 0;
#endif
	g_graphics_busy = 0;
	for (;;) {
		g_graphics_busy = 0;
		const legacy_u8 *framebuffer = (const legacy_u8 *)(uintptr_t)chan_in_word(channel);
		const legacy_u8 *palette = (const legacy_u8 *)(uintptr_t)chan_in_word(channel);
		g_graphics_busy = 1;
		uint32_t frame_index = chan_in_word(channel);
		memcpy(palette_snapshot, palette, XMOS_PALETTE_BYTES);
		memcpy(framebuffer_snapshot, framebuffer, XMOS_FRAME_BYTES);
		if (frame_index <= 25U) {
			uint32_t nonzero_pixels = 0;
			for (uint32_t pixel = 0; pixel < XMOS_FRAME_BYTES; pixel++) {
				nonzero_pixels += framebuffer_snapshot[pixel] != 0;
			}
			printf("[XMOS] snapshot frame=%lu nonzero=%lu framebuffer=%p palette=%p\n",
				   (unsigned long)frame_index, (unsigned long)nonzero_pixels,
				   (const void *)framebuffer, (const void *)palette);
		}
#if RESTUNTS_EMBEDDED_SAVE_FRAMES
		uint32_t save_start = get_reference_time();
		graphics_save_frame(framebuffer_snapshot, palette_snapshot, file_index++);
		uint32_t save_elapsed = get_reference_time() - save_start;
		printf("[XMOS] Saved frame %lu: elapsed=%u us\n",
			   (unsigned long)(file_index - 1U), (unsigned)(save_elapsed / 100U));
#else
		if (frame_index <= 25U || frame_index % 100U == 0) {
			printf("[XMOS] Presented frame %lu (file saving disabled)\n",
				   (unsigned long)frame_index);
		}
#endif
		for (uint32_t index = 0; index < 256; index++) {
			uint32_t offset = index * 3;
			uint8_t red = ((palette_snapshot[offset] & 63U) * 255U + 31U) / 63U;
			uint8_t green = ((palette_snapshot[offset + 1] & 63U) * 255U + 31U) / 63U;
			uint8_t blue = ((palette_snapshot[offset + 2] & 63U) * 255U + 31U) / 63U;
			lcd_palette[index] = rgb565(red, green, blue);
		}
		s_chan_out_word(c_lcd, DD_SET_PALETTE);
		s_chan_out_buf_byte(c_lcd, (const uint8_t *)lcd_palette, sizeof(lcd_palette));
		s_chan_out_word(c_lcd, DD_WRITE);
		s_chan_out_buf_byte(c_lcd, framebuffer_snapshot, sizeof(framebuffer_snapshot));
		g_graphics_busy = 0;
		chan_out_word(channel, 0);
	}
}


void main_tile_0(void)
{
	legacy_s8 program_name[] = "restunts";
	legacy_s8 *arguments[] = {program_name, 0};
#if defined(RESTUNTS_LEGACY_MEMORY_IN_SOC_RAM)
	/* Enable SoC RAM before the legacy memory arena is first touched. */
	__asm__ volatile ("csrrs zero, 0x7c2, %0" : : "r"(1u) : "memory");
	memset((void *)(uintptr_t)VX_SOC_RAM_BASE, 0, VX_SOC_RAM_SIZE);
#endif
	restunts_embedded_set_debug(RESTUNTS_EMBEDDED_DEBUG);
	restunts_embedded_set_frame_saver(0);

	chanend_t c_lcd = chanend_alloc();
	channel_t c_graphics = chan_alloc();
	printf("c_lcd app=%p\n", (void *)c_lcd);
	chanend_set_dest(c_lcd, 0x80030002); // From tile 1, main_tile_1

	PAR_JOBS(	PJOB(timer_thread, ()),
				PJOB(graphics_thread, (c_graphics.end_b, c_lcd)),
				PJOB(game_thread, (c_graphics.end_a, 1, arguments))	);
}

void main_tile_1(void)
{
	chanend_t c_framebuffer = chanend_alloc();
	printf("c_lcd serv=%p\n", (void *)c_framebuffer);
	chanend_set_dest(c_framebuffer, 0x80020002); // From tile 1, main_tile_1
	channel_t c_lcd = s_chan_alloc();
	PAR_JOBS(
		PJOB(lcd, (c_lcd.end_b)),
		PJOB(remote_render, (c_lcd.end_a, c_framebuffer))
	);
	return;
}


// Network main
NETWORK_MAIN(
  TILE_MAIN_VOID(main_tile_1, 1),
  TILE_MAIN_VOID(main_tile_0, 0)
)