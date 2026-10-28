#!/usr/bin/env python3
"""Joins ARMSX2's shader caches (GSCacheFile's blob stores: <name>.idx + <name>.bin) of several devices into one.

    python3 port/bt4/gsx/merge_cache.py <out base> <in base> [<in base> ...]
    e.g. merge_cache.py port/bt4/gsx/cache/vulkan_shaders_c0483380 cube/vulkan_shaders_c0483380 rp5/vulkan_shaders_c0483380

The stores must have the same header (kind, key size, stamp: the same ARMSX2 build and options). Every entry is
checked as ARMSX2 reads it (entry hash, data hash: XXH3-64) and the first store's entry wins a key both have.
Needs the Python package xxhash. See cache/README.md."""
import struct
import sys

import xxhash

HEADER = 56


def read(base):
    idx = open(base + ".idx", "rb").read()
    data = open(base + ".bin", "rb").read()
    head = idx[:HEADER]
    magic, kind, fmt, key_size = struct.unpack_from("<4I", head)
    if xxhash.xxh3_64_intdigest(head[:HEADER - 8]) != struct.unpack_from("<Q", head, HEADER - 8)[0]:
        sys.exit(f"{base}.idx: its header is damaged")
    size = key_size + 32
    entries = []
    for at in range(HEADER, len(idx) - size + 1, size):
        rec = idx[at:at + size]
        offset, length, aux, data_hash, entry_hash = struct.unpack_from("<QIIQQ", rec, key_size)
        if entry_hash != xxhash.xxh3_64_intdigest(rec[:size - 8]) or offset + length > len(data):
            print(f"{base}.idx: stops at a damaged entry ({len(entries)} read)")
            break
        blob = data[offset:offset + length]
        if xxhash.xxh3_64_intdigest(blob) != data_hash:
            print(f"{base}.bin: an entry fails its checksum; left out")
            continue
        entries.append((rec[:key_size], blob, aux))
    return head, key_size, entries


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    out, ins = sys.argv[1], sys.argv[2:]
    head, key_size, merged = read(ins[0])
    keys = {k for k, _, _ in merged}
    for base in ins[1:]:
        h, ks, entries = read(base)
        if h[:HEADER - 24] != head[:HEADER - 24] or ks != key_size:
            sys.exit(f"{base}: another kind of store or another build (its stamp differs from {ins[0]}'s)")
        added = 0
        for k, blob, aux in entries:
            if k not in keys:
                keys.add(k)
                merged.append((k, blob, aux))
                added += 1
        print(f"{base}: {len(entries)} entries, {added} new")
    idx, data = bytearray(head), bytearray()
    for k, blob, aux in merged:
        rec = k + struct.pack("<QIIQ", len(data), len(blob), aux, xxhash.xxh3_64_intdigest(blob))
        idx += rec + struct.pack("<Q", xxhash.xxh3_64_intdigest(rec))
        data += blob
    open(out + ".bin", "wb").write(data)
    open(out + ".idx", "wb").write(idx)
    print(f"{out}: {len(merged)} entries, {len(data)} bytes of data")


if __name__ == "__main__":
    main()
