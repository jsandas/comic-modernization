#!/usr/bin/env python3
"""Patch the built .com to set mode/in_x/in_y bytes by searching for markers.
Usage: patch_com.py <input.com> <output.com> --mode <0|1> --x <0-255> --y <0-255>
"""
import sys
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('infile')
parser.add_argument('outfile')
parser.add_argument('--mode', type=int, default=0)
parser.add_argument('--x', type=int, default=None)
parser.add_argument('--y', type=int, default=None)
# Enemy args
parser.add_argument('--enemy-x', type=int, default=None)
parser.add_argument('--enemy-y', type=int, default=None)
parser.add_argument('--player-x', type=int, default=None)
parser.add_argument('--player-y', type=int, default=None)
# Fireball args
parser.add_argument('--fb-x', type=int, default=None)
parser.add_argument('--fb-y', type=int, default=None)
parser.add_argument('--fb-vel', type=int, default=None)
args = parser.parse_args()

with open(args.infile, 'rb') as f:
    data = bytearray(f.read())

def patch_marker_if_present(marker: bytes, value: int):
    idx = data.find(marker)
    if idx == -1:
        return False
    pos = idx + len(marker)
    if pos >= len(data):
        raise SystemExit("Marker at EOF")
    data[pos] = value & 0xFF
    return True

patch_marker_if_present(b'MD', args.mode)
if args.x is not None:
    patch_marker_if_present(b'INX', args.x)
if args.y is not None:
    patch_marker_if_present(b'INY', args.y)
# Enemy
if args.enemy_x is not None:
    patch_marker_if_present(b'ENX', args.enemy_x)
if args.enemy_y is not None:
    patch_marker_if_present(b'ENY', args.enemy_y)
if args.player_x is not None:
    patch_marker_if_present(b'PLX', args.player_x)
if args.player_y is not None:
    patch_marker_if_present(b'PLY', args.player_y)
# Fireball
if args.fb_x is not None:
    patch_marker_if_present(b'FBX', args.fb_x)
if args.fb_y is not None:
    patch_marker_if_present(b'FBY', args.fb_y)
if args.fb_vel is not None:
    patch_marker_if_present(b'FBV', args.fb_vel)

with open(args.outfile, 'wb') as f:
    f.write(data)

print(f"Patched {args.outfile}: mode={args.mode} x={args.x} y={args.y} enemy_x={args.enemy_x} enemy_y={args.enemy_y} fb_x={args.fb_x} fb_y={args.fb_y} fb_vel={args.fb_vel}")
