org 0x100
bits 16

; Minimal harness exposing check_vertical_enemy_map_collision and
; check_horizontal_enemy_map_collision for unit tests.

; Configuration bytes (searchable markers for patching):
mode_marker: db 'MD'   ; 0 = vertical, 1 = horizontal, 2 = enemy_leap, 3 = fireball
mode: db 0
INX_MARK: db 'INX'     ; input x (legacy collision use)
in_x: db 0
INY_MARK: db 'INY'     ; input y (legacy collision use)
in_y: db 0

; Enemy markers
ENX_MARK: db 'ENX'
en_x: db 0
ENY_MARK: db 'ENY'
en_y: db 0
ENBH_MARK: db 'EBH'
en_beh: db 0
PLX_MARK: db 'PLX'
pl_x: db 0
PLY_MARK: db 'PLY'
pl_y: db 0

; Fireball markers
FBX_MARK: db 'FBX'
fb_x: db 0
FBY_MARK: db 'FBY'
fb_y: db 0
FBV_MARK: db 'FBV'
fb_vel: db 0

tileset_last_passable: db 0

; Reserve tilemap (128 * 10) bytes; default 0 (passable)
; We'll keep a reasonably sized map in the .com; 1280 bytes.
MAP_MARK: db 'MAP'
map_tiles:
    times (128*10) db 0

; probe markers for debugging
PX_MARK: db 'PX'
probe_index: dw 0
probe_val: db 0

; result byte (0/1)
result_mark: db 'RS'
result: db 0

; Enemy result block: RNE <x_vel> <y_vel>
en_res_mark: db 'RNE'
en_res_xvel: db 0
en_res_yvel: db 0

; Fireball result block: RNF <x> <y> <active>
fb_res_mark: db 'RNF'
fb_res_x: db 0
fb_res_y: db 0
fb_res_active: db 0

; Helper macros/constants
%define MAP_WIDTH_TILES 128

section .text

ST_MARK: db 'ST'

start:
    ; mark that we entered start
    mov word [probe_index], 0x1234
    mov byte [probe_val], 0x12
    ; load inputs into registers and dispatch on mode
    mov ah, [in_x]
    mov al, [in_y]
    ; write AH/AL into debug slots map_tiles+1290/1291 for inspection
    mov si, map_tiles
    add si, 1290
    mov [si], ah
    mov si, map_tiles
    add si, 1291
    mov [si], al
    mov dl, [mode]
    cmp dl, 0
    je .do_vertical
    cmp dl, 1
    je .do_horizontal
    cmp dl, 2
    je .do_enemy_leap
    cmp dl, 3
    je .do_fireball
    cmp dl, 4
    je .do_probe_map
    jmp .finish_noop

.do_vertical:
    call check_vertical_enemy_map_collision
    jmp .write_collision_result

.do_horizontal:
    call check_horizontal_enemy_map_collision
    jmp .write_collision_result

.write_collision_result:
    jc .collision
    mov byte [result], 0
    jmp .finish
.collision:
    mov byte [result], 1
    jmp .finish

; mode 2: forced enemy leap (deterministic mini-AI for unit tests)
; Inputs: en_x, en_y, pl_x, pl_y, en_beh
; Outputs: en_res_xvel, en_res_yvel
; Writes to debug slots at map_tiles + 1283,1284 respectively to avoid label/address ambiguity
.do_enemy_leap:
    ; choose x velocity based on enemy.x vs player.x
    mov al, [en_x]
    mov bl, [pl_x]
    cmp al, bl
    jae .enemy_right
    ; write -1 into debug slot
    mov al, 0xFF
    mov si, map_tiles
    add si, 1283
    mov [si], al
    jmp .set_leap_y
.enemy_right:
    mov al, 1
    mov si, map_tiles
    add si, 1283
    mov [si], al
.set_leap_y:
    mov al, 0xF9   ; -7 as unsigned byte
    mov si, map_tiles
    add si, 1284
    mov [si], al
    jmp .finish

; mode 3: simplified fireball tick
; Inputs: fb_x, fb_y, fb_vel, en_x, en_y
; Behavior: fb_x += fb_vel; fb_y += (fb_vel >= 0 ? 1 : -1);
; If abs(fb_x - en_x) <= 1 && abs(fb_y - en_y) <= 1 then deactivate (active=0)
.do_fireball:
    mov al, [fb_x]
    mov bl, [fb_vel]
    add al, bl
    mov [fb_res_x], al
    mov al, [fb_y]
    cmp bl, 0
    jl .fb_move_up
    inc al
    jmp .fb_apply_y
.fb_move_up:
    dec al
.fb_apply_y:
    mov [fb_res_y], al
    ; check collision with enemy
    mov cl, [en_x]
    mov dl, [en_y]
    mov al, [fb_res_x]
    mov ah, [fb_res_y]
    ; compute abs(al - cl) into al
    sub al, cl
    jae .x_ok
    neg al
