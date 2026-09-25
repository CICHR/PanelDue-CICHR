#!/usr/bin/env python3
from pathlib import Path
import argparse
import struct
import sys

try:
    from PIL import Image
except ImportError:
    print('Pillow is required. Install it with: py -m pip install pillow', file=sys.stderr)
    raise SystemExit(2)


def rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def from_rgb565(value):
    r5 = (value >> 11) & 0x1F
    g6 = (value >> 5) & 0x3F
    b5 = value & 0x1F
    return (
        (r5 << 3) | (r5 >> 2),
        (g6 << 2) | (g6 >> 4),
        (b5 << 3) | (b5 >> 2),
    )


def encode_png(source, destination):
    image = Image.open(source).convert('RGB')
    width, height = image.size
    if (width, height) not in ((480, 272), (800, 480)):
        raise ValueError(f'Unsupported splash size {width}x{height}. Use 480x272 or 800x480.')

    pixels = image.load()
    stream = []
    for y in range(height - 1, -1, -1):
        for x in range(width):
            stream.append(rgb565(*pixels[x, y]))

    encoded = bytearray(struct.pack('<HH', width, height))
    i = 0
    while i < len(stream):
        colour = stream[i]
        run = 1
        while i + run < len(stream) and stream[i + run] == colour and run < 65536:
            run += 1
        encoded += struct.pack('<HH', run - 1, colour)
        i += run

    Path(destination).write_bytes(encoded)
    print(f'Created {destination}: {width}x{height}, {len(encoded)} bytes')


def decode_bin(source, destination):
    data = Path(source).read_bytes()
    if len(data) < 4 or len(data) % 2:
        raise ValueError('Invalid PanelDue splash binary.')

    width, height = struct.unpack_from('<HH', data, 0)
    expected = width * height
    pixels = []
    offset = 4
    while offset + 4 <= len(data) and len(pixels) < expected:
        repeat_minus_one, colour = struct.unpack_from('<HH', data, offset)
        offset += 4
        pixels.extend([colour] * (repeat_minus_one + 1))

    if len(pixels) != expected:
        raise ValueError(f'Invalid RLE stream: decoded {len(pixels)} pixels, expected {expected}.')

    image = Image.new('RGB', (width, height))
    out = image.load()
    pos = 0
    for y in range(height - 1, -1, -1):
        for x in range(width):
            out[x, y] = from_rgb565(pixels[pos])
            pos += 1

    image.save(destination)
    print(f'Created {destination}: {width}x{height}')


def main():
    parser = argparse.ArgumentParser(description='Encode or decode PanelDue CICHR splash screens.')
    sub = parser.add_subparsers(dest='command', required=True)

    enc = sub.add_parser('encode', help='Convert a PNG to PanelDue splash .bin format')
    enc.add_argument('source')
    enc.add_argument('destination')

    dec = sub.add_parser('decode', help='Convert a PanelDue splash .bin to an editable PNG')
    dec.add_argument('source')
    dec.add_argument('destination')

    args = parser.parse_args()
    if args.command == 'encode':
        encode_png(args.source, args.destination)
    else:
        decode_bin(args.source, args.destination)


if __name__ == '__main__':
    main()
