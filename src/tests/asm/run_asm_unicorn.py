#!/usr/bin/env python3
import argparse
import sys
import os
try:
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_READ
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
from collections import deque
# enemy params
parser.add_argument('--enemy-x', type=int, default=None)
parser.add_argument('--enemy-y', type=int, default=None)
parser.add_argument('--player-x', type=int, default=None)
parser.add_argument('--player-y', type=int, default=None)
# fireball params
parser.add_argument('--fb-x', type=int, default=None)
parser.add_argument('--fb-y', type=int, default=None)
parser.add_argument('--fb-vel', type=int, default=None)
parser.add_argument('--force-jump', action='store_true', default=False, help='For debugging: patch CALL into JMP when a CALL is detected (one-shot)')
parser.add_argument('--force-aggr', action='store_true', default=False, help='Install aggressive mem-write hook immediately (global)')
parser.add_argument('--force-decision', action='store_true', default=False, help='Enable decision-region instrumentation (0x1670-0x16b0)')
parser.add_argument('--inline-calls', action='store_true', default=False, help='Do not isolate callees: let CALL targets execute inline in the main emulator')

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
        # C++ tests pass pixel coords in pixels. The ASM harness expects a small encoded
        # value where tile = floor(x / 16) and the low bit signals whether the box
        # spans into the next tile (non-zero pixel offset inside the tile).
        tile = val // 16
        span = 1 if (val % 16) != 0 else 0
        out_val = (tile << 1) | span
        print(f"[run_asm_unicorn.py] converting {marker.decode()} from {val} (pixels) -> {out_val} (asm units: tile={tile}, span={span})", file=_sys.stderr)
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
    # Also try to show the debug area (DBG) if present in the file
    dbg_marker_off = data.find(b'DBG')
    if dbg_marker_off != -1:
        dbg_start = dbg_marker_off + len(b'DBG')
        try:
            dbg = [data[dbg_start + i] for i in range(0, 20)]
            print(f"[run_asm_unicorn.py] disk debug-slot @ {dbg_start} = {dbg}", file=sys.stderr)
        except Exception:
            pass
    print(0)
    sys.exit(0)

from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_CS, UC_X86_REG_SS, UC_X86_REG_ES
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
# Monotonic counter for memory writes so we can see ordering of writes (helps locate clobbering)
mem_write_counter = 0
trace_active = False
trace_remaining = 0
# Track recent DBG writes so we can report exact write ordering when inspecting return-slot clobbers
dbg_write_history = []  # list of dicts: {'seq':int, 'address':int, 'offset':int, 'size':int, 'written_hex':str, 'pc':int}
dbg_last_write = {}     # map offset -> (seq, written_hex, pc)
# Isolated-run write tracking (separate emulator instance)
isolated_mem_write_counter = 0
isolated_dbg_write_history = []
isolated_dbg_last_write = {}
# Unconditional per-instruction control-flow trace (short window after CALL)
cf_trace_active = False
cf_trace_remaining = 0
cf_trace_origin = None
last_call_target = None
call_targets_seen = []
inline_call_active = False
inline_return_linear = None
inline_restore_ss = False
inline_saved_ss = None
inline_saved_sp = None
inline_used_helper = False
# Recent executed IP history (address, opcode_hex) so we can find the immediate writer
last_ips = deque(maxlen=256)
# Watchpoints for call-target addresses we want to log and single-step in the main run
entry_watchpoints = set()
# When we see a CALL, set this so the next executed instruction can be inspected
expect_post_call = None
# debug: if set, patch CALL -> JMP at runtime when we see it in the main run
force_jump_on_call = False
# Aggressive per-instruction mem-write monitoring (installed temporarily)
aggressive_hook_id = None
aggressive_active = False
aggressive_range_start = 0x1690
aggressive_range_end = 0x16a0

