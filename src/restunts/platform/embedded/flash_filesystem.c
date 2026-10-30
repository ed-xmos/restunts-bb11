#include "../../c/platform.h"
#include "pff.h"
#include "pffdiskio.h"
#include <xmos_flash.h>
#include <xmos_flashdata.h>
#include <xcore/hwtimer.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define FLASH_FILE_HANDLE 5U
#define FLASH_FILENAME_LENGTH 13U

static const flash_ports_t flash_ports = {
	.flash_cs = XS1_PORT_1B,
	.flash_sclk = XS1_PORT_1C,
	.flash_sio = XS1_PORT_4B,
	.flash_clk_blk = XS1_CLKBLK_1,
};
static flash_handle_t flash_handle;
static flashdata_handle_t flashdata_handle;
static FATFS filesystem;
static unsigned int data_address;
static unsigned int data_size;
static int flash_connected;
static int flashdata_mounted;
static int mount_attempted;
static int filesystem_mounted;
static int file_open;
static legacy_s16 file_error;
static legacy_u32 file_position;
static uint32_t file_read_ticks;
static uint32_t file_bytes_read;
static char open_filename[FLASH_FILENAME_LENGTH];
static legacy_s8 found_filename[FLASH_FILENAME_LENGTH];

static void flash_filesystem_shutdown(void)
{
	if (flashdata_mounted) {
		(void)flashdata_unmount(&flashdata_handle);
		flashdata_mounted = 0;
	}
	if (flash_connected) {
		flash_disconnect(&flash_handle);
		flash_connected = 0;
	}
	filesystem_mounted = 0;
	file_open = 0;
}

DSTATUS pff_disk_initialize(void)
{
	flash_clock_config_t config = {
		.flash_clock = flash_clock_reference,
		.flash_input_clock_div = 4,
		.flash_output_clock_div = 4,
		.flash_port_input_edge = flash_clock_input_edge_rising,
		.flash_port_pad_delay = flash_port_pad_delay_0,
	};
	flash_connect(&flash_handle, &flash_ports, config);
	flash_connected = 1;
	int result = flashdata_mount(&flashdata_handle, &flash_handle, 16384);
	if (result != flashdata_result_success) {
		printf("[FFS] flashdata_mount failed: %d\n", result);
		return STA_NOINIT;
	}
	flashdata_mounted = 1;
	int address = flashptn_data_address(&flashdata_handle);
	int size = flashptn_data_size(&flashdata_handle);
	if (address < 0 || size <= 0) {
		printf("[FFS] Invalid data partition: address=%d size=%d\n", address, size);
		return STA_NOINIT;
	}
	data_address = (unsigned int)address;
	data_size = (unsigned int)size;
	return 0;
}

DRESULT pff_disk_readp(BYTE *buffer, DWORD sector, UINT offset, UINT count)
{
	if (!flashdata_mounted) {
		return RES_NOTRDY;
	}
	if (buffer == NULL || offset > 512U || count > 512U - offset ||
		sector >= data_size / 512U) {
		return RES_PARERR;
	}
	flash_read(&flash_handle, data_address + sector * 512U + offset, buffer, count);
	return RES_OK;
}

static int flash_filesystem_mount(void)
{
	if (!mount_attempted) {
		mount_attempted = 1;
		FRESULT result = pf_mount(&filesystem);
		if (result != FR_OK) {
			printf("[FFS] pf_mount failed: %d\n", result);
			flash_filesystem_shutdown();
		} else {
			filesystem_mounted = 1;
			printf("[FFS] Read-only FAT mounted at 0x%x (%u bytes)\n",
				   data_address, data_size);
		}
	}
	return filesystem_mounted;
}

static int flash_filename(const legacy_s8 *path, char filename[FLASH_FILENAME_LENGTH])
{
	const char *name = (const char *)path;
	for (const char *character = name; *character != 0; character++) {
		if (*character == '/' || *character == '\\' || *character == ':') {
			name = character + 1;
		}
	}
	size_t length = strlen(name);
	if (length == 0 || length >= FLASH_FILENAME_LENGTH ||
		strchr(name, '*') != NULL || strchr(name, '?') != NULL) {
		return 0;
	}
	for (size_t index = 0; index <= length; index++) {
		char value = name[index];
		filename[index] = value >= 'a' && value <= 'z' ? value - ('a' - 'A') : value;
	}
	return 1;
}

