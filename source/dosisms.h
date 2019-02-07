/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef DOSISMS_H
#define DOSISMS_H

//
// dosisms.h: I'd call it dos.h, but the name's taken
//

int dos_lockmem(void *addr, int size);
int dos_unlockmem(void *addr, int size);

typedef union {
    struct {
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t res;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
    } d;
    struct {
	uint16_t di, di_hi;
	uint16_t si, si_hi;
	uint16_t bp, bp_hi;
	uint16_t res, res_hi;
	uint16_t bx, bx_hi;
	uint16_t dx, dx_hi;
	uint16_t cx, cx_hi;
	uint16_t ax, ax_hi;
	uint16_t flags;
	uint16_t es;
	uint16_t ds;
	uint16_t fs;
	uint16_t gs;
	uint16_t ip;
	uint16_t cs;
	uint16_t sp;
	uint16_t ss;
    } x;
    struct {
	uint8_t edi[4];
	uint8_t esi[4];
	uint8_t ebp[4];
	uint8_t res[4];
	uint8_t bl, bh, ebx_b2, ebx_b3;
	uint8_t dl, dh, edx_b2, edx_b3;
	uint8_t cl, ch, ecx_b2, ecx_b3;
	uint8_t al, ah, eax_b2, eax_b3;
    } h;
} regs_t;

unsigned int ptr2real(void *ptr);
void *real2ptr(unsigned int real);
void *far2ptr(unsigned int farptr);
unsigned int ptr2far(void *ptr);

int dos_inportb(int port);
int dos_inportw(int port);
void dos_outportb(int port, int val);
void dos_outportw(int port, int val);

void dos_irqenable(void);
void dos_irqdisable(void);
void dos_registerintr(int intr, void (*handler) (void));
void dos_restoreintr(int intr);

int dos_int86(int vec);

void *dos_getmemory(int size);
void dos_freememory(void *ptr);

void dos_usleep(int usecs);

int dos_getheapsize(void);

extern regs_t regs;

#endif /* DOSISMS_H */
