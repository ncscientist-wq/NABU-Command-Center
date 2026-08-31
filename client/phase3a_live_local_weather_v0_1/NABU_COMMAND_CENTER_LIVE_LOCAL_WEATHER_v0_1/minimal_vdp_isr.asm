SECTION code_user

PUBLIC _ncc_install_minimal_vdp_isr
PUBLIC ncc_minimal_vdp_isr

EXTERN _clock_frame_counter
EXTERN __tms9918_status_register

; Installed z88dk places NABU's eight two-byte IM2 handlers at I:0000.
; Vector 6 is the VDP handler, hence table offset 6.
_ncc_install_minimal_vdp_isr:
    di
    ld a,i
    ld h,a
    ld l,6
    ld de,ncc_minimal_vdp_isr
    ld (hl),e
    inc hl
    ld (hl),d
    ei
    ret

; Direct bounded frame ISR: acknowledge VDP status, retain the installed
; library status shadow, count one frame, return. No callback dispatcher.
ncc_minimal_vdp_isr:
    di
    push af
    in a,($A1)
    ld (__tms9918_status_register),a
    ld a,(_clock_frame_counter)
    inc a
    ld (_clock_frame_counter),a
    pop af
    ei
    ret
