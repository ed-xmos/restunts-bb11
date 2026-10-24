#include "stubs.h"
#include "../../c/platform.h"
#include "../../c/keyboard.h"
#include "../../c/game_input.h"
#include "../../c/audio.h"
#include "../../c/menu_background.h"
#include "../../c/opponent_portrait.h"
#include "../../c/opponent_animation.h"
#include "../../c/skybox_hires.h"
#include "../../c/render_workers.h"
#include "../../c/restunts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void full_data_initialize(void);

#define EMBEDDED_FILE_COUNT 64U
#define EMBEDDED_FILE_FIRST_HANDLE 5U
#define EMBEDDED_KEY_CAPACITY 32U
#define EMBEDDED_KEY_STATE_CAPACITY 64U
#define EMBEDDED_FRAME_BYTES (320U * 200U)
#define EMBEDDED_PALETTE_BYTES (256U * 3U)
#define EMBEDDED_VIDEO_SEGMENT 0xA000U
#define EMBEDDED_TIMER_CALLBACK_COUNT DOS_TIMER_USABLE_CALLBACK_COUNT

struct EMBEDDED_KEY_STATE {
	legacy_s16 key;
	legacy_u8 down;
};

static FILE *embedded_files[EMBEDDED_FILE_COUNT];
static struct EMBEDDED_KEY_STATE embedded_key_states[EMBEDDED_KEY_STATE_CAPACITY];
static legacy_s16 embedded_key_queue[EMBEDDED_KEY_CAPACITY];
static legacy_u32 embedded_key_read;
static legacy_u32 embedded_key_count;
static legacy_s16 embedded_mouse_buttons;
static legacy_s16 embedded_mouse_x;
static legacy_s16 embedded_mouse_y;
static legacy_s16 embedded_joystick_x;
static legacy_s16 embedded_joystick_y;
static legacy_s16 embedded_joystick_flags;
static legacy_u8 embedded_joystick_enabled;
static legacy_u8 embedded_palette[EMBEDDED_PALETTE_BYTES];
static void (*embedded_timer_callbacks[EMBEDDED_TIMER_CALLBACK_COUNT])(void);
static legacy_u32 embedded_game_ticks;
static legacy_u32 embedded_realtime_ticks;
static legacy_u32 embedded_slow_ticks;
static legacy_u8 embedded_callbacks_suspended;
static legacy_s16 embedded_file_error;
static restunts_embedded_sound_callback embedded_sound_callback;
static void *embedded_sound_userdata;
static restunts_embedded_frame_callback embedded_frame_callback;
static void *embedded_frame_userdata;

static void embedded_sound_emit(legacy_u16 command, legacy_s16 channel, legacy_s16 value,
								legacy_u16 parameter, const void *resource,
								const legacy_u8 *data, legacy_u16 length)
{
	if (embedded_sound_callback == NULL) {
		return;
	}
	struct RESTUNTS_EMBEDDED_SOUND_EVENT event = {
		command, channel, value, parameter, resource, data, length};
	embedded_sound_callback(&event, embedded_sound_userdata);
}

void restunts_embedded_set_sound_callback(restunts_embedded_sound_callback callback,
										  void *userdata)
{
	embedded_sound_callback = callback;
	embedded_sound_userdata = userdata;
}

void restunts_embedded_set_frame_callback(restunts_embedded_frame_callback callback,
										  void *userdata)
{
	embedded_frame_callback = callback;
	embedded_frame_userdata = userdata;
}

void restunts_embedded_queue_key(legacy_s16 key)
{
	if (embedded_key_count < EMBEDDED_KEY_CAPACITY) {
		legacy_u32 index = (embedded_key_read + embedded_key_count) % EMBEDDED_KEY_CAPACITY;
		embedded_key_queue[index] = key;
		embedded_key_count++;
	}
}

