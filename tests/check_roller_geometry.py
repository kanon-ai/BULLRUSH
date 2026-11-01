"""Verify preserved geometry against a separately supplied v0.2.1 ROM."""
from pathlib import Path
import argparse, re, struct, sys
import numpy as np
P=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(P))
from robot import body_pose
q=argparse.ArgumentParser()
q.add_argument('--baseline',type=Path,required=True)
q.add_argument('--rom',type=Path,default=P/'rom/BULLRUSH-CITY-INTERCEPT-INTERNAL-98H.ROM')
a=q.parse_args();old=a.baseline.read_bytes();new=a.rom.read_bytes()
assert len(old)==len(new)==4194304
for bank in [1,2,55,62,19,20]:
    o=bank*16384
    nv,nf,vb,fb=struct.unpack_from('<BBHH',old,o)
    nv2,nf2,vb2,fb2=struct.unpack_from('<BBHH',new,o)
    assert (nv,vb)==(nv2,vb2)
    assert old[o+10:o+10+vb]==new[o+10:o+10+vb],bank
assert old[34*16384:35*16384]==new[34*16384:35*16384]
assert old[64*16384:236*16384]==new[64*16384:236*16384]
settings=[(l,.28+.35*d,l*.45,0,d) for d in (0,.33,.66,1) for l in (-.8,-.4,0,.4,.8)]
settings +=[(.55,.7,.5,.3,1),(.75,.8,.7,.8,1),(.4,.65,.4,1,.7),(0,.6,0,.8,.5)]
needed=[]
for args in settings:
    m,feet,_=body_pose(*args);v=np.array(m.v)
    expected=np.rint(feet[1])-np.rint(feet[0])
    needed.append(int(not np.all(v[140:148]-v[84:92]==expected)))
text=(P/'src/game.c').read_text()
flags=list(map(int,re.search(r'boot_exact\[24\]=\{([^}]+)',text)[1].split(',')))
assert flags==needed,(flags,needed)
assert new[1*16384+1]+2*new[2*16384+1]==old[1*16384+1]+2*old[2*16384+1]
print('PASS: original vertices,24 poses,rotation tables and per-robot face count preserved; exact fallback covers rounded poses')
