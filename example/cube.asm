; Modified Version Of https://github.com/wernsey/chip8/blob/master/examples/syntax.asm

; Clear the screen
CLS
	
; Load the variables' initial values
LD  V0 1
LD  V4 1
LD  V1 10
LD  I sprite1
DRW V0 V1 8
LD  Ve 1  

; The main loop
loop:
	; Store the current position 
LD V2 V0
LD V3 V1
	
	; If the X direction is 0 go to sub1...
SE V4 0
JP sub1
	
	; ...otherwise add 1 to the box' position 
ADD V0 1

	; If you reached the right edge of the screen change direction
	SNE V0 56
LD  V4 1
jp draw1
sub1: 
	; subtract 1 from the box' position
SUB V0 Ve
	
	; If you reached the left edge of the screen change direction
SNE V0 0
LD  V4 0 

; Draw the box
draw1:  
	; Load the address of the sprite's graphics into register I
LD  I sprite1
	; Erase the sprite at the old position
DRW V2 V3 8
	; Draw the sprite at the new position
DRW V0 V1 8
	; Return to the start of the loop
	JP  loop

; Binary data of the sprite.
; 1s represent pixels to be drawn 0s are blank pixels.
sprite1:
db  %01111110,
	%10000001,
	%10100101,
	%10111101,
	%10111101,
	%10011001,
	%10000001,
	%01111110,
