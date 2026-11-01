from pathlib import Path
import os,sys,subprocess,struct,json,math,hashlib,re
import shutil
import numpy as np
from PIL import Image,ImageDraw
from mesh import Mesh
from robot import body_pose,compressed_vertices
from hudfont import label
import city,textures
P=Path(__file__).resolve().parent;O=P/'out';O.mkdir(exist_ok=True)
TOOL=Path(os.environ.get('SDCC_BIN',Path(shutil.which('sdcc') or 'sdcc').resolve().parent))
os.environ['PATH']=str(TOOL)+os.pathsep+os.environ['PATH']
def run(args):subprocess.run([str(x) for x in args],cwd=P,check=True)
def clean(m):
    remap=[];vs=[];seen={}
    for v in m.v:
        v=tuple(round(x) for x in v)
        if v not in seen:seen[v]=len(vs);vs.append(v)
        remap.append(seen[v])
    m.v=vs;m.f=[([remap[i] for i in ids],[0,0,0],c) for ids,n,c in m.f];return m
def packed(m,kind=None):
    m=clean(m);m,uv=textures.mapped(m,kind);data=m.data()+uv
    assert len(data)<=16384
    return bytearray(data+bytes(16384-len(data)))
def building(kind):
    full=city.tile_mesh(kind);m=Mesh()
    # Extract one independent building, so depth ordering isn't tile-wide.
    for ids,n,c in full.f:
        pts=np.array([full.v[i] for i in ids],float);mid=pts.mean(axis=0)
        if mid[0]>150 and mid[2]>150:
            pts-=np.array([330,0,330]);m.poly(pts.tolist(),n,c)
    return m
def records(path):
    for line in path.read_text().splitlines():
        r=bytes.fromhex(line[1:]);assert sum(r)%256==0
        if r[3]==0:yield int.from_bytes(r[1:3],'big'),r[4:4+r[0]]
