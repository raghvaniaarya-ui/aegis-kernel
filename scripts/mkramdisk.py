#!/usr/bin/env python3
"""
Ramdisk image builder tool
Builds a ramdisk image from a directory of files
"""
import os
import struct
import sys

RAMDISK_MAGIC = 0x444B4D52  # 'RMDK'

def build_ramdisk(input_dir, output_file):
    # Collect all files
    files = []
    for root, dirs, filenames in os.walk(input_dir):
        for fname in filenames:
            fpath = os.path.join(root, fname)
            rel_path = os.path.relpath(fpath, input_dir)
            fsize = os.path.getsize(fpath)
            files.append((rel_path, fpath, fsize))

    if not files:
        print(f"No files found in {input_dir}")
        return 1

    count = len(files)
    print(f"Found {count} files")

    # Calculate offsets
    header_size = 4 + 4 + 8  # magic + file_count + total_size
    entry_size = 64 + 8 + 8 + 4  # name + offset + size + flags
    entries_size = len(files) * entry_size
    data_offset = 4 + 4 + 8 + len(files) * entry_size
    data_offset = (data_offset + 7) & ~7

    # Calculate total size
    total_size = data_offset
    for _, _, fsize in files:
        total_size += fsize
    total_size = (total_size + 7) & ~7

    print(f"Building ramdisk: {len(files)} files, {total_size} bytes")

    with open(output_file, 'wb') as out:
        # Write header
        out.write(struct.pack('<I', 0x444B4D52))  # RMDK magic
        out.write(struct.pack('<I', len(files)))  # file count
        total_size = 4 + 4 + 8 + len(files) * (64 + 8 + 8 + 4)
        total_size = (total_size + 7) & ~7
        for _, _, fsize in files:
            total_size += fsize
        out.write(struct.pack('<Q', total_size))

        # Write entries
        current_offset = 4 + 4 + 8 + len(files) * (64 + 8 + 8 + 4)
        current_offset = (current_offset + 7) & ~7

        for rel_name, _, fsize in files:
            entry = bytearray(64 + 8 + 8 + 4)
            name_bytes = os.path.basename(rel_name).encode('utf-8')[:63]
            entry[:len(name_bytes)] = name_bytes
            struct.pack_into('<Q', entry, 64, current_offset)
            struct.pack_into('<Q', entry, 72, os.path.getsize(os.path.join(sys.argv[1], rel_path)))
            struct.pack_into('<I', entry, 80, 0)
            out.write(entry)
            current_offset += os.path.getsize(fpath)

        # Align data start
        current_pos = out.tell()
        data_start = (current_pos + 7) & ~7
        if data_start > current_pos:
            out.write(b'\x00' * (data_start - current_pos))

        # Write file data
        for rel_name, fpath, fsize in files:
            with open(fpath, 'rb') as inf:
                out.write(inf.read())

    print(f"Created ramdisk: {output_file}")
    return 0

def main(input_dir, output_file):
    return build_ramdisk(input_dir, output_file)

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input_dir> <output_ramdisk>")
        sys.exit(1)
    sys.exit(main(sys.argv[1], sys.argv[2]))