def hook_code(emu, address, size, user_data):
    global exec_count, trace_active, trace_remaining, last_call_target, expect_post_call, single_step_active, single_step_remaining, single_step_origin, cf_trace_active, cf_trace_remaining, cf_trace_origin, aggressive_hook_id, aggressive_active, inline_call_active, inline_return_linear, inline_restore_ss, inline_saved_ss, inline_saved_sp, inline_used_helper
    try:
        opcode = emu.mem_read(address, size)
    except Exception:
        return
    # record recent execution history (linear address and first few opcode bytes)
    try:
        last_ips.append((address, opcode.hex()))
    except Exception:
        pass
    # If we expected to inspect the instruction after a CALL, do so now and clear the flag
    try:
        global expect_post_call
        if expect_post_call is not None:
            from unicorn.x86_const import UC_X86_REG_IP, UC_X86_REG_CS, UC_X86_REG_SS, UC_X86_REG_SP, UC_X86_REG_DS, UC_X86_REG_ES
            regs_post = {
                'IP': emu.reg_read(UC_X86_REG_IP),
                'CS': emu.reg_read(UC_X86_REG_CS),
                'SS': emu.reg_read(UC_X86_REG_SS),
                'SP': emu.reg_read(UC_X86_REG_SP),
                'DS': emu.reg_read(UC_X86_REG_DS),
                'ES': emu.reg_read(UC_X86_REG_ES)
            }
            linear_stack = (regs_post['SS'] << 4) + regs_post['SP']
            try:
                stack_dump = [emu.mem_read(linear_stack + i, 1)[0] for i in range(0, 16)]
            except Exception:
                stack_dump = []
            print(f"[run_asm_unicorn.py] POST-CALL CHECK @ {hex(address)} expecting {hex(expect_post_call)}; REGS: {regs_post}; stack_top_linear={hex(linear_stack)} stack_bytes={stack_dump}", file=sys.stderr)
            expect_post_call = None
    except Exception:
        pass
    # If this address matches an entry watchpoint (set when a CALL was observed),
    # emit a one-shot detailed dump and enable an instruction window for inspection.
    try:
        if address in entry_watchpoints:
            try:
                from unicorn.x86_const import UC_X86_REG_IP, UC_X86_REG_CS, UC_X86_REG_SS, UC_X86_REG_SP, UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI
                regs_entry = {
                    'IP': emu.reg_read(UC_X86_REG_IP),
                    'CS': emu.reg_read(UC_X86_REG_CS),
                    'SS': emu.reg_read(UC_X86_REG_SS),
                    'SP': emu.reg_read(UC_X86_REG_SP),
                    'AX': emu.reg_read(UC_X86_REG_AX),
                    'BX': emu.reg_read(UC_X86_REG_BX),
                    'CX': emu.reg_read(UC_X86_REG_CX),
                    'DX': emu.reg_read(UC_X86_REG_DX),
                    'SI': emu.reg_read(UC_X86_REG_SI),
                    'DI': emu.reg_read(UC_X86_REG_DI)
                }
                linear_stack = (regs_entry['SS'] << 4) + regs_entry['SP']
                try:
                    stack_dump = [emu.mem_read(linear_stack + i, 1)[0] for i in range(0, 32)]
                except Exception:
                    stack_dump = []
                print(f"[run_asm_unicorn.py] ENTRY-HIT @ {hex(address)}; REGS: {regs_entry}; stack_top_linear={hex(linear_stack)} stack_bytes={stack_dump}", file=sys.stderr)
                # Enable a short instruction trace window so subsequent instruction
                # activity (particularly memory writes) is captured verbosely.
                trace_active = True
                trace_remaining = 2000
            except Exception:
                pass
            try:
                entry_watchpoints.discard(address)
            except Exception:
                pass
    except Exception:
        pass

    # Decision-region instrumentation (controlled by --force-decision):
    try:
        if args.force_decision:
            # decision address window chosen for inspection
            decision_start = 0x1670
            decision_end = 0x16b0
            if decision_start <= address <= decision_end:
                try:
                    # gather registers and flags
                    from unicorn.x86_const import UC_X86_REG_IP, UC_X86_REG_CS, UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP
                    regs_dec = {
                        'IP': emu.reg_read(UC_X86_REG_IP),
                        'CS': emu.reg_read(UC_X86_REG_CS),
                        'AX': emu.reg_read(UC_X86_REG_AX),
                        'BX': emu.reg_read(UC_X86_REG_BX),
                        'CX': emu.reg_read(UC_X86_REG_CX),
                        'DX': emu.reg_read(UC_X86_REG_DX),
                        'SI': emu.reg_read(UC_X86_REG_SI),
                        'DI': emu.reg_read(UC_X86_REG_DI),
                        'BP': emu.reg_read(UC_X86_REG_BP),
                        'SP': emu.reg_read(UC_X86_REG_SP)
                    }
                    # try to read flags
                    try:
                        from unicorn.x86_const import UC_X86_REG_EFLAGS
                        flags = emu.reg_read(UC_X86_REG_EFLAGS)
                    except Exception:
                        flags = None
                    try:
                        instr = emu.mem_read(address, 8)
                        instr_hex = instr.hex()
                    except Exception:
                        instr_hex = '<err>'
                    print(f"[run_asm_unicorn.py] DECISION HIT @ {hex(address)} OPC={instr_hex} REGS={regs_dec} FLAGS={flags}", file=sys.stderr)
                    # If this looks like a conditional branch, print computed target
                    b0 = instr[0] if len(instr) > 0 else None
                    if b0 is not None:
                        # short Jcc 0x70-0x7F
                        if 0x70 <= b0 <= 0x7F and len(instr) >= 2:
                            rel = instr[1] if instr[1] < 0x80 else instr[1] - 0x100
                            target = address + 2 + rel
                            print(f"[run_asm_unicorn.py] DECISION BRANCH SHORT @{hex(address)} -> {hex(target)}", file=sys.stderr)
                        # Jcc near 0F 80..8F imm16
                        elif b0 == 0x0F and len(instr) >= 4 and 0x80 <= instr[1] <= 0x8F:
                            rel = instr[2] | (instr[3] << 8)
                            if rel & 0x8000:
                                rel -= 0x10000
                            target = address + 4 + rel
                            print(f"[run_asm_unicorn.py] DECISION BRANCH NEAR @{hex(address)} -> {hex(target)}", file=sys.stderr)
                    # Enable a short trace window so we capture the immediate control-flow
                    trace_active = True
                    trace_remaining = max(trace_remaining, 64)
                except Exception:
                    pass
    except Exception:
        pass

    # If single-step window is active, emit full per-instruction register dumps
    # and stop single-stepping on RET or when the counter expires.
    try:
        global single_step_active, single_step_remaining
        if single_step_active:
            try:
                from unicorn.x86_const import UC_X86_REG_IP, UC_X86_REG_CS, UC_X86_REG_SS, UC_X86_REG_SP, UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_DS, UC_X86_REG_ES
                regs_now = {
                    'IP': emu.reg_read(UC_X86_REG_IP),
                    'CS': emu.reg_read(UC_X86_REG_CS),
                    'SS': emu.reg_read(UC_X86_REG_SS),
                    'SP': emu.reg_read(UC_X86_REG_SP),
                    'AX': emu.reg_read(UC_X86_REG_AX),
                    'BX': emu.reg_read(UC_X86_REG_BX),
                    'CX': emu.reg_read(UC_X86_REG_CX),
                    'DX': emu.reg_read(UC_X86_REG_DX),
                    'SI': emu.reg_read(UC_X86_REG_SI),
                    'DI': emu.reg_read(UC_X86_REG_DI),
                    'DS': emu.reg_read(UC_X86_REG_DS),
                    'ES': emu.reg_read(UC_X86_REG_ES)
                }
                pc_now = (regs_now['CS'] << 4) + regs_now['IP']
                try:
                    instr = emu.mem_read(pc_now, 8)
                    instr_hex = instr.hex()
                except Exception:
                    instr_hex = '<err>'
                print(f"[run_asm_unicorn.py] STEP @{hex(address)} PC={hex(pc_now)} OPC={opcode.hex()} INSTR={instr_hex} REGS={regs_now}", file=sys.stderr)
                # If this instruction is a RET (0xC3) we probably finished the callee
                if opcode[0] == 0xC3:
                    print(f"[run_asm_unicorn.py] STEP saw RET at {hex(address)} — ending single-step", file=sys.stderr)
                    # If an inline-call is active, handle the inline return explicitly
                    try:
                        if inline_call_active:
                            # pop return (adjust SP) and jump to stored return IP
                            try:
                                from unicorn.x86_const import UC_X86_REG_SP, UC_X86_REG_SS, UC_X86_REG_CS, UC_X86_REG_IP
                                sp_now = emu.reg_read(UC_X86_REG_SP)
                                emu.reg_write(UC_X86_REG_SP, (sp_now + 2) & 0xFFFF)
                                # set IP to inline_return_linear (offset within current CS)
                                cs_now = emu.reg_read(UC_X86_REG_CS)
                                ret_off = inline_return_linear - (cs_now << 4)
                                emu.reg_write(UC_X86_REG_IP, ret_off & 0xFFFF)
                                # if we used a helper stack slot, restore original SS/SP
                                try:
                                    if inline_restore_ss:
                                        emu.reg_write(UC_X86_REG_SS, inline_saved_ss)
                                        emu.reg_write(UC_X86_REG_SP, inline_saved_sp)
                                        inline_restore_ss = False
                                        inline_used_helper = False
                                except Exception as _e:
                                    print(f"[run_asm_unicorn.py] error restoring SS/SP after inline return: {_e}", file=sys.stderr)
                                print(f"[run_asm_unicorn.py] INLINE-RET: returning to {hex(inline_return_linear)}; cleared inline state", file=sys.stderr)
                            except Exception as _e:
                                print(f"[run_asm_unicorn.py] error performing inline return: {_e}", file=sys.stderr)
                            inline_call_active = False
                            inline_return_linear = None
                    except Exception:
                        pass
                    single_step_active = False
                    single_step_remaining = 0
                else:
                    single_step_remaining -= 1
                    if single_step_remaining <= 0:
                        single_step_active = False
            except Exception:
                single_step_active = False
    except Exception:
        pass

    # If single-step finished and an aggressive hook is present, remove it
    try:
        if not single_step_active and aggressive_active and aggressive_hook_id is not None:
            try:
                uc.hook_del(aggressive_hook_id)
            except Exception:
                pass
            aggressive_hook_id = None
            aggressive_active = False
            print(f"[run_asm_unicorn.py] AGGRESSIVE monitor removed after single-step", file=sys.stderr)
    except Exception:
        pass

    # Unconditional per-instruction control-flow trace (CFTRACE):
    # when active, log the executed IP each instruction and stop on RET or expiration.
    try:
        global cf_trace_active, cf_trace_remaining, cf_trace_origin
        if cf_trace_active:
            try:
                from unicorn.x86_const import UC_X86_REG_IP, UC_X86_REG_CS, UC_X86_REG_DS, UC_X86_REG_ES
                regs_cf = {
                    'IP': emu.reg_read(UC_X86_REG_IP),
                    'CS': emu.reg_read(UC_X86_REG_CS),
                    'DS': emu.reg_read(UC_X86_REG_DS),
                    'ES': emu.reg_read(UC_X86_REG_ES)
                }
                pc_cf = (regs_cf['CS'] << 4) + regs_cf['IP']
                try:
                    instr = emu.mem_read(pc_cf, 8)
                    instr_hex = instr.hex()
                except Exception:
                    instr_hex = '<err>'
                print(f"[run_asm_unicorn.py] CFTRACE @{hex(address)} PC={hex(pc_cf)} OPC={opcode.hex()} INSTR={instr_hex} REGS={regs_cf} ORIGIN={hex(cf_trace_origin) if cf_trace_origin else None}", file=sys.stderr)
                # stop on RET variants
                if opcode[0] in (0xC3, 0xC2, 0xCB, 0xCA):
                    print(f"[run_asm_unicorn.py] CFTRACE saw RET at {hex(address)} — ending CFTRACE", file=sys.stderr)
                    cf_trace_active = False
                    cf_trace_remaining = 0
                else:
                    cf_trace_remaining -= 1
                    if cf_trace_remaining <= 0:
                        cf_trace_active = False
            except Exception:
                cf_trace_active = False
    except Exception:
        pass

    # If we're in trace_active mode, print everything and count down, and do aggressive control-flow logging
    if trace_active:
        try:
            print(f"[run_asm_unicorn.py] TRACE @ {hex(address)} opcode_bytes: {opcode.hex()}", file=sys.stderr)
        except Exception:
            pass
        try:
            # Aggressively log branch/call/ret targets so we can see control-flow divergence
            b0 = opcode[0] if len(opcode) > 0 else None
            if b0 is not None:
                try:
                    from unicorn.x86_const import UC_X86_REG_IP, UC_X86_REG_CS, UC_X86_REG_SS, UC_X86_REG_SP, UC_X86_REG_DS, UC_X86_REG_ES
                    regs_br = {
                        'IP': emu.reg_read(UC_X86_REG_IP),
                        'CS': emu.reg_read(UC_X86_REG_CS),
                        'SS': emu.reg_read(UC_X86_REG_SS),
                        'SP': emu.reg_read(UC_X86_REG_SP),
                        'DS': emu.reg_read(UC_X86_REG_DS),
                        'ES': emu.reg_read(UC_X86_REG_ES)
                    }
                    pc = (regs_br['CS'] << 4) + regs_br['IP']
                except Exception:
                    regs_br = {}
                    pc = address
                # CALL near rel16
                if b0 == 0xE8 and len(opcode) >= 3:
                    rel = opcode[1] | (opcode[2] << 8)
                    if rel & 0x8000:
                        rel -= 0x10000
                    target = address + size + rel
                    print(f"[run_asm_unicorn.py] TRACE CALL @{hex(address)} -> {hex(target)} REGS={regs_br}", file=sys.stderr)
                # JMP rel16
                elif b0 == 0xE9 and len(opcode) >= 3:
                    rel = opcode[1] | (opcode[2] << 8)
                    if rel & 0x8000:
                        rel -= 0x10000
                    target = address + size + rel
                    print(f"[run_asm_unicorn.py] TRACE JMP @{hex(address)} -> {hex(target)} REGS={regs_br}", file=sys.stderr)
                # JMP rel8
                elif b0 == 0xEB and len(opcode) >= 2:
                    rel = opcode[1] if opcode[1] < 0x80 else opcode[1] - 0x100
                    target = address + size + rel
                    print(f"[run_asm_unicorn.py] TRACE JMP8 @{hex(address)} -> {hex(target)} REGS={regs_br}", file=sys.stderr)
                # Conditional short jump 0x70-0x7F
                elif 0x70 <= b0 <= 0x7F and len(opcode) >= 2:
                    rel = opcode[1] if opcode[1] < 0x80 else opcode[1] - 0x100
                    target = address + size + rel
                    ft = address + size
                    print(f"[run_asm_unicorn.py] TRACE JCC @{hex(address)} -> {hex(target)} (fallthrough {hex(ft)}) REGS={regs_br}", file=sys.stderr)
                # Jcc near 0F 80..8F imm16
                elif b0 == 0x0F and len(opcode) >= 4 and 0x80 <= opcode[1] <= 0x8F:
                    rel = opcode[2] | (opcode[3] << 8)
                    if rel & 0x8000:
                        rel -= 0x10000
                    target = address + size + rel
                    ft = address + size
                    print(f"[run_asm_unicorn.py] TRACE JCC_NEAR @{hex(address)} -> {hex(target)} (fallthrough {hex(ft)}) REGS={regs_br}", file=sys.stderr)
                # RET variants
                elif b0 in (0xC3, 0xC2, 0xCB, 0xCA):
                    try:
                        linear_stack = (regs_br.get('SS', 0) << 4) + regs_br.get('SP', 0)
                        lo = emu.mem_read(linear_stack, 1)[0]
                        hi = emu.mem_read(linear_stack + 1, 1)[0]
                        ret_ip = lo | (hi << 8)
                        print(f"[run_asm_unicorn.py] TRACE RET @{hex(address)} will return to IP {hex(ret_ip)} (stack_linear {hex(linear_stack)}) REGS={regs_br}", file=sys.stderr)
                    except Exception:
                        print(f"[run_asm_unicorn.py] TRACE RET @{hex(address)} REGS={regs_br}", file=sys.stderr)
                # LOOP / JCXZ / JRCXZ short jumps 0xE0-0xE3
                elif b0 in (0xE0, 0xE1, 0xE2, 0xE3) and len(opcode) >= 2:
                    rel = opcode[1] if opcode[1] < 0x80 else opcode[1] - 0x100
                    target = address + size + rel
                    print(f"[run_asm_unicorn.py] TRACE LOOP/JCXZ @{hex(address)} -> {hex(target)} REGS={regs_br}", file=sys.stderr)
        except Exception:
            pass
        try:
            trace_remaining -= 1
            if trace_remaining <= 0:
                trace_active = False
        except Exception:
            pass
    # Print the first few executed opcodes for debugging (regular mode)
    if exec_count < 2000:
        try:
            print(f"[run_asm_unicorn.py] exec @ {hex(address)} opcode_bytes: {opcode.hex()}", file=sys.stderr)
        except Exception:
            pass
    exec_count += 1
    if opcode[0] == 0xCD:
        emu.emu_stop()
    # Detect CALL opcode (0xE8) and print its computed target and bytes there
    if opcode[0] == 0xE8 and len(opcode) >= 3:
        rel = opcode[1] | (opcode[2] << 8)
        # sign-extend 16-bit
        if rel & 0x8000:
            rel = rel - 0x10000
        target = address + size + rel
        try:
            # Print register/context at point of CALL (pre-call state)
            from unicorn.x86_const import UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_SS
            regs_pre = {
                'AX': emu.reg_read(UC_X86_REG_AX),
                'BX': emu.reg_read(UC_X86_REG_BX),
                'CX': emu.reg_read(UC_X86_REG_CX),
                'DX': emu.reg_read(UC_X86_REG_DX),
                'SI': emu.reg_read(UC_X86_REG_SI),
                'DI': emu.reg_read(UC_X86_REG_DI),
                'BP': emu.reg_read(UC_X86_REG_BP),
                'SP': emu.reg_read(UC_X86_REG_SP),
                'SS': emu.reg_read(UC_X86_REG_SS),
                'DS': emu.reg_read(UC_X86_REG_DS),
                'ES': emu.reg_read(UC_X86_REG_ES)
            }
            # Capture a small snapshot of stack bytes so we can replay stack contents for isolated emulation
            try:
                stack_linear = (regs_pre['SS'] << 4) + regs_pre['SP']
                stack_bytes = []
                for i in range(0, 64):
                    stack_bytes.append(emu.mem_read(stack_linear + i, 1)[0])
                regs_pre['stack'] = stack_bytes
            except Exception:
                regs_pre['stack'] = []

            print(f"[run_asm_unicorn.py] call at {hex(address)} -> target {hex(target)}; PRE-REGS: {regs_pre}", file=sys.stderr)
            nb = emu.mem_read(target, 16)
            print(f"[run_asm_unicorn.py] call-target bytes @ {hex(target)}: {nb.hex()}", file=sys.stderr)
            # Record target for later isolated inspection and set an entry watchpoint
            last_call_target = target
            # If requested, emulate the CALL inline now by pushing the return IP onto
            # the caller's stack and forcing IP to the callee target. This runs the
            # callee inside the main emulator so its memory writes are visible there.
            try:
                if getattr(args, 'inline_calls', False):
                    try:
                        from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_SS, UC_X86_REG_SP, UC_X86_REG_IP
                        cs = regs_pre.get('CS', emu.reg_read(UC_X86_REG_CS))
                        ss = regs_pre.get('SS', emu.reg_read(UC_X86_REG_SS))
                        sp = regs_pre.get('SP', emu.reg_read(UC_X86_REG_SP))
                        # compute the return IP (linear) and the offset to store
                        ret_linear = (ss << 4) + ((sp - 2) & 0xFFFF)
                        ret_ip_linear = address + size
                        ret_ip_off = ret_ip_linear - (cs << 4)
                        ret_lo = ret_ip_off & 0xFF
                        ret_hi = (ret_ip_off >> 8) & 0xFF
                        # write return IP into caller's stack (low, high)
                        try:
                            emu.mem_write(ret_linear, bytes([ret_lo]))
                            emu.mem_write(ret_linear + 1, bytes([ret_hi]))
                        except Exception as _e:
                            print(f"[run_asm_unicorn.py] error writing return IP for inline call: {_e}", file=sys.stderr)
                            # Fallback: if the intended stack slot isn't writable, allocate a
                            # helper return slot inside the mapped image and switch SS/SP
                            # to point there for the duration of the inline call.
                            try:
                                helper_off = ORG + 0x300
                                helper_linear = ADDRESS + helper_off
                                helper_lo = ret_ip_linear & 0xFF
                                helper_hi = (ret_ip_linear >> 8) & 0xFF
                                emu.mem_write(helper_linear, bytes([helper_lo]))
                                emu.mem_write(helper_linear + 1, bytes([helper_hi]))
                                # If we have a captured caller stack snapshot, copy a window of it
                                # into the helper area so the callee sees the same stack layout.
                                # Copy more bytes (256) to reduce risk of divergence.
                                try:
                                    stack_bytes = regs_pre.get('stack') if isinstance(regs_pre, dict) else None
                                except Exception:
                                    stack_bytes = None
                                stack_window = 256
                                try:
                                    if stack_bytes:
                                        maxc = min(stack_window, len(stack_bytes))
                                        for i in range(maxc):
                                            emu.mem_write(helper_linear + 2 + i, bytes([stack_bytes[i]]))
                                    else:
                                        # best-effort read from original stack linear address
                                        try:
                                            orig_stack_base = (ss << 4) + (sp & 0xFFFF)
                                            read_len = stack_window
                                            stack_data = emu.mem_read(orig_stack_base, read_len)
                                            for i, b in enumerate(bytes(stack_data)):
                                                emu.mem_write(helper_linear + 2 + i, bytes([b]))
                                        except Exception:
                                            pass
                                except Exception:
                                    pass

                                # Build a trampoline stub in the image that will push registers,
                                # CALL the callee (so the callee's RET returns to the stub),
                                # then pop registers and RET back to the original flow.
                                try:
                                    helper_code_off = ORG + 0x500
                                    helper_code_linear = ADDRESS + helper_code_off
                                    # push ax,bx,cx,dx,si,di,bp
                                    push_bytes = b'\x50\x53\x51\x52\x56\x57\x55'
                                    # placeholder for CALL (E8 + rel16)
                                    call_opcode = b'\xE8' + b'\x00\x00'
                                    # pop bp,di,si,dx,cx,bx,ax; ret
                                    pop_bytes = b'\x5D\x5F\x5E\x5A\x59\x5B\x58\xC3'
                                    # compute rel16 for the CALL (target is a linear address);
                                    # compute the relative offset based on stub linear address
                                    call_site_next = helper_code_linear + len(push_bytes) + len(call_opcode)
                                    # compute relative offset to the callee target (linear)
                                    rel_val = (target - call_site_next) & 0xFFFF
                                    call_bytes = b'\xE8' + (rel_val.to_bytes(2,'little'))
                                    stub = push_bytes + call_bytes + pop_bytes
                                    # Write stub out
                                    emu.mem_write(helper_code_linear, stub)
                                    # Prepare helper SS/SP to point at the copied stack window end so
                                    # pushes in the stub and callee will land inside our copied window.
                                    new_ss = (helper_linear >> 4)
                                    stack_base = helper_linear + 2
                                    new_sp = (stack_base + stack_window) - (new_ss << 4)
                                    # save old SS/SP to restore on return
                                    from unicorn.x86_const import UC_X86_REG_SS, UC_X86_REG_SP
                                    inline_saved_ss = emu.reg_read(UC_X86_REG_SS)
                                    inline_saved_sp = emu.reg_read(UC_X86_REG_SP)
                                    # set SS/SP to helper region
                                    emu.reg_write(UC_X86_REG_SS, new_ss)
                                    emu.reg_write(UC_X86_REG_SP, new_sp & 0xFFFF)
                                    inline_restore_ss = True
                                    inline_used_helper = True
                                    # Jump to trampoline stub to perform the inline CALL there
                                    from unicorn.x86_const import UC_X86_REG_IP, UC_X86_REG_CS
                                    cs = regs_pre.get('CS', emu.reg_read(UC_X86_REG_CS))
                                    helper_stub_off = helper_code_linear - (cs << 4)
                                    emu.reg_write(UC_X86_REG_IP, helper_stub_off & 0xFFFF)
                                except Exception as _e:
                                    print(f"[run_asm_unicorn.py] trampoline helper allocation failed: {_e}", file=sys.stderr)
                                    inline_restore_ss = False
                                    inline_used_helper = False
                            except Exception as _ee:
                                print(f"[run_asm_unicorn.py] helper return slot allocation failed: {_ee}", file=sys.stderr)
                            except Exception as _ee:
                                print(f"[run_asm_unicorn.py] helper return slot allocation failed: {_ee}", file=sys.stderr)
                        # update SP to reflect pushed return IP
                        try:
                            # If we used a helper stack region, the SP was already set to
                            # point at the helper return slot; do NOT overwrite it with
                            # the original caller SP value (that would be in a different SS).
                            if not inline_used_helper:
                                emu.reg_write(UC_X86_REG_SP, (sp - 2) & 0xFFFF)
                        except Exception:
                            pass
                        # If we didn't start execution through the trampoline helper, set IP
                        try:
                            if not inline_used_helper:
                                target_ip = target - (cs << 4)
                                emu.reg_write(UC_X86_REG_IP, target_ip & 0xFFFF)
                            else:
                                print(f"[run_asm_unicorn.py] trampoline in use; execution moved to helper stub", file=sys.stderr)
                        except Exception as _e:
                            print(f"[run_asm_unicorn.py] error jumping inline to callee: {_e}", file=sys.stderr)
                        # record inline-return info so we can handle RET even if the write failed
                        try:
                            inline_call_active = True
                            inline_return_linear = ret_ip_linear
                        except Exception:
                            pass
                        print(f"[run_asm_unicorn.py] INLINE-CALL: pushed return {hex(ret_ip_linear)} -> stack_linear {hex(ret_linear)} SP={(sp-2)&0xFFFF}; jumping to {hex(target)}", file=sys.stderr)
                        # enable tracing/single-step to observe callee activity
                        trace_active = True
                        trace_remaining = max(trace_remaining, 2000)
                        single_step_active = True
                        single_step_remaining = 400
                        single_step_origin = address
                    except Exception as _e:
                        print(f"[run_asm_unicorn.py] inline-call internal error: {_e}", file=sys.stderr)
            except Exception:
                pass
            # By default we record call targets and perform isolated emulation later so
            # we can inspect callee behavior without letting it run in the main harness.
            # Some debugging workflows need the callee to execute inline in the main
            # emulator so memory writes are visible there; support that with
            # --inline-calls to disable callers/return-watch/isolation.
            if not getattr(args, 'inline_calls', False):
                call_targets_seen.append((target, regs_pre))
            try:
                entry_watchpoints.add(target)
            except Exception:
                pass
            # Optionally force-call-to-jmp patch for main-run debugging
            try:
                if args.force_jump:
                    global force_jump_on_call
                    force_jump_on_call = True
                    # Patch the CALL opcode to a JMP right now so the main run will
                    # transfer control to the callee unconditionally (one-shot)
                    try:
                        oldb = emu.mem_read(address, 1)[0]
                        if oldb == 0xE8:
                            emu.mem_write(address, bytes([0xE9]))
                            print(f"[run_asm_unicorn.py] patched CALL->JMP at {hex(address)} for debugging", file=sys.stderr)
                    except Exception:
                        pass
            except Exception:
                pass
            # Install a one-shot watch on the caller's return IP slot (two bytes) so
            # we can detect if it gets overwritten before the callee completes.
            try:
                ss = regs_pre.get('SS', entry_seg)
                sp = regs_pre.get('SP', 0) or 0
                # return IP will be at SP - 2 after the CALL pushes it
                ret_sp = (sp - 2) & 0xFFFF
                ret_linear = (ss << 4) + ret_sp
                # Add a mem-write hook for the two bytes of return IP (two hooks: low, high)
                try:
                    # If inline-calls is set, skip installing return watch hooks so the
                    # callee executes normally inside the main emulator.
                    if not getattr(args, 'inline_calls', False):
                        hid_lo = uc.hook_add(UC_HOOK_MEM_WRITE, hook_mem_write_return, None, ret_linear, ret_linear)
                        hid_hi = uc.hook_add(UC_HOOK_MEM_WRITE, hook_mem_write_return, None, ret_linear+1, ret_linear+1)
                        # Add read hooks so we can see read-before-write patterns
                        hid_rlo = uc.hook_add(UC_HOOK_MEM_READ, hook_mem_read_return, None, ret_linear, ret_linear)
                        hid_rhi = uc.hook_add(UC_HOOK_MEM_READ, hook_mem_read_return, None, ret_linear+1, ret_linear+1)
                        # Capture a pre-call snapshot of the DBG area (if present) so we can diff later
                        pre_dbg = None
                        try:
                            if dbg_runtime is not None:
                                pre_dbg = bytes([emu.mem_read(dbg_runtime + i, 1)[0] for i in range(0, dbg_len)])
                        except Exception:
                            pre_dbg = None
                        return_watchpoints[ret_linear] = {'hook': hid_lo, 'read_hook': hid_rlo, 'call_site': address, 'target': target, 'pre_regs': regs_pre, 'pre_dbg': pre_dbg}
                        return_watchpoints[ret_linear+1] = {'hook': hid_hi, 'read_hook': hid_rhi, 'call_site': address, 'target': target, 'pre_regs': regs_pre, 'pre_dbg': pre_dbg}
                        print(f"[run_asm_unicorn.py] installed RETURN-WRITE/READ watch at {hex(ret_linear)} / {hex(ret_linear+1)} for CALL {hex(address)} -> {hex(target)}", file=sys.stderr)
                    else:
                        print(f"[run_asm_unicorn.py] --inline-calls enabled; skipping return-watch for CALL {hex(address)} -> {hex(target)}", file=sys.stderr)
                except Exception as _e:
                    print(f"[run_asm_unicorn.py] error installing return watch: {_e}", file=sys.stderr)
            except Exception:
                pass
            # Set expectation so we can inspect the next executed instruction after CALL
            expect_post_call = target
            # Enable instruction tracing for the next few hundred instructions so we
            # capture the actual code path the callee executes when invoked from
            # the main harness (helps compare with isolated runs).
            trace_active = True
            trace_remaining = 2000
            # Also start a single-step window so we dump register state per-instruction
            # starting as soon as control transfers (this helps catch early clobbers).
            try:
                single_step_active = True
                single_step_remaining = 200
                single_step_origin = address
            except Exception:
                pass
            # Start an unconditional per-instruction control-flow trace (CFTRACE) so we
            # log every executed IP for a short window after the CALL (main run only).
            try:
                cf_trace_active = True
                cf_trace_remaining = 400
                cf_trace_origin = address
            except Exception:
                pass
        except Exception:
            pass
    # If we've seen a recent CALL, detect arrival at the target and dump regs/memory once
    try:
        if last_call_target is not None and address == last_call_target:
            try:
                print(f"[run_asm_unicorn.py] HIT call-target at {hex(address)} — dumping regs & DBG area", file=sys.stderr)
                from unicorn.x86_const import UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_DS, UC_X86_REG_ES
                regs = {
                    'AX': emu.reg_read(UC_X86_REG_AX),
                    'BX': emu.reg_read(UC_X86_REG_BX),
                    'CX': emu.reg_read(UC_X86_REG_CX),
                    'DX': emu.reg_read(UC_X86_REG_DX),
                    'SI': emu.reg_read(UC_X86_REG_SI),
                    'DI': emu.reg_read(UC_X86_REG_DI),
                    'BP': emu.reg_read(UC_X86_REG_BP),
                    'SP': emu.reg_read(UC_X86_REG_SP),
                    'DS': emu.reg_read(UC_X86_REG_DS),
                    'ES': emu.reg_read(UC_X86_REG_ES)
                }
                print(f"[run_asm_unicorn.py] REGS @ {hex(address)}: {regs}", file=sys.stderr) 
                # dump DBG area bytes if present
                dbg_marker = data.find(b'DBG')
                if dbg_marker != -1:
                    dbg_runtime = ADDRESS + ORG + dbg_marker + len(b'DBG')
                    dbg = [emu.mem_read(dbg_runtime + i, 1)[0] for i in range(0, 20)]
                    print(f"[run_asm_unicorn.py] DBG @ {hex(dbg_runtime)} = {dbg}", file=sys.stderr)
                # enable instruction trace for a short window starting at the call target
                trace_active = True
                trace_remaining = 2000
                # also start single-step to capture per-instruction register/memory behavior
                try:
                    single_step_active = True
                    single_step_remaining = 200
                except Exception:
                    pass
            except Exception as e:
                print(f"[run_asm_unicorn.py] error dumping call target: {e}", file=sys.stderr)
            # clear the one-shot target so this dump only runs once
            last_call_target = None
        # Extra debug: when we see a store into [si] (opcode 0x88 0x24 or 0x88 0x04), print ds:si
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
        # Detect mov byte [imm16], imm8 opcode 0xC6 0x06 <lo> <hi> <imm8>
        if len(opcode) >= 5 and opcode[0] == 0xC6 and opcode[1] == 0x06:
            imm = opcode[2] | (opcode[3] << 8)
            imm8 = opcode[4]
            try:
                linear = (emu.reg_read(UC_X86_REG_DS) << 4) + imm
                print(f"[run_asm_unicorn.py] mov byte [imm16],imm8 at {hex(imm)} (linear {hex(linear)}) -> {hex(imm8)}", file=sys.stderr)
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

        # If we're executing within the aggressive range, enable an aggressive mem-write monitor
        try:
            if aggressive_range_start <= address <= aggressive_range_end and not aggressive_active:
                try:
                    def hook_mem_write_aggr(emu_aggr, access_aggr, addr_aggr, size_aggr, value_aggr, user_data_aggr):
                        try:
                            global mem_write_counter
                            mem_write_counter += 1
                            seq = mem_write_counter
                            # read back the bytes written if possible
                            try:
                                written = emu_aggr.mem_read(addr_aggr, size_aggr)
                                written_hex = written.hex()
                            except Exception:
                                written_hex = '<err>'
                            try:
                                from unicorn.x86_const import UC_X86_REG_IP, UC_X86_REG_CS, UC_X86_REG_DS, UC_X86_REG_ES
                                regs_w = {
                                    'IP': emu_aggr.reg_read(UC_X86_REG_IP),
                                    'CS': emu_aggr.reg_read(UC_X86_REG_CS),
                                    'DS': emu_aggr.reg_read(UC_X86_REG_DS),
                                    'ES': emu_aggr.reg_read(UC_X86_REG_ES)
                                }
                            except Exception:
                                regs_w = {}
                            pc = (regs_w.get('CS', 0) << 4) + regs_w.get('IP', 0)
                            print(f"[run_asm_unicorn.py] AGGR MEMWRITE SEQ={seq} @ {hex(addr_aggr)} size {size_aggr} val {written_hex} pc {hex(pc)} REGS {regs_w}", file=sys.stderr)
                        except Exception:
                            pass
                    aggressive_hook_id = uc.hook_add(UC_HOOK_MEM_WRITE, hook_mem_write_aggr)
                    aggressive_active = True
                    print(f"[run_asm_unicorn.py] AGGRESSIVE monitor installed at exec {hex(address)}", file=sys.stderr)
                except Exception as _e:
                    print(f"[run_asm_unicorn.py] error installing aggressive monitor: {_e}", file=sys.stderr)
        except Exception:
            pass
    except Exception:
        pass

