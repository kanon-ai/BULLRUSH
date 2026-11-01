; BULLRUSH playable city pursuit / Geo3D V2
.module skyweave
.area ROM (ABS)
.org 0x4000
.ascii "AB"
.dw boot,0,0,0
.ds 6
boot:
 di
 ld sp,#0xF300
 ld hl,#0x4100
 ld de,#0xE800
 ld bc,#0xA00
 ldir
 jp start
VIEW = 0xE444
VIEWKEY = 0xE445
PAGE = 0xE200
FRAME = 0xE202
REC = 0xC000
CMD = 0xE400
PTR = 0xE410
COUNT = 0xE412
DONE = 0xE420
MODEL = 0xE414
MODEL_PTR = 0xE416
SOUND = 0xE418
SOUND_KIND = 0xE419
TEX = 0xE440
KEYOLD = 0xE441
TOPOLOGY = 0xE442
GEOMHDR = 0xE480
GEOMDATA = 0xE486
SKY = 0xE430
WALL = 0xE450
WCOUNT = 0xE470
.org 0xE800
start:
 di
 ld sp,#0xF300
 call 0x0138
 rlca
 rlca
 and #3
 ld c,a
 ld b,#0
 ld hl,#0xFCC1
 add hl,bc
 ld a,(hl)
 and #0x80
 or c
 ld c,a
 inc hl
 inc hl
 ld a,(hl)
 rrca
 rrca
 rrca
 rrca
 and #12
 or c
 ld h,#0x80
 call 0x0024
 di
 in a,(0xFF)
 xor #1
 out (0xFE),a
 ld a,(0x002D)
 cp #3
 jr c,cpu_ready
 ld a,#0x81
 call 0x0180
 di
cpu_ready:
 ld a,(0x002D)
 cp #3
 jp c,v2_error
 call require_v2
 xor a
 ld (VIEW),a
 ld (VIEWKEY),a
 ld (KEYOLD),a
 ld (PAGE),a
 ld hl,#0
 ld (FRAME),hl
 ld (DONE),hl
 ld a,#1
 ld (TEX),a
 xor a
 out (0x9C),a
 ld hl,#regs
regloop:
 ld b,(hl)
 inc hl
 ld a,b
 cp #255
 jr z,regdone
 ld a,(hl)
 inc hl
 call wreg
 jr regloop
regdone:
 xor a
 ld b,#16
 call wreg
 ld hl,#palette
 ld bc,#0x009A
 otir
 ld b,#0
 otir
 ld b,#0
 otir
 ld hl,#asset_blocks
asset_loop:
 ld a,(hl)
 inc hl
 cp #255
 jr z,assets_ready
 ld (0x6000),a
 ld a,(hl)
 inc hl
 push hl
 ld b,#14
 call wreg
 xor a
 out (0x99),a
 ld a,#0x40
 out (0x99),a
 ld hl,#0x4000
 ld de,#16384
 call vramblock
 pop hl
 jr asset_loop
assets_ready:
 ld a,#0x6F
 out (0x9D),a
 ld a,#1
 out (0x9F),a
 xor a
 ld (0xE447),a
 ld (0xE448),a
 ld (0xE449),a
 ld (0xE44A),a
 ld a,#255
 ld (0xE446),a
 call cache_models
 ld a,#0x60
 out (0x9D),a
 xor a
 out (0x9F),a
 out (0x9F),a
 ld a,#112
 out (0x9F),a
 ld a,#3
 out (0x9F),a
 xor a
 out (0x9F),a
 ld a,#0x18
 out (0x9D),a
 ld hl,#camera
 ld bc,#0x0C9F
 otir
 ld a,#0x40
 ld b,#1
 call wreg
 ; Copy fixed C engine into page-2 RAM; mapper switches only page 1.
 ld a,#63
 ld (0x6000),a
 ld hl,#0x4000
 ld de,#0x8000
 ld bc,#0x4000
 ldir
 call GAME_INIT
