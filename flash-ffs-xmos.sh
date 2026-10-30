#!/usr/bin/env bash
if [[ "${BASH_SOURCE[0]}" != "$0" ]]; then
	printf 'Run this helper as ./flash-ffs-xmos.sh; do not source it.\n' >&2
	return 2
fi

set -euo pipefail

restunts_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
cd "$restunts_root"

filesystem_dir=${RESTUNTS_XMOS_FILESYSTEM_DIR:-"$restunts_root/../lib_filesystem/lib_filesystem"}
image_creator_dir="$filesystem_dir/disk_image_creators/fat"
stunts_dir="$restunts_root/stunts"
image="$restunts_root/cmake/bin/ffs_image.img"
factory_image="$restunts_root/cmake/bin/restunts.xe"
boot_partition_size=${RESTUNTS_XMOS_BOOT_PARTITION_SIZE:-1048576}

if ! command -v xflash >/dev/null 2>&1; then
	printf 'xflash is not available. Source the XMOS environment first.\n' >&2
	exit 127
fi
if [[ ! -r "$factory_image" ]]; then
	printf 'Cannot read %s; run ./build-xmos.sh first.\n' "$factory_image" >&2
	exit 1
fi
if [[ ! -f "$image_creator_dir/Makefile" || ! -d "$stunts_dir" ]]; then
	printf 'Cannot find the FAT image creator or stunts asset directory.\n' >&2
	exit 1
fi

make -C "$image_creator_dir"
mapfile -d '' -t files < <(find "$stunts_dir" -type f ! -iname '*.bmp' ! -path "$stunts_dir/frames.ffconcat" -print0)
if [[ ${#files[@]} -eq 0 ]]; then
	printf 'No files found for the filesystem image.\n' >&2
	exit 1
fi
"$image_creator_dir/fat_image_creator" "$image" "${files[@]}"
xflash --boot-partition-size "$boot_partition_size" --data "$image" --factory "$factory_image"