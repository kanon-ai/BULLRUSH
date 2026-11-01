# Public build validation — 2026-11-01

- Rebuilt from the sanitized public sources and original material atlas, without a baseline ROM; `python build.py --rebuild-assets` completed.
- Windows / Python 3.13.5 / NumPy 2.4.6 / Pillow 12.2.0 / SDCC 4.6.0 #16555.
- Result: 4,194,304 bytes, ASCII16 (256 x 16 KiB banks).
- SHA256: `48cdd7f8c236da36005922bc8000981fa6e9e6a4e6a04919a61318df10fa5443`.
- Byte-identical to the tested development ROM and packaged ROM. The game implementation was not changed for publication.
- Existing compiler warnings about unused locals and optimizer-changed conditional flow remain; the build is not claimed warning-free.
- Rebuilt ROM launched using the public run.py launcher, developer openMSX commit `67811936bee936c15e3fbdbdffe01755fd349876`, Panasonic_FS-A1ST_V9968 + geo3d. Executable SHA256 `7b959886acbb8581c03969846666b524a7046f4f1497f6c6e7c49b1506d4c002`.
- tests/smoke.tcl exercised title, Enter start, dash, turn and cockpit view through 19 emulated seconds. Screenshots inspected. This is a startup/control smoke check, not a full playthrough.
- Development validation records are provided separately: validation.json, comparison.json, profile-summary.json and radar-validation.json. These include key-only combat/loss/retry and explicitly marked position-injected diagnostics. Publication did not repeat every development test; byte identity establishes the unchanged game binary.
- No physical hardware, FPGA or BlueMSX+ validation. Geo3D emulation does not fully model geometry cycles; emulator update rates are not physical performance predictions.
- Public previews use the existing key-only recording without changing playback speed or interpolating frames. Only the caption band was regenerated using the project bitmap font. The GIF is a 35-second excerpt, under 15 MB. MP4 contains the original generated PSG effects; no BGM.
