#!/usr/bin/env bash
if [[ "${BASH_SOURCE[0]}" != "$0" ]]; then
	printf 'Run this helper as ./run-embedded-native.sh; do not source it.\n' >&2
	return 2
fi

set -euo pipefail
if [[ "${RESTUNTS_NATIVE_TRACE:-0}" == 1 ]]; then
	set -x
fi

restunts_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
build_dir=${RESTUNTS_NATIVE_BUILD_DIR:-out/embedded-native}
run_timeout=${RESTUNTS_NATIVE_TIMEOUT:-60s}
if [[ "$build_dir" != /* ]]; then
	build_dir="$restunts_root/$build_dir"
fi
runner="$build_dir/restunts_embedded_runner"

if [[ ! -x "$runner" ]]; then
	"$restunts_root/build-embedded-native.sh"
fi
if ! command -v timeout >/dev/null 2>&1; then
	printf 'The run helper requires the timeout command.\n' >&2
	exit 127
fi

cd "$restunts_root/stunts"
exec timeout --signal=INT --kill-after=2s "$run_timeout" "$runner" "$@"