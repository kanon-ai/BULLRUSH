"""Build separately named internal/external I/O editions without binary search/replace."""
from pathlib import Path
import hashlib, json, re, shutil, subprocess, sys
P = Path(__file__).resolve().parent
PORTS = {0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9F}
def convert(text):
    lines, changes = [], []
    for n, line in enumerate(text.splitlines(), 1):
        command = line.split(';')[0]
        updated = command
        if re.match(r'\s*(out\s*\(|in\s+a,\s*\()', command, re.I):
            updated = re.sub(r'0x(9[89ABCDF])\b', lambda m: '0x%02X' % (int(m[1], 16)-16), command, flags=re.I)
        elif re.match(r'\s*ld\s+(bc|c),\s*#0x', command, re.I):
            m = re.search(r'#0x([0-9a-f]+)', command, re.I)
            value = int(m[1], 16)
            if value & 255 in PORTS:
                updated = command[:m.start(1)] + format(value-16, '0'+str(len(m[1]))+'X') + command[m.end(1):]
        if updated != command:
            changes.append({'line': n, 'before': command.strip(), 'after': updated.strip()})
        lines.append(updated + line[len(command):])
    return '\n'.join(lines)+'\n', changes

def main():
    out = P/'out'
    internal = out/'BULLRUSH-CITY-INTERCEPT.ROM'
    if not internal.exists():
        subprocess.run([sys.executable, str(P/'build.py'), '--rebuild-assets'], check=True)
    external = out/'external-build'
    external.mkdir(parents=True, exist_ok=True)
    for f in P.glob('*.py'):
        if f.name != 'build_io.py': shutil.copy2(f, external/f.name)
    shutil.copy2(P/'city.json', external/'city.json')
    for name in ['assets/material-atlas-v1.png','assets/baseline-assets.rom','src/baseline.asm','src/renderer.template.asm','src/game.c','src/palette.inc','src/tables.h']:
        target=external/name; target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(P/name,target)
    template = external/'src/renderer.template.asm'
    converted, changes = convert(template.read_text())
    template.write_text(converted)
    source = external/'src/game.c'
    text = source.read_text()
    for old,new in [('__at(0x99) vctrl','__at(0x89) vctrl'),('__at(0x9B) vcmd','__at(0x8B) vcmd')]:
        assert text.count(old)==1
        text=text.replace(old,new)
    text, inline_changes = convert(text)
    assert len(inline_changes) == 1  # HUD command OTIR port in inline assembly
    source.write_text(text)
    subprocess.run([sys.executable, str(external/'build.py')],check=True)
    a=internal.read_bytes();b=(external/'out/BULLRUSH-CITY-INTERCEPT.ROM').read_bytes()
    assert len(a)==len(b)==4194304
    diff=[i for i,(x,y) in enumerate(zip(a,b)) if x!=y]
    assert diff
    # Only immediate I/O port operands may differ. This also catches changed
    # addresses, register values, ROM bank layout or accidentally patched data.
    for i in diff:
        assert a[i] in PORTS and b[i]==a[i]-16, (i,a[i],b[i])
        assert a[i-1] in (0xD3,0xDB,0x0E,0x01), (i,a[i-1])
    for suffix,data in [('INTERNAL-98H',a),('EXTERNAL-88H',b)]:
        (out/('BULLRUSH-CITY-INTERCEPT-'+suffix+'.ROM')).write_bytes(data)
    report={'bytes':len(a),'internal_sha256':hashlib.sha256(a).hexdigest(),'external_sha256':hashlib.sha256(b).hexdigest(),'changed_operand_count':len(diff),'rom_offsets':diff,'renderer_source_changes':changes,'c_inline_changes':inline_changes,'c_sfr_changes':{'vctrl':'99h -> 89h','vcmd':'9Bh -> 8Bh'},'geometry_assets_and_other_bytes_identical':True}
    (out/'io-variants.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ['rom_offsets','renderer_source_changes']},indent=2))
if __name__=='__main__': main()