main:
 call GAME_TICK
 ld a,(PAGE)
 xor #1
 ld (PAGE),a
 ld hl,#background
 call load15
 ; Game draws an opaque sky/ground backing in the hidden page.
 call GAME_BACKDROP
 call prepare_foot_records
 ; Upload changed protagonist pose before either foot can be drawn.
 ld a,(VIEW)
 or a
 jr nz,body_pose_ready
 ld a,(0xE44B)
 ld b,a
 ld a,(0xE446)
 cp b
 jr z,body_pose_ready
 ld a,#1
 ld (MODEL),a
 ld (0x6000),a
 call load_geometry
body_pose_ready:
 ld a,#0x46
 out (0x9D),a
 xor a
 out (0x9F),a
 ld a,(PAGE)
 out (0x9F),a
 ld hl,#REC
 ld (PTR),hl
 ld a,(0xC500)
 ld (COUNT),a
object_loop:
 ld hl,#0x4000
 ld (MODEL_PTR),hl
 ld a,(COUNT)
 ld e,a
 ld d,#0
 ld hl,#REC+1600
 or a
 sbc hl,de
 ld a,(hl)
 cp #255
 jr z,skip_object
 ld (MODEL),a
 ld (0x6000),a
 call load_geometry
 xor a
 out (0x9D),a
 ld hl,(PTR)
 ld bc,#0x189F
 otir
 ld (PTR),hl
 ld a,(MODEL)
 cp #2
 jp z,grouped_foot
 cp #3
 jp z,grouped_foot
 ld a,#0x48
 out (0x9D),a
 ld a,(TEX)
 add a,a
 add a,a
 or #3
 out (0x9F),a
geo_wait:
 call sample_action_keys
 in a,(0x9D)
 and #1
 jr nz,geo_wait
 jr next_object
skip_object:
 ld hl,(PTR)
 ld de,#24
 add hl,de
 ld (PTR),hl
next_object:
 ld a,(COUNT)
 dec a
 ld (COUNT),a
 jr nz,object_loop
 call GAME_HUD
 call vblank
 ld a,(PAGE)
 rrca
 rrca
 rrca
 or #31
 ld b,#2
 call wreg
 ld hl,(DONE)
 inc hl
 ld (DONE),hl
 jp main
load_or_reuse_geometry:
 ; Identical left/right wheel geometry can remain resident for the second wheel.
 ld a,(COUNT)
 cp #5
 jr z,reuse_enemy_wheel
 cp #2
 jp nz,load_geometry
 ld a,(REC+247)
 jr reuse_check
reuse_enemy_wheel:
 ld a,(REC+244)
reuse_check:
 cp #255
 ret nz
 jp load_geometry
load_geometry:
 ld a,(MODEL)
 cp #1
 jr nz,select_resident
 ; Only the protagonist uses changing poses. Static rivals have their own slot.
 ld a,(0xE44B)
 ld b,a
 ld a,(0xE446)
 cp b
 jr z,select_resident
 ld a,b
 ld (0xE446),a
 ld h,#0x40
 ld l,#0
 add a,a
 add a,h
 ld h,a
 push hl
 ld a,#0x68
 out (0x9D),a
 xor a
 out (0x9F),a
 ld a,#0x40
 out (0x9D),a
 xor a
 out (0x9F),a
 ld a,#0x50
 out (0x9D),a
 ld a,#34
 ld (0x6000),a
 pop hl
 ld b,#148
body_vertex_loop:
 ld a,(hl)
 inc hl
 xor #0x80
 out (0x9F),a
 rlca
 sbc a,a
 out (0x9F),a
 ld a,(hl)
 inc hl
 out (0x9F),a
 xor a
 out (0x9F),a
 ld a,(hl)
 inc hl
 xor #0x80
 out (0x9F),a
 rlca
 sbc a,a
 out (0x9F),a
 djnz body_vertex_loop
 ld a,#1
 ld (0x6000),a
