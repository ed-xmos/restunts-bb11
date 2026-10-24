#include "stubs.h"

#ifndef RESTUNTS_EMBEDDED_DEBUG
#define RESTUNTS_EMBEDDED_DEBUG 0
#endif

int main(void)
{
	legacy_s8 program_name[] = "restunts";
	legacy_s8 *arguments[] = {program_name, 0};
	restunts_embedded_set_debug(RESTUNTS_EMBEDDED_DEBUG);
	return restunts_embedded_run(1, arguments);
}