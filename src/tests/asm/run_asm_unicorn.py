#!/usr/bin/env python3
import argparse
import sys
import os
try:
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    HAVE_UNICORN = True
except Exception:
    HAVE_UNICORN = False

parser = argparse.ArgumentParser()
parser.add_argument('mode', type=int)
# Optional legacy positional coordinates (mode x y)
parser.add_argument('posx', nargs='?', type=int, default=None)
parser.add_argument('posy', nargs='?', type=int, default=None)
parser.add_argument('--x', type=int, default=None)
parser.add_argument('--y', type=int, default=None)
# Debug: emit argv to stderr so callers (tests) can diagnose how the script was invoked
import sys as _sys
print(f"[run_asm_unicorn.py] ARGV: {_sys.argv}", file=_sys.stderr)
# enemy params
parser.add_argument('--enemy-x', type=int, default=None)
parser.add_argument('--enemy-y', type=int, default=None)
parser.add_argument('--player-x', type=int, default=None)
parser.add_argument('--player-y', type=int, default=None)
# fireball params
parser.add_argument('--fb-x', type=int, default=None)
parser.add_argument('--fb-y', type=int, default=None)
parser.add_argument('--fb-vel', type=int, default=None)
# Optional: set an individual map tile: --set-tile TX TY VAL
parser.add_argument('--set-tile', nargs=3, type=int, metavar=('TX','TY','VAL'), default=None)
# Allow legacy positional invocation: `run_asm_unicorn.py <mode> <x> <y>`
args = parser.parse_args()
# If legacy positional args were used (posx/posy), map them into flags for compatibility
if args.x is None and args.posx is not None:
    args.x = args.posx
if args.y is None and args.posy is not None:
    args.y = args.posy
mode = args.mode

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
COM_PATH = os.path.join(ROOT, 'build', 'asm', 'collision_harness.com')
if not os.path.isfile(COM_PATH):
    os.system(os.path.join(os.path.dirname(__file__), 'build_asm.sh'))

with open(COM_PATH, 'rb') as f:
    data = bytearray(f.read())

# Collect patches
patches = []
patches.append((b'MD', mode))
if args.x is not None:
    patches.append((b'INX', args.x))
if args.y is not None:
    patches.append((b'INY', args.y))
if args.enemy_x is not None:
    patches.append((b'ENX', args.enemy_x))
if args.enemy_y is not None:
    patches.append((b'ENY', args.enemy_y))
if args.player_x is not None:
    patches.append((b'PLX', args.player_x))
if args.player_y is not None:
    patches.append((b'PLY', args.player_y))
if args.fb_x is not None:
    patches.append((b'FBX', args.fb_x))
if args.fb_y is not None:
    patches.append((b'FBY', args.fb_y))
if args.fb_vel is not None:
    patches.append((b'FBV', args.fb_vel))

for marker, val in patches:
    idx = data.find(marker)
    if idx == -1:
        print(f"[run_asm_unicorn.py] marker {marker} not found", file=_sys.stderr)
        continue
    # Translate coordinates from C++ pixel units into the ASM harness units where needed.
    out_val = val
    if marker in (b'INX', b'INY') and val is not None:
        # C++ tests pass pixel coords; ASM harness expects coord / 8 (pixel->tile quarter units used by harness)
        out_val = val // 8
        print(f"[run_asm_unicorn.py] converting {marker.decode()} from {val} (pixels) -> {out_val} (asm units)", file=_sys.stderr)
    print(f"[run_asm_unicorn.py] patching marker {marker} at {idx} -> value {out_val}", file=_sys.stderr)
    data[idx+len(marker)] = out_val & 0xFF

# Optional: set an individual map tile at TX,TY to VAL (map width 128)
if args.set_tile is not None:
    tx, ty, val = args.set_tile
    map_marker = data.find(b'MAP')
    if map_marker != -1:
        map_start = map_marker + len(b'MAP')
        off = map_start + ty * 128 + tx
        if 0 <= off < len(data):
            old = data[off]
            data[off] = val & 0xFF
            print(f"[run_asm_unicorn.py] set tile at offset {off} (map_start {map_start}) from {old} to {data[off]}", file=_sys.stderr)

if not HAVE_UNICORN:
    # Fallback: write patched .com and run with DOSBox
    patched = os.path.join(ROOT, 'build', 'asm', 'collision_harness_patched.com')
    with open(patched, 'wb') as f:
        f.write(data)
    cmd = f"dosbox -noconsole -c \"mount c '{ROOT}/build'\" -c c: -c 'asm/collision_harness_patched.com' -c exit"
    os.system(cmd)
    # Try to read output from result file if present (fallback)
    resfile = os.path.join(ROOT, 'build', 'result.bin')
    if os.path.isfile(resfile):
        with open(resfile, 'rb') as fr:
            b = fr.read()
            if b:
                print(b[0])
                sys.exit(0)
    # Also try to show the debug slot after map (map_start + 1280) if present
    map_marker_off = data.find(b'MAP')
    if map_marker_off != -1:
        debug_off = map_marker_off + len(b'MAP') + 1280
        if 0 <= debug_off < len(data):
            print(f"[run_asm_unicorn.py] disk debug-slot @ {debug_off} = {data[debug_off]}", file=sys.stderr)
    print(0)
    sys.exit(0)

