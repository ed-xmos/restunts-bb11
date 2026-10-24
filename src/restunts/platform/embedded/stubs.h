#ifndef RESTUNTS_EMBEDDED_STUBS_H
#define RESTUNTS_EMBEDDED_STUBS_H

#include "../../c/legacy.h"
#include "../../c/restunts.h"

enum RESTUNTS_EMBEDDED_SOUND_COMMAND {
	RESTUNTS_EMBEDDED_SOUND_PREPARE,
	RESTUNTS_EMBEDDED_SOUND_ACTIVATE,
	RESTUNTS_EMBEDDED_SOUND_START,
	RESTUNTS_EMBEDDED_SOUND_END,
	RESTUNTS_EMBEDDED_SOUND_RELEASE,
	RESTUNTS_EMBEDDED_SOUND_PITCH,
	RESTUNTS_EMBEDDED_SOUND_CONTROL,
	RESTUNTS_EMBEDDED_SOUND_VOLUME,
	RESTUNTS_EMBEDDED_SOUND_DRIVER_DATA,
	RESTUNTS_EMBEDDED_SOUND_RESET
};

struct RESTUNTS_EMBEDDED_SOUND_EVENT {
	legacy_u16 command;
	legacy_s16 channel;
	legacy_s16 value;
	legacy_u16 parameter;
	const void *resource;
	const legacy_u8 *data;
	legacy_u16 length;
};

typedef void (*restunts_embedded_sound_callback)(
	const struct RESTUNTS_EMBEDDED_SOUND_EVENT *event, void *userdata);
typedef void (*restunts_embedded_frame_callback)(const legacy_u8 *framebuffer,
												const legacy_u8 *palette, void *userdata);

void restunts_embedded_set_sound_callback(restunts_embedded_sound_callback callback,
										  void *userdata);
void restunts_embedded_set_frame_callback(restunts_embedded_frame_callback callback,
										  void *userdata);
void restunts_embedded_queue_key(legacy_s16 key);
void restunts_embedded_set_key_state(legacy_s16 key, legacy_u8 down);
void restunts_embedded_set_mouse(legacy_s16 buttons, legacy_s16 x, legacy_s16 y);
void restunts_embedded_set_joystick(legacy_s16 x, legacy_s16 y, legacy_s16 flags);
legacy_s16 restunts_embedded_run(legacy_s16 argc, legacy_s8 *argv[]);
void restunts_embedded_tick_10ms(void);
void restunts_embedded_present_frame(void);
const legacy_u8 *restunts_embedded_framebuffer(void);
const legacy_u8 *restunts_embedded_palette(void);

#endif