uc.hook_add(UC_HOOK_CODE, hook_code)

# Add a MEM_WRITE hook to capture any writes into the DBG region so we can see if
# the main harness run performs the same internal writes we observed in isolated runs.
dbg_marker = data.find(b'DBG')
dbg_runtime = None
# Increase DBG observation window to cover internal per-iteration slots
dbg_len = 64
if dbg_marker != -1:
    dbg_runtime = ADDRESS + ORG + dbg_marker + len(b'DBG')
    try:
        def hook_mem_write(emu, access, address, size, value, user_data):
            try:
                # increment global write counter so we can see ordering of writes
                global mem_write_counter
                mem_write_counter += 1
                seq = mem_write_counter
                # consider single_step_active as part of the 'in trace' context
                try:
                    from unicorn.x86_const import UC_X86_REG_IP
                except Exception:
                    pass
                in_trace = trace_active or single_step_active
                if dbg_runtime is None:
                    return
                if address >= dbg_runtime and address < dbg_runtime + dbg_len:
                    offset = address - dbg_runtime
                    from unicorn.x86_const import UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_IP, UC_X86_REG_CS, UC_X86_REG_DS, UC_X86_REG_ES
                    regs = {
                        'AX': emu.reg_read(UC_X86_REG_AX),
                        'BX': emu.reg_read(UC_X86_REG_BX),
                        'CX': emu.reg_read(UC_X86_REG_CX),
                        'DX': emu.reg_read(UC_X86_REG_DX),
                        'SI': emu.reg_read(UC_X86_REG_SI),
                        'DI': emu.reg_read(UC_X86_REG_DI),
                        'BP': emu.reg_read(UC_X86_REG_BP),
                        'SP': emu.reg_read(UC_X86_REG_SP),
                        'IP': emu.reg_read(UC_X86_REG_IP),
                        'CS': emu.reg_read(UC_X86_REG_CS),
                        'DS': emu.reg_read(UC_X86_REG_DS),
                        'ES': emu.reg_read(UC_X86_REG_ES)
                    }
                    pc = (regs['CS'] << 4) + regs['IP']
                    try:
                        instr = emu.mem_read(pc, 8)
                        instr_hex = instr.hex()
                    except Exception:
                        instr_hex = '<err>'
                    # Read back the written bytes to avoid relying on 'value' which may be
                    # sized differently by the emulator callback; show them in hex.
                    try:
                        written = emu.mem_read(address, size)
                        written_hex = written.hex()
                    except Exception:
                        written_hex = '<err>'
                    print(f"[run_asm_unicorn.py] DBG MEMWRITE SEQ={seq} IN_TRACE={in_trace} @ {hex(address)} offset {offset} size {size} written {written_hex} pc {hex(pc)} instr {instr_hex} PRE-REGS {regs}", file=sys.stderr)
                    # For writes into internal per-iteration slots (offset >= 1) include a short
                    # recent-IP summary to help identify the writer instruction.
                    try:
                        # Record DBG write history and per-offset last-write info for ordering
                        global dbg_write_history, dbg_last_write
                        rec = {'seq': seq, 'address': address, 'offset': offset, 'size': size, 'written_hex': written_hex, 'pc': pc}
                        dbg_write_history.append(rec)
                        dbg_last_write[offset] = (seq, written_hex, pc)
                        # keep history bounded
                        if len(dbg_write_history) > 1024:
                            dbg_write_history.pop(0)
                    except Exception:
                        pass
                    try:
                        if offset >= 1:
                            hist = list(last_ips)[-8:]
                            print(f"[run_asm_unicorn.py] DBG RECENT IPS (last {len(hist)}):", file=sys.stderr)
                            for haddr, hbytes in hist:
                                print(f"  - {hex(haddr)}: {hbytes}", file=sys.stderr)
                    except Exception:
                        pass
            except Exception:
                pass
        uc.hook_add(UC_HOOK_MEM_WRITE, hook_mem_write, None, dbg_runtime, dbg_runtime + dbg_len - 1)
        print(f"[run_asm_unicorn.py] added DBG mem-write hook for {hex(dbg_runtime)}-{hex(dbg_runtime+dbg_len-1)}", file=sys.stderr)
        # Also add a MEM_READ hook for the DBG region so we can see what the
        # harness and callee read from/debug against while running in-context.
        def hook_mem_read_dbg(emu, access, address, size, value, user_data):
            try:
                if dbg_runtime is None:
                    return
                if address >= dbg_runtime and address < dbg_runtime + dbg_len:
                    offset = address - dbg_runtime

                    from unicorn.x86_const import UC_X86_REG_IP, UC_X86_REG_CS, UC_X86_REG_DS, UC_X86_REG_ES
                    regs = {
                        'IP': emu.reg_read(UC_X86_REG_IP),
                        'CS': emu.reg_read(UC_X86_REG_CS),
                        'DS': emu.reg_read(UC_X86_REG_DS),
                        'ES': emu.reg_read(UC_X86_REG_ES)
                    }
                    pc = (regs['CS'] << 4) + regs['IP']
                    try:
                        instr = emu.mem_read(pc, 8)
                        instr_hex = instr.hex()
                    except Exception:
                        instr_hex = '<err>'
                    # read value may be None for some hooks; attempt to read actual memory
                    try:
                        val = emu.mem_read(address, size)
                        val_hex = val.hex()
                    except Exception:
                        val_hex = '<err>'
                    print(f"[run_asm_unicorn.py] DBG MEMREAD @ {hex(address)} offset {offset} size {size} val {val_hex} pc {hex(pc)} instr {instr_hex}", file=sys.stderr)
            except Exception:
                pass
        try:
            uc.hook_add(UC_HOOK_MEM_READ, hook_mem_read_dbg, None, dbg_runtime, dbg_runtime + dbg_len - 1)
            print(f"[run_asm_unicorn.py] added DBG mem-read hook for {hex(dbg_runtime)}-{hex(dbg_runtime+dbg_len-1)}", file=sys.stderr)
        except Exception as e:
            print(f"[run_asm_unicorn.py] error adding DBG mem-read hook: {e}", file=sys.stderr)
        # Optional: install a global aggressive mem-write hook if requested
        if args.force_aggr:
            try:
                def hook_mem_write_aggr_global(emu_aggr, access_aggr, addr_aggr, size_aggr, value_aggr, user_data_aggr):
                    try:
                        global mem_write_counter
                        mem_write_counter += 1
                        seq = mem_write_counter
                        try:
                            written = emu_aggr.mem_read(addr_aggr, size_aggr)
                            written_hex = written.hex()
                        except Exception:
                            written_hex = '<err>'
                        try:
                            from unicorn.x86_const import UC_X86_REG_IP, UC_X86_REG_CS
                            regs_w = {'IP': emu_aggr.reg_read(UC_X86_REG_IP), 'CS': emu_aggr.reg_read(UC_X86_REG_CS)}
                        except Exception:
                            regs_w = {}
                        pc = (regs_w.get('CS', 0) << 4) + regs_w.get('IP', 0)
                        print(f"[run_asm_unicorn.py] AGGR_GLOBAL MEMWRITE SEQ={seq} @ {hex(addr_aggr)} size {size_aggr} val {written_hex} pc {hex(pc)} REGS {regs_w}", file=sys.stderr)
                    except Exception:
                        pass
                uc.hook_add(UC_HOOK_MEM_WRITE, hook_mem_write_aggr_global)
                print(f"[run_asm_unicorn.py] installed global AGGRESSIVE mem-write hook (--force-aggr)", file=sys.stderr)
            except Exception as _e:
                print(f"[run_asm_unicorn.py] error installing global aggressive hook: {_e}", file=sys.stderr)
        # Add a MEM_READ hook for the map region so we can see reads the callee does
        # against the tilemap (addresses computed from MAP marker).
        map_marker = data.find(b'MAP')
        if map_marker != -1:
            map_start = ADDRESS + ORG + map_marker + len(b'MAP')
            map_len = 128 * 16  # observe first 16 rows (should cover typical tests)
            def hook_mem_read_map(emu, access, address, size, value, user_data):
                try:
                    if address >= map_start and address < map_start + map_len:
                        from unicorn.x86_const import UC_X86_REG_IP, UC_X86_REG_CS, UC_X86_REG_AX, UC_X86_REG_DS, UC_X86_REG_ES
                        regs = {
                            'IP': emu.reg_read(UC_X86_REG_IP),
                            'CS': emu.reg_read(UC_X86_REG_CS),
                            'AX': emu.reg_read(UC_X86_REG_AX),
                            'DS': emu.reg_read(UC_X86_REG_DS),
                            'ES': emu.reg_read(UC_X86_REG_ES)
                        }
                        pc = (regs['CS'] << 4) + regs['IP']
                        try:
                            instr = emu.mem_read(pc, 8)
                            instr_hex = instr.hex()
                        except Exception:
                            instr_hex = '<err>'
                        try:
                            val = emu.mem_read(address, size)
                            val_hex = val.hex()
                        except Exception:
                            val_hex = '<err>'
                        print(f"[run_asm_unicorn.py] MAP MEMREAD @ {hex(address)} size {size} val {val_hex} pc {hex(pc)} instr {instr_hex} PRE-REGS {regs}", file=sys.stderr)
                except Exception:
                    pass
            try:
                uc.hook_add(UC_HOOK_MEM_READ, hook_mem_read_map, None, map_start, map_start + map_len - 1)
                print(f"[run_asm_unicorn.py] added MAP mem-read hook for {hex(map_start)}-{hex(map_start+map_len-1)}", file=sys.stderr)
                # Also add a mem-write hook for the map region so we can detect writes that
                # might clobber tilemap or be used to zero/overwrite the DBG area indirectly.
                def hook_mem_write_map(emu_map, access_map, address_map, size_map, value_map, user_data_map):
                    try:
                        global mem_write_counter
                        mem_write_counter += 1
                        seq = mem_write_counter
                        in_trace = trace_active or single_step_active
                        if address_map >= map_start and address_map < map_start + map_len:
                            from unicorn.x86_const import UC_X86_REG_IP, UC_X86_REG_CS, UC_X86_REG_AX, UC_X86_REG_DS, UC_X86_REG_ES
                            regs_map = {
                                'IP': emu_map.reg_read(UC_X86_REG_IP),
                                'CS': emu_map.reg_read(UC_X86_REG_CS),
                                'AX': emu_map.reg_read(UC_X86_REG_AX),
                                'DS': emu_map.reg_read(UC_X86_REG_DS),
                                'ES': emu_map.reg_read(UC_X86_REG_ES)
                            }
                            pc_map = (regs_map['CS'] << 4) + regs_map['IP']
                            try:
                                instr = emu_map.mem_read(pc_map, 8)
                                instr_hex = instr.hex()
                            except Exception:
                                instr_hex = '<err>'
                            try:
                                val = emu_map.mem_read(address_map, size_map)
                                val_hex = val.hex()
                            except Exception:
                                val_hex = '<err>'
                            print(f"[run_asm_unicorn.py] MAP MEMWRITE SEQ={seq} IN_TRACE={in_trace} @ {hex(address_map)} size {size_map} val {val_hex} pc {hex(pc_map)} instr {instr_hex} PRE-REGS {regs_map}", file=sys.stderr)
                    except Exception:
                        pass
                try:
                    uc.hook_add(UC_HOOK_MEM_WRITE, hook_mem_write_map, None, map_start, map_start + map_len - 1)
                    print(f"[run_asm_unicorn.py] added MAP mem-write hook for {hex(map_start)}-{hex(map_start+map_len-1)}", file=sys.stderr)
                except Exception as e:
                    print(f"[run_asm_unicorn.py] error adding MAP mem-write hook: {e}", file=sys.stderr)
            except Exception as e:
                print(f"[run_asm_unicorn.py] error adding MAP mem-read hook: {e}", file=sys.stderr)
    except Exception as e:
        print(f"[run_asm_unicorn.py] error adding DBG mem-write hook: {e}", file=sys.stderr)