select_resident:
 ld a,(MODEL)
 cp #2
 jr z,foot_material
 cp #3
 jr nz,resident_material_ready
foot_material:
 ld a,(COUNT)
 ld e,a
 ld d,#0
 ld hl,#REC+1700
 or a
 sbc hl,de
 ld a,(hl)
 cp #55
 ld a,#22
 jr z,foot_material_ready
 ld a,(hl)
 cp #62
 ld a,#23
 jr z,foot_material_ready
 ld a,#2
foot_material_ready:
 ld (0x6000),a
resident_material_ready:
 ld a,#0x54
 out (0x9D),a
 ld hl,#0x4006
 ld bc,#0x049F
 otir
 ld a,#0x42
 out (0x9D),a
 ld a,(0x4000)
 out (0x9F),a
 xor a
 out (0x9F),a
 ld a,#15
 out (0x9F),a
 xor a
 out (0x9F),a
 ld a,#0x59
 out (0x9D),a
 ld a,(0x4001)
 out (0x9F),a
 ret
texture_key:
 in a,(0xAA)
 ld d,a
 and #0xF0
 or #5
 out (0xAA),a
 in a,(0xA9)
 cpl
 and #2
 ld e,a
 ld a,d
 out (0xAA),a
 ld a,(KEYOLD)
 ld d,a
 ld a,e
 ld (KEYOLD),a
 or a
 ret z
 ld a,d
 or a
 ret nz
 ld a,(TEX)
 xor #1
 ld (TEX),a
 ret
foreground_pass:
 ld a,(REC+270)
 cp #255
 ret z
 ld (0x6000),a
 ld hl,(REC+268)
 ld (MODEL_PTR),hl
 ld a,#10
 ld (COUNT),a
 call load_geometry
 xor a
 out (0x9D),a
 ld hl,#identity_pose
 ld bc,#0x189F
 otir
 ld a,#0x48
 out (0x9D),a
 ld a,(TEX)
 add a,a
 add a,a
 or #3
 out (0x9F),a
foreground_wait:
 in a,(0x9D)
 and #1
 jr nz,foreground_wait
 ld a,#3
 ld (COUNT),a
 ret
select_view:
 ld a,(VIEW)
 or a
 ret z
 ld hl,#REC+288
 ld de,#REC
 ld bc,#168
 ldir
 ld hl,#REC+456
 ld de,#REC+240
 ld bc,#7
 ldir
 ld a,#255
 ld (REC+247),a
 ld (REC+248),a
 ld (REC+249),a
 ld hl,#REC+463
 ld de,#REC+268
 ld bc,#3
 ldir
 ret
view_key:
 in a,(0xAA)
 ld d,a
 and #0xF0
 or #3
 out (0xAA),a
 in a,(0xA9)
 cpl
 and #1
 ld e,a
 ld a,d
 out (0xAA),a
 ld a,(VIEWKEY)
 ld d,a
 ld a,e
 ld (VIEWKEY),a
 or a
 ret z
 ld a,d
 or a
 ret nz
 ld a,(VIEW)
 xor #1
 ld (VIEW),a
 ret
cockpit_overlay:
 ld a,(VIEW)
 or a
 ret z
 ld hl,#cockpit_rects
 ld a,#10
cock_rect_loop:
 push af
 push hl
 call load15
 ld a,(PAGE)
 ld b,a
 ld a,(CMD+7)
 add a,b
 ld (CMD+7),a
 call send15
 call waitce
 pop hl
 ld de,#15
 add hl,de
 pop af
 dec a
 jr nz,cock_rect_loop
 ret
