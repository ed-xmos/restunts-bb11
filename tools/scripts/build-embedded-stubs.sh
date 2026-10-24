#!/usr/bin/env bash
set -euo pipefail

restunts_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
build_dir=${RESTUNTS_EMBEDDED_BUILD_DIR:-out/embedded-stubs}
jobs=${RESTUNTS_BUILD_JOBS:-2}

case "${1:-}" in
	-h|--help)
		printf 'Usage: %s\n' "$(basename -- "$0")"
		printf 'Set RESTUNTS_EMBEDDED_BUILD_DIR or RESTUNTS_BUILD_JOBS to override defaults.\n'
		exit 0
		;;
	"") ;;
	*) printf 'Unexpected argument: %s\n' "$1" >&2; exit 2 ;;
esac

if [[ "$build_dir" != /* ]]; then
	build_dir="$restunts_root/$build_dir"
fi

cmake_args=(-DRESTUNTS_BUILD_EMBEDDED_STUBS=ON)
if [[ -n "${RESTUNTS_CMAKE_TOOLCHAIN_FILE:-}" ]]; then
	cmake_args+=("-DCMAKE_TOOLCHAIN_FILE=$RESTUNTS_CMAKE_TOOLCHAIN_FILE")
fi

cmake -S "$restunts_root" -B "$build_dir" "${cmake_args[@]}"
cmake --build "$build_dir" --target restunts_embedded_stubs --parallel "$jobs"