void restunts_embedded_set_key_state(legacy_s16 key, legacy_u8 down)
{
	legacy_u32 free_index = EMBEDDED_KEY_STATE_CAPACITY;
	for (legacy_u32 index = 0; index < EMBEDDED_KEY_STATE_CAPACITY; index++) {
		if (embedded_key_states[index].key == key) {
			embedded_key_states[index].down = down != 0;
			return;
		}
		if (free_index == EMBEDDED_KEY_STATE_CAPACITY && embedded_key_states[index].down == 0) {
			free_index = index;
		}
	}
	if (down != 0 && free_index < EMBEDDED_KEY_STATE_CAPACITY) {
		embedded_key_states[free_index].key = key;
		embedded_key_states[free_index].down = 1;
	}
}

void restunts_embedded_set_mouse(legacy_s16 buttons, legacy_s16 x, legacy_s16 y)
{
	embedded_mouse_buttons = buttons;
	embedded_mouse_x = x;
	embedded_mouse_y = y;
}

void restunts_embedded_set_joystick(legacy_s16 x, legacy_s16 y, legacy_s16 flags)
{
	embedded_joystick_x = x;
	embedded_joystick_y = y;
	embedded_joystick_flags = flags;
}

legacy_s16 restunts_embedded_run(legacy_s16 argc, legacy_s8 *argv[])
{
	full_data_initialize();
	return run_main_menu_loop(argc, argv);
}

void restunts_embedded_tick_10ms(void)
{
	embedded_realtime_ticks++;
	if (!embedded_callbacks_suspended) {
		embedded_game_ticks++;
		if ((embedded_game_ticks % DOS_TIMER_DEFAULT_DIVIDER_PERIOD) == 0) {
			embedded_slow_ticks++;
		}
		for (legacy_u32 index = 0; index < EMBEDDED_TIMER_CALLBACK_COUNT; index++) {
			if (embedded_timer_callbacks[index] != NULL) {
				embedded_timer_callbacks[index]();
			}
		}
	}
}

const legacy_u8 *restunts_embedded_framebuffer(void)
{
	return (const legacy_u8 *)dos_memory_make_pointer(EMBEDDED_VIDEO_SEGMENT, 0);
}

const legacy_u8 *restunts_embedded_palette(void)
{
	return embedded_palette;
}

void restunts_embedded_present_frame(void)
{
	if (embedded_frame_callback != NULL) {
		embedded_frame_callback(restunts_embedded_framebuffer(), restunts_embedded_palette(),
								embedded_frame_userdata);
	}
}

void *_memcpy(void *destination, const void *source, legacy_u16 length)
{
	return memcpy(destination, source, length);
}

void *__fmemcpy(void *destination, const void *source, legacy_u16 length)
{
	return memcpy(destination, source, length);
}

legacy_s16 dos_write_stdout(const legacy_s8 *text, legacy_u16 length)
{
	return fwrite(text, 1, length, stdout) == length ? (legacy_s16)length : -1;
}

legacy_s16 dos_write_stderr(const legacy_s8 *text, legacy_u16 length)
{
	return fwrite(text, 1, length, stderr) == length ? (legacy_s16)length : -1;
}

void dos_show_startup_error(const legacy_s8 *message)
{
	(void)dos_write_stderr(message, (legacy_u16)strlen((const char *)message));
}

void dos_process_exit(legacy_s16 status)
{
	exit(status);
}

legacy_s16 dos_data_stack_segments_match(void)
{
	return 1;
}

void dos_install_divide_error_handler(void) {}
void dos_interrupts_disable(void) {}
void dos_interrupts_enable(void) {}
void dos_set_critical_error_handler(legacy_s16(far *callback)(void)) { (void)callback; }

legacy_u16 dos_file_open(const legacy_s8 *path, legacy_s16 create)
{
	for (legacy_u16 handle = EMBEDDED_FILE_FIRST_HANDLE; handle < EMBEDDED_FILE_COUNT; handle++) {
		if (embedded_files[handle] == NULL) {
			embedded_files[handle] = fopen((const char *)path,
											   create == DOS_FILE_OPEN_EXISTING ? "rb" : "wb+");
			embedded_file_error = embedded_files[handle] == NULL;
			return embedded_files[handle] != NULL ? handle : 0;
		}
	}
	embedded_file_error = 1;
	return 0;
}