# One-shot return-address watchpoints: when we see a CALL we monitor the two bytes
# on the caller's stack that hold the return IP so we can detect early overwrites.
return_watchpoints = {}

def hook_mem_write_return(emu, access, address, size, value, user_data):
    try:
        # address is linear; check if this address is in our active return watchpoints
        if address in return_watchpoints:
            info = return_watchpoints.get(address, {})
            try:
                from unicorn.x86_const import UC_X86_REG_IP, UC_X86_REG_CS, UC_X86_REG_SP, UC_X86_REG_SS, UC_X86_REG_DS, UC_X86_REG_ES
                regs_now = {
                    'IP': emu.reg_read(UC_X86_REG_IP),
                    'CS': emu.reg_read(UC_X86_REG_CS),
                    'SP': emu.reg_read(UC_X86_REG_SP),
                    'SS': emu.reg_read(UC_X86_REG_SS),
                    'DS': emu.reg_read(UC_X86_REG_DS),
                    'ES': emu.reg_read(UC_X86_REG_ES)
                }
            except Exception:
                regs_now = {}
            call_site = info.get('call_site')
            target = info.get('target')
            pre_regs = info.get('pre_regs')
            print(f"[run_asm_unicorn.py] RETURN-WRITE @ {hex(address)} size {size} val {value} call_site {hex(call_site) if call_site else None} target {hex(target) if target else None} PRE-REGS {pre_regs} CURR-REGS {regs_now}", file=sys.stderr)
            # Capture and print a short recent-IP history so we can identify the writer instruction
            try:
                hist = list(last_ips)[-16:]
                print(f"[run_asm_unicorn.py] RECENT IPS (last {len(hist)}):", file=sys.stderr)
                for haddr, hbytes in hist:
                    print(f"  - {hex(haddr)}: {hbytes}", file=sys.stderr)
            except Exception:
                pass
            # Immediately dump a small byte window around the current PC and around the return-slot
            try:
                pc = (regs_now.get('CS', 0) << 4) + regs_now.get('IP', 0)
                # bytes around PC
                win_start = max(pc - 16, ADDRESS)
                win_len = 48
                try:
                    win = emu.mem_read(win_start, win_len)
                    print(f"[run_asm_unicorn.py] BYTES AROUND PC {hex(pc)} (start {hex(win_start)} len {win_len}): {win.hex()}", file=sys.stderr)
                except Exception:
                    pass
                # bytes around stack near SP
                try:
                    linear_stack = (regs_now.get('SS', 0) << 4) + regs_now.get('SP', 0)
                    sbytes = emu.mem_read(max(linear_stack - 16, ADDRESS), 64)
                    print(f"[run_asm_unicorn.py] STACK AROUND SP {hex(linear_stack)}: {sbytes.hex()}", file=sys.stderr)
                except Exception:
                    pass
                # value now at return-slot (read back 2 bytes)
                try:
                    val_bytes = emu.mem_read(address, size)
                    print(f"[run_asm_unicorn.py] RETURN-SLOT AFTER WRITE @ {hex(address)} size {size}: {val_bytes.hex()}", file=sys.stderr)
                except Exception:
                    pass
                # Diff DBG area against pre-call snapshot (if we captured one)
                try:
                    pre_dbg = info.get('pre_dbg') if info is not None else None
                    if pre_dbg is not None and dbg_runtime is not None:
                        try:
                            cur_dbg = emu.mem_read(dbg_runtime, dbg_len)
                            diffs = []
                            for i in range(min(len(pre_dbg), len(cur_dbg))):
                                if pre_dbg[i] != cur_dbg[i]:
                                    diffs.append((i, pre_dbg[i], cur_dbg[i]))
                            if diffs:
                                print(f"[run_asm_unicorn.py] DBG DIFFS (offset, before, after):", file=sys.stderr)
                                for off, bpre, bcur in diffs:
                                    print(f"  - offset {off}: {hex(bpre)} -> {hex(bcur)}", file=sys.stderr)
                            else:
                                print(f"[run_asm_unicorn.py] DBG DIFF: no changes within observed window", file=sys.stderr)
                        except Exception:
                            pass
                except Exception:
                    pass
                # Diff stack snapshot captured at pre-call against current memory at same addresses
                try:
                    pre_regs = info.get('pre_regs') if info is not None else None
                    pre_stack = pre_regs.get('stack') if pre_regs else None
                    if pre_stack:
                        try:
                            ss_pre = pre_regs.get('SS', 0)
                            sp_pre = pre_regs.get('SP', 0) or 0
                            base = (ss_pre << 4) + sp_pre
                            cur_stack = emu.mem_read(base, len(pre_stack))
                            sdiffs = []
                            for i in range(len(pre_stack)):
                                if pre_stack[i] != cur_stack[i]:
                                    sdiffs.append((i, pre_stack[i], cur_stack[i]))
                            if sdiffs:
                                print(f"[run_asm_unicorn.py] STACK DIFFS at base {hex(base)} (idx, before, after):", file=sys.stderr)
                                for idx, before, after in sdiffs:
                                    try:
                                        global dbg_last_write
                                        lw = dbg_last_write.get(off)
                                        if lw:
                                            seq, whex, wpc = lw
                                            print(f"  - offset {off}: {hex(bpre)} -> {hex(bcur)} (last_write SEQ={seq} val={whex} pc={hex(wpc)})", file=sys.stderr)
                                        else:
                                            print(f"  - offset {off}: {hex(bpre)} -> {hex(bcur)} (no recent write recorded)", file=sys.stderr)
                                    except Exception:
                                        print(f"  - offset {off}: {hex(bpre)} -> {hex(bcur)}", file=sys.stderr)
                            else:
                                # If there were no diffs, still print the most recent DBG writes to
                                # help identify later clobbers or header overwrites.
                                try:
                                    global dbg_write_history
                                    recent = dbg_write_history[-8:]
                                    if recent:
                                        print(f"[run_asm_unicorn.py] RECENT DBG WRITES (most recent first):", file=sys.stderr)
                                        for r in reversed(recent):
                                            print(f"  - SEQ={r['seq']} OFF={r['offset']} ADDR={hex(r['address'])} SIZE={r['size']} VAL={r['written_hex']} PC={hex(r['pc'])}", file=sys.stderr)
                                except Exception:
                                    pass
                                print(f"[run_asm_unicorn.py] STACK DIFF: no changes in captured stack snapshot", file=sys.stderr)
                        except Exception:
                            pass
                except Exception:
                    pass
                # Attempt to find the nearest store-like opcode in recent history
                try:
                    found = None
                    for haddr, hbytes in reversed(hist[-12:]):
                        if len(hbytes) >= 2:
                            b0 = int(hbytes[0:2], 16)
                            # heuristics for store opcodes
                            if b0 in (0xC6, 0xA2, 0x88, 0xA3):
                                found = (haddr, hbytes)
                                break
                    if found:
                        print(f"[run_asm_unicorn.py] LIKELY WRITER @ {hex(found[0])}: {found[1]}", file=sys.stderr)
                except Exception:
                    pass
                # After printing the likely writer, dump a backward byte window and try to
                # disassemble it (capstone if available) to show the instruction sequence
                # leading up to the writer PC. This helps determine whether the writer is
                # intentionally storing into the return slot or is part of another sequence.
                try:
                    back_start = max(pc - 32, ADDRESS)
                    back_len = min(96, pc - back_start + 32)
                    try:
                        back_bytes = emu.mem_read(back_start, back_len)
                        print(f"[run_asm_unicorn.py] BYTES BACKWARD FROM PC start {hex(back_start)} len {back_len}: {back_bytes.hex()}", file=sys.stderr)
                    except Exception:
                        back_bytes = None
                    if back_bytes:
                        try:
                            # Try capstone if available
                            from capstone import Cs, CS_ARCH_X86, CS_MODE_16
                            cs = Cs(CS_ARCH_X86, CS_MODE_16)
                            cs.detail = False
                            print(f"[run_asm_unicorn.py] BACKWARD DISASM (around {hex(pc)}):", file=sys.stderr)
                            for insn in cs.disasm(bytes(back_bytes), back_start):
                                print(f"  {hex(insn.address)}:\t{insn.mnemonic}\t{insn.op_str}", file=sys.stderr)
                                # highlight store-like instructions
                                if insn.mnemonic.startswith('mov') and '[' in insn.op_str:
                                    print(f"[run_asm_unicorn.py] BACKWARD DISASM HINT: possible store at {hex(insn.address)} -> {insn.mnemonic} {insn.op_str}", file=sys.stderr)
                        except Exception as _e:
                            # No capstone available; print a simple hexdump for inspection
                            print(f"[run_asm_unicorn.py] no capstone disasm available ({_e}); raw backward bytes shown", file=sys.stderr)
                except Exception:
                    pass
            except Exception:
                pass
            # Start an aggressive CFTRACE window so we capture the writer's context
            try:
                global cf_trace_active, cf_trace_remaining, cf_trace_origin, trace_active, trace_remaining
                cf_trace_active = True
                cf_trace_remaining = 1200
                cf_trace_origin = address
                # Also enable the branch trace so conditional jumps are logged
                trace_active = True
                trace_remaining = 2000
            except Exception:
                pass
            # Persist the write watch: increment a hit counter so we can observe repeated
            # overwrites. Do not remove the hook so subsequent writes are captured.
            try:
                info['hits'] = info.get('hits', 0) + 1
                print(f"[run_asm_unicorn.py] RETURN-WRITE HIT COUNT for {hex(address)} = {info['hits']}", file=sys.stderr)
            except Exception:
                pass
            # Keep the hook and entry so we can continue observing further writes; this
            # helps detect later clobbers that might zero internal DBG slots.
            try:
                return_watchpoints[address] = info
            except Exception:
                pass
    except Exception:
        pass

