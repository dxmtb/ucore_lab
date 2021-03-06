#include <mmu.h>
#include <memlayout.h>

# Each non-boot CPU ("AP") is started up in response to a STARTUP
# IPI from the boot CPU.  Section B.4.2 of the Multi-Processor
# Specification says that the AP will start in real mode with CS:IP
# set to XY00:0000, where XY is an 8-bit value sent with the
# STARTUP. Thus this code must start at a 4096-byte boundary.
#
# Because this code sets DS to zero, it must sit
# at an address in the low 2^16 bytes.
#
# Startothers (in main.c) sends the STARTUPs one at a time.
# It copies this code (start) at 0x7000.  It puts the address of
# a newly allocated per-core stack in start-4,the address of the
# place to jump to (mpenter) in start-8, and the physical address
# of entrypgdir in start-12.
#
# This code is identical to bootasm.S except:
#   - it does not need to enable A20
#   - it uses the address at start-4, start-8, and start-12

.set PROT_MODE_CSEG,        0x8                     # kernel code segment selector
.set PROT_MODE_DSEG,        0x10                    # kernel data segment selector
.set PROT_MODE_CSEG2,        0x18                     # kernel code segment selector2
.set CR0_PE_ON,             0x1                     # protected mode enable flag

.code16
.globl start
start:
  cli

  xorw    %ax,%ax
  movw    %ax,%ds
  movw    %ax,%es
  movw    %ax,%ss

  lgdt    gdtdesc
  movl    %cr0, %eax
  orl     $CR0_PE_ON, %eax
  movl    %eax, %cr0

  ljmp $PROT_MODE_CSEG2, $start32

.code32
start32:

  movw    $PROT_MODE_DSEG, %ax
  movw    %ax, %ds
  movw    %ax, %es
  movw    %ax, %ss
  movw    %ax, %fs
  movw    %ax, %gs

  movl (start-4+KERNBASE), %esp
  call modify

  ljmp $PROT_MODE_CSEG, $0x0 #addr to be filled by modify

  movw    $0x8a00, %ax
  movw    %ax, %dx
  outw    %ax, %dx
  movw    $0x8ae0, %ax
  outw    %ax, %dx
spin:
  jmp     spin

modify:
  movl (%esp), %eax
  movl (start-8+KERNBASE), %ebx
  movl %ebx, (0x1+KERNBASE)(%eax)
  ret

.p2align 2
gdt:
    SEG_NULL                                     # null seg
    SEG_ASM(STA_X | STA_R, - KERNBASE, 0xFFFFFFFF)      # code segment
    SEG_ASM(STA_W, - KERNBASE, 0xFFFFFFFF)              # data segment
    SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)           # code seg for bootloader and kernel
gdtdesc:
  .word   (gdtdesc - gdt - 1)
  .long   gdt

