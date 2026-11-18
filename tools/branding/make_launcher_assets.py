#!/usr/bin/env python3
"""NimboPlay launcher icon packer. stdlib only: PNG => PS5 512px RGBA and Windows ICO.

The canonical user-supplied icon is assets/branding/nimboplay_launcher_256.png.
This intentionally does not touch the transparent INTERNAL menu logo.
"""
import argparse
import binascii
import pathlib
import struct
import zlib

SIGNATURE = b"\x89PNG\r\n\x1a\n"


def chunks(data):
    assert data.startswith(SIGNATURE), "Expected PNG source"
    pos = len(SIGNATURE)
    while pos + 12 <= len(data):
        length = struct.unpack_from(">I", data, pos)[0]
        kind = data[pos + 4:pos + 8]
        payload = data[pos + 8:pos + 8 + length]
        assert pos + 12 + length <= len(data), "Truncated PNG"
        assert binascii.crc32(kind + payload) & 0xFFFFFFFF == struct.unpack_from(">I", data, pos + 8 + length)[0], "PNG CRC mismatch"
        yield kind, payload
        pos += 12 + length
        if kind == b"IEND":
            break


def unpack_rgba(data):
    parts = list(chunks(data))
    ihdr = next(payload for kind, payload in parts if kind == b"IHDR")
    width, height, depth, typ, compression, filtering, interlace = struct.unpack(">IIBBBBB", ihdr)
    assert (depth, compression, filtering, interlace) == (8, 0, 0, 0)
    assert typ in (2, 3, 6), "Unsupported PNG type (expected RGB, palette, or RGBA)"
    bpp = {2: 3, 3: 1, 6: 4}[typ]
    stride = width * bpp
    raw = zlib.decompress(b"".join(payload for kind, payload in parts if kind == b"IDAT"))
    palette = next((payload for kind, payload in parts if kind == b"PLTE"), b"")
    alpha = next((payload for kind, payload in parts if kind == b"tRNS"), b"")
    prev = bytearray(stride)
    offset = 0
    rows = []
    for _ in range(height):
        f = raw[offset]
        cur = bytearray(raw[offset + 1:offset + 1 + stride])
        assert len(cur) == stride
        offset += 1 + stride
        assert f in range(5)
        for i in range(stride):
            a = cur[i - bpp] if i >= bpp else 0
            b = prev[i]
            c = prev[i - bpp] if i >= bpp else 0
            if f == 1:
                cur[i] = (cur[i] + a) & 255
            elif f == 2:
                cur[i] = (cur[i] + b) & 255
            elif f == 3:
                cur[i] = (cur[i] + ((a + b) // 2)) & 255
            elif f == 4:
                p = a + b - c
                ds = (abs(p - a), abs(p - b), abs(p - c))
                cur[i] = (cur[i] + (a if ds[0] <= ds[1] and ds[0] <= ds[2] else b if ds[1] <= ds[2] else c)) & 255
        prev = cur
        if typ == 6:
            rows.append(bytes(cur))
        elif typ == 2:
            rows.append(b"".join(bytes((*cur[i:i+3], 255)) for i in range(0, stride, 3)))
        else:
            rows.append(b"".join(bytes((*palette[idx * 3:idx * 3 + 3], alpha[idx] if idx < len(alpha) else 255))
                                 for idx in cur))
    return width, height, rows


def png_chunk(kind, data):
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", binascii.crc32(kind + data) & 0xFFFFFFFF)


def ps5_icon(png):
    width, height, rows = unpack_rgba(png)
    assert width == height and width >= 128
    # This 2x nearest mapping is deterministic across build environments.
    # The output is standard 512x512, 8-bit RGBA for PS5 icon0.png.
    target = 512
    out = bytearray()
    for y in range(target):
        row = rows[y * height // target]
        out.append(0)  # PNG filter None
        for x in range(target):
            i = (x * width // target) * 4
            out.extend(row[i:i + 4])
    ihdr = struct.pack(">IIBBBBB", target, target, 8, 6, 0, 0, 0)
    return SIGNATURE + png_chunk(b"IHDR", ihdr) + png_chunk(b"IDAT", zlib.compress(out, 9)) + png_chunk(b"IEND", b"")


def windows_ico(png):
    width, height, _ = unpack_rgba(png)
    assert width == height and width <= 256, "ICO input should be a square PNG up to 256px"
    # Windows supports a PNG-compressed ICO entry. 0 encodes 256px.
    header = struct.pack("<HHH", 0, 1, 1)
    entry = struct.pack("<BBBBHHII", 0 if width == 256 else width,
                        0 if height == 256 else height, 0, 0, 1, 32, len(png), 22)
    return header + entry + png


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True)
    parser.add_argument("--ps5")
    parser.add_argument("--ico")
    args = parser.parse_args()
    data = pathlib.Path(args.source).read_bytes()
    assert args.ps5 or args.ico, "Choose --ps5 and/or --ico"
    for path, payload in [(args.ps5, ps5_icon(data) if args.ps5 else b""),
                          (args.ico, windows_ico(data) if args.ico else b"")]:
        if path:
            dst = pathlib.Path(path)
            dst.parent.mkdir(parents=True, exist_ok=True)
            dst.write_bytes(payload)
            print("NimboPlay icon:", dst, len(payload), "bytes")


if __name__ == "__main__":
    main()
