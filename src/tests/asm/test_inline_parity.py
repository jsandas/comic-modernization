#!/usr/bin/env python3
import subprocess, re, sys, os

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
RUN = os.path.join(os.path.dirname(__file__), 'run_asm_unicorn.py')
if not os.path.exists(RUN):
    print('run_asm_unicorn.py not found', file=sys.stderr)
    sys.exit(2)

cases = [(48,42),(48,50),(48,58),(56,42),(56,50),(56,58),(64,42),(64,50),(64,58)]
failed = []

for x,y in cases:
    print(f'=== CASE {x},{y} ===')
    base = f'mode1_{x}_{y}'
    isol_log = f'tmp_logs/sweep_with_aggr/decision/{base}_isol.log'
    inline_log = f'tmp_logs/sweep_with_aggr/decision/{base}_inline_tramp.log'
    os.makedirs(os.path.dirname(isol_log), exist_ok=True)
    # Run isolated (no --inline-calls)
    cmd = ['python3', RUN, '1', '--x', str(x), '--y', str(y), '--set-tile', '4', '3', '1', '--force-decision', '--force-aggr']
    with open(isol_log, 'wb') as out:
        r = subprocess.run(cmd, stdout=out, stderr=subprocess.STDOUT)
    # parse isolated PCs
    with open(isol_log, 'r') as f:
        content = f.read()
    pcs = re.findall(r'PC=(0x[0-9a-fA-F]+)', content)
    pcs = list(dict.fromkeys(pcs))  # unique
    if not pcs:
        print('No isolated PCs found; skipping', file=sys.stderr)
        failed.append((x,y,'no_isolated_pcs'))
        continue
    # Run inline trampoline
    cmd = ['python3', RUN, '1', '--x', str(x), '--y', str(y), '--set-tile', '4', '3', '1', '--force-decision', '--force-aggr', '--inline-calls']
    with open(inline_log, 'wb') as out:
        r = subprocess.run(cmd, stdout=out, stderr=subprocess.STDOUT)
    with open(inline_log,'r') as f:
        inline_content = f.read()
    missing = [pc for pc in pcs if pc not in inline_content]
    if missing:
        print(f'FAIL {x},{y}: missing PCs in inline run: {missing}')
        failed.append((x,y,missing))
    else:
        print(f'PASS {x},{y}: all PCs present')

if failed:
    print('\nSUMMARY: FAILED CASES:')
    for t in failed:
        print(t)
    sys.exit(1)
else:
    print('\nAll cases passed parity check')
    sys.exit(0)
