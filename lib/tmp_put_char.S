TI_GDT = 0
RPL0 = 0
SELECTOR_VIDEO = (0x0006 << 3) + TI_GDT + RPL0

.global put_char
put_char:
    push %eax
    push %ecx
    push %edx
    push %ebx
    push %esi
    push %edi
    push %esp
    push %ebp

    movw $SELECTOR_VIDEO, %ax       
    movw %ax, %gs
    movw $0x03d4, %dx               
    movb $0x0e, %al                 
    outb %al, %dx
    movw $0x03d5, %dx              
    inb %dx, %al                    
    movb %al, %ah
    movw $0x03d4, %dx
    movb $0x0f, %al
    outb %al, %dx
    movw $0x03d5, %dx 
    inb %dx, %al
    movw %ax, %bx
    movl 36(%esp), %ecx            
    cmpb $0x0d, %cl                 
    jz .is_carriage_return
    cmpb $0x0a, %cl
    jz .is_line_feed
    cmpb $0x08, %cl                
    jz .is_backspace
    jmp .put_other
 .is_backspace:
    dec %bx
    shl $1, %bx
    movb $0x20, %gs:(%bx)         
    inc %bx
    movb $0x07, %gs:(%bx)
    shr $1, %bx
    jmp .set_cursor
.put_other:
    shl $1, %bx                   
    movb %cl, %gs:(%bx)           
    inc %bx
    movb $0x07, %gs:(%bx)         
    shr $1, %bx                   
    inc %bx                       
    cmp $2000, %bx
    jl .set_cursor               
.is_line_feed:                    
.is_carriage_return:               
    xor %dx, %dx                   
    mov %bx, %ax                   
    mov $80, %si                   
    div %si                        
    sub %dx, %bx                   
.is_carriage_return_end:          
    add $80, %bx
    cmp $2000, %bx
.is_line_feed_end:                
    jl .set_cursor
.roll_screen:
    cld
    movl $960, %ecx
    movl $0xc00b80a0, %esi
    movl $0xc00b8000, %edi
    rep movsl
    movl $3840, %ebx
    movl $80, %ecx
.cls:
    movw $0x0720, %gs:(%ebx)
    add $2, %ebx
    loop .cls
    movw $1920, %bx

.set_cursor:
    movw $0x03d4, %dx
    movb $0x0e, %al
    outb %al, %dx
    movw $0x03d5, %dx
    movb %bh, %al
    outb %al, %dx

    movw $0x03d4, %dx
    movb $0x0f, %al
    outb %al, %dx
    movw $0x03d5, %dx
    movb %bl, %al
    outb %al, %dx
.put_char_done:
    pop %ebp
    pop %esp
    pop %edi
    pop %esi
    pop %ebx
    pop %edx
    pop %ecx
    pop %eax
    ret