cockpit_rects:
 .dw 0,0,0,14,5,184
 .db 2,0,0xC0
 .dw 0,0,251,14,5,184
 .db 2,0,0xC0
 .dw 0,0,5,179,246,19
 .db 1,0,0xC0
 .dw 0,0,5,179,246,2
 .db 4,0,0xC0
 .dw 0,0,17,185,52,7
 .db 11,0,0xC0
 .dw 0,0,20,187,35,3
 .db 13,0,0xC0
 .dw 0,0,188,185,48,7
 .db 2,0,0xC0
 .dw 0,0,191,187,29,3
 .db 8,0,0xC0
 .dw 0,0,122,105,12,1
 .db 13,0,0xC0
 .dw 0,0,127,100,1,11
 .db 13,0,0xC0
identity_pose:
 .dw 16384,0,0,0,16384,0,0,0,16384,0,0,0
wreg:
 out (0x99),a
 ld a,b
 or #0x80
 out (0x99),a
 ret
status2:
 ld a,#2
 ld b,#15
 call wreg
 in a,(0x99)
 ret
waitce:
 call status2
 and #1
 jr nz,waitce
 ret
vblank:
 call sample_action_keys
 call status2
 and #0x40
 jr nz,vblank
vbnext:
 call status2
 and #0x40
 jr z,vbnext
 ret
load15:
 ld de,#CMD
 ld bc,#15
 ldir
 ret
send15:
 ld a,#32
 ld b,#17
 call wreg
 ld hl,#CMD
 ld bc,#0x0F9B
 otir
 ret
sample_action_keys:
 push af
 push bc
 push de
 in a,(0xAA)
 ld d,a
 and #0xF0
 ld e,a
 ld a,e
 or #8
 out (0xAA),a
 in a,(0xA9)
 cpl
 and #1
 ld b,a
 ld a,(0xE447)
 or b
 ld (0xE447),a
 ld a,e
 or #3
 out (0xAA),a
 in a,(0xA9)
 cpl
 and #255
 ld b,a
 ld a,(0xE448)
 or b
 ld (0xE448),a
 ld a,e
 or #4
 out (0xAA),a
 in a,(0xA9)
 cpl
 and #255
 ld b,a
 ld a,(0xE449)
 or b
 ld (0xE449),a
 ld a,e
 or #7
 out (0xAA),a
 in a,(0xA9)
 cpl
 and #128
 ld b,a
 ld a,(0xE44A)
 or b
 ld (0xE44A),a
 ld a,d
 out (0xAA),a
 pop de
 pop bc
 pop af
 ret
geoblock:
 ld c,#0x9F
 jr block
vramblock:
 ld c,#0x98
block:
 ld a,d
 or a
 jr z,tail
blockloop:
 ld b,#0
 otir
 dec a
 jr nz,blockloop
tail:
 ld a,e
 or a
 ret z
 ld b,e
 otir
 ret
asset_blocks:
 .db 4,3,5,7,6,8,7,9,52,10,53,11,54,12,56,13,57,14,58,15,255
regs:
 .db 0,14,1,0,2,31,7,0,8,10,9,128,21,0,20,17,51,0,52,0,53,0,54,0,55,255,56,0,57,255,58,3,255
camera:
 .dw 170,128,106,4,256,212
light:
 .dw -5000,10500,-11500
background:
 .dw 0,512,0,14,256,184
 .db 0,0,0x30
hudtop:
 .dw 0,512,0,0,256,14
 .db 0,0,0xD0
hudbottom:
 .dw 0,710,0,198,256,14
 .db 0,0,0xD0
; v2 keeps face topology in RAM above the dynamic 0..255 region.
first_face:
 ld a,#0x56
 out (0x9D),a
 ld a,l
 out (0x9F),a
 ld a,h
 out (0x9F),a
 ret
require_v2:
 ld a,#0x49
 out (0x9D),a
 in a,(0x9F)
 cp #0x20
 jr nz,v2_error
 ld a,#0x6F
 out (0x9D),a
 in a,(0x9F)
 and #0xF0
 cp #0xA0
 ret z