legacy_s16 dos_file_close(legacy_u16 handle)
{
	if (handle < EMBEDDED_FILE_FIRST_HANDLE || handle >= EMBEDDED_FILE_COUNT ||
		embedded_files[handle] == NULL) {
		embedded_file_error = 1;
		return -1;
	}
	legacy_s16 result = (legacy_s16)fclose(embedded_files[handle]);
	embedded_files[handle] = NULL;
	embedded_file_error = result != 0;
	return result;
}

legacy_u16 dos_file_read(legacy_u16 handle, void far *destination, legacy_u16 length)
{
	if (handle < EMBEDDED_FILE_FIRST_HANDLE || handle >= EMBEDDED_FILE_COUNT ||
		embedded_files[handle] == NULL) {
		embedded_file_error = 1;
		return 0;
	}
	size_t result = fread(destination, 1, length, embedded_files[handle]);
	embedded_file_error = ferror(embedded_files[handle]) != 0;
	return (legacy_u16)result;
}

legacy_u16 dos_file_write(legacy_u16 handle, const void far *source, legacy_u16 length)
{
	if (handle < EMBEDDED_FILE_FIRST_HANDLE || handle >= EMBEDDED_FILE_COUNT ||
		embedded_files[handle] == NULL) {
		embedded_file_error = 1;
		return 0;
	}
	size_t result = fwrite(source, 1, length, embedded_files[handle]);
	embedded_file_error = ferror(embedded_files[handle]) != 0;
	return (legacy_u16)result;
}

legacy_s16 dos_file_seek(legacy_u16 handle, legacy_s32 offset, legacy_s16 origin)
{
	static const legacy_s32 origins[] = {SEEK_SET, SEEK_CUR, SEEK_END};
	if (handle < EMBEDDED_FILE_FIRST_HANDLE || handle >= EMBEDDED_FILE_COUNT ||
		embedded_files[handle] == NULL || origin < DOS_FILE_SEEK_BEGIN ||
		origin > DOS_FILE_SEEK_END) {
		embedded_file_error = 1;
		return -1;
	}
	legacy_s16 result = (legacy_s16)fseek(embedded_files[handle], offset, origins[origin]);
	embedded_file_error = result != 0;
	return result;
}

legacy_s32 dos_file_tell(legacy_u16 handle)
{
	if (handle < EMBEDDED_FILE_FIRST_HANDLE || handle >= EMBEDDED_FILE_COUNT ||
		embedded_files[handle] == NULL) {
		embedded_file_error = 1;
		return -1;
	}
	long result = ftell(embedded_files[handle]);
	embedded_file_error = result < 0;
	return (legacy_s32)result;
}

legacy_s16 dos_file_error(void) { return embedded_file_error; }

legacy_s16 dos_file_remove(const legacy_s8 *path)
{
	legacy_s16 result = (legacy_s16)remove((const char *)path);
	embedded_file_error = result != 0;
	return result;
}

const legacy_s8 *dos_file_find_first(const legacy_s8 *query)
{
	(void)query;
	return NULL;
}

const legacy_s8 *dos_file_find_next(void) { return NULL; }

void kb_init_interrupt(void) {}
void kb_exit_handler(void) {}
void dos_kb_set_numlock(void) {}
void dos_kb_clear_numlock(void) {}
void flush_stdin(void) {}

legacy_s16 kb_get_key_state(legacy_s16 key)
{
	for (legacy_u32 index = 0; index < EMBEDDED_KEY_STATE_CAPACITY; index++) {
		if (embedded_key_states[index].down != 0 && embedded_key_states[index].key == key) {
			return -1;
		}
	}
	return 0;
}

legacy_s16 kb_read_char(void)
{
	if (embedded_key_count == 0) {
		return 0;
	}
	legacy_s16 result = embedded_key_queue[embedded_key_read];
	embedded_key_read = (embedded_key_read + 1U) % EMBEDDED_KEY_CAPACITY;
	embedded_key_count--;
	return result;
}

