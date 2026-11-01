#include <string.h>
/* BULLRUSH: CITY INTERCEPT. Original real-time game logic; no route playback.
   TurboR/R800 + V9968 + Geo3D V2. Fixed ROM page 8000-BFFF. */
typedef unsigned char u8;
typedef signed char i8;
typedef unsigned int u16;
typedef signed int i16;
typedef signed long i32;
typedef unsigned long u32;
__sfr __at(0x99) vctrl;
__sfr __at(0x9B) vcmd;
__sfr __at(0xAA) keyrow;
__sfr __at(0xA9) keybits;
__sfr __at(0xE6) timerlo;
__sfr __at(0xE7) timerhi;
__sfr __at(0xA0) psgreg;
__sfr __at(0xA1) psgdata;
#define MEM(a) (*(volatile u8 *)(a))
#define WORD(a) (*(volatile u16 *)(a))
#define PAGE MEM(0xE200)
#define VIEW MEM(0xE444)
#define TEX MEM(0xE440)
#define TITLE 0
#define PLAY 1
#define WIN 2
#define LOSE 3
#define PAUSED 4
#define EXTRACT 5
#include "tables.h"
typedef struct {i16 x,y,z;u8 bank,angle,pose,ground;} Obj;
Obj objects[48];
i16 projection_z[48],projection_x[48],sort_depth;
typedef struct {i16 x,z;u8 segment,angle,roll;} Patrol;
Patrol patrol[3];
u8 enemyhp[4],focus,alive,armor,swingmask,shotpending[4];
u16 sparks[4],fireclock[4],invulnerable,ramclock;
i16 shotx[4],shotz[4];
static const u8 patrol_routes[3][12]={{1,2,6,10,14,13,9,5,1,0,4,5},{8,4,5,6,7,11,15,14,10,9,8,12},{11,7,3,2,1,0,4,8,12,13,14,15}};
u8 order[48], count, state, oldspace, oldc, oldt, oldp, oldenter;
u8 angle, camangle, eangle, targetnode, previousnode, enode, rng;
u8 hp, energy, damage, combo, attack, hitflash, cooldown, spotted, jump, escape;
u16 timeleft, clockpart, playtime, tick, dt, best, score, jumpclock, stuntclock, beat;
i16 px,pz,ex,ez,ey,speed,espeed,cx,cy,cz,turn,slide,dist,oldx,oldz;
i16 sv,cv,pitch,cpitch;
u8 boost,key8,key2,key3,key4,key5,key6,key7;
u8 hudcmd[15];
u8 soundleft,soundkind,mute,oldm;
u16 extraction;
i16 lean;
u8 roll,eroll;

i16 ab(i16 x){return x<0?-x:x;}
i16 mul(i16 x,i16 q) __naked {
 x;q;
 __asm
 push bc
 ld b,d
 ld c,e
 ld a,h
 xor b
 push af
 bit 7,h
 jr z,00001$
 xor a
 sub l
 ld l,a
 sbc a,a
 sub h
 ld h,a
00001$:
 bit 7,b
 jr z,00002$
 xor a
 sub c
 ld c,a
 sbc a,a
 sub b
 ld b,a
00002$:
 .db 0xed,0xc3
 ld d,e
 ld e,h
 pop af
 or a
 jp p,00003$
 xor a
 sub e
 ld e,a
 sbc a,a
 sub d
 ld d,a
00003$:
 pop bc
 ret
 __endasm;
}
u8 random8(void){rng^=rng<<3;rng^=rng>>5;rng^=rng<<1;return rng;}
u8 keys(u8 row){u8 saved=keyrow,v;keyrow=(saved&240)|row;v=~keybits;keyrow=saved;return v;}
void reg(u8 n,u8 v){vctrl=v;vctrl=n|128;}
void waitv(void){reg(15,2);do{MEM(0xE447)|=keys(8)&1;}while(vctrl&1);}
void command(u8 op,u16 sx,u16 sy,u16 x,u16 y,u16 w,u16 h,u8 col){
 u8 i;waitv();hudcmd[0]=sx;hudcmd[1]=sx>>8;hudcmd[2]=sy;hudcmd[3]=sy>>8;
 hudcmd[4]=x;hudcmd[5]=x>>8;hudcmd[6]=y;hudcmd[7]=(y>>8)+PAGE;
 hudcmd[8]=w;hudcmd[9]=w>>8;hudcmd[10]=h;hudcmd[11]=h>>8;
 hudcmd[12]=col;hudcmd[13]=0;hudcmd[14]=op;reg(17,32);__asm
 ld hl,#_hudcmd
 ld bc,#0x0f9b
 otir
 __endasm;
}
void box(u8 x,u8 y,u16 w,u8 h,u8 c){if(w&&h)command(0xC0,0,0,x,y,w,h,c);}
void text(u8 x,u8 y,const char *s){while(*s){u8 ch=*s++;if(ch<32||ch>95)ch=32;ch-=32;command(0xD0,(ch&31)*6,212+(ch>>5)*10,x,y,6,9,0);x+=6;}}
void number(u8 x,u8 y,u16 n,u8 digits){char b[6];u8 i=digits;b[i]=0;while(i){b[--i]='0'+n%10;n/=10;}text(x,y,b);}
u8 clear_at(i16 x,i16 z){u8 i;if(x< -420||z< -420||x>3300||z>3300)return 0;
 for(i=0;i<BUILDINGS;i++){if(ab(x-buildings[i].x)<buildings[i].r+34&&ab(z-buildings[i].z)<buildings[i].r+34)return 0;}return 1;}
