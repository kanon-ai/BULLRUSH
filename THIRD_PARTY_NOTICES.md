# Licenses and asset provenance

## Original project

Copyright (c) 2026 Kanon (kanon-ai). Project code and procedural geometry are licensed under MIT; see LICENSE. The robot is an original project design. No commercial-game models, characters, sound samples or music are included. PSG effects are synthesized by game code. HUD and public preview captions use the hand-authored bitmap glyphs in hudfont.py; no font file is distributed.

The original AI-generated material atlas was created without reference images; see assets/PROVENANCE.md for the prompt. It is offered under MIT only to the extent the contributors hold applicable rights. No exclusive copyright or absence of all third-party claims is guaranteed.

## Geo3D and V9968

The demo targets and uses register/interface information from these projects. Retained MIT acknowledgements are in LICENSES/Geo3D-MIT.txt (Alex Moncks) and LICENSES/V9968-MIT.txt (HRA!). No FPGA bitstream, emulator binary, BIOS or firmware is bundled.

- https://github.com/alexmoncks/V9968_Cartridge
- https://github.com/hra1129/V9968_Cartridge

## SDCC runtime linked into the ROM

SDCC 4.6.0 #16555 links these z80.lib modules: divunsigned, modunsigned, mul, _divslong, _divulong, modsigned, _mullong and divsigned. Their compiled code is present in the ROM. They are GPL-2.0-or-later with the SDCC linking exception, NOT relicensed as MIT. The exception states that linking with other files, some compiled with SDCC, does not by itself make the executable subject to GPL. Original copyright notices and the complete exception are retained in each source header under third_party/sdcc; COPYING.txt supplies GPL v2.

Included source files were retrieved unmodified from official SDCC SVN revision 16555:
https://svn.code.sf.net/p/sdcc/code/!svn/bc/16555/trunk/sdcc/device/lib/

Files prefixed z80_ originate in the z80/ subdirectory. The toolchain used for this build was distributed by MSXgl at commit ab4b26feda36189e677aa23b56acfdc3218f7953, tools/sdcc. The compiler and its binaries are not bundled. Runtime library source is supplied for provenance and licensing; the normal game build links the installed SDCC library.

Official licensing overview: https://sourceforge.net/p/sdcc/wiki/SDCC%20Library%20Licenses/

## External tools

Python, NumPy, Pillow, SDCC, openMSX and FFmpeg are separately installed tools with their own licenses. Requirements identify Python dependencies; their packages are not included in this repository. No claim of endorsement by their developers is made.
