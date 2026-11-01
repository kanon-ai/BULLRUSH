set auto_save_settings false
set renderer SDLGL-PP
set throttle off
set speed 100
set sound_driver null
set power on
set pause off
set log [open out/smoke.txt w]
proc snap {name} {
 global log
 puts $log "AF=[reg AF] BC=[reg BC] DE=[reg DE] HL=[reg HL] SP=[reg SP] stk=[debug read memory [reg SP]]/[debug read memory [expr {[reg SP]+1}]]/[debug read memory [expr {[reg SP]+2}]]/[debug read memory [expr {[reg SP]+3}]] table=[debug read memory 0xECB2]/[debug read memory 0xECB3]"
 puts $log "$name time=[machine_info time] PC=[reg PC] state=[debug read memory 0xD800] hp=[debug read memory 0xD801] tick=[debug read memory 0xD810] dt=[debug read memory 0xD812] objects=[debug read memory 0xD815]"
 flush $log
 screenshot -raw "out/$name.png"
}
after time 6 {snap title}
after time 7 {keymatrixdown 7 128}
after time 7.5 {keymatrixup 7 128;keymatrixdown 8 32;keymatrixdown 6 1}
after time 11 {snap dash;keymatrixdown 8 128}
after time 13 {keymatrixup 8 128;snap turn}
after time 15 {keymatrixup 8 32;keymatrixup 6 1;keymatrixdown 3 4}
after time 17 {keymatrixup 3 4;keymatrixdown 3 1}
after time 17.5 {keymatrixup 3 1}
after time 19 {snap cockpit;close $log;exit}
after realtime 60 {catch {close $log};exit}
