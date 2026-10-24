#!/usr/bin/env bash
set -euo pipefail

restunts_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)

if [[ $# -ne 1 || "$1" == "-h" || "$1" == "--help" ]]; then
	printf 'Usage: %s /path/to/your-toolchain.cmake\n' "$(basename -- "$0")"
	if [[ $# -ne 1 ]]; then
		exit 2
	fi
	exit 0
fi

toolchain_file=$1
if [[ "$toolchain_file" != /* ]]; then
	toolchain_file="$PWD/$toolchain_file"
fi
if [[ ! -f "$toolchain_file" || ! -r "$toolchain_file" ]]; then
	printf 'Toolchain file is not a readable file: %s\n' "$toolchain_file" >&2
	exit 2
fi

export RESTUNTS_CMAKE_TOOLCHAIN_FILE=$toolchain_file
export RESTUNTS_EMBEDDED_BUILD_DIR=${RESTUNTS_EMBEDDED_BUILD_DIR:-out/embedded-target}
exec "$restunts_root/tools/scripts/build-embedded-stubs.sh"