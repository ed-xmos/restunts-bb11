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

target=${RESTUNTS_XMOS_HW_TARGET:-XK-EVK-XU416-800.xn}
case "$target" in
	XK-EVK-XU416-800.xn) target_slug=xk-evk-xu416 ;;
	XCORE-AI-EXPLORER-800.xn) target_slug=xcore-ai-explorer ;;
	*)
		printf 'Unsupported XMOS hardware target: %s\n' "$target" >&2
		exit 2
		;;
esac

toolchain_root=${XMOS_TOOL_PATH:-$XMOS_CMAKE_PATH}
if [[ "$toolchain_root" == */build/xcommon_cmake ]]; then
	toolchain_root=${toolchain_root%/build/xcommon_cmake}
fi
toolchain_slug=${toolchain_root##*/}
toolchain_slug=${toolchain_slug,,}
toolchain_slug=${toolchain_slug//[^a-z0-9]/-}
default_build_dir="out/xmos-$target_slug-$toolchain_slug"

build_dir=${RESTUNTS_XMOS_BUILD_DIR:-$default_build_dir}
if [[ "$build_dir" != /* ]]; then
	build_dir="$restunts_root/$build_dir"
fi

cmake_args=(-DRESTUNTS_BUILD_XMOS=ON "-DRESTUNTS_XMOS_HW_TARGET=$target")
if [[ -n "${RESTUNTS_XMOS_DEBUG:-}" ]]; then
	cmake_args+=("-DRESTUNTS_XMOS_DEBUG=$RESTUNTS_XMOS_DEBUG")
fi
if [[ -n "${RESTUNTS_XMOS_SAVE_FRAMES:-}" ]]; then
	cmake_args+=("-DRESTUNTS_XMOS_SAVE_FRAMES=$RESTUNTS_XMOS_SAVE_FRAMES")
fi
cmake -S "$restunts_root" -B "$build_dir" "${cmake_args[@]}"
if [[ "$clean_build" -eq 1 ]]; then
	cmake --build "$build_dir" --target clean
fi
cmake --build "$build_dir" --parallel 4