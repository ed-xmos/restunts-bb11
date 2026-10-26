#include "stubs.h"

#include <xcore/channel.h>
#include <xcore/parallel.h>
#include <xcore/hwtimer.h>
#include <stdint.h>
#include <stdio.h>

#ifndef RESTUNTS_EMBEDDED_DEBUG
#define RESTUNTS_EMBEDDED_DEBUG 0
#endif
#ifndef RESTUNTS_EMBEDDED_SAVE_FRAMES
#define RESTUNTS_EMBEDDED_SAVE_FRAMES 0
#endif

#define XMOS_BMP_HEADER_BYTES (14U + 40U + 256U * 4U)
#define XMOS_FRAME_BYTES (320U * 200U)

chanend_t g_c_graphics_side = 0;

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
		hwtimer_delay(timer, XS1_TIMER_KHZ * 10);
		printf("tick\n");
		restunts_embedded_tick_10ms();
	}
}

DECLARE_JOB(graphics_thread, (chanend_t));
void graphics_thread(chanend_t channel)
{
	for (;;) {
		uint32_t frame_index = chan_in_word(channel);
		const legacy_u8 *framebuffer = (const legacy_u8 *)(uintptr_t)chan_in_word(channel);
		const legacy_u8 *palette = (const legacy_u8 *)(uintptr_t)chan_in_word(channel);
		graphics_save_frame(framebuffer, palette, frame_index);
		printf("[XMOS] Saved frame %lu\n", frame_index);
	}
}


int main(void)
{
	legacy_s8 program_name[] = "restunts";
	legacy_s8 *arguments[] = {program_name, 0};
	restunts_embedded_set_debug(RESTUNTS_EMBEDDED_DEBUG);
	restunts_embedded_set_frame_saver(0);

	channel_t c_graphics = chan_alloc();

	PAR_JOBS(	PJOB(timer_thread, ()),
				PJOB(graphics_thread, (c_graphics.end_b)),
				PJOB(game_thread, (c_graphics.end_a, 1, arguments))	);
	return 0;
}