legacy_s16 dos_kb_get_char(void) { return kb_parse_key(kb_read_char()); }
legacy_s16 kb_call_readchar_callback(void) { return kb_read_char(); }
legacy_s16 kb_checking(void) { return embedded_key_count != 0 ? embedded_key_queue[embedded_key_read] : 0; }
legacy_s16 kb_check(void) { embedded_key_count = 0; return 0; }

legacy_s16 dos_get_joy_flags(void) { return embedded_joystick_enabled ? embedded_joystick_flags : 0; }
void dos_joystick_reset_calibration(void) { embedded_joystick_enabled = 1; }
void dos_joystick_set_enabled(legacy_u8 enabled) { embedded_joystick_enabled = enabled != 0; }
legacy_u8 dos_joystick_is_enabled(void) { return embedded_joystick_enabled; }
legacy_s16 dos_joystick_get_scaled_axis(legacy_u16 axis_index)
{
	return !embedded_joystick_enabled ? 0
			 : axis_index == 0 ? embedded_joystick_x
			 : axis_index == 1 ? embedded_joystick_y
								 : 0;
}

legacy_s16 dos_mouse_init(legacy_s16 width, legacy_s16 height)
{
	(void)width;
	(void)height;
	return -1;
}
void dos_mouse_set_minmax(legacy_s16 min_x, legacy_s16 min_y, legacy_s16 max_x, legacy_s16 max_y)
{
	(void)min_x; (void)min_y; (void)max_x; (void)max_y;
}
void dos_mouse_set_position(legacy_s16 x, legacy_s16 y) { embedded_mouse_x = x; embedded_mouse_y = y; }
void dos_mouse_get_state(legacy_s16 *buttons, legacy_s16 *x, legacy_s16 *y)
{
	*buttons = embedded_mouse_buttons; *x = embedded_mouse_x; *y = embedded_mouse_y;
}
legacy_u16 dos_mouse_get_button_count(void) { return 3; }

legacy_s16 dos_timer_register_callback(void(far *callback)(void))
{
	for (legacy_u32 index = 0; index < EMBEDDED_TIMER_CALLBACK_COUNT; index++) {
		if (embedded_timer_callbacks[index] == NULL) {
			embedded_timer_callbacks[index] = callback;
			return DOS_TIMER_CALLBACK_REGISTRATION_SUCCEEDED;
		}
	}
	return DOS_TIMER_CALLBACK_REGISTRATION_FAILED;
}
void dos_timer_unregister_callback(void(far *callback)(void))
{
	for (legacy_u32 index = 0; index < EMBEDDED_TIMER_CALLBACK_COUNT; index++) {
		if (embedded_timer_callbacks[index] == callback) {
			embedded_timer_callbacks[index] = NULL;
		}
	}
}
void dos_timer_reset_counter(void) { embedded_game_ticks = 0; }
void dos_timer_set_callbacks_suspended(legacy_s16 suspended)
{
	embedded_callbacks_suspended = suspended != 0;
}
legacy_u32 dos_timer_get_realtime_counter(void) { return embedded_realtime_ticks; }
legacy_u32 timer_get_counter(void) { return embedded_game_ticks; }
legacy_u32 timer_get_delta(void)
{
	static legacy_u32 previous;
	legacy_u32 current = embedded_game_ticks;
	legacy_u32 result = current - previous;
	previous = current;
	return result;
}
legacy_u32 timer_get_slow_counter(void) { return embedded_slow_ticks; }
void dos_timer_setup_interrupt(void)
{
	memset(embedded_timer_callbacks, 0, sizeof(embedded_timer_callbacks));
	embedded_game_ticks = 0;
	embedded_realtime_ticks = 0;
	embedded_slow_ticks = 0;
	embedded_callbacks_suspended = 0;
}
void dos_timer_shutdown(void) { memset(embedded_timer_callbacks, 0, sizeof(embedded_timer_callbacks)); }

