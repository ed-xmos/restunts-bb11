#!/usr/bin/env bash
if [[ "${BASH_SOURCE[0]}" != "$0" ]]; then
	printf 'Run this helper as ./build-xmos.sh; do not source it.\n' >&2
	return 2
fi

set -euo pipefail
if [[ "${RESTUNTS_XMOS_TRACE:-0}" == 1 ]]; then
	set -x
fi

restunts_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)

clean_build=0
xmos_cmake_path=
while [[ $# -gt 0 ]]; do
	case "$1" in
		--clean) clean_build=1 ;;
		-h|--help)
			printf 'Usage: %s [--clean] [directory-containing-xcommon.cmake]\n' \
				"$(basename -- "$0")"
			exit 0
			;;
		-*)
			printf 'Unknown option: %s\n' "$1" >&2
			printf 'Usage: %s [--clean] [directory-containing-xcommon.cmake]\n' \
				"$(basename -- "$0")" >&2
			exit 2
			;;
		*)
			if [[ -n "$xmos_cmake_path" ]]; then
				printf 'Only one xcommon directory may be provided.\n' >&2
				exit 2
			fi
			xmos_cmake_path=$1
			;;
	esac
	shift
done

if [[ -n "$xmos_cmake_path" ]]; then
	XMOS_CMAKE_PATH=$xmos_cmake_path
	if [[ "$XMOS_CMAKE_PATH" != /* ]]; then
		XMOS_CMAKE_PATH="$PWD/$XMOS_CMAKE_PATH"
	fi
	export XMOS_CMAKE_PATH
fi

if [[ -z "${XMOS_CMAKE_PATH:-}" ]]; then
	printf 'XMOS_CMAKE_PATH is not set. Source the XMOS environment or pass its CMake directory.\n' >&2
	exit 2
fi
if [[ ! -r "$XMOS_CMAKE_PATH/xcommon.cmake" ]]; then
	printf 'Cannot read %s/xcommon.cmake\n' "$XMOS_CMAKE_PATH" >&2
	exit 2
fi

build_dir=${RESTUNTS_XMOS_BUILD_DIR:-out/xmos}
if [[ "$build_dir" != /* ]]; then
	build_dir="$restunts_root/$build_dir"
fi

cmake_args=(-DRESTUNTS_BUILD_XMOS=ON)
if [[ -n "${RESTUNTS_XMOS_DEBUG:-}" ]]; then
	cmake_args+=("-DRESTUNTS_XMOS_DEBUG=$RESTUNTS_XMOS_DEBUG")
fi
cmake -S "$restunts_root" -B "$build_dir" "${cmake_args[@]}"
if [[ "$clean_build" -eq 1 ]]; then
	cmake --build "$build_dir" --target clean
fi
cmake --build "$build_dir" --parallel 4