v2_error:
 ; Unsupported engine: stop before using extended RAM; red diagnostic screen.
 ld a,#0x88
 out (0x99),a
 ld a,#0x87
 out (0x99),a
 xor a
 out (0x99),a
 ld a,#0x80
 out (0x99),a
 ld a,#0x50
 out (0x99),a
 ld a,#0x81
 out (0x99),a
v2_error_stop:
 jr v2_error_stop
cache_models:
 ld hl,#resident_models
cache_model_next:
 ld a,(hl)
 inc hl
 cp #255
 ret z
 push hl
 ld (MODEL),a
 ld (0x6000),a
 ld a,#0x68
 out (0x9D),a
 ld a,(0x4007)
 out (0x9F),a
 ld a,#0x40
 out (0x9D),a
 ld a,(0x4006)
 out (0x9F),a
 ld a,#0x50
 out (0x9D),a
 ld hl,#0x400A
 ld de,(0x4002)
 call geoblock
 ld a,#0x6C
 out (0x9D),a
 ld a,(0x4009)
 out (0x9F),a
 ld a,#0x58
 out (0x9D),a
 ld a,(0x4008)
 out (0x9F),a
 ld a,#0x52
 out (0x9D),a
 ld de,(0x4004)
 call geoblock
 ld a,#0x6E
 out (0x9D),a
 ld a,(0x4009)
 out (0x9F),a
 ld a,#0x65
 out (0x9D),a
 ld a,(0x4008)
 out (0x9F),a
 push hl
 ld a,(0x4001)
 ld l,a
 ld h,#0
 add hl,hl
 add hl,hl
 add hl,hl
 ex de,hl
 pop hl
 ld a,#0x53
 out (0x9D),a
 call geoblock
 pop hl
 jr cache_model_next

; Keep the roller projection and depth-sort it together with its own boot.
; No simplification, per-side scaling, or fixed front/back mask.

prepare_foot_records:
 ld a,(0xC500)
 ld b,a
 ld e,a
 ld d,#0
 ld hl,#REC+1600
 or a
 sbc hl,de
 push hl
 pop ix
 ld hl,#REC+1700
 or a
 sbc hl,de
 push hl
 pop iy
 ld de,#REC+2048
foot_record_loop:
 ld a,0(ix)
 cp #2
 jr z,copy_foot_record
 cp #3
 jr nz,foot_record_next
copy_foot_record:
 push bc
 push de
 ld a,0(iy)
 and #0x7F
 add a,#0x20
 ld l,a
 ld h,#0xC5
 ld a,(hl)
 ld l,a
 ld h,#0
 add hl,hl
 add hl,hl
 add hl,hl
 ld b,h
 ld c,l
 add hl,hl
 add hl,bc
 ld bc,#REC
 add hl,bc
 ld bc,#24
 ldir
 push af
 ld a,0(ix)
 cp #3
 call z,shift_right_boot
 pop af
 ; A retains the owner's sorted record index through LDIR.
 ld e,a
 ld d,#0
 ld hl,#REC+1600
 add hl,de
 ld a,(0xC500)
 ld e,a
 or a
 sbc hl,de
 ld a,(hl)
 bit 7,0(iy)
 jr z,foot_owner_ready
 or #0x80
foot_owner_ready:
 ld 0(iy),a
 pop de
 pop bc
foot_record_next:
 ex de,hl
 ld de,#24
 add hl,de
 ex de,hl
 inc ix
 inc iy
 djnz foot_record_loop
 ret


