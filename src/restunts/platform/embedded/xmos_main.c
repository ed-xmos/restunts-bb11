#include "stubs.h"

#include <xcore/parallel.h>
#include <xcore/hwtimer.h>
#include <stdio.h>

#ifndef RESTUNTS_EMBEDDED_DEBUG
#define RESTUNTS_EMBEDDED_DEBUG 0
#endif
#ifndef RESTUNTS_EMBEDDED_SAVE_FRAMES
#define RESTUNTS_EMBEDDED_SAVE_FRAMES 0
#endif

DECLARE_JOB(game_thread, (legacy_s16, legacy_s8 **));
void game_thread(legacy_s16 argc, legacy_s8 **argv)
{
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

int main(void)
{
	legacy_s8 program_name[] = "restunts";
	legacy_s8 *arguments[] = {program_name, 0};
	restunts_embedded_set_debug(RESTUNTS_EMBEDDED_DEBUG);
	restunts_embedded_set_frame_saver(RESTUNTS_EMBEDDED_SAVE_FRAMES);

	PAR_JOBS(PJOB(timer_thread, ()), PJOB(game_thread, (1, arguments)));
	return 0;
}