; TH05 MAIN BFNT workspace fallback in the existing patch-owned tail.
.386
.model use16 large _TEXT
include ReC98.inc
; BINARY intentionally undefined: original MAIN owns _address_0.
extrn HMEM_ALLOC:far
extrn HMEM_FREE:far
extrn SUPER_ENTRY_PAT:far
extrn SMEM_WGET:far
extrn SMEM_RELEASE:far
extrn mem_TopSeg:word
extrn mem_EndMark:word
extrn mem_TopHeap:word
public REPLAY_BFNT_ALLOC_FIRST, REPLAY_BFNT_ALLOC_SECOND, REPLAY_BFNT_FREE_BOTH
public REPLAY_BFNT_FREE_ONE, REPLAY_BFNT_HEAP_MODE
public REPLAY_BFNT_CLEANUP_DISPATCH
extrn REPLAY_BFNT_INVALID_CLEANUP_RETURN:near
PATCH_ALIGN_TEXT segment para public 'CODE' use16
    assume CS:PATCH_ALIGN_TEXT,DS:nothing
REPLAY_BFNT_PATCH_START label byte
; TH05 MAIN BFNT conversion workspaces can use released heap holes.
; These helpers own only the two temporary conversion buffers.
REPLAY_BFNT_HEAP_MODE db 1

REPLAY_BFNT_ALLOC_FIRST proc far
    push bp
    mov bp,sp
    push bx
    mov bx,ss:[bp+6]
    add bx,15
    rcr bx,1
    shr bx,1
    shr bx,1
    shr bx,1
    cmp byte ptr cs:REPLAY_BFNT_HEAP_MODE,2
    je short @@heap
    mov byte ptr cs:REPLAY_BFNT_HEAP_MODE,1
    test bx,bx
    jz short @@heap
    cmp word ptr ds:mem_TopSeg,0
    je short @@heap
    mov ax,bx
    shl ax,1
    add ax,word ptr ds:mem_EndMark
    jc short @@heap
    cmp word ptr ds:mem_TopHeap,ax
    jb short @@heap
    mov byte ptr cs:REPLAY_BFNT_HEAP_MODE,0
    push word ptr ss:[bp+6]
    call far ptr SMEM_WGET
    jmp short @@done
@@heap:
    mov byte ptr cs:REPLAY_BFNT_HEAP_MODE,1
    push bx
    call far ptr HMEM_ALLOC
    jnc short @@done
    mov ax,InsufficientMemory
@@done:
    pop bx
    pop bp
    retf 2
REPLAY_BFNT_ALLOC_FIRST endp

REPLAY_BFNT_ALLOC_SECOND proc far
    push bp
    mov bp,sp
    cmp byte ptr cs:REPLAY_BFNT_HEAP_MODE,0
    jne short @@heap
    push word ptr ss:[bp+6]
    call far ptr SMEM_WGET
    jnc short @@done
    push ax
    push cx
    call far ptr SMEM_RELEASE
    pop ax
    stc
    jmp short @@done
@@heap:
    mov byte ptr cs:REPLAY_BFNT_HEAP_MODE,2
    push word ptr ss:[bp+6]
    call far ptr REPLAY_BFNT_ALLOC_FIRST
    jnc short @@done
    push ax
    push cx
    call far ptr HMEM_FREE
    pop ax
    stc
@@done:
    pop bp
    retf 2
REPLAY_BFNT_ALLOC_SECOND endp

REPLAY_BFNT_FREE_ONE proc far
    cmp byte ptr cs:REPLAY_BFNT_HEAP_MODE,0
    jne short @@heap
    jmp far ptr SMEM_RELEASE
@@heap:
    jmp far ptr HMEM_FREE
REPLAY_BFNT_FREE_ONE endp

REPLAY_BFNT_FREE_BOTH proc far
    push bp
    mov bp,sp
    push es
    call far ptr REPLAY_BFNT_FREE_ONE
    push word ptr ss:[bp+6]
    call far ptr REPLAY_BFNT_FREE_ONE
    pop bp
    stc
    retf 2
REPLAY_BFNT_FREE_BOTH endp

; Reached only by the original seven same-segment call sites.
REPLAY_BFNT_CLEANUP_DISPATCH proc far
    push bp
    mov bp,sp
    cmp word ptr ss:[bp+2],offset REPLAY_BFNT_INVALID_CLEANUP_RETURN
    pop bp
    je short REPLAY_BFNT_FREE_BOTH
    jmp short REPLAY_BFNT_FREE_ONE
REPLAY_BFNT_CLEANUP_DISPATCH endp

; Existing contribution is29B and this contribution starts at next paragraph.
; 13 modulo16 restores the original CRT paragraph phase D deterministically.
db ((13 - (($ - REPLAY_BFNT_PATCH_START) MOD 16) + 16) MOD 16) dup (90h)
PATCH_ALIGN_TEXT ends
end