legacy_s16 dos_video_get_status(void) { return 0; }
legacy_s16 video_get_status(void) { return dos_video_get_status(); }
void dos_video_set_palette(legacy_u16 start, legacy_u16 count, legacy_u8 *palette)
{
	if (start >= 256U) return;
	if (count > 256U - start) count = 256U - start;
	memcpy(embedded_palette + (size_t)start * 3U, palette, (size_t)count * 3U);
}
void dos_video_set_mode_13h(void)
{
	memset((void *)restunts_embedded_framebuffer(), 0, EMBEDDED_FRAME_BYTES);
}
legacy_u8 dos_video_enable_planar_pages(void) { return 0; }
void dos_video_set_write_planes(legacy_u8 mask) { (void)mask; }
void dos_video_set_read_plane(legacy_u8 plane) { (void)plane; }
void dos_video_show_page(legacy_u16 address) { (void)address; restunts_embedded_present_frame(); }
void dos_video_set_mode4(void) { dos_video_set_mode_13h(); }
void dos_video_set_mode7(void) {}

legacy_u8 dos_audio_driver_initialize(void) { return 1; }
void dos_audio_driver_load_bank(void far *bank)
{
	embedded_sound_emit(RESTUNTS_EMBEDDED_SOUND_DRIVER_DATA, 0, 0, 0, bank, NULL, 0);
}
void dos_audio_driver_prepare_context(legacy_s16 channel, struct AUDIO_CONTEXT *context,
									  legacy_u8 *timer, void far *resource)
{
	(void)context;
	embedded_sound_emit(RESTUNTS_EMBEDDED_SOUND_PREPARE, channel, 0, 0, resource, timer, 0);
}
void dos_audio_driver_set_context_value(legacy_s16 channel, struct AUDIO_CONTEXT *context,
										legacy_u16 value)
{
	(void)context;
	embedded_sound_emit(RESTUNTS_EMBEDDED_SOUND_PITCH, channel, (legacy_s16)value, 0, NULL, NULL, 0);
}
void dos_audio_driver_set_control(legacy_s16 channel, struct AUDIO_CONTEXT *context,
								  legacy_u16 control, legacy_u16 value)
{
	(void)context;
	embedded_sound_emit(RESTUNTS_EMBEDDED_SOUND_CONTROL, channel, (legacy_s16)value, control,
						NULL, NULL, 0);
}
void dos_audio_driver_set_pitch(legacy_u8 *timer, legacy_s16 pitch, legacy_s16 channel)
{
	(void)timer;
	embedded_sound_emit(RESTUNTS_EMBEDDED_SOUND_PITCH, channel, pitch, 0, NULL, NULL, 0);
}
void dos_audio_driver_send_data(legacy_u16 length, legacy_u8 *data)
{
	embedded_sound_emit(RESTUNTS_EMBEDDED_SOUND_DRIVER_DATA, 0, 0, 0, NULL, data, length);
}
void dos_audio_driver_activate_context(legacy_s16 channel, struct AUDIO_CONTEXT *context,
									   legacy_u8 *timer, legacy_s16 pitch, legacy_u16 parameter,
									   void far *resource)
{
	(void)context;
	embedded_sound_emit(RESTUNTS_EMBEDDED_SOUND_ACTIVATE, channel, pitch, parameter, resource,
						timer, 0);
}
void dos_audio_driver_release_channel(legacy_s16 channel)
{
	embedded_sound_emit(RESTUNTS_EMBEDDED_SOUND_RELEASE, channel, 0, 0, NULL, NULL, 0);
}
void dos_audio_driver_start_context(legacy_s16 channel, struct AUDIO_CONTEXT *context)
{
	(void)context;
	embedded_sound_emit(RESTUNTS_EMBEDDED_SOUND_START, channel, 0, 0, NULL, NULL, 0);
}
void dos_audio_driver_end_context(legacy_s16 channel, struct AUDIO_CONTEXT *context)
{
	(void)context;
	embedded_sound_emit(RESTUNTS_EMBEDDED_SOUND_END, channel, 0, 0, NULL, NULL, 0);
}
void dos_audio_driver_reset(void) { embedded_sound_emit(RESTUNTS_EMBEDDED_SOUND_RESET, 0, 0, 0, NULL, NULL, 0); }
void dos_audio_driver_start(void) {}
void dos_audio_driver_suspend_context(legacy_s16 channel, struct AUDIO_CONTEXT *context,
									  legacy_u16 value, void far *resource)
{
	(void)context;
	embedded_sound_emit(RESTUNTS_EMBEDDED_SOUND_CONTROL, channel, (legacy_s16)value, 0,
						resource, NULL, 0);
}
void dos_audio_driver_suspend_all(struct AUDIO_CONTEXT *contexts) { (void)contexts; }
void dos_audio_driver_set_master_state(legacy_s16 operation, void far *state)
{
	(void)operation; (void)state;
}
void dos_audio_bind_channel_context(legacy_s16 channel, void far *resource)
{
	embedded_sound_emit(RESTUNTS_EMBEDDED_SOUND_PREPARE, channel, 0, 0, resource, NULL, 0);
}
void dos_audio_set_channel_volume(legacy_s16 channel, legacy_s16 volume)
{
	embedded_sound_emit(RESTUNTS_EMBEDDED_SOUND_VOLUME, channel, volume, 0, NULL, NULL, 0);
}
void dos_audio_set_context_pitch(legacy_s16 channel, legacy_s16 pitch)
{
	embedded_sound_emit(RESTUNTS_EMBEDDED_SOUND_PITCH, channel, pitch, 0, NULL, NULL, 0);
}
void dos_audio_shutdown(void) {}

