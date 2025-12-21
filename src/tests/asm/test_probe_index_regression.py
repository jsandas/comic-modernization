#!/usr/bin/env python3
import subprocess, os, sys, re

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
RUN = os.path.join(os.path.dirname(__file__), 'run_asm_unicorn.py')

cmd = ['python3', RUN, '1', '--x', '56', '--y', '50', '--set-tile', '3', '3', '1', '--force-decision', '--inline-calls']
print('Running:', ' '.join(cmd))
proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
out = proc.stdout.decode()
print(out)
if proc.returncode != 0:
    print('run_asm_unicorn.py failed with exit code', proc.returncode)
    sys.exit(proc.returncode)

m_pidx = re.search(r'probe_index @ [0-9]+ = (\d+)', out)
m_idx = re.search(r'computed tile idx \(file offset\) = (\d+)', out)
if not m_pidx or not m_idx:
    print('Could not find probe_index or computed idx in output')
    sys.exit(2)
if m_pidx.group(1) != m_idx.group(1):
    print('Mismatch: probe_index', m_pidx.group(1), 'computed', m_idx.group(1))
    sys.exit(1)
print('OK: probe_index matches computed tile idx')
sys.exit(0)