legacy_u16 dos_file_open(const legacy_s8 *path, legacy_s16 create)
{
	char filename[FLASH_FILENAME_LENGTH];
	file_error = 1;
	if (create != DOS_FILE_OPEN_EXISTING) {
		return 0;
	}
	if (file_open) {
		printf("[FFS] Rejected second open: %s\n", (const char *)path);
		return 0;
	}
	if (!flash_filename(path, filename) || !flash_filesystem_mount()) {
		return 0;
	}
	FRESULT result = pf_open(filename);
	if (result != FR_OK) {
		printf("[FFS] Open failed %s: result=%d\n", filename, result);
		return 0;
	}
	memcpy(open_filename, filename, strlen(filename) + 1U);
	file_read_ticks = 0;
	file_bytes_read = 0;
	file_open = 1;
	file_position = 0;
	file_error = 0;
	return FLASH_FILE_HANDLE;
}

legacy_s16 dos_file_close(legacy_u16 handle)
{
	file_error = handle != FLASH_FILE_HANDLE || !file_open;
	if (file_error) {
		return -1;
	}
	if (file_bytes_read != 0) {
		printf("[FFS] Read %s: elapsed=%u us\n",
			   open_filename, (unsigned)(file_read_ticks / 100U));
	}
	file_open = 0;
	return 0;
}

legacy_u16 dos_file_read(legacy_u16 handle, void far *destination, legacy_u16 length)
{
	file_error = handle != FLASH_FILE_HANDLE || !file_open;
	if (file_error || file_position >= filesystem.fsize) {
		return 0;
	}
	UINT count = 0;
	uint32_t start = get_reference_time();
	FRESULT result = pf_read(destination, length, &count);
	file_read_ticks += get_reference_time() - start;
	file_error = result != FR_OK;
	file_position += count;
	file_bytes_read += count;
	return (legacy_u16)count;
}

legacy_u16 dos_file_write(legacy_u16 handle, const void far *source, legacy_u16 length)
{
	(void)handle;
	(void)source;
	(void)length;
	file_error = 1;
	return 0;
}

legacy_s16 dos_file_seek(legacy_u16 handle, legacy_s32 offset, legacy_s16 origin)
{
	file_error = 1;
	if (handle != FLASH_FILE_HANDLE || !file_open || origin < DOS_FILE_SEEK_BEGIN ||
		origin > DOS_FILE_SEEK_END) {
		return -1;
	}
	int64_t base = origin == DOS_FILE_SEEK_CURRENT ? file_position :
				   origin == DOS_FILE_SEEK_END ? filesystem.fsize : 0;
	int64_t position = base + offset;
	if (position < 0 || position > INT32_MAX || pf_lseek((DWORD)position) != FR_OK) {
		return -1;
	}
	file_position = (legacy_u32)position;
	file_error = 0;
	return 0;
}

legacy_s32 dos_file_tell(legacy_u16 handle)
{
	file_error = handle != FLASH_FILE_HANDLE || !file_open;
	return file_error ? -1 : (legacy_s32)file_position;
}

legacy_s16 dos_file_error(void)
{
	return file_error;
}

legacy_s16 dos_file_remove(const legacy_s8 *path)
{
	(void)path;
	file_error = 1;
	return -1;
}

const legacy_s8 *dos_file_find_first(const legacy_s8 *query)
{
	char filename[FLASH_FILENAME_LENGTH];
	if (!flash_filename(query, filename)) {
		return NULL;
	}
	legacy_u16 handle = dos_file_open(query, DOS_FILE_OPEN_EXISTING);
	if (handle == 0) {
		return NULL;
	}
	(void)dos_file_close(handle);
	memset(found_filename, 0, sizeof(found_filename));
	memcpy(found_filename, filename, strlen(filename));
	return found_filename;
}

const legacy_s8 *dos_file_find_next(void)
{
	return NULL;
}