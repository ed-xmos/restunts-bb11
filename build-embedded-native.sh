#!/usr/bin/env bash
if [[ "${BASH_SOURCE[0]}" != "$0" ]]; then
	printf 'Run this helper as ./build-embedded-native.sh; do not source it.\n' >&2
	return 2
fi

set -euo pipefail
if [[ "${RESTUNTS_NATIVE_TRACE:-0}" == 1 ]]; then
	set -x
fi

restunts_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
build_dir=${RESTUNTS_NATIVE_BUILD_DIR:-out/embedded-native}
jobs=${RESTUNTS_BUILD_JOBS:-2}
clean_build=0
while [[ $# -gt 0 ]]; do
	case "$1" in
		--clean) clean_build=1 ;;
		-h|--help)
			printf 'Usage: %s [--clean]\n' "$(basename -- "$0")"
			printf 'Set RESTUNTS_NATIVE_BUILD_DIR or RESTUNTS_BUILD_JOBS to override defaults.\n'
			exit 0
			;;
		*)
			printf 'Unknown option: %s\nUsage: %s [--clean]\n' "$1" "$(basename -- "$0")" >&2
			exit 2
			;;
	esac
	shift
done

if [[ "$build_dir" != /* ]]; then
	build_dir="$restunts_root/$build_dir"
fi

cmake -S "$restunts_root" -B "$build_dir" -DRESTUNTS_BUILD_EMBEDDED_STUBS=ON
if [[ "$clean_build" -eq 1 ]]; then
	cmake --build "$build_dir" --target clean
fi
cmake --build "$build_dir" --target restunts_embedded_runner --parallel "$jobs"