# Internal and external VDP editions

External: V9968 88h–8Ch; Geo3D index/status 8Dh, data 8Fh. Tested with Panasonic_FS-A1ST + HRA_V9968 + geo3d88, selecting videosource V9968.
Internal: V9968 98h–9Ch; Geo3D index/status 9Dh, data 9Fh. Tested with Panasonic_FS-A1ST_V9968 + geo3d.

Port assignments and binding were checked against the developer openMSX HRA_V9968.xml, geo3d88.xml and geo3d.xml profiles. geo3d88 explicitly binds to VDP id V9968; the internal profile binds to id VDP. Profile names are for the tested developer build, not a claim that standard openMSX includes them.

Both ROMs check Geo3D ID 20h and the V2 extended-memory configuration before proceeding. Startup unlocks Port#4 (bit7=0) before the register list, sets R#21=0 and R#20=11h (HS/EPAL, FIL disabled). No register values or memory geometry changed for external I/O. The C engine executes from RAM 8000h–BFFFh after relocation, not permanently mapped ROM.

build_io.py converts only explicit assembly IN/OUT port operands, C/BC block-I/O port loads, two C SFR declarations and the inline HUD OTIR port. It compiles the external source in an isolated build folder and checks every changed ROM byte is an immediate port operand, exactly 10h below the internal value. Report: 108 changed bytes; all other bytes, including assets and gameplay, identical. No global binary byte replacement is used.

Both editions were run through tests/io-smoke.tcl for 25 emulated seconds: Enter start, forward boost, turn, lateral movement, C cockpit/chase, T textures, Space attack. State logs and screenshots confirmed gameplay, moving coordinates, camera 0/1/0 and texture 1/0. The two machine profiles initialize at different times; these are successful per-profile checks, not claims of frame-for-frame timing identity. Both editions also completed the key-only four-enemy combat/persistent-wreck test in v0.2.2; see ROLLER_FIX.md.

Hardware, FPGA and BlueMSX+ remain untested. Emulator speed is not physical performance. Connect to the external V9968 video output when using an external board. Neither ROM auto-selects ports.

## SHA256 (both 4,194,304 bytes / ASCII16)

- INTERNAL-98H: `b5938c2d3676734ef8d75b687fee2db14667f28b93a37675d747b8a5f4cb863c`
- EXTERNAL-88H: `4a4ec0f07b1b49fe4ab0c8e674fcea1fc8c29e5c690b2fe5b0ac7a1174b56ca2`

Preview GIF/MP4 show the internal edition at original speed. They are not hardware captures or separate external-performance measurements.
