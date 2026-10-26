#!/usr/bin/env bash
if [[ "${BASH_SOURCE[0]}" != "$0" ]]; then
	printf 'Run this helper as ./play-frames.sh; do not source it.\n' >&2
	return 2
fi

set -euo pipefail

restunts_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
frame_dir="$restunts_root/stunts"
frame_rate=15
loop=0
directory_set=0

while [[ $# -gt 0 ]]; do
	case "$1" in
		--fps)
			if [[ $# -lt 2 || ! "$2" =~ ^[0-9]+([.][0-9]+)?$ ]]; then
				printf 'Expected a positive number after --fps.\n' >&2
				exit 2
			fi
			frame_rate=$2
			shift
			;;
		--loop) loop=1 ;;
		-h|--help)
			printf 'Usage: %s [--fps rate] [--loop] [frame-directory]\n' \
				"$(basename -- "$0")"
			exit 0
			;;
		-*)
			printf 'Unknown option: %s\n' "$1" >&2
			exit 2
			;;
		*)
			if [[ "$directory_set" -ne 0 ]]; then
				printf 'Only one frame directory may be provided.\n' >&2
				exit 2
			fi
			frame_dir=$1
			if [[ "$frame_dir" != /* ]]; then
				frame_dir="$PWD/$frame_dir"
			fi
			directory_set=1
			;;
	esac
	shift
done

if ! command -v ffplay >/dev/null 2>&1; then
	printf 'ffplay is not installed or not on PATH.\n' >&2
	exit 127
fi
frame_start=0
if [[ ! -f "$frame_dir/fr0000.bmp" ]]; then
	if [[ -f "$frame_dir/fr0001.bmp" ]]; then
		frame_start=1
	else
		printf 'No fr0000.bmp or fr0001.bmp found in %s\n' "$frame_dir" >&2
		exit 1
	fi
fi

ffplay_args=(-framerate "$frame_rate" -start_number "$frame_start")
if [[ "$loop" -ne 0 ]]; then
	ffplay_args=(-stream_loop -1 "${ffplay_args[@]}")
fi
exec ffplay "${ffplay_args[@]}" -i "$frame_dir/fr%04d.bmp"