void reset(void){u8 j;for(j=0;j<4;j++){enemyhp[j]=100;sparks[j]=0;fireclock[j]=j*2100;shotpending[j]=0;shotx[j]=shotz[j]=0;}armor=100;alive=4;focus=0;swingmask=0;invulnerable=ramclock=0;for(j=0;j<3;j++){u8 n=patrol_routes[j][0];patrol[j].x=(n%4)*960;patrol[j].z=(n/4)*960;patrol[j].segment=1;patrol[j].angle=0;patrol[j].roll=0;}px=0;pz=0;ex=1920;ez=960;ey=0;angle=0;camangle=0;eangle=64;speed=0;espeed=0;
 enode=6;previousnode=5;targetnode=7;hp=100;energy=255;damage=0;combo=0;attack=0;hitflash=0;cooldown=0;
 spotted=0;jump=0;escape=0;timeleft=0;clockpart=0;playtime=0;score=0;jumpclock=0;stuntclock=0;beat=0;
 VIEW=0;TEX=1;extraction=0;lean=0;roll=eroll=0;state=PLAY;}
void game_init(void){reset();state=TITLE;best=0;tick=0;rng=93;oldspace=oldc=oldt=oldp=oldenter=0;mute=oldm=soundleft=soundkind=0;}
void sound(u8 kind){soundkind=kind;soundleft=kind==3?24:kind==4?30:7;}
void audio(void){u16 period;u8 v=0;
 if(soundleft&&state!=PAUSED&&!mute){soundleft--;v=soundleft>5?12:soundleft*2;period=soundkind==1?250-soundleft*20:soundkind==2?70+soundleft*40:soundkind==3?340-(24-soundleft)*10:100+(30-soundleft)*20;
  psgreg=4;psgdata=period;psgreg=5;psgdata=period>>8;
 }
 psgreg=7;psgdata=0xBB;psgreg=10;psgdata=v;
}
void choose_target(void){u8 options[4],n=0,v,tries=0;u16 bestd=0,d;i16 x,z;
 if(enode%4)options[n++]=enode-1;if(enode%4<3)options[n++]=enode+1;if(enode>3)options[n++]=enode-4;if(enode<12)options[n++]=enode+4;
 v=options[random8()%n];
 /* Randomized evasive choice at intersections; remain in the district. */
 while(tries++<n){u8 k=options[(tries+rng)%n];x=(k%4)*960;z=(k/4)*960;d=ab(x-px)+ab(z-pz);
  if(k!=previousnode&&d>=bestd){bestd=d;v=k;}}
 /* Retail-to-retail diagonal: jump over two low shop rooftops. */
 if(node_low[enode]&&(random8()&1)){u8 j;for(j=0;j<16;j++)if(node_low[j]&&ab((i16)(j%4)-(enode%4))==1&&ab((i16)(j/4)-(enode/4))==1){v=j;jump=2;break;}}
 previousnode=enode;targetnode=v;
}
void patrol_step(void){u8 i;for(i=0;i<3;i++){Patrol*e=&patrol[i];u8 n=patrol_routes[i][e->segment],a; i16 dx=(n%4)*960-e->x,dz=(n/4)*960-e->z,step=(i32)(390+i*40)*dt>>12,delta;
 if(!enemyhp[i+1])continue;
 if(ab(dx)+ab(dz)<=step){e->x=(n%4)*960;e->z=(n/4)*960;e->segment=(e->segment+1)%12;continue;}
 if(ab(dx)>ab(dz)){e->x+=dx<0?-step:step;a=dx<0?192:64;}else{e->z+=dz<0?-step:step;a=dz<0?128:0;}
 delta=(i8)(a-e->angle);e->angle+=(i8)(delta>6?6:delta< -6?-6:delta);e->roll+=(u8)step;
}}
i16 enemyx(u8 i){return i?patrol[i-1].x:ex;}
i16 enemyz(u8 i){return i?patrol[i-1].z:ez;}
void hit(u8 i,u8 amount){if(!enemyhp[i])return;enemyhp[i]=enemyhp[i]>amount?enemyhp[i]-amount:0;sparks[i]=2200;hitflash=8;score+=amount*10;if(!enemyhp[i]){alive--;score+=500;shotpending[i]=0;}hp=enemyhp[0];sound(2);}
void combat(void){u8 i;u16 closest=65535; i16 x,z,d;
 if(invulnerable)invulnerable=invulnerable>dt?invulnerable-dt:0;
 if(ramclock)ramclock=ramclock>dt?ramclock-dt:0;
 if((key8&1)&&!cooldown){attack=8;cooldown=13;swingmask=0;sound(1);}
 for(i=0;i<4;i++){
  MEM(0xE447)|=keys(8)&1;
  if(sparks[i])sparks[i]=sparks[i]>dt?sparks[i]-dt:0;
  if(!enemyhp[i])continue;
  x=enemyx(i);z=enemyz(i);d=ab(x-px)+ab(z-pz);
  if(d<closest){closest=d;focus=i;}
  if(attack&&!(swingmask&(1<<i))&&ab(x-px)<200&&ab(z-pz)<200&&(i||ey<180)){swingmask|=1<<i;hit(i,25);}
  if(!attack&&!ramclock&&speed>350&&ab(x-px)<85&&ab(z-pz)<85&&(i||ey<90)){hit(i,15);ramclock=1800;speed/=2;}
  if(!enemyhp[i])continue;
  fireclock[i]+=dt;
  if(fireclock[i]>10000){fireclock[i]=0;shotpending[i]=0;
   if(d<850&&clear_at((x+px)/2,(z+pz)/2)){shotx[i]=px;shotz[i]=pz;shotpending[i]=1;}}
  if(shotpending[i]&&fireclock[i]>=1500){shotpending[i]=0;
   if(!invulnerable&&ab(px-shotx[i])<145&&ab(pz-shotz[i])<145){armor=armor>10?armor-10:0;invulnerable=2400;sound(2);}}
 }
 if(!armor){state=LOSE;speed=0;if(score>best)best=score;sound(4);}
}
void simulation(void){i16 step,dx,dz,desired,delta;u16 elapsed;
 elapsed=dt;clockpart+=elapsed;beat+=elapsed;stuntclock+=elapsed;
 while(clockpart>=3995){clockpart-=3995;playtime++;}
 
 patrol_step();
 if(cooldown)cooldown--;if(hitflash)hitflash--;if(attack)attack--;
 turn=((key8&128)||(key3&2)?1:0)-((key8&16)||(key2&64)?1:0);
 slide=((key3&4)?1:0)-((key4&64)?1:0);
 boost=(key6&1)&&energy>4;
 desired=((key8&32)||(key5&16))?(boost?1050:700):((key8&64)||(key5&1))?-230:0;
 step=(i16)(dt>>2);if(step<12)step=12;
 if(speed<desired){speed+=step;if(speed>desired)speed=desired;}if(speed>desired){speed-=step;if(speed<desired)speed=desired;}
 delta=(i16)(dt>>6);if(!delta)delta=1;angle+=(i8)(turn*delta);
 step=(i16)(((i32)speed*dt)>>12);dx=mul(step,sintab[angle]);dz=mul(step,sintab[(u8)(angle+64)]);
 if(slide){i16 st=(i16)(((i32)(boost?600:370)*dt)>>12)*slide;dx+=mul(st,sintab[(u8)(angle+64)]);dz-=mul(st,sintab[angle]);}
 oldx=px;oldz=pz;if(clear_at(px+dx,pz))px+=dx;else speed/=2;if(clear_at(px,pz+dz))pz+=dz;else speed/=2;
 roll+=(u8)(ab(px-oldx)+ab(pz-oldz));eroll+=(u8)(step);
 if(boost&&(ab(speed)>250||slide)){u8 drain=1+(dt>>7);energy=energy>drain?energy-drain:0;}else if(energy<255){u16 e=energy+2+(dt>>6);energy=e>255?255:e;}
 if(enemyhp[0]){
 dx=(targetnode%4)*960-ex;dz=(targetnode/4)*960-ez;
 dist=ab(ex-px)+ab(ez-pz);if(dist<1250)spotted=1;
 espeed=dist<650?540:410;if(hp<40)espeed+=35;if(sparks[0])espeed=90;
 step=(i16)(((i32)espeed*dt)>>12);
 if(ab(dx)+ab(dz)<=step*2){ex=(targetnode%4)*960;ez=(targetnode/4)*960;enode=targetnode;jump=0;choose_target();}
 else if(jump==2){i16 st=step*181/256;ex+=dx<0?-st:st;ez+=dz<0?-st:st;desired=dx>0?(dz>0?32:96):(dz>0?224:160);delta=(i8)((u8)desired-eangle);eangle+=(i8)(delta>6?6:delta< -6?-6:delta);}
 else if(ab(dx)>ab(dz)){ex+=dx<0?-step:step;desired=dx<0?192:64;delta=(i8)((u8)desired-eangle);eangle+=(i8)(delta>6?6:delta< -6?-6:delta);}
 else {ez+=dz<0?-step:step;desired=dz<0?128:0;delta=(i8)((u8)desired-eangle);eangle+=(i8)(delta>6?6:delta< -6?-6:delta);}
 /* Reckless firing damages the district, with muzzle flash in the HUD. */
 if(stuntclock>15000){stuntclock=0;if(damage<99)damage+=2;}
 /* Short low-hop over traffic is cosmetic; world position remains deterministic. */
 if(beat>27000)beat=0;
 if(jump==2){i16 progress=960-ab((targetnode%4)*960-ex);if(progress<0)progress=0;ey=mul(240,sintab[(u8)((i32)progress*128/960)]);}else ey=0;
 }else ey=0;
 combat();
}

