#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../c/platform.h"

#define MEMORY_BYTES 0x100000UL
#define MEMORY_FIRST_SEGMENT 0x1000U
#define MEMORY_END_SEGMENT 0xA000U
#define EXTERNAL_FIRST_SEGMENT 0xE000U
#define EXTERNAL_PAGE_COUNT 512U
#define EXTERNAL_PAGE_MASK LEGACY_U16_MAX
#define MEMORY_PARAGRAPH_BYTES 16U
#define MEMORY_PARAGRAPH_SHIFT 4U
#define MEMORY_PSP_ADDRESS 0x028E0UL

#if defined(__XMOS__) && defined(USE_EXTMEM)
#define RESTUNTS_EXTMEM_BSS __attribute__((section(".ExtMem.bss")))
#else
#define RESTUNTS_EXTMEM_BSS
#endif

/* Resource files contain real 16:16 addresses after relocation. Keep their
 * address space independent of the host pointer size, including VGA A000:0.
 * Native static/stack objects use registered pages outside the resource arena. */
static legacy_u8 memory[MEMORY_BYTES] RESTUNTS_EXTMEM_BSS;
static uintptr_t external_pages[EXTERNAL_PAGE_COUNT];
static legacy_u16 external_page_count;

static void memory_error(void)
{
	fputs("Invalid legacy memory address\n", stderr);
	dos_process_exit(1);
}

static legacy_u16 external_segment(uintptr_t address)
{
	uintptr_t page = address & ~(uintptr_t)EXTERNAL_PAGE_MASK;
	for (legacy_u16 index = 0; index < external_page_count; index++) {
		if (external_pages[index] == page) {
			return EXTERNAL_FIRST_SEGMENT + index;
		}
	}
	if (external_page_count == EXTERNAL_PAGE_COUNT) {
		memory_error();
		return 0;
	}
	external_pages[external_page_count] = page;
	return EXTERNAL_FIRST_SEGMENT + external_page_count++;
}

void *dos_memory_make_pointer(legacy_u16 segment, legacy_u16 offset)
{
	if (segment == 0 && offset == 0) {
		return NULL;
	}
	if (segment >= EXTERNAL_FIRST_SEGMENT) {
		legacy_u16 index = segment - EXTERNAL_FIRST_SEGMENT;
		if (index >= external_page_count) {
			memory_error();
			return NULL;
		}
		return (void *)(external_pages[index] + offset);
	}
	legacy_u32 address = (legacy_u32)segment * MEMORY_PARAGRAPH_BYTES + offset;
	if (address >= MEMORY_BYTES) {
		memory_error();
		return NULL;
	}
	return memory + address;
}

legacy_u16 dos_memory_pointer_segment(const void *pointer)
{
	uintptr_t address = (uintptr_t)pointer;
	uintptr_t base = (uintptr_t)memory;
	if (pointer == NULL) {
		return 0;
	}
	if (address >= base && address < base + MEMORY_BYTES) {
		return (legacy_u16)((address - base) >> MEMORY_PARAGRAPH_SHIFT);
	}
	return external_segment(address);
}

legacy_u16 dos_memory_pointer_offset(const void *pointer)
{
	uintptr_t address = (uintptr_t)pointer;
	uintptr_t base = (uintptr_t)memory;
	if (address >= base && address < base + MEMORY_BYTES) {
		return (legacy_u16)((address - base) & (MEMORY_PARAGRAPH_BYTES - 1U));
	}
	return (legacy_u16)(address & EXTERNAL_PAGE_MASK);
}

void *dos_memory_make_near_pointer(legacy_u16 offset)
{
	/* Native callers use real pointers; this is only the legacy null case. */
	if (offset != 0) {
		memory_error();
	}
	return NULL;
}

void *dos_memory_get_psp(void)
{
	return memory + MEMORY_PSP_ADDRESS;
}

legacy_u16 dos_memory_allocate(legacy_u16 paragraphs)
{
	return paragraphs <= MEMORY_END_SEGMENT - MEMORY_FIRST_SEGMENT ? MEMORY_FIRST_SEGMENT : 0;
}

legacy_u16 dos_memory_resize(legacy_u16 segment, legacy_u16 paragraphs)
{
	if (segment != MEMORY_FIRST_SEGMENT) {
		return 0;
	}
	legacy_u16 available = MEMORY_END_SEGMENT - segment;
	return paragraphs < available ? paragraphs : available;
}
