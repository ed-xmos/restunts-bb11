#!/usr/bin/env bash
if [[ "${BASH_SOURCE[0]}" != "$0" ]]; then
	printf 'Run this helper as ./run-xmos.sh; do not source it.\n' >&2
	return 2
fi

set -euo pipefail
restunts_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
xmos_image="$restunts_root/cmake/bin/restunts.xe"

if ! command -v xsim >/dev/null 2>&1; then
	printf 'xsim is not available. Source the XMOS environment first.\n' >&2
	exit 127
fi
if [[ ! -r "$xmos_image" ]]; then
	printf 'Cannot read %s; build the XMOS application first.\n' "$xmos_image" >&2
	exit 1
fi

cd "$restunts_root/stunts"
exec xsim "$xmos_image"