void add(u8 bank,i16 x,i16 y,i16 z,u8 a,u8 pose,u8 ground){u8 i;if(count>=48)return;i=count++;objects[i].bank=bank;objects[i].x=x;objects[i].y=y;objects[i].z=z;objects[i].angle=a;objects[i].pose=pose;objects[i].ground=ground;}
/* Same strict signed-depth compare/exchange order as the C loop. */
void fast_sort(u8 first) __naked {
 first;
 __asm
 push ix
 push iy
 push bc
 ld e,a
 ld d,#0
 ld hl,#_order
 add hl,de
 push hl
 pop ix
 ld a,(_count)
 sub e
 ld b,a
01001$:
 ld a,b
 cp #2
 jr c,01006$
 ld c,b
 dec c
 ld l,0(ix)
 ld h,#0
 add hl,hl
 ld de,#_projection_z
 add hl,de
 ld e,(hl)
 inc hl
 ld d,(hl)
 ld (_sort_depth),de
 push ix
 pop iy
 inc iy
01002$:
 ld l,0(iy)
 ld h,#0
 add hl,hl
 ld de,#_projection_z
 add hl,de
 ld e,(hl)
 inc hl
 ld d,(hl)
 ld hl,(_sort_depth)
 ld a,h
 xor #0x80
 ld h,a
 ld a,d
 xor #0x80
 cp h
 jr c,01004$
 jr nz,01003$
 ld a,e
 cp l
 jr c,01004$
 jr z,01004$
01003$:
 ld a,0(ix)
 ld h,0(iy)
 ld 0(ix),h
 ld 0(iy),a
 ld (_sort_depth),de
01004$:
 inc iy
 dec c
 jr nz,01002$
 inc ix
 dec b
 jr 01001$
01006$:
 pop bc
 pop iy
 pop ix
 ret
 __endasm;
}
static const u8 boot_exact[24]={0,0,0,0,0,1,1,0,1,1,1,1,0,1,1,0,0,0,0,0,0,0,1,1};
void scene(void){u8 i,j,k,n,pose,epose; i16 dx,dz,z,x,y,dc,ds,near[12];u8 idx[12];i16 *rec=(i16*)0xC000;u8 *banks=(u8*)0xC000+1600,*poses=(u8*)0xC000+1700;
 count=0;
 {i16 d=(i8)(angle-camangle);camangle+=(i8)(d>8?8:d< -8?-8:d);}
 if(VIEW){camangle=angle;cy=120;cx=px;cz=pz;pitch=20;cpitch=255;}else{cy=190;cx=px-mul(330,sintab[camangle]);cz=pz-mul(330,sintab[(u8)(camangle+64)]);pitch=60;cpitch=249;}

 
 sv=sintab[camangle];cv=sintab[(u8)(camangle+64)];
 /* Roads draw first, buildings and robots are depth sorted as independent objects. */
 for(i=0;i<16;i++){x=(i%4)*960;z=(i/4)*960;dx=x-cx;dz=z-cz;y=mul(dx,sv)+mul(dz,cv);
  if(y> -650&&y<2100&&ab(mul(dx,cv)-mul(dz,sv))<y+1600)add(15,x,0,z,0,0,1);}
 n=count;
 for(i=0;i<12;i++){near[i]=32000;idx[i]=255;}
 for(i=0;i<BUILDINGS;i++){
  MEM(0xE447)|=keys(8)&1;
  /* Extraction cut: keep the distant skyline, clear foreground occluders. */
  

  dx=buildings[i].x-cx;dz=buildings[i].z-cz;z=mul(dx,sv)+mul(dz,cv);x=mul(dx,cv)-mul(dz,sv);
  if(z< -180||z>2000||ab(x)>z+440)continue;y=ab(dx)+ab(dz);
  for(j=0;j<12;j++)if(y<near[j]){for(k=11;k>j;k--){near[k]=near[k-1];idx[k]=idx[k-1];}near[j]=y;idx[j]=i;break;}}
 for(j=0;j<12;j++){i=idx[j];if(i!=255)add(buildings[i].bank,buildings[i].x,0,buildings[i].z,0,0,0);}
 {i16 target=(turn+slide)*64;if(target>64)target=64;if(target< -64)target=-64;
 if(lean<target){lean+=16;if(lean>target)lean=target;}if(lean>target){lean-=16;if(lean<target)lean=target;}}
 pose=(ab(speed)<120?0:ab(speed)<350?1:ab(speed)<600?2:3)*5+(u8)((lean+64)/32);
 if(attack)pose=20+((8-attack)/2);epose=17;MEM(0xE44B)=pose;
 if(!VIEW){add(1,px,0,pz,angle,pose,0);for(i=0;i<2;i++){x=feet[pose][i*3];y=feet[pose][i*3+1];z=feet[pose][i*3+2];add(2+i,px+mul(x,sintab[(u8)(angle+64)])+mul(z,sintab[angle]),y,pz-mul(x,sintab[angle])+mul(z,sintab[(u8)(angle+64)]),angle,roll,0);}}
 for(j=0;j<4;j++){i16 wx,wz;u8 ea=j?patrol[j-1].angle:eangle,er=j?patrol[j-1].roll:eroll; i16 eh=j?0:ey;
  wx=enemyx(j);wz=enemyz(j);if(ab(wx-px)+ab(wz-pz)>=2400)continue;
  if(!enemyhp[j])eh=sparks[j]>1000?(sparks[j]-1000)/30:0;
  add(enemyhp[j]?(j==1?62:55):(j==1?20:19),wx,eh,wz,ea,17,0);
  for(i=0;i<2;i++){volatile i16 fx,fz;if(enemyhp[j]){x=feet[17][i*3];y=feet[17][i*3+1];z=feet[17][i*3+2];}else{x=i?42:-42;y=14;z=-55;er=0;}fx=wx+mul(x,sintab[(u8)(ea+64)])+mul(z,sintab[ea]);fz=wz-mul(x,sintab[ea])+mul(z,sintab[(u8)(ea+64)]);add(enemyhp[j]?2+i:26,fx,eh+y,fz,ea,er,0);}
  if(sparks[j])add(21,wx,80+(2200-sparks[j])/24,wz,tick*11,0,0);
  if(shotpending[j]){i16 t=(i32)fireclock[j]*256/1500;add(18,wx+mul(shotx[j]-wx,t),90,wz+mul(shotz[j]-wz,t),ea,0,0);}
 }
 for(i=0;i<count;i++){order[i]=i;dx=objects[i].x-cx;dz=objects[i].z-cz;projection_z[i]=mul(dx,sv)+mul(dz,cv);projection_x[i]=mul(dx,cv)-mul(dz,sv);}
 fast_sort(n);
 MEM(0xC500)=count;
 for(i=0;i<count;i++){u8 index=order[i];Obj *o=&objects[index];u8 a=o->angle-camangle,view=VIEW?1:0;
  MEM(0xE447)|=keys(8)&1;
  if((o->bank==2||o->bank==3||o->bank==26)&&view<2){MEM(0x6000)=64+view*86+a/3;memcpy(rec,(const void*)(0x4000+(u16)(a%3)*4608+(u16)o->pose*18),18);}
  else{MEM(0x6000)=60;memcpy(rec,(const void*)(0x4000+(u16)view*4608+(u16)a*18),18);
   if((o->bank==2||o->bank==3||o->bank==26)){u8 r;for(r=0;r<3;r++){i16 aa=rec[r*3+1],bb=rec[r*3+2];rec[r*3+1]=mul(aa,sintab[(u8)(o->pose+64)])+mul(bb,sintab[o->pose]);rec[r*3+2]=-mul(aa,sintab[o->pose])+mul(bb,sintab[(u8)(o->pose+64)]);}}}
  y=o->y-cy;z=projection_z[index];rec[9]=projection_x[index];rec[10]=mul(y,cpitch)+mul(z,pitch);rec[11]=mul(z,cpitch)-mul(y,pitch);
  banks[-(i16)(count-i)]=o->bank;poses[-(i16)(count-i)]=o->pose;
  MEM(0xC520+index)=i;
  if(o->bank==2||o->bank==3){
   u8 owner=index-(o->bank==2?1:2);poses[-(i16)(count-i)]=owner;
   if(o->bank==3&&objects[owner].bank==1&&boot_exact[pose])poses[-(i16)(count-i)]|=128;
  }
  rec+=12;
 }

}
void game_tick(void){u8 enter;dt=timerlo;dt|=(u16)timerhi<<8;timerlo=0;dt>>=6;if(dt<10)dt=260;tick++;
 key2=keys(2);key3=keys(3)|MEM(0xE448);key4=keys(4)|MEM(0xE449);MEM(0xE448)=MEM(0xE449)=0;key5=keys(5);key6=keys(6);key7=keys(7)|MEM(0xE44A);key8=keys(8)|MEM(0xE447);MEM(0xE44A)=MEM(0xE447)=0;enter=(key7&128)!=0;
 if(state==TITLE||state==WIN||state==LOSE){if(enter&&!oldenter)reset();}
 else if((key4&32)&&!oldp){state=state==PAUSED?PLAY:PAUSED;}
 if((key3&1)&&!oldc)VIEW^=1;if((key5&2)&&!oldt)TEX^=1;
 if(state==PLAY)simulation();

 if(state==TITLE){angle=0;camangle=0;}
 if((key4&4)&&!oldm)mute^=1;oldm=key4&4;audio();
 oldenter=enter;oldspace=key8&1;oldc=key3&1;oldt=key5&2;oldp=key4&32;
 scene();
 /* Stable telemetry, read-only to the program; QA records real key-driven state. */
 MEM(0xD800)=state;MEM(0xD801)=hp;WORD(0xD802)=px;WORD(0xD804)=pz;WORD(0xD806)=ex;WORD(0xD808)=ez;
 WORD(0xD80A)=timeleft;WORD(0xD80C)=score;MEM(0xD80E)=angle;MEM(0xD80F)=energy;WORD(0xD810)=tick;WORD(0xD812)=dt;
 {u8 j;for(j=0;j<3;j++){u16 at=0xD840+j*8;WORD(at)=patrol[j].x;WORD(at+2)=patrol[j].z;MEM(at+4)=patrol[j].angle;MEM(at+5)=patrol[j].roll;}}
 MEM(0xD814)=damage;MEM(0xD815)=count;MEM(0xD816)=spotted;MEM(0xD817)=cooldown;MEM(0xD818)=armor;MEM(0xD819)=alive;MEM(0xD81A)=swingmask;
 {u8 i;for(i=0;i<4;i++){u16 at=0xD860+i*12;WORD(at)=enemyx(i);WORD(at+2)=enemyz(i);MEM(at+4)=enemyhp[i];WORD(at+5)=sparks[i];MEM(at+7)=shotpending[i];WORD(at+8)=fireclock[i];} }
}
void game_backdrop(void){
 box(0,86,256,112,2);box(0,13,256,1,2);
 if(TEX){command(0xD0,camangle,696,0,14,256-camangle,72,0);if(camangle)command(0xD0,0,696,256-camangle,14,camangle,72,0);}else box(0,14,256,72,12);
 /* Horizon silhouettes scroll with heading, never repeat a sun/moon. */
 {u8 i;for(i=0;i<5;i++){u8 x=i*57-camangle;box(x,68+(i&3)*3,19,18-(i&3)*3,11);}}
}
void radar(void){u8 i,x,y;command(0xD0,0,768,204,17,49,49,0);
 x=208+(px<0?0:px/74);y=60-(pz<0?0:pz/74);box(x,y,3,3,5);
 for(i=0;i<4;i++){i16 exx=enemyx(i),ezz=enemyz(i);x=208+(exx<0?0:exx/74);y=60-(ezz<0?0:ezz/74);if(enemyhp[i])box(x,y,3,3,i==focus?7:12);else box(x,y,2,2,4);}

}
void game_hud(void){
 command(0xD0,0,468,0,0,256,13,0);command(0xD0,0,481,0,198,256,14,0);
 number(94,2,alive,1);number(156,2,focus+1,1);box(168,4,(u16)enemyhp[focus]*80/100,4,hitflash?8:12);
 box(38,202,energy/8,4,8);number(102,200,armor,3);number(162,200,score,5);radar();
 if(state==PLAY){if(!alive)text(8,20,"DISTRICT CLEAR / FREE PATROL");else if(hitflash)text(8,20,"IMPACT / ARMOR BREAK");
  if(attack){box(98,113,18,2,8);box(140,113,18,2,8);}
  if(invulnerable){box(0,14,3,184,7);box(253,14,3,184,7);}
 }
 if(VIEW){box(0,174,256,24,1);box(0,174,256,2,11);text(8,182,"POLICE / NO FIRE");text(164,182,"C / CAMERA");box(121,104,14,1,13);box(128,98,1,13,13);}
 if(state==TITLE){box(14,46,228,139,1);box(14,46,228,2,12);text(48,56,"BULLRUSH / CITY INTERCEPT");text(31,73,"STOP ALL FOUR STOLEN UNITS");text(30,86,"ARROWS / WASD  MOVE AND TURN");text(30,98,"Q E  SLIDE     SHIFT  BOOST");text(30,110,"SPACE  STRIKE  C  CAMERA");text(30,122,"T  TEXTURES  P PAUSE  M MUTE");text(30,140,"NO TIME LIMIT. WATCH ARMOR.");text(59,160,"ENTER / BEGIN PATROL");}
 if(state==LOSE){box(30,58,196,112,1);box(30,58,196,2,7);text(62,72,"POLICE UNIT DISABLED");text(54,88,"ARMOR DEPLETED");text(62,106,"SCORE");number(128,106,score,5);text(62,119,"BEST");number(128,119,best,5);text(62,148,"ENTER / RETRY");}
 if(state==PAUSED){box(51,78,154,48,1);text(78,89,"PATROL PAUSED");text(72,108,"P / CONTINUE");}
 waitv();
}