# Read hook for the return-slot: logs reads and triggers a short CFTRACE so we can
# capture read-then-write patterns and the reader's context.
def hook_mem_read_return(emu, access, address, size, value, user_data):
    try:
        if address in return_watchpoints:
            info = return_watchpoints.get(address, {})
            try:
                from unicorn.x86_const import UC_X86_REG_IP, UC_X86_REG_CS, UC_X86_REG_SP, UC_X86_REG_SS, UC_X86_REG_AX, UC_X86_REG_DS, UC_X86_REG_ES
                regs_now = {
                    'IP': emu.reg_read(UC_X86_REG_IP),
                    'CS': emu.reg_read(UC_X86_REG_CS),
                    'SP': emu.reg_read(UC_X86_REG_SP),
                    'SS': emu.reg_read(UC_X86_REG_SS),
                    'AX': emu.reg_read(UC_X86_REG_AX),
                    'DS': emu.reg_read(UC_X86_REG_DS),
                    'ES': emu.reg_read(UC_X86_REG_ES)
                }
            except Exception:
                regs_now = {}
            call_site = info.get('call_site')
            target = info.get('target')
            pre_regs = info.get('pre_regs')
            try:
                val_read = emu.mem_read(address, size).hex()
            except Exception:
                val_read = '<err>'
            print(f"[run_asm_unicorn.py] RETURN-READ @ {hex(address)} size {size} val {val_read} call_site {hex(call_site) if call_site else None} target {hex(target) if target else None} PRE-REGS {pre_regs} CURR-REGS {regs_now}", file=sys.stderr)
            # trigger a short cftrace to capture the surrounding control flow at reader
            try:
                global cf_trace_active, cf_trace_remaining, cf_trace_origin
                cf_trace_active = True
                cf_trace_remaining = 400
                cf_trace_origin = address
            except Exception:
                pass
    except Exception:
        pass

