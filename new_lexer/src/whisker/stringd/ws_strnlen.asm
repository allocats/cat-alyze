global ws_strnlen

%define PAGE_SIZE 4096
%define VEC_SIZE 128
%define CHUNK_SIZE 32
%define CHUNK(x) CHUNK_SIZE * x
 
section .text
ws_strnlen:
    push r12

    xor rax, rax

    test rdi, rdi
    jz .done

    test rsi, rsi
    jz .done

    cmp rsi, VEC_SIZE
    jb .scalar_loop

    vpxor ymm0, ymm0

.align_loop:
    cmp byte [rdi], 0
    je .done

    test rdi, 31
    jz .check_page

    inc rax
    inc rdi
    jmp .align_loop

.scalar_loop:
    cmp rax, rsi
    jge .reached_max

    cmp byte [rdi], 0 
    je .done 

    inc rax 
    inc rdi 
    jmp .scalar_loop

.reached_max:
    mov rax, rsi
    jmp .done

.check_page:
    mov rdx, rdi 
    and rdx, PAGE_SIZE - 1
    cmp rdx, PAGE_SIZE - VEC_SIZE 
    ja .scalar_loop

align 32
.avx2_loop:
    lea rcx, [rax + VEC_SIZE]
    cmp rcx, rsi
    ja .scalar_loop 

    vmovdqa ymm1, [rdi]
    vpminub ymm5, ymm1, [rdi + CHUNK(1)]

    vmovdqa ymm3, [rdi + CHUNK(2)]
    vpminub ymm6, ymm3, [rdi + CHUNK(3)]

    vpminub ymm7, ymm5, ymm6

    vpcmpeqb ymm7, ymm7, ymm0
    vptest ymm7, ymm7 
    jnz .found_null

    add rax, VEC_SIZE
    add rdi, VEC_SIZE
    jmp .avx2_loop

.found_null:
    vpcmpeqb ymm1, ymm1, ymm0
    vpmovmskb r8d, ymm1

    vpcmpeqb ymm2, ymm0, [rdi + CHUNK(1)]
    vpmovmskb r9d, ymm2

    vpcmpeqb ymm3, ymm3, ymm0
    vpmovmskb r10d, ymm3

    vpcmpeqb ymm4, ymm0, [rdi + CHUNK(3)]
    vpmovmskb r11d, ymm4

    mov edx, r8d
    xor rcx, rcx

    mov r12d, CHUNK(1)
    test edx, edx
    cmovz edx, r9d
    cmovz ecx, r12d

    mov r12d, CHUNK(2)
    test edx, edx
    cmovz edx, r10d
    cmovz ecx, r12d

    mov r12d, CHUNK(3)
    test edx, edx
    cmovz edx, r11d
    cmovz ecx, r12d

    tzcnt edx, edx

    add rax, rdx
    add rax, rcx

.done:
    vzeroupper
    pop r12
    ret