def build():
    if '--rebuild-assets' in sys.argv or not (P/'assets/baseline-assets.rom').exists():
        run([sys.executable,P/'asset_build.py'])
        (P/'assets/baseline-assets.rom').write_bytes((O/'BULLRUSH.ROM').read_bytes())
    raw=(P/'assets/baseline-assets.rom').read_bytes();banks=[bytearray(raw[i:i+16384]) for i in range(0,len(raw),16384)]
    for i in range(64):
        if i not in [0,1,2,4,5,6,7,*range(8,19),34,*range(52,59),63]:banks[i]=bytearray(16384)
    for i,kind in enumerate(city.KINDS):banks[8+i]=packed(building(kind),kind)
    road=Mesh();road.poly([(-480,0,-480),(-480,0,480),(480,0,480),(480,0,-480)],[0,0,0],2)
    for x in (-330,330):
        for z in (-330,330):road.poly([(x-154,1,z-154),(x-154,1,z+154),(x+154,1,z+154),(x+154,1,z-154)],[0,0,0],3)
    for a in (-400,-280,280,400):
        road.poly([(-3,1,a-30),(-3,1,a+30),(3,1,a+30),(3,1,a-30)],[0,0,0],8)
        road.poly([(a-30,1,-3),(a-30,1,3),(a+30,1,3),(a+30,1,-3)],[0,0,0],8)
    banks[15]=packed(road)
    bolt=Mesh();bolt.box(0,0,0,9,9,100,8);banks[18]=packed(bolt)
    # 24 connected joint poses; runtime selects based on steering, speed, strike.
    settings=[(lean,.28+.35*dash,lean*.45,0,dash) for dash in (0,.33,.66,1) for lean in (-.8,-.4,0,.4,.8)]
    settings +=[(.55,.7,.5,.3,1),(.75,.8,.7,.8,1),(.4,.65,.4,1,.7),(0,.6,0,.8,.5)]
    feet=[];poses=bytearray()
    for args in settings:
        m,f,j=body_pose(*args);poses+=compressed_vertices(m);feet.append([round(x) for v in f for x in v])
    banks[34]=poses+bytes(16384-len(poses))
    # Original crowd red face colors; preserve every original bitmap pixel.
    palpath=P/'src/palette.inc';paltext=palpath.read_text();values=[int(v) for v in paltext.split('.db ')[1].strip().split(',')]
    used=set()
    for b in [4,5,6,7,52,53,54,56,57,58]:used.update(banks[b])
    slots=[i for i in range(16,128) if i not in used][:3];assert len(slots)==3
    for slot,rgb in zip(slots,[(112,24,32),(200,48,48),(248,112,88)]):values[slot*3:slot*3+3]=[round(v*31/255) for v in rgb]
    palpath.write_text('palette:\n .db '+','.join(map(str,values))+'\n')
    red=bytearray(banks[1]);nv,nf,vb,fb=struct.unpack_from('<BBHH',red)
    for i in range(nf):
        off=6+vb+11*i+10;c=red[off];red[off]={4:slots[0],5:slots[1],6:slots[2]}.get(c,c)
    banks[62]=red
    (O/'crowd-provenance.json').write_text(json.dumps({'source':'bullrush-crowd-cockpit-v2','added':['one red robot','two blue robots'],'total_robots':5,'red_palette_slots':slots,'geometry':'same original body and wheels','patrol':'live original-city road-node routes; no prerecorded player movement','capture_target':'all four enemies, independent HP; destroyed wrecks persist'},indent=2))
    # Wreck uses original armor topology, folded forward and down; blue/red share vertices.
    for dest,source in [(19,55),(20,62)]:
        wreck=bytearray(banks[source]);nv,nf,vb,fb=struct.unpack_from('<BBHH',wreck)
        vv=np.frombuffer(bytes(banks[34][17*512:17*512+nv*3]),dtype=np.uint8).reshape(nv,3).astype(np.float64);vv[:,[0,2]]-=128
        # Rigid 90-degree fall preserves panel thickness instead of flattening armor.
        yy=vv[:,1].copy();zz=vv[:,2].copy();vv[:,1]=-zz;vv[:,2]=yy-65
        # Detach recognizable head/forearms/legs slightly while retaining topology.
        vv[24:36,0]+=12;vv[24:36,2]+=12
        vv[36:60,0]-=18;vv[36:60,2]+=10
        vv[92:116,0]+=18;vv[92:116,2]-=8
        vv[68:92,0]-=8;vv[124:148,0]+=8
        vv[:,1]-=vv[:,1].min()
        wreck[6:6+vb]=np.rint(vv).astype('<i2').tobytes();banks[dest]=wreck
    sparks=Mesh()
    for i in range(10):
        a=i*math.tau/10;x=math.cos(a)*(35+i%3*16);z=math.sin(a)*(35+i%3*16);y=(i%4)*13
        sparks.poly([(x-4,y,z),(x+4,y+3,z),(x,y+17,z+5)],[0,0,0],8 if i%2 else 5)
    banks[21]=packed(sparks)
    # V2 resident geometry: player poses stream only when changed; all rivals share pose17.
    geom={};vnext=296;fnext=0
    static_bases={1:0,55:148,62:148}
    for bank in [1,55,62,2,*range(8,16),18,19,20,21]:
        src=bytes(banks[bank]);nv,nf,vb,fb=struct.unpack_from('<BBHH',src);vertices=src[6:6+vb]
        if bank in [55,62]:
            compact=np.frombuffer(bytes(banks[34][17*512:17*512+nv*3]),dtype=np.uint8).reshape(nv,3).astype(np.int16);compact[:,[0,2]]-=128;vertices=compact.astype('<i2').tobytes()
        if bank==19:static_bases[20]=vnext
        base=static_bases.get(bank,vnext)
        if bank not in static_bases:vnext+=nv
        facebase=fnext;fnext+=nf;faces=bytearray()
        for j in range(nf):
            face=src[6+vb+j*11:6+vb+(j+1)*11];ids=[v+base for v in face[:4]];hi=sum((v>>8)<<(2*k) for k,v in enumerate(ids));faces+=bytes([v&255 for v in ids])+bytes([hi])+face[4:]
        uv=src[6+vb+fb:6+vb+fb+nf*8]
        blob=bytes([nv,nf])+struct.pack('<HHHH',len(vertices),len(faces),base,facebase)+vertices+faces+uv
        banks[bank]=bytearray(blob+bytes(16384-len(blob)));geom[bank]={'vertices':nv,'faces':nf,'vfirst':base,'ffirst':facebase}
    assert vnext<=1024 and fnext<=1024,(vnext,fnext)
    (O/'resident-models.json').write_text(json.dumps({'vertex_highwater':vnext,'face_highwater':fnext,'models':geom},indent=2))
    # Human-authored district; 4 x 4 junctions, mixed low and high buildings.
    kinds=['office','retail','tower','retail','housing','plaza','retail','office','retail','office','plaza','service','office','retail','housing','civic']
    buildings=[]
    for i,kind in enumerate(kinds):
        if kind=='plaza':continue
        for x in (-330,330):
            for z in (-330,330):buildings.append((i%4*960+x,i//4*960+z,round(city.MAP['districts'][kind]['width']/2),8+city.KINDS.index(kind)))
    table='static const i16 sintab[256]={'+','.join(str(round(math.sin(i*math.pi/128)*256)) for i in range(256))+'};\n'
    table+='typedef struct {i16 x,z,r;u8 bank;} Building;\n#define BUILDINGS '+str(len(buildings))+'\nstatic const Building buildings[BUILDINGS]={'+','.join('{'+','.join(map(str,b))+'}' for b in buildings)+'};\n'
    table+='static const i16 feet[24][6]={'+','.join('{'+','.join(map(str,f))+'}' for f in feet)+'};\n'
    table+='static const u8 node_low[16]={'+','.join('1' if k=='retail' else '0' for k in kinds)+'};\n'
    (P/'src/tables.h').write_text(table)
    font=Image.new('L',(256,20),1);d=ImageDraw.Draw(font)
    for i in range(64):label(d,((i%32)*6,(i//32)*10-2),chr(32+i),fill=5)
    b=np.frombuffer(bytes(banks[4]),dtype=np.uint8).reshape(64,256).copy();b[20:40]=np.asarray(font);banks[4]=bytearray(b.tobytes())
    # Exact fixed-point matrix tables: preserve the old integer rounding.
    banks.extend(bytearray(16384) for _ in range(256-len(banks)))
    trig=[round(math.sin(i*math.pi/128)*256) for i in range(256)]
    q=lambda a,b:(abs(a*b)//256)*(1 if a*b>=0 else -1)
    yawtables=[]
    for pit,cpit in [(60,249),(20,255),(30,254)]:
        mats=[]
        for a in range(256):
            ds=trig[a];dc=trig[(a+64)&255];mats.append([dc*64,0,ds*64,-q(ds,pit)*64,cpit*64,q(dc,pit)*64,-q(ds,cpit)*64,-pit*64,q(dc,cpit)*64])
        yawtables.append(mats)
    yawraw=struct.pack('<'+'h'*(3*256*9),*[v for mats in yawtables for m in mats for v in m]);banks[60][:len(yawraw)]=yawraw
    for view in range(2):
        for a in range(256):
            bank=64+view*86+a//3;offset=(a%3)*4608;data=[]
            for roll in range(256):
                m=yawtables[view][a].copy();ds=trig[roll];dc=trig[(roll+64)&255]
                for r in range(3):
                    aa=m[r*3+1];bb=m[r*3+2];m[r*3+1]=q(aa,dc)+q(bb,ds);m[r*3+2]=-q(aa,ds)+q(bb,dc)
                data.extend(m)
            blob=struct.pack('<'+'h'*len(data),*data);banks[bank][offset:offset+len(blob)]=blob
    # Static HUD text and radar are exact copies of the original runtime composition.
    hud=np.ones((27,256),dtype=np.uint8);radar=np.ones((49,49),dtype=np.uint8);radar[1:48,1:48]=3
    fontdata=np.asarray(font)
    def puttext(x,y,text):
        for ch in text:
            n=ord(ch)-32;hud[y:y+9,x:x+6]=fontdata[(n//32)*10:(n//32)*10+9,(n%32)*6:(n%32)*6+6];x+=6
    for x,y,t in [(4,2,'BULLRUSH'),(64,2,'LEFT'),(124,2,'UNIT'),(4,15,'BOOST'),(78,15,'HP'),(126,15,'SCORE')]:puttext(x,y,t)
    hud[3:9,167:249]=3;hud[16:22,37:71]=3
    for i in range(4):radar[3:44,4+i*13]=11;radar[3+i*13,4:45]=11
    page=np.frombuffer(bytes(banks[5]),dtype=np.uint8).reshape(64,256).copy();page[20:47]=hud;banks[5]=bytearray(page.tobytes())
    page=np.frombuffer(bytes(banks[54]),dtype=np.uint8).reshape(64,256).copy();page[:49,:49]=radar;banks[54]=bytearray(page.tobytes())
    # Static C ROM page; globals only D000-D7FF. No CRT or initialized data.
    run([TOOL/'sdcc.exe','-mz80','--std-sdcc11','--opt-code-speed','--no-std-crt0','--code-loc','0x8100','--data-loc','0xD000','-Wl-b_HOME=0xBE00','-I'+str(P/'src'),'-o',O/'game.ihx',P/'src/game.c'])
    mp=(O/'game.map').read_text()
    for area,limit in [('_CODE',0xBE00),('_HOME',0xC000),('_DATA',0xD800)]:
        found=re.search(r'^'+area+r'\s+([0-9A-F]{8})\s+([0-9A-F]{8})',mp,re.M);assert found and int(found[1],16)+int(found[2],16)<=limit,(area,found.groups() if found else None)
    symbols={}
    for line in (O/'game.map').read_text().splitlines():
        mt=re.search(r'\b([0-9A-F]{8})\s+(_game_(?:init|tick|backdrop|hud))\b',line)
        if mt:symbols[mt[2]]=int(mt[1],16)
    assert len(symbols)==4,symbols
    banks[63]=bytearray(16384)
    for a,b in records(O/'game.ihx'):
        assert 0x8000<=a and a+len(b)<=0xC000,(hex(a),len(b))
        banks[63][a-0x8000:a-0x8000+len(b)]=b
    s=(P/'src/renderer.template.asm').read_text().replace('.include "palette.inc"', '.include "'+(P/'src/palette.inc').as_posix()+'"')
    for k,v in symbols.items():s=s.replace(k[1:].upper(),str(v))
    (P/'src/player.asm').write_text(s)
    run([TOOL/'sdasz80.exe','-los',O/'player.rel',P/'src/player.asm'])
    run([TOOL/'sdldz80.exe','-n','-i',O/'player',O/'player.rel'])
    banks[0][0x100:0xB00]=bytes(0xA00)
    for a,b in records(O/'player.ihx'):
        if a>=0xE800:assert a+len(b)<=0xF200;a=a-0xE800+0x4100
        banks[0][a-0x4000:a-0x4000+len(b)]=b
    rom=b''.join(banks);(O/'BULLRUSH-CITY-INTERCEPT.ROM').write_bytes(rom)
    info={'rom_bytes':len(rom),'sha256':hashlib.sha256(rom).hexdigest(),'entrypoints':symbols,'buildings':len(buildings),'poses':24,'mapper':'ASCII16'}
    (O/'manifest.json').write_text(json.dumps(info,indent=2));print(info)
if __name__=='__main__':build()
