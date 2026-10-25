#define _POSIX_C_SOURCE 199309L

#include "stubs.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NATIVE_DEFAULT_FRAME_LIMIT 10UL
#define NATIVE_FRAME_PIXELS (320U * 200U)

struct NATIVE_CAPTURE {
	unsigned long frame_limit;
	unsigned long frame_count;
};

static void *native_timer_thread(void *userdata)
{
	(void)userdata;
	struct timespec tick_period = {0, 10000000L};
	for (;;) {
		restunts_embedded_tick_10ms();
		while (nanosleep(&tick_period, &tick_period) != 0 && errno == EINTR) {
		}
		tick_period.tv_sec = 0;
		tick_period.tv_nsec = 10000000L;
	}
	return NULL;
}

static void native_frame_callback(const legacy_u8 *framebuffer, const legacy_u8 *palette,
								  void *userdata)
{
	(void)palette;
	struct NATIVE_CAPTURE *capture = userdata;
	size_t nonzero_pixels = 0;
	for (size_t pixel = 0; pixel < NATIVE_FRAME_PIXELS; pixel++) {
		nonzero_pixels += framebuffer[pixel] != 0;
	}
	capture->frame_count++;
	printf("[native] frame=%lu nonzero-pixels=%zu\n", capture->frame_count, nonzero_pixels);
	fflush(stdout);
	if (capture->frame_count >= capture->frame_limit) {
		exit(EXIT_SUCCESS);
	}
}

static int native_parse_frame_limit(const char *value, unsigned long *frame_limit)
{
	char *end = NULL;
	errno = 0;
	unsigned long parsed = strtoul(value, &end, 10);
	if (errno != 0 || end == value || *end != '\0' || parsed == 0) {
		return 0;
	}
	*frame_limit = parsed;
	return 1;
}

int main(int argc, char *argv[])
{
	struct NATIVE_CAPTURE capture = {NATIVE_DEFAULT_FRAME_LIMIT, 0};
	legacy_u8 debug_enabled = 0;
	for (int index = 1; index < argc; index++) {
		if (strcmp(argv[index], "--debug") == 0) {
			debug_enabled = 1;
		} else if (strcmp(argv[index], "--frames") == 0 && index + 1 < argc) {
			if (!native_parse_frame_limit(argv[++index], &capture.frame_limit)) {
				fprintf(stderr, "Invalid frame count: %s\n", argv[index]);
				return EXIT_FAILURE;
			}
		} else if (strcmp(argv[index], "--help") == 0 || strcmp(argv[index], "-h") == 0) {
			printf("Usage: %s [--frames count] [--debug]\n", argv[0]);
			return EXIT_SUCCESS;
		} else {
			fprintf(stderr, "Unknown argument: %s\nUsage: %s [--frames count] [--debug]\n",
					argv[index], argv[0]);
			return EXIT_FAILURE;
		}
	}

	restunts_embedded_set_debug(debug_enabled);
	restunts_embedded_set_frame_saver(1);
	restunts_embedded_set_frame_callback(native_frame_callback, &capture);

	pthread_t timer_thread;
	int thread_result = pthread_create(&timer_thread, NULL, native_timer_thread, NULL);
	if (thread_result != 0) {
		fprintf(stderr, "Could not start timer thread: %s\n", strerror(thread_result));
		return EXIT_FAILURE;
	}

	legacy_s8 program_name[] = "restunts";
	legacy_s8 *game_arguments[] = {program_name, NULL};
	return restunts_embedded_run(1, game_arguments);
}