from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_CS, UC_X86_REG_SS
ADDRESS = 0x1000
MEM_SIZE = 0x4000
uc = Uc(UC_ARCH_X86, UC_MODE_16)
uc.mem_map(ADDRESS, MEM_SIZE)
ORG = 0x100
# Write the .com contents at ADDRESS + ORG so that labels assembled with 'org 0x100'
# address the expected runtime memory offsets (segment:offset addressing uses offsets >= ORG)
uc.mem_write(ADDRESS + ORG, bytes(data))
# Set segment registers so that segmented memory accesses (eg. [bx]) map into the loaded image
segment_value = ADDRESS >> 4
uc.reg_write(UC_X86_REG_DS, segment_value)
uc.reg_write(UC_X86_REG_CS, segment_value)
uc.reg_write(UC_X86_REG_SS, segment_value)
# Entry point for .com programs is at offset ORG (PSP header occupies first 256 bytes)
# Set segmented registers so the program entry (CS:IP) maps to ADDRESS + ORG
entry_linear = ADDRESS + ORG
entry_seg = entry_linear >> 4
entry_ip = entry_linear & 0xF
uc.reg_write(UC_X86_REG_CS, entry_seg)
# Use a separate DS that points to the file load base (ADDRESS)
data_seg = ADDRESS >> 4
uc.reg_write(UC_X86_REG_DS, data_seg)
uc.reg_write(UC_X86_REG_SS, entry_seg)
uc.reg_write(UC_X86_REG_IP, entry_ip)
# Also set EIP as a fallback (linear)
uc.reg_write(UC_X86_REG_EIP, entry_linear)
# Debug: confirm registers
try:
    ip = uc.reg_read(UC_X86_REG_IP)
    eip = uc.reg_read(UC_X86_REG_EIP)
    ds = uc.reg_read(UC_X86_REG_DS)
    cs = uc.reg_read(UC_X86_REG_CS)
    ss = uc.reg_read(UC_X86_REG_SS)
    print(f"[run_asm_unicorn.py] regs IP={hex(ip)} EIP={hex(eip)} CS={hex(cs)} DS={hex(ds)} SS={hex(ss)} entry_linear={hex(entry_linear)} entry_seg={hex(entry_seg)} entry_ip={hex(entry_ip)} data_seg={hex(data_seg)}", file=sys.stderr)
except Exception:
    pass

# Run until int 21 or program end

exec_count = 0

def hook_code(emu, address, size, user_data):
    global exec_count
    try:
        opcode = emu.mem_read(address, size)
    except Exception:
        return
    # Print the first few executed opcodes for debugging
    if exec_count < 200:
        try:
            print(f"[run_asm_unicorn.py] exec @ {hex(address)} opcode_bytes: {opcode.hex()}", file=sys.stderr)
        except Exception:
            pass
    exec_count += 1
    if opcode[0] == 0xCD:
        emu.emu_stop()
    # Extra debug: when we see a store into [si] (opcode 0x88 0x24 or 0x88 0x04), print ds:si
    try:
        if len(opcode) >= 2 and opcode[0] == 0x88 and opcode[1] in (0x24, 0x04):
            ds = emu.reg_read(UC_X86_REG_DS)
            si = emu.reg_read(UC_X86_REG_IP)  # placeholder; will replace with actual SI read below
            try:
                from unicorn.x86_const import UC_X86_REG_SI
                si = emu.reg_read(UC_X86_REG_SI)
                print(f"[run_asm_unicorn.py] store-[si] about to run: DS={hex(ds)} SI={hex(si)} linear={hex((ds<<4)+si)}", file=sys.stderr)
            except Exception:
                pass
        # Detect mov r8, [imm16] pattern (eg. mov ah, [in_x]) -> opcode 0x8A 0x26 <lo> <hi>
        if len(opcode) >= 4 and opcode[0] == 0x8A and opcode[1] == 0x26:
            imm = opcode[2] | (opcode[3] << 8)
            try:
                v = emu.mem_read((emu.reg_read(UC_X86_REG_DS) << 4) + imm, 1)[0]
                print(f"[run_asm_unicorn.py] mov r8,[{hex(imm)}] -> mem[{hex((emu.reg_read(UC_X86_REG_DS)<<4)+imm)}] = {v}", file=sys.stderr)
            except Exception:
                pass
        # Detect mov al,[imm16] pattern opcode 0xA0 <lo> <hi>
        if len(opcode) >= 3 and opcode[0] == 0xA0:
            imm = opcode[1] | (opcode[2] << 8)
            try:
                v = emu.mem_read((emu.reg_read(UC_X86_REG_DS) << 4) + imm, 1)[0]
                print(f"[run_asm_unicorn.py] mov al,[{hex(imm)}] -> mem[{hex((emu.reg_read(UC_X86_REG_DS)<<4)+imm)}] = {v}", file=sys.stderr)
            except Exception:
                pass
        # Detect mov [imm16], al (store into absolute memory) opcode 0xA2 <lo> <hi>
        if len(opcode) >= 3 and opcode[0] == 0xA2:
            imm = opcode[1] | (opcode[2] << 8)
            try:
                ax = emu.reg_read(UC_X86_REG_EIP)  # just to trigger reg read
                al = emu.reg_read(UC_X86_REG_EIP) & 0xFF
            except Exception:
                al = None
            try:
                print(f"[run_asm_unicorn.py] mov [imm],al at {hex(imm)} (linear {hex((emu.reg_read(UC_X86_REG_DS)<<4)+imm)}) AL={al}", file=sys.stderr)
            except Exception:
                pass
    except Exception:
        pass

uc.hook_add(UC_HOOK_CODE, hook_code)

try:
    # Find explicit start marker in the binary if present
    start_marker = data.find(b'ST')
    if start_marker != -1:
        # Start marker and other labels are expressed relative to ORG (0x100)
        start_addr = ADDRESS + ORG + start_marker + len(b'ST')
    else:
        # fallback: typical .com entry at ORG offset
        start_addr = ADDRESS + ORG
    uc.emu_start(start_addr, ADDRESS + ORG + len(data))
except Exception:
    pass

# Read an emulated memory snapshot starting at the in-memory base (ADDRESS + ORG)
# so marker offsets (which are assembled with ORG) match the indices in the snapshot.
mem = uc.mem_read(ADDRESS + ORG, len(data) + ORG)

# If we modified the map, print the value from the emulated memory for sanity
if args.set_tile is not None:
    tx, ty, val = args.set_tile
    map_marker = data.find(b'MAP')
    if map_marker != -1:
        map_start = map_marker + len(b'MAP')
        off = map_start + ty * 128 + tx
        try:
            # map tile file offset -> runtime memory offset includes ORG
            memval = uc.mem_read(ADDRESS + ORG + off, 1)[0]
            print(f"[run_asm_unicorn.py] emu mem at offset {off} = {memval}", file=_sys.stderr)
        except Exception as e:
            print(f"[run_asm_unicorn.py] error reading emu mem: {e}", file=_sys.stderr)
        # Read debug slot just after map region
        try:
            # Note: assembled code uses ORG 0x100, so runtime memory offsets are file offsets + ORG
            ORG = 0x100
            debug_mem_off = map_start + ORG + 1280
            dbg = [uc.mem_read(ADDRESS + debug_mem_off + i, 1)[0] for i in range(0, 20)]
            # print first 20 debug bytes after map for inspection
            print(f"[run_asm_unicorn.py] emu debug-slot @ {debug_mem_off} = {dbg}", file=_sys.stderr)
            # Extra debug: for vertical/horizontal modes, compute the tile index the ASM used
            try:
                if mode in (0, 1):
                    inx_idx = data.find(b'INX')
                    iny_idx = data.find(b'INY')
                    if inx_idx != -1 and iny_idx != -1:
                        ix = data[inx_idx + len(b'INX')]
                        iy = data[iny_idx + len(b'INY')]
                        ax = ix
                        bx = iy
                        # ASM divides coords by 2 before using tile indices
                        tx = ix >> 1
                        ty = iy >> 1
                        map_marker = data.find(b'MAP')
                        map_start = map_marker + len(b'MAP')
                        idx = map_start + ty * 128 + tx
                        val = uc.mem_read(ADDRESS + ORG + (idx), 1)[0]
                        print(f"[run_asm_unicorn.py] computed tile idx (file offset) = {idx}, mem val = {val}", file=_sys.stderr)
            except Exception:
                pass
        except Exception as e:
            print(f"[run_asm_unicorn.py] error reading debug-slot: {e}", file=_sys.stderr)

# For collision modes (0,1) prefer RS
if mode in (0,1):
    idx = mem.find(b'RS')
    if idx != -1:
        # print surrounding bytes for debugging
        print(mem[idx:idx+6])
        print(mem[idx+2])
        sys.exit(0)
# Enemy result for mode 2
if mode == 2:
    idx = mem.find(b'RNE')
    if idx != -1:
        xvel = mem[idx+1]
        yvel = mem[idx+2]
        print(f"{xvel} {yvel}")
        sys.exit(0)
# Fireball result for mode 3
if mode == 3:
    idx = mem.find(b'RNF')
    if idx != -1:
        fx = mem[idx+1]
        fy = mem[idx+2]
        active = mem[idx+3]
        print(f"{fx} {fy} {active}")
        sys.exit(0)
# Fallback: try any of them in a sensible order
idx = mem.find(b'RS')
if idx != -1:
    print(mem[idx+2])
    sys.exit(0)
idx = mem.find(b'RNE')
if idx != -1:
    xvel = mem[idx+1]
    yvel = mem[idx+2]
    print(f"{xvel} {yvel}")
    sys.exit(0)
idx = mem.find(b'RNF')
if idx != -1:
    fx = mem[idx+1]
    fy = mem[idx+2]
    active = mem[idx+3]
    print(f"{fx} {fy} {active}")
    sys.exit(0)
print(0)