; Right boot = identical left boot plus the exact foot-center displacement.
; Source records are already in camera space; no extra matrix multiplication.
shift_right_boot:
 bit 7,0(iy)
 ret nz
 push ix
 push iy
 push de
 ld a,0(iy)
 inc a
 add a,#0x20
 ld l,a
 ld h,#0xC5
 ld l,(hl)
 ld h,#0
 add hl,hl
 add hl,hl
 add hl,hl
 ld b,h
 ld c,l
 add hl,hl
 add hl,bc
 ld bc,#REC
 add hl,bc
 push hl
 pop iy
 ; DE is one past this destination boot record.
 ex de,hl
 ld bc,#24
 or a
 sbc hl,bc
 push hl
 pop ix
 ld bc,#2048
 or a
 sbc hl,bc
 ex de,hl
 push de
 ld hl,#18
 add hl,de
 ld e,(hl)
 inc hl
 ld d,(hl)
 ld l,18(iy)
 ld h,19(iy)
 ex de,hl
 or a
 sbc hl,de
 ld e,18(ix)
 ld d,19(ix)
 add hl,de
 ld 18(ix),l
 ld 19(ix),h
 pop de
 push de
 ld hl,#20
 add hl,de
 ld e,(hl)
 inc hl
 ld d,(hl)
 ld l,20(iy)
 ld h,21(iy)
 ex de,hl
 or a
 sbc hl,de
 ld e,20(ix)
 ld d,21(ix)
 add hl,de
 ld 20(ix),l
 ld 21(ix),h
 pop de
 push de
 ld hl,#22
 add hl,de
 ld e,(hl)
 inc hl
 ld d,(hl)
 ld l,22(iy)
 ld h,23(iy)
 ex de,hl
 or a
 sbc hl,de
 ld e,22(ix)
 ld d,23(ix)
 add hl,de
 ld 22(ix),l
 ld 23(ix),h
 pop de
 pop de
 pop iy
 pop ix
 ret

grouped_foot:
 ld a,#0x59
 out (0x9D),a
 xor a
 out (0x9F),a
 ld a,#0x48
 out (0x9D),a
 ld a,#3
 out (0x9F),a
foot_project_wait:
 in a,(0x9D)
 and #1
 jr nz,foot_project_wait
 ; Project the boot with its owner body's matrix, preserving wheel rotation.
 ld hl,(PTR)
 ld de,#2024
 add hl,de
 xor a
 out (0x9D),a
 ld bc,#0x189F
 otir
 ld a,(COUNT)
 ld e,a
 ld d,#0
 ld hl,#REC+1700
 or a
 sbc hl,de
 ld a,(hl)
 ld (0xE44C),a
 and #0x7F
 ld (0x6000),a
 ld hl,#0x5E00
 ld a,(0xE44C)
 bit 7,a
 jr z,foot_vertex_ready
 ld l,#0x80
 push hl
 inc hl
 inc hl
 call patch_exact_boot_faces
 pop hl
foot_vertex_ready:
 ld a,#0x54
 out (0x9D),a
 ld b,#2
 ld c,#0x9F
 otir
 ld a,#0x42
 out (0x9D),a
 ld a,#8
 out (0x9F),a
 xor a
 out (0x9F),a
 ld a,#0x59
 out (0x9D),a
 ld a,#108
 out (0x9F),a
 ld a,#0x48
 out (0x9D),a
 ld a,#3
 out (0x9F),a
foot_render_wait:
 call sample_action_keys
 in a,(0x9D)
 and #1
 jr nz,foot_render_wait
 ld a,(0xE44C)
 bit 7,a
 jp z,next_object
 ld hl,#0x5E02
 call patch_exact_boot_faces
 jp next_object


patch_exact_boot_faces:
 push hl
 ld a,#0x6C
 out (0x9D),a
 ld a,#(BOOT_FACE_FIRST / 256)
 out (0x9F),a
 ld a,#0x58
 out (0x9D),a
 ld a,#(BOOT_FACE_FIRST % 256)
 out (0x9F),a
 ld a,#0x52
 out (0x9D),a
 pop hl
 ld bc,#0x489F
 otir
 ret

resident_models:
 .db 1,55,62,2,22,23,8,9,10,11,12,13,14,15,18,19,20,21,255

.org 0x5000
.include "palette.inc"

