from pathlib import Path
import argparse, os, subprocess
P = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description="Run BULLRUSH with a separately installed Geo3D V2 openMSX.")
parser.add_argument("--openmsx", default=os.environ.get("BULLRUSH_OPENMSX"))
parser.add_argument("--rom", type=Path, default=P / "rom/BULLRUSH-CITY-INTERCEPT.ROM")
parser.add_argument("--script", type=Path)
a = parser.parse_args()
if not a.openmsx:
    parser.error("Specify --openmsx PATH or BULLRUSH_OPENMSX. Standard openMSX is not sufficient.")
exe = Path(a.openmsx).resolve()
if not exe.is_file() or not a.rom.is_file():
    parser.error("Emulator executable or ROM not found.")
env = os.environ.copy()
env["OPENMSX_SYSTEM_DATA"] = str(exe.parent / "share")
for key, sub in [("OPENMSX_USER_DATA", "userdata"), ("OPENMSX_HOME", "home")]:
    folder = P / "runtime" / sub
    folder.mkdir(parents=True, exist_ok=True)
    env[key] = str(folder)
cmd = [str(exe), "-machine", "Panasonic_FS-A1ST_V9968", "-ext", "geo3d", "-cart", str(a.rom.resolve()), "-romtype", "ASCII16"]
if a.script:
    cmd += ["-script", str(a.script.resolve())]
subprocess.run(cmd, cwd=P, env=env, check=True)
