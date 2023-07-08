

; Extra New Lines are for checking whether
; parser is doing it's job or not, LMAO ASS, funi wor



CLS ; <----- NO BOOM BOOM, NO ERROR
JP 0x2fe;lmao comments without spaces work toooo omaigoddd
SE   V2 0xFe
SNE  V4 0x6F
SE   V2 V4
CALL 0x69E
LD   VE 0x83
LD   VE V1
ADD  V2 V8
ADD  V2 0x69
OR   V2 VF
AND  V4 V1
XOR  V3 V6
SUB  V7 V9
SHR  V7 V2
SUBN V3 V4
SHL  V2 V7
SNE  V1 V2
LDI  0x2FF
JP0  0x69
RND  V8 33
SKP  V6
SKNP V3
DTLD V2 ; Set V2 = delay timer value.
LDK  V0 ; Wait for a key press, store the value of the key in V0.
LDDT V1 ; Set delay timer = V1.
LDST V1 ; Set sound timer = V1.
ADDI V0 ; Set I = I + V0.
LDF  V3 ; Set I = location of sprite for digit Vx.
LDB  V8 ; Store BCD representation of Vx in memory locations I, I+1, and I+2.
LD[I] V7 ; Store registers V0 through Vx in memory starting at location I.
[I]LD V3 ; Read registers V0 through Vx from memory starting at location I.