void menu_background_draw(const struct SPRITE *target, const struct SHAPE2D *original,
						  enum MENU_BACKGROUND background)
{
	(void)target; (void)original; (void)background;
}
void menu_background_draw_overlay(const struct SPRITE *target, const struct SHAPE2D *overlay,
								  enum MENU_BACKGROUND background)
{
	(void)target; (void)overlay; (void)background;
}
void menu_background_unload(void) {}
void opponent_portrait_draw(const struct SPRITE *target, const struct SHAPE2D *original,
							legacy_u8 opponent)
{
	(void)target; (void)original; (void)opponent;
}
void opponent_portrait_draw_at(const struct SPRITE *target, const struct SHAPE2D *original,
								legacy_u8 opponent, legacy_s16 x, legacy_s16 y)
{
	(void)target; (void)original; (void)opponent; (void)x; (void)y;
}
void opponent_portrait_unload(void) {}
void opponent_animation_load(legacy_u8 opponent, legacy_u8 won) { (void)opponent; (void)won; }
void opponent_animation_unload(void) {}
legacy_s32 opponent_animation_draw(const struct SPRITE *target, legacy_s16 x, legacy_s16 y,
								   legacy_s16 width, legacy_s16 height)
{
	(void)target; (void)x; (void)y; (void)width; (void)height;
	return 0;
}
void skybox_hires_set_palette(const legacy_u8 *palette) { (void)palette; }
void skybox_hires_unload(void) {}
void skybox_hires_draw(const struct SPRITE *target, legacy_s16 theme, legacy_s16 image,
					   legacy_s32 width, legacy_s32 height, legacy_s32 x, legacy_s32 y)
{
	(void)target; (void)theme; (void)image; (void)width; (void)height; (void)x; (void)y;
}
legacy_s32 skybox_hires_render(const struct SPRITE *target, const struct SKYBOX *scenery,
							   struct SHAPE2D *const shapes[SKYBOX_IMAGE_COUNT], legacy_s16 theme,
							   const struct MATRIX *rotation, legacy_s16 direction, legacy_s16 angle,
							   legacy_s16 camera_y, legacy_s16 detail)
{
	(void)target; (void)scenery; (void)shapes; (void)theme; (void)rotation;
	(void)direction; (void)angle; (void)camera_y; (void)detail;
	return 0;
}

legacy_s32 render_workers_count(void) { return 0; }
legacy_s32 render_workers_run(legacy_s32 count, void (*job)(void *, legacy_s32), void *context)
{
	for (legacy_s32 index = 0; index < count; index++) job(context, index);
	return 0;
}
void render_workers_shutdown(void) {}