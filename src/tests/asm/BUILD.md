This directory will contain assembly test harnesses and build scripts.

Planned files:
 - collision_harness.asm  # small harness that calls original ASM collision routines
 - build_asm.sh           # assemble/link with NASM + djlink to produce an .exe
 - run_asm_dosbox.sh      # helper to run exe in DOSBox and capture result
 - run_asm_unicorn.py     # optional python harness that runs binary in Unicorn emulator

For now these are TODOs; a C++ reference test harness exists in src/tests/collision_tests.cpp.