# May 2003 emulator capture status

The supplied unmodified May 2003 debug and ship executables were tried in an
isolated xemu 0.8.136 session using the locally available Complex 4627 BIOS and
MCPX 1.0. Each attempt used a separate HDD leaf and EEPROM copy. The staged ISO
retained the original init file and the requested ec_interior_test/mainmenu
caches; original build files were not edited.

The Vulkan attempt failed in xemu's external-memory display interop before a
game frame. The OpenGL attempts reached a guest kernel halt (KMODE exception
1E / access violation C0000005, halt instruction at 800151EF) with both original
executables. QMP register snapshots and logs are under
[2003-reference-xemu](../../analysis/pstencil/2003-reference-xemu/).
Both sessions were closed after recording the halt. No original 2003 gameplay
frame was captured. The black boot image is diagnostic evidence only.

The successful native pStencil captures prove current-asset behavior and
Original restoration. They are not matching original frames or evidence of
complete 2003 framebuffer parity. CPU instruction oracles and recovered asset
schemas provide independent, explicitly bounded reference evidence.
