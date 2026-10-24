#include "stubs.h"

int main(void)
{
	legacy_s8 program_name[] = "restunts";
	legacy_s8 *arguments[] = {program_name, 0};
	return restunts_embedded_run(1, arguments);
}