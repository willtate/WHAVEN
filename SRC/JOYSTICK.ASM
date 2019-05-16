.386p
.model small

 MaxJoyValue = 5000

.DATA

_Joy_xb db ?
_Joy_yb db ?
_Joy_xs db ?
_Joy_ys db ?
_Joy_x  dw ?
_Joy_y  dw ?

PUBLIC _Joy_xb
PUBLIC _Joy_yb
PUBLIC _Joy_xs
PUBLIC _Joy_ys
PUBLIC _Joy_x
PUBLIC _Joy_y

.CODE

ALIGN  4

;=================
;
; JoyStick_Vals
;
;=================

PROC   JoyStick_Vals_
PUBLIC JoyStick_Vals_


; Read the absolute joystick values

   pushf                ; Save some registers
   push  ebp
   cli                  ; Make sure an interrupt doesn't screw the timings

   mov   dx, 0201h
   in    al, dx
   out   dx, al         ; Clear the resistors

   mov   ah, BYTE PTR [_Joy_xb]   ; Get masks into registers
   mov   ch, BYTE PTR [_Joy_yb]

   xor   si, si         ; Clear count registers
   xor   di, di
   xor   bh, bh         ; Clear high byte of bx for later

   mov   ebp, MaxJoyValue

   @@LOOP:
   in    al, dx         ; Get bits indicating whether all are finished

   dec   ebp             ; Check bounding register
   jz    done           ; We have a silly value - abort

   mov   bl, al         ; Duplicate the bits
   and   bl, ah         ; Mask off useless bits (in [xb])
   add   si, bx         ; Possibly increment count register
   mov   cl, bl         ; Save for testing later

   mov   bl, al
   and   bl, ch         ; [yb]
   add   di, bx

   add   cl, bl
   jnz   @@LOOP         ; If both bits were 0, drop out

   done:

   mov   cl, [_Joy_xs]  ; Get the number of bits to shift
   shr   si, cl         ;  and shift the count that many times

   mov   cl, [_Joy_ys]
   shr   di, cl

   mov   [_Joy_x], si   ; Store the values into the variables
   mov   [_Joy_y], di

   pop   ebp
   popf                 ; Restore the registers

   ret

   ENDP  JoyStick_Vals_

   END

