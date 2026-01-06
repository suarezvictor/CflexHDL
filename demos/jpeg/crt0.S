// (C) 2023 Victor Suarez Rovere <suarezvictor@gmail.com>
// SPDX-License-Identifier: BSD-2-Clause
//
// (C) Litex and Misoc developers
// see LITEX_CONTRIBUTORS file for additional authors

//the fastcode_init code could be implemented in the main.c file
.global main
.global isr_handler
.global _start

_start:
  j crt_init
  nop
  nop
  nop
  nop
  nop
  nop
  nop

.global  trap_entry
trap_entry:
  sw x1,  - 1*4(sp)
  sw x5,  - 2*4(sp)
  sw x6,  - 3*4(sp)
  sw x7,  - 4*4(sp)
  sw x10, - 5*4(sp)
  sw x11, - 6*4(sp)
  sw x12, - 7*4(sp)
  sw x13, - 8*4(sp)
  sw x14, - 9*4(sp)
  sw x15, -10*4(sp)
  sw x16, -11*4(sp)
  sw x17, -12*4(sp)
  sw x28, -13*4(sp)
  sw x29, -14*4(sp)
  sw x30, -15*4(sp)
  sw x31, -16*4(sp)
  addi sp,sp,-16*4
  call isr_handler
  lw x1 , 15*4(sp)
  lw x5,  14*4(sp)
  lw x6,  13*4(sp)
  lw x7,  12*4(sp)
  lw x10, 11*4(sp)
  lw x11, 10*4(sp)
  lw x12,  9*4(sp)
  lw x13,  8*4(sp)
  lw x14,  7*4(sp)
  lw x15,  6*4(sp)
  lw x16,  5*4(sp)
  lw x17,  4*4(sp)
  lw x28,  3*4(sp)
  lw x29,  2*4(sp)
  lw x30,  1*4(sp)
  lw x31,  0*4(sp)
  addi sp,sp,16*4
  mret
  .text


crt_init:
  la sp, _fstack
  la a0, trap_entry
  csrw mtvec, a0

data_init:
  la a0, _fast_data
  la a1, _efast_data
  la a2, _fast_data_loadaddr
data_loop:
  beq a0,a1,data_done
  lw a3,0(a2)
  sw a3,0(a0)
  add a0,a0,4
  add a2,a2,4
  j data_loop
data_done:

bss_init:
  la a0, _fbss
  la a1, _ebss
bss_loop:
  beq a0,a1,bss_done
  sw zero,0(a0)
  add a0,a0,4
  j bss_loop
bss_done:

fastcode_init:
  la a0, _fast_text
  la a1, _efast_text
  la a2, _fast_text_loadaddr
fastcode_loop:
  beq a0,a1,fastcode_done
  lw a3,0(a2)
  sw a3,0(a0)
  add a0,a0,4
  add a2,a2,4
  j fastcode_loop
fastcode_done:

  li a0, 0x880  //880 enable timer + external interrupt sources (until mstatus.MIE is set, they will never trigger an interrupt)
  csrw mie,a0

  call main
infinit_loop:
  j infinit_loop
  
  