# We'll add the mem-write hook on demand for specific linear addresses when we see a CALL.
# Use a general add so we can register per-address hooks later.
try:
    # no-op add to ensure the hook function is available for registration (actual hooks will be added per-call)
    pass
except Exception:
    pass

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

# After the main run, optionally inspect any call targets we observed by running
# them in an isolated Uc instance. This is skipped if --inline-calls is set so
# callees execute inline in the main emulator and their effects are captured
# there.
if not getattr(args, 'inline_calls', False) and call_targets_seen:
    print(f"[run_asm_unicorn.py] call_targets_seen = {call_targets_seen}", file=sys.stderr)
    for target, regs_pre in call_targets_seen:
        try:
            print(f"[run_asm_unicorn.py] Running isolated emulation at target {hex(target)} with PRE-REGS {regs_pre}", file=sys.stderr)
            # Create a fresh emulator instance and run a short window at the target
            subuc = Uc(UC_ARCH_X86, UC_MODE_16)
            subuc.mem_map(ADDRESS, MEM_SIZE)
            subuc.mem_write(ADDRESS + ORG, bytes(data))
            subuc.reg_write(UC_X86_REG_CS, entry_seg)
            # Restore DS/ES from PRE-REGS if available (important for 16-bit data addressing)
            subuc.reg_write(UC_X86_REG_DS, regs_pre.get('DS', data_seg))
            try:
                from unicorn.x86_const import UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_SS, UC_X86_REG_ES
                subuc.reg_write(UC_X86_REG_AX, regs_pre.get('AX', 0))
                subuc.reg_write(UC_X86_REG_BX, regs_pre.get('BX', 0))
                subuc.reg_write(UC_X86_REG_CX, regs_pre.get('CX', 0))
                subuc.reg_write(UC_X86_REG_DX, regs_pre.get('DX', 0))
                subuc.reg_write(UC_X86_REG_SI, regs_pre.get('SI', 0))
                subuc.reg_write(UC_X86_REG_DI, regs_pre.get('DI', 0))
                subuc.reg_write(UC_X86_REG_BP, regs_pre.get('BP', 0))
                # Simulate the callee stack state: CALL pushes return IP (2 bytes), so the callee's SP = pre_call_SP - 2
                pre_sp = regs_pre.get('SP', 0) or 0x200
                callee_sp = (pre_sp - 2) & 0xFFFF
                subuc.reg_write(UC_X86_REG_SP, callee_sp)
                # Restore SS from captured PRE-REGS (do not overwrite with entry_seg)
                subuc.reg_write(UC_X86_REG_SS, regs_pre.get('SS', entry_seg))
                # Restore ES if available
                subuc.reg_write(UC_X86_REG_ES, regs_pre.get('ES', regs_pre.get('DS', data_seg)))
                # If we captured stack bytes, write them into sub-emu memory at the same linear addresses
                if regs_pre.get('stack'):
                    try:
                        stack_base_linear = (regs_pre.get('SS', entry_seg) << 4) + (regs_pre.get('SP', 0) or 0x200)
                        for i, b in enumerate(regs_pre.get('stack')):
                            subuc.mem_write(stack_base_linear + i, bytes([b]))
                        # Write a fake return IP at callee_sp so RET will land somewhere safe if executed
                        ret_ip = entry_ip  # return to benign address in the image
                        ret_lo = ret_ip & 0xFF
                        ret_hi = (ret_ip >> 8) & 0xFF
                        ret_linear = (regs_pre.get('SS', entry_seg) << 4) + callee_sp
                        try:
                            subuc.mem_write(ret_linear, bytes([ret_lo]))
                            subuc.mem_write(ret_linear + 1, bytes([ret_hi]))
                            print(f"[run_asm_unicorn.py] restored {len(regs_pre.get('stack'))} stack bytes and fake return IP into isolated emulator at {hex(stack_base_linear)} / return at {hex(ret_linear)}", file=sys.stderr)
                        except Exception as _e:
                            print(f"[run_asm_unicorn.py] error writing fake return IP: {_e}", file=sys.stderr)
                    except Exception as _e:
                        print(f"[run_asm_unicorn.py] error restoring stack bytes: {_e}", file=sys.stderr)
            except Exception:
                pass
            # place stack base
            subuc.reg_write(UC_X86_REG_SS, regs_pre.get('SS', entry_seg))
            # Add a code hook to print the first few opcodes and detect stores
            def sub_hook(emu_sub, addr_sub, size_sub, user_data_sub):
                # declare globals early to avoid "used prior to global declaration" errors
                global isolated_mem_write_counter, isolated_dbg_write_history, isolated_dbg_last_write
                try:
                    op = emu_sub.mem_read(addr_sub, size_sub)
                    if addr_sub < target + 0x200:
                        print(f"[run_asm_unicorn.py][isolated] exec @ {hex(addr_sub)} bytes: {op.hex()}", file=sys.stderr)
                    # detect mov byte [imm16], imm8
                    if len(op) >= 5 and op[0] == 0xC6 and op[1] == 0x06:
                        off = op[2] | (op[3] << 8)
                        imm8 = op[4]
                        linear = (emu_sub.reg_read(UC_X86_REG_DS) << 4) + off
                        print(f"[run_asm_unicorn.py][isolated] mov byte [imm16],imm8 at {hex(off)} (linear {hex(linear)}) -> {hex(imm8)}", file=sys.stderr)
                        # record isolated DBG write if target falls in DBG runtime
                        try:
                            if dbg_marker != -1:
                                dbg_runtime_local = ADDRESS + ORG + dbg_marker + len(b'DBG')
                                if linear >= dbg_runtime_local and linear < dbg_runtime_local + dbg_len:
                                    offset = linear - dbg_runtime_local
                                    isolated_mem_write_counter += 1
                                    seq = isolated_mem_write_counter
                                    rec = {'seq': seq, 'address': linear, 'offset': offset, 'size': 1, 'written_hex': f"{imm8:02x}", 'pc': addr_sub}
                                    isolated_dbg_write_history.append(rec)
                                    isolated_dbg_last_write[offset] = (seq, f"{imm8:02x}", addr_sub)
                                    if len(isolated_dbg_write_history) > 1024:
                                        isolated_dbg_write_history.pop(0)
                                    print(f"[run_asm_unicorn.py][isolated][DBG] MEMWRITE SEQ={seq} @ {hex(linear)} offset {offset} val {hex(imm8)} pc {hex(addr_sub)}", file=sys.stderr)
                        except Exception:
                            pass
                    # detect mov [imm16], al
                    if len(op) >= 3 and op[0] == 0xA2:
                        off = op[1] | (op[2] << 8)
                        linear = (emu_sub.reg_read(UC_X86_REG_DS) << 4) + off
                        try:
                            from unicorn.x86_const import UC_X86_REG_AX
                            al = emu_sub.reg_read(UC_X86_REG_AX) & 0xFF
                        except Exception:
                            al = None
                        print(f"[run_asm_unicorn.py][isolated] mov [imm],al at {hex(off)} (linear {hex(linear)}) AL={al}", file=sys.stderr)
                        try:
                            if dbg_marker != -1:
                                dbg_runtime_local = ADDRESS + ORG + dbg_marker + len(b'DBG')
                                if linear >= dbg_runtime_local and linear < dbg_runtime_local + dbg_len:
                                    offset = linear - dbg_runtime_local
                                    isolated_mem_write_counter += 1
                                    seq = isolated_mem_write_counter
                                    rec = {'seq': seq, 'address': linear, 'offset': offset, 'size': 1, 'written_hex': f"{al:02x}" if al is not None else '<unk>', 'pc': addr_sub}
                                    isolated_dbg_write_history.append(rec)
                                    isolated_dbg_last_write[offset] = (seq, f"{al:02x}" if al is not None else '<unk>', addr_sub)
                                    if len(isolated_dbg_write_history) > 1024:
                                        isolated_dbg_write_history.pop(0)
                                    print(f"[run_asm_unicorn.py][isolated][DBG] MEMWRITE SEQ={seq} @ {hex(linear)} offset {offset} val {hex(al) if al is not None else '<unk>'} pc {hex(addr_sub)}", file=sys.stderr)
                        except Exception:
                            pass
                except Exception:
                    pass
            try:
                subuc.hook_add(UC_HOOK_CODE, sub_hook)
                subuc.emu_start(target, target + 0x200)
            except Exception:
                pass
            # dump DBG area after isolated run
            dbg_marker = data.find(b'DBG')
            if dbg_marker != -1:
                dbg_runtime = ADDRESS + ORG + dbg_marker + len(b'DBG')
                dbg = [subuc.mem_read(dbg_runtime + i, 1)[0] for i in range(0, 20)]
                print(f"[run_asm_unicorn.py] isolated DBG @ {hex(dbg_runtime)} = {dbg}", file=sys.stderr)
                # Print recent isolated-run DBG writes for ordering comparison
                try:
                    recent = isolated_dbg_write_history[-16:]
                    if recent:
                        print(f"[run_asm_unicorn.py] isolated RECENT DBG WRITES (most recent first):", file=sys.stderr)
                        for r in reversed(recent):
                            print(f"  - SEQ={r['seq']} OFF={r['offset']} ADDR={hex(r['address'])} SIZE={r['size']} VAL={r['written_hex']} PC={hex(r['pc'])}", file=sys.stderr)
                except Exception:
                    pass
                # Optionally replay isolated DBG writes into the main emulator memory
                # so they appear in main-run traces and summaries. This is useful when
                # --inline-calls is requested and we want the main emu state to reflect
                # what the isolated callee did without changing emulation flow.
                try:
                    if getattr(args, 'inline_calls', False) and isolated_dbg_write_history:
                        print(f"[run_asm_unicorn.py] REPLAYING {len(isolated_dbg_write_history)} isolated DBG writes into main emu", file=sys.stderr)
                        for r in isolated_dbg_write_history:
                            try:
                                addr = r['address']
                                size = r.get('size', 1)
                                written_hex = r.get('written_hex')
                                if written_hex is None or written_hex == '<unk>':
                                    continue
                                val = int(written_hex, 16)
                                # Write into main emulator memory at the linear address
                                try:
                                    uc.mem_write(addr, bytes([val]) if size == 1 else (val.to_bytes(size, 'little')))
                                except Exception as _e:
                                    print(f"[run_asm_unicorn.py] error writing to main emu at {hex(addr)}: {_e}", file=sys.stderr)
                                # Record it in the main dbg history so summaries include it
                                try:
                                    mem_write_counter += 1
                                    seq = mem_write_counter
                                    rec_main = {'seq': seq, 'address': addr, 'offset': r.get('offset'), 'size': size, 'written_hex': written_hex, 'pc': r.get('pc')}
                                    dbg_write_history.append(rec_main)
                                    dbg_last_write[r.get('offset')] = (seq, written_hex, r.get('pc'))
                                    print("[run_asm_unicorn.py] REPLAYED ISOLATED MEMWRITE SEQ={seq} @ {addr} size {size} val {written_hex} pc {pc}".format(seq=seq, addr=hex(addr), size=size, written_hex=written_hex, pc=(hex(r.get("pc")) if r.get("pc") else None)), file=sys.stderr)
                                except Exception as _e:
                                    print(f"[run_asm_unicorn.py] error recording replayed write: {_e}", file=sys.stderr)
                            except Exception:
                                pass
                except Exception as e:
                    print(f"[run_asm_unicorn.py] replay isolated writes failed: {e}", file=sys.stderr)
                # Also print last-write per-offset summary for the isolated run
                try:
                    if isolated_dbg_last_write:
                        print(f"[run_asm_unicorn.py] isolated LAST DBG WRITES (per-offset):", file=sys.stderr)
                        for off, info in sorted(isolated_dbg_last_write.items()):
                            seq, whex, wpc = info
                            print(f"  - offset {off}: SEQ={seq} VAL={whex} PC={hex(wpc)}", file=sys.stderr)
                except Exception:
                    pass
        except Exception as e:
            print(f"[run_asm_unicorn.py] isolated run error: {e}", file=sys.stderr)

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
            dbg_marker = data.find(b'DBG')
            if dbg_marker != -1:
                dbg_start_file = dbg_marker + len(b'DBG')
                dbg_runtime = ADDRESS + ORG + dbg_marker + len(b'DBG')
                try:
                    dbg = [uc.mem_read(dbg_runtime + i, 1)[0] for i in range(0, 20)]
                    print(f"[run_asm_unicorn.py] emu debug-slot @ {dbg_runtime} = {dbg}", file=_sys.stderr)
                except Exception:
                    pass
                # Extra debug: for vertical/horizontal modes, compute the tile index the ASM used
                try:
                    if mode in (0, 1):
                        inx_idx = data.find(b'INX')
                        iny_idx = data.find(b'INY')
                        if inx_idx != -1 and iny_idx != -1:
                            ix = data[inx_idx + len(b'INX')]
                            iy = data[iny_idx + len(b'INY')]
                            tx = ix >> 1
                            ty = iy >> 1
                            map_marker = data.find(b'MAP')
                            map_start = map_marker + len(b'MAP')
                            idx = map_start + ty * 128 + tx
                            val = uc.mem_read(ADDRESS + ORG + (idx), 1)[0]
                            print(f"[run_asm_unicorn.py] computed tile idx (file offset) = {idx}, mem val = {val}", file=_sys.stderr)
                            # Print named debug slots in the DBG region
                            named_slots = {
                                0: 'entry',
                                1: 'left',
                                2: 'right',
                                3: 'top',
                                4: 'bottom',
                                5: 'tile_val',
                                6: 'threshold',
                                7: 'cur_tx',
                                8: 'cur_ty',
                                9: 'collision_marker',
                                10: 'row_marker',
                                11: 'flags_lo',
                                12: 'flags_hi',
                                13: 'fire_active'
                            }
                            for off, name in named_slots.items():
                                try:
                                    v = uc.mem_read(dbg_runtime + off, 1)[0]
                                    print(f"[run_asm_unicorn.py] debug_slot {name} @ {dbg_start_file+off} = {v}", file=_sys.stderr)
                                except Exception as _e:
                                    pass
                except Exception:
                    pass
        except Exception as e:
            print(f"[run_asm_unicorn.py] error reading debug-slot: {e}", file=_sys.stderr)

# For collision modes (0,1) prefer RS
if mode in (0,1):
    idx = mem.find(b'RS')
    if idx != -1:
        # print surrounding bytes for debugging to stderr so stdout remains numeric
        try:
            print(mem[idx:idx+6], file=_sys.stderr)
        except Exception:
            pass
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