.x_ok:
    cmp al, 1
    ja .no_collision
    ; check Y
    sub ah, dl
    jae .y_ok
    neg ah
.y_ok:
    cmp ah, 1
    ja .no_collision
    ; collision! write active=0 to debug slot
    mov al, 0
    mov si, map_tiles
    add si, 1285
    mov [si], al
    jmp .finish
.no_collision:
    mov al, 1
    mov si, map_tiles
    add si, 1285
    mov [si], al
    jmp .finish

; mode 4: probe map tile at given coordinates; return its byte in 'result'
.do_probe_map:
    ; mark that probe entry was executed
    mov word [probe_index], 0xBEEF
    mov byte [probe_val], 0xAA
    ; inputs in AH (x) and AL (y)
    push ax
    push bx
    push cx
    push dx

    mov cx, ax    ; cl = y, ch = x
    mov bl, ah
    xor bh, bh    ; bx = x
    xor ah, ah    ; ax = y

    ; divide coords by 2
    shr bx, 1
    shr ax, 1

    ; multiply ax by 128 (map width)
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    add ax, bx

    mov bx, map_tiles
    ; store pointer read and memory at pointer for debugging
    mov ax, bx
    mov [probe_index], ax
    mov al, [bx]
    mov [probe_val], al

    add bx, ax

    mov al, [bx]
    ; store raw tile value to a debug slot immediately after the map region
    mov si, map_tiles
    add si, 1280
    mov [si], al
    mov [result], al

    pop dx
    pop cx
    pop bx
    pop ax
    jmp .finish

.finish_noop:
    nop
.finish:
    ; write results to file 'result.bin' so DOSBox runs can report it
    mov dx, result       ; buffer pointer
    mov cx, 1            ; size
    mov ah, 0x3C         ; create file
    mov dx, file_name
    int 0x21
    jc .no_file
    mov bx, ax           ; file handle
    mov ah, 0x40         ; write file
    mov dx, result
    mov cx, 1
    int 0x21
    mov ah, 0x3E         ; close file
    int 0x21
.no_file:
    mov ah, 0x4C
    mov al, 0
    int 0x21

file_name: db 'result.bin',0

;-----------------------------------------------------------------
; Collision routines (extracted and simplified from R3sw1989.asm)
;-----------------------------------------------------------------

; External data references used by the code:
; [current_tiles_ptr] -> word pointer to tile map
; [tileset_last_passable] -> byte threshold

current_tiles_ptr: dw map_tiles

; check_vertical_enemy_map_collision:
check_vertical_enemy_map_collision:
    push ax
    push bx
    push cx
    push dx

    mov cx, ax    ; cl = y, ch = x
    mov bl, ah
    xor bh, bh    ; bx = x
    xor ah, ah    ; ax = y

    ; divide coords by 2
    shr bx, 1
    shr ax, 1

    ; multiply ax by 128 (map width)
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    add ax, bx

    ; store computed index (AX) low/high bytes to debug slots after map
    mov si, map_tiles
    add si, 1281
    mov [si], al
    mov si, map_tiles
    add si, 1282
    mov [si], ah

    mov bx, map_tiles
    add bx, ax
    ; debug: store computed index and byte at that index
    mov ax, bx
    mov [probe_index], ax
    mov al, [bx]
    mov [probe_val], al
    ; expose the raw tile value into result for debugging
    mov [result], al

    mov ax, cx
    ; store raw tile value to a debug slot immediately after the map region
    mov si, map_tiles
    add si, 1280
    mov [si], al

    mov cl, [tileset_last_passable]
    cmp [bx], cl
    jg .return_collision_v
    test ah, 1
    jz .return_no_collision_v
    cmp [bx + 1], cl
    jg .return_collision_v
.return_no_collision_v:
    clc
    pop dx
    pop cx
    pop bx
    pop ax
    ret
.return_collision_v:
    stc
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; check_horizontal_enemy_map_collision:
check_horizontal_enemy_map_collision:
    push ax
    push bx
    push cx
    push dx

    mov cx, ax
    mov bl, ah
    xor bh, bh
    xor ah, ah

    shr bx, 1
    shr ax, 1

    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    add ax, bx

    mov bx, map_tiles
    add bx, ax
    ; debug: store computed index and byte at that index
    mov ax, bx
    mov [probe_index], ax
    mov al, [bx]
    mov [probe_val], al
    ; expose the raw tile value into result for debugging
    mov [result], al

    mov ax, cx
    ; store raw tile value to a debug slot immediately after the map region
    mov si, map_tiles
    add si, 1280
    mov [si], al

    mov cl, [tileset_last_passable]
    cmp [bx], cl
    jg .return_collision_h
    test al, 1
    jz .return_no_collision_h
    cmp [bx + MAP_WIDTH_TILES], cl
    jg .return_collision_h
.return_no_collision_h:
    clc
    pop dx
    pop cx
    pop bx
    pop ax
    ret
.return_collision_h:
    stc
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; make sure there's at least a small instruction at end
nop
