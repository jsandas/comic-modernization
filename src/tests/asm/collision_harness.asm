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

; Dedicated debug area (non-overlapping named slots)
DBG_MARK: db 'DBG'
dbg_entry: db 0          ; handler entry marker
dbg_left: db 0           ; left tile
dbg_right: db 0          ; right tile
dbg_top: db 0            ; top tile
dbg_bottom: db 0         ; bottom tile
dbg_tile_val: db 0       ; last tile value read
dbg_threshold: db 0      ; threshold used for comparisons
dbg_cur_tx: db 0         ; current tx being checked
dbg_cur_ty: db 0         ; current ty being checked
dbg_collision_marker: db 0
dbg_row_marker: db 0
dbg_flags_lo: db 0       ; AL
dbg_flags_hi: db 0       ; AH
dbg_fire_active: db 0    ; fireball active result

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
    ; write AH/AL into debug flags for inspection
    mov [dbg_flags_hi], ah
    mov [dbg_flags_lo], al
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
    ; debug: mark entry to vertical handler (named slot)
    mov byte [dbg_entry], 0xBB
    mov byte [dbg_row_marker], 0xF0
    call check_vertical_enemy_map_collision
    mov byte [dbg_row_marker], 0xF9
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
    ; collision! write active=0 to debug slot (named)
    mov al, 0
    mov [dbg_fire_active], al
    jmp .finish
.no_collision:
    mov al, 1
    mov [dbg_fire_active], al
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
    mov [dbg_tile_val], al
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
    ; preserve registers
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp

    ; debug: mark entry (internal)
    mov byte [dbg_entry], 0x12
    ; trace: write probe_val = 0x12 to make entry visible to runner hook
    mov al, 0x12
    mov [probe_val], al

    ; Read inputs (AH=in_x, AL=in_y)
    ; compute tile_x = AH >> 1 into BL
    mov al, ah
    shr al, 1
    mov bl, al
    xor bh, bh
    ; compute tile_y = AL >> 1 into CL
    mov al, al
    mov al, [in_y]
    shr al, 1
    mov cl, al
    ; marker: tile_y computed
    mov byte [dbg_entry], 0x22
    mov al, 0x22
    mov [probe_val], al

    ; compute span bits
    mov al, ah
    and al, 1
    mov dl, al     ; DL = span_x
    mov al, [in_y]
    and al, 1
    mov dh, al     ; DH = span_y
    ; marker: span bits computed
    mov byte [dbg_entry], 0x33
    mov al, 0x33
    mov [probe_val], al

    ; prepare row counter BP = tile_y
    mov byte [dbg_row_marker], 0x21
    mov ax, cx
    mov bp, ax
    ; prepare left tile in DI = BX
    mov di, bx
    ; compute right = left + span_x into BX
    mov si, bx
    mov al, dl
    cbw
    add si, ax
    mov bx, si
    ; compute bottom = top + span_y into CX
    mov si, bp
    mov al, dh
    cbw
    add si, ax
    mov cx, si

    mov byte [dbg_row_marker], 0xEE
    mov ax, di
    mov [dbg_left], al
    mov [dbg_right], bl
    mov ax, bp
    mov [dbg_top], al
    mov [dbg_bottom], cl

    ; rows: ty from BP (saved tile_y) to CX (bottom)
    mov ax, bp
    mov bp, ax      ; BP = ty
.v_row_loop2:
    mov byte [dbg_row_marker], 0x31
    ; row_base = bp * 128
    mov ax, bp
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    mov dx, ax      ; DX = row_base

    ; columns: tx from left (load from debug left slot) to right (BX)
    mov si, map_tiles
    add si, 1290
    mov al, [si]
    mov ah, 0
    mov di, ax
    mov byte [dbg_row_marker], 0x41
.v_col_loop2:
    mov ax, dx
    add ax, di      ; AX = index
    mov si, map_tiles
    add si, ax
    mov al, [si]
    ; save tile value into DL
    mov dl, al
    ; debug: tile value
    mov [dbg_tile_val], dl
    ; threshold into CL and debug
    mov cl, [tileset_last_passable]
    mov [dbg_threshold], cl
    ; cur tx low -> dbg_cur_tx
    mov ax, di
    mov [dbg_cur_tx], al
    ; cur ty low -> dbg_cur_ty
    mov ax, bp
    mov [dbg_cur_ty], al
    ; compare tile (DL) vs threshold (CL)
    mov byte [dbg_collision_marker], 0x10
    mov al, 0x44
    mov [probe_val], al
    cmp dl, cl
    ja .v_collision_found2
    inc di
    mov ax, di
    cmp ax, bx
    jle .v_col_loop2
    ; next row
    inc bp
    mov ax, bp
    cmp ax, cx
    jle .v_row_loop2
    ; no collision -> named marker
    mov byte [dbg_collision_marker], 0x55
    clc
    jmp .v_cleanup

.v_collision_found2:
    mov byte [dbg_collision_marker], 0x99
    stc
    jmp .v_cleanup

.v_cleanup:
    ; mark that cleanup was reached
    mov byte [dbg_entry], 0x77
    mov al, 0x77
    mov [probe_val], al
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret


.v_ret:
    ; return preserving CF (set by stc/clc above)
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
    ; marker: horizontal entry
    mov byte [dbg_entry], 0x21
    ; trace: mark probe_val for horizontal entry
    mov al, 0x21
    mov [probe_val], al

    mov cx, ax    ; CX = original AX (CH = orig in_x, CL = orig in_y)
    mov bl, ah
    xor bh, bh
    xor ah, ah

    ; compute tile coords (tile_x = x>>1, tile_y = y>>1)
    shr bx, 1      ; BX = tile_x
    shr ax, 1      ; AX = tile_y

    ; compute right and bottom bounds based on span bits
    mov si, bx     ; SI = left_tile
    mov di, ax     ; DI = top_tile
    test ch, 1
    jz .h_no_right
    inc si
.h_no_right:
    test cl, 1
    jz .h_no_bottom
    inc di
.h_no_bottom:

    ; iterate rows from tile_y (AX) to DI (bottom_tile)
    mov cx, ax
.h_row_loop:
    mov byte [dbg_row_marker], 0x51
    ; compute base = cx * 128
    mov dx, cx
    shl dx, 1
    shl dx, 1
    shl dx, 1
    shl dx, 1
    shl dx, 1
    shl dx, 1
    shl dx, 1

    ; loop columns tx from left (in BX) to SI (right_tile)
    mov bp, bx
    mov byte [dbg_row_marker], 0x61
.h_col_loop:
    mov ax, dx
    add ax, bp
    mov bx, map_tiles
    add bx, ax
    mov byte [dbg_collision_marker], 0x20
    mov al, [bx]
    mov [dbg_tile_val], al
    ; mark compare point in horizontal
    mov al, 0x44
    mov [probe_val], al
    mov cl, [tileset_last_passable]
    cmp al, cl
    jg .h_collision_found
    inc bp
    cmp bp, si
    jle .h_col_loop

    inc cx
    cmp cx, di
    jle .h_row_loop

    ; no collision found: write debug marker 0x55 (named)
    mov byte [dbg_collision_marker], 0x55
    clc
    jmp .h_ret

.h_collision_found:
    ; collision found: write debug marker 0x99 (named)
    mov byte [dbg_collision_marker], 0x99
    stc
    jmp .h_ret

.h_ret:
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; make sure there's at least a small instruction at end
nop
