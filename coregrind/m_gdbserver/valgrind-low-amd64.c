/* Low level interface to valgrind, for the remote server for GDB integrated
   in valgrind.
   Copyright (C) 2011
   Free Software Foundation, Inc.

   This file is part of VALGRIND.
   It has been inspired from a file from gdbserver in gdb 6.6.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

#include "server.h"
#include "target.h"
#include "regdef.h"
#include "regcache.h"

#include "pub_core_machine.h"
#include "pub_core_threadstate.h"
#include "pub_core_transtab.h"
#include "pub_core_gdbserver.h" 

#include "valgrind_low.h"

#include "libvex_guest_amd64.h"
/* GDBTD: ??? have a cleaner way to get the f80 <> f64 conversion functions */
/* below include needed for conversion f80 <> f64 */
#include "../../VEX/priv/guest_generic_x87.h" 

/* below loosely inspired from file generated with gdb regdat.sh */

static struct reg regs[] = {
  { "rax", 0, 64 },
  { "rbx", 64, 64 },
  { "rcx", 128, 64 },
  { "rdx", 192, 64 },
  { "rsi", 256, 64 },
  { "rdi", 320, 64 },
  { "rbp", 384, 64 },
  { "rsp", 448, 64 },
  { "r8", 512, 64 },
  { "r9", 576, 64 },
  { "r10", 640, 64 },
  { "r11", 704, 64 },
  { "r12", 768, 64 },
  { "r13", 832, 64 },
  { "r14", 896, 64 },
  { "r15", 960, 64 },
  { "rip", 1024, 64 },
  { "eflags", 1088, 32 },
  { "cs", 1120, 32 },
  { "ss", 1152, 32 },
  { "ds", 1184, 32 },
  { "es", 1216, 32 },
  { "fs", 1248, 32 },
  { "gs", 1280, 32 },
  { "st0", 1312, 80 },
  { "st1", 1392, 80 },
  { "st2", 1472, 80 },
  { "st3", 1552, 80 },
  { "st4", 1632, 80 },
  { "st5", 1712, 80 },
  { "st6", 1792, 80 },
  { "st7", 1872, 80 },
  { "fctrl", 1952, 32 },
  { "fstat", 1984, 32 },
  { "ftag", 2016, 32 },
  { "fiseg", 2048, 32 },
  { "fioff", 2080, 32 },
  { "foseg", 2112, 32 },
  { "fooff", 2144, 32 },
  { "fop", 2176, 32 },
  { "xmm0", 2208, 128 },
  { "xmm1", 2336, 128 },
  { "xmm2", 2464, 128 },
  { "xmm3", 2592, 128 },
  { "xmm4", 2720, 128 },
  { "xmm5", 2848, 128 },
  { "xmm6", 2976, 128 },
  { "xmm7", 3104, 128 },
  { "xmm8", 3232, 128 },
  { "xmm9", 3360, 128 },
  { "xmm10", 3488, 128 },
  { "xmm11", 3616, 128 },
  { "xmm12", 3744, 128 },
  { "xmm13", 3872, 128 },
  { "xmm14", 4000, 128 },
  { "xmm15", 4128, 128 },
  { "mxcsr", 4256, 32  },
#if defined(VGO_linux)
  { "orig_rax", 4288, 64 },
#endif
  /*
  { "fs_base", 4352, 64 },
  { "gs_base", 4416, 64 },
  */
  { "ymm0h", 4480, 128 },
  { "ymm1h", 4608, 128 },
  { "ymm2h", 4736, 128 },
  { "ymm3h", 4864, 128 },
  { "ymm4h", 4992, 128 },
  { "ymm5h", 5120, 128 },
  { "ymm6h", 5248, 128 },
  { "ymm7h", 5376, 128 },
  { "ymm8h", 5504, 128 },
  { "ymm9h", 5632, 128 },
  { "ymm10h", 5760, 128 },
  { "ymm11h", 5888, 128 },
  { "ymm12h", 6016, 128 },
  { "ymm13h", 6144, 128 },
  { "ymm14h", 6272, 128 },
  { "ymm15h", 6400, 128 },
  { "xmm16", 6528, 128 },
  { "xmm17", 6656, 128 },
  { "xmm18", 6784, 128 },
  { "xmm19", 6912, 128 },
  { "xmm20", 7040, 128 },
  { "xmm21", 7168, 128 },
  { "xmm22", 7296, 128 },
  { "xmm23", 7424, 128 },
  { "xmm24", 7552, 128 },
  { "xmm25", 7680, 128 },
  { "xmm26", 7808, 128 },
  { "xmm27", 7936, 128 },
  { "xmm28", 8064, 128 },
  { "xmm29", 8192, 128 },
  { "xmm30", 8320, 128 },
  { "xmm31", 8448, 128 },
  { "ymm16h", 8576, 128 },
  { "ymm17h", 8704, 128 },
  { "ymm18h", 8832, 128 },
  { "ymm19h", 8960, 128 },
  { "ymm20h", 9088, 128 },
  { "ymm21h", 9216, 128 },
  { "ymm22h", 9344, 128 },
  { "ymm23h", 9472, 128 },
  { "ymm24h", 9600, 128 },
  { "ymm25h", 9728, 128 },
  { "ymm26h", 9856, 128 },
  { "ymm27h", 9984, 128 },
  { "ymm28h", 10112, 128 },
  { "ymm29h", 10240, 128 },
  { "ymm30h", 10368, 128 },
  { "ymm31h", 10496, 128 },
  { "k0", 10624, 64 },
  { "k1", 10688, 64 },
  { "k2", 10752, 64 },
  { "k3", 10816, 64 },
  { "k4", 10880, 64 },
  { "k5", 10944, 64 },
  { "k6", 11008, 64 },
  { "k7", 11072, 64 },
  { "zmm0h", 11136, 256 },
  { "zmm1h", 11392, 256 },
  { "zmm2h", 11648, 256 },
  { "zmm3h", 11904, 256 },
  { "zmm4h", 12160, 256 },
  { "zmm5h", 12416, 256 },
  { "zmm6h", 12672, 256 },
  { "zmm7h", 12928, 256 },
  { "zmm8h", 13184, 256 },
  { "zmm9h", 13440, 256 },
  { "zmm10h", 13696, 256 },
  { "zmm11h", 13952, 256 },
  { "zmm12h", 14208, 256 },
  { "zmm13h", 14464, 256 },
  { "zmm14h", 14720, 256 },
  { "zmm15h", 14976, 256 },
  { "zmm16h", 15232, 256 },
  { "zmm17h", 15488, 256 },
  { "zmm18h", 15744, 256 },
  { "zmm19h", 16000, 256 },
  { "zmm20h", 16256, 256 },
  { "zmm21h", 16512, 256 },
  { "zmm22h", 16768, 256 },
  { "zmm23h", 17024, 256 },
  { "zmm24h", 17280, 256 },
  { "zmm25h", 17536, 256 },
  { "zmm26h", 17792, 256 },
  { "zmm27h", 18048, 256 },
  { "zmm28h", 18304, 256 },
  { "zmm29h", 18560, 256 },
  { "zmm30h", 18816, 256 },
  { "zmm31h", 19072, 256 }
};
static const char *expedite_regs[] = { "rbp", "rsp", "rip", 0 };
#define max_num_regs (sizeof (regs) / sizeof (regs[0]))
static int dyn_num_regs; // if no AVX, we have to give less registers to gdb.


static
CORE_ADDR get_pc (void)
{
   unsigned long pc;

   collect_register_by_name ("rip", &pc);
   
   dlog(1, "stop pc is %p\n", (void *) pc);
   return pc;
}

static
void set_pc (CORE_ADDR newpc)
{
   Bool mod;
   supply_register_by_name ("rip", &newpc, &mod);
   if (mod)
      dlog(1, "set pc to %p\n", C2v (newpc));
   else
      dlog(1, "set pc not changed %p\n", C2v (newpc));
}

/* store registers in the guest state (gdbserver_to_valgrind)
   or fetch register from the guest state (valgrind_to_gdbserver). */
static
void transfer_register (ThreadId tid, int abs_regno, void * buf,
                        transfer_direction dir, int size, Bool *mod)
{
   ThreadState* tst = VG_(get_ThreadState)(tid);
   int set = abs_regno / dyn_num_regs;
   int regno = abs_regno % dyn_num_regs;
   *mod = False;

   VexGuestAMD64State* amd64 = (VexGuestAMD64State*) get_arch (set, tst);

   switch (regno) { 
   // numbers here have to match the order of regs above.
   // Attention: gdb order does not match valgrind order.
   case 0:  VG_(transfer) (&amd64->guest_RAX, buf, dir, size, mod); break;
   case 1:  VG_(transfer) (&amd64->guest_RBX, buf, dir, size, mod); break;
   case 2:  VG_(transfer) (&amd64->guest_RCX, buf, dir, size, mod); break;
   case 3:  VG_(transfer) (&amd64->guest_RDX, buf, dir, size, mod); break;
   case 4:  VG_(transfer) (&amd64->guest_RSI, buf, dir, size, mod); break;
   case 5:  VG_(transfer) (&amd64->guest_RDI, buf, dir, size, mod); break;
   case 6:  VG_(transfer) (&amd64->guest_RBP, buf, dir, size, mod); break;
   case 7:  VG_(transfer) (&amd64->guest_RSP, buf, dir, size, mod); break;
   case 8:  VG_(transfer) (&amd64->guest_R8,  buf, dir, size, mod); break;
   case 9:  VG_(transfer) (&amd64->guest_R9,  buf, dir, size, mod); break;
   case 10: VG_(transfer) (&amd64->guest_R10, buf, dir, size, mod); break;
   case 11: VG_(transfer) (&amd64->guest_R11, buf, dir, size, mod); break;
   case 12: VG_(transfer) (&amd64->guest_R12, buf, dir, size, mod); break;
   case 13: VG_(transfer) (&amd64->guest_R13, buf, dir, size, mod); break;
   case 14: VG_(transfer) (&amd64->guest_R14, buf, dir, size, mod); break;
   case 15: VG_(transfer) (&amd64->guest_R15, buf, dir, size, mod); break;
   case 16: VG_(transfer) (&amd64->guest_RIP, buf, dir, size, mod); break;
   case 17: 
      if (dir == valgrind_to_gdbserver) {
         ULong rflags;
         /* we can only retrieve the real flags (set 0)
            retrieving shadow flags is not ok */
         if (set == 0)
            rflags = LibVEX_GuestAMD64_get_rflags (amd64);
         else
            rflags = 0;
         VG_(transfer) (&rflags, buf, dir, size, mod); 
      } else {
         *mod = False; //GDBTD? how do we store rflags in libvex_guest_amd64.h ???
      }
      break; 
   case 18: *mod = False; break; //GDBTD VG_(transfer) (&amd64->guest_CS, buf, dir, size, mod);
   case 19: *mod = False; break; //GDBTD VG_(transfer) (&amd64->guest_SS, buf, dir, size, mod);
   case 20: *mod = False; break; //GDBTD VG_(transfer) (&amd64->guest_DS, buf, dir, size, mod);
   case 21: *mod = False; break; //GDBTD VG_(transfer) (&amd64->guest_ES, buf, dir, size, mod);
   case 22: *mod = False; break; //GDBTD VG_(transfer) (&amd64->guest_FS, buf, dir, size, mod);
   case 23: VG_(transfer) (&amd64->guest_GS_CONST, buf, dir, size, mod); break;
   case 24:
   case 25:
   case 26:
   case 27: /* register 24 to 31 are float registers 80 bits but 64 bits in valgrind */
   case 28:
   case 29:
   case 30:
   case 31: 
      if (dir == valgrind_to_gdbserver) {
         UChar fpreg80[10];
         convert_f64le_to_f80le ((UChar *)&amd64->guest_FPREG[regno-24],
                                 fpreg80);
         VG_(transfer) (&fpreg80, buf, dir, sizeof(fpreg80), mod);
      } else {
         ULong fpreg64;
         convert_f80le_to_f64le (buf, (UChar *)&fpreg64); 
         VG_(transfer) (&amd64->guest_FPREG[regno-24], &fpreg64,
                        dir, sizeof(fpreg64), mod);
      }
      break;
   case 32: 
      if (dir == valgrind_to_gdbserver) {
         // vex only models the rounding bits (see libvex_guest_amd64.h)
         UWord value = 0x037f;
         value |= amd64->guest_FPROUND << 10;
         VG_(transfer)(&value, buf, dir, size, mod);
      } else {
         *mod = False; // GDBTD???? VEX equivalent fcrtl
      }
      break;
   case 33: 
      if (dir == valgrind_to_gdbserver) {
         UWord value = amd64->guest_FC3210;
         value |= (amd64->guest_FTOP & 7) << 11;
         VG_(transfer)(&value, buf, dir, size, mod); 
      } else {
         *mod = False; // GDBTD???? VEX equivalent fstat
      }
      break;
   case 34: 
      if (dir == valgrind_to_gdbserver) {
         // vex doesn't model these precisely
         UWord value = 
            ((amd64->guest_FPTAG[0] ? 0 : 3) << 0)  | 
            ((amd64->guest_FPTAG[1] ? 0 : 3) << 2)  | 
            ((amd64->guest_FPTAG[2] ? 0 : 3) << 4)  | 
            ((amd64->guest_FPTAG[3] ? 0 : 3) << 6)  | 
            ((amd64->guest_FPTAG[4] ? 0 : 3) << 8)  | 
            ((amd64->guest_FPTAG[5] ? 0 : 3) << 10) | 
            ((amd64->guest_FPTAG[6] ? 0 : 3) << 12) | 
            ((amd64->guest_FPTAG[7] ? 0 : 3) << 14);
         VG_(transfer)(&value, buf, dir, size, mod); 
      } else {
         *mod = False; // GDBTD???? VEX equivalent ftag
      }
      break;
   case 35: *mod = False; break; // GDBTD ??? equivalent of fiseg
   case 36: *mod = False; break; // GDBTD ??? equivalent of fioff
   case 37: *mod = False; break; // GDBTD ??? equivalent of foseg
   case 38: *mod = False; break; // GDBTD ??? equivalent of fooff
   case 39: *mod = False; break; // GDBTD ??? equivalent of fop
   case 40: VG_(transfer) (&amd64->guest_ZMM0[0],  buf, dir, size, mod); break;
   case 41: VG_(transfer) (&amd64->guest_ZMM1[0],  buf, dir, size, mod); break;
   case 42: VG_(transfer) (&amd64->guest_ZMM2[0],  buf, dir, size, mod); break;
   case 43: VG_(transfer) (&amd64->guest_ZMM3[0],  buf, dir, size, mod); break;
   case 44: VG_(transfer) (&amd64->guest_ZMM4[0],  buf, dir, size, mod); break;
   case 45: VG_(transfer) (&amd64->guest_ZMM5[0],  buf, dir, size, mod); break;
   case 46: VG_(transfer) (&amd64->guest_ZMM6[0],  buf, dir, size, mod); break;
   case 47: VG_(transfer) (&amd64->guest_ZMM7[0],  buf, dir, size, mod); break;
   case 48: VG_(transfer) (&amd64->guest_ZMM8[0],  buf, dir, size, mod); break;
   case 49: VG_(transfer) (&amd64->guest_ZMM9[0],  buf, dir, size, mod); break;
   case 50: VG_(transfer) (&amd64->guest_ZMM10[0], buf, dir, size, mod); break;
   case 51: VG_(transfer) (&amd64->guest_ZMM11[0], buf, dir, size, mod); break;
   case 52: VG_(transfer) (&amd64->guest_ZMM12[0], buf, dir, size, mod); break;
   case 53: VG_(transfer) (&amd64->guest_ZMM13[0], buf, dir, size, mod); break;
   case 54: VG_(transfer) (&amd64->guest_ZMM14[0], buf, dir, size, mod); break;
   case 55: VG_(transfer) (&amd64->guest_ZMM15[0], buf, dir, size, mod); break;
   case 56: 
      if (dir == valgrind_to_gdbserver) {
         // vex only models the rounding bits (see libvex_guest_x86.h)
         UWord value = 0x1f80;
         value |= amd64->guest_SSEROUND << 13;
         VG_(transfer)(&value, buf, dir, size, mod); 
      } else {
         *mod = False;  // GDBTD???? VEX equivalent mxcsr
      }
      break;
   case 57: *mod = False; break; // GDBTD???? VEX equivalent { "orig_rax"},
   case 58: VG_(transfer) (&amd64->guest_ZMM0[4],  buf, dir, size, mod); break;
   case 59: VG_(transfer) (&amd64->guest_ZMM1[4],  buf, dir, size, mod); break;
   case 60: VG_(transfer) (&amd64->guest_ZMM2[4],  buf, dir, size, mod); break;
   case 61: VG_(transfer) (&amd64->guest_ZMM3[4],  buf, dir, size, mod); break;
   case 62: VG_(transfer) (&amd64->guest_ZMM4[4],  buf, dir, size, mod); break;
   case 63: VG_(transfer) (&amd64->guest_ZMM5[4],  buf, dir, size, mod); break;
   case 64: VG_(transfer) (&amd64->guest_ZMM6[4],  buf, dir, size, mod); break;
   case 65: VG_(transfer) (&amd64->guest_ZMM7[4],  buf, dir, size, mod); break;
   case 66: VG_(transfer) (&amd64->guest_ZMM8[4],  buf, dir, size, mod); break;
   case 67: VG_(transfer) (&amd64->guest_ZMM9[4],  buf, dir, size, mod); break;
   case 68: VG_(transfer) (&amd64->guest_ZMM10[4], buf, dir, size, mod); break;
   case 69: VG_(transfer) (&amd64->guest_ZMM11[4], buf, dir, size, mod); break;
   case 70: VG_(transfer) (&amd64->guest_ZMM12[4], buf, dir, size, mod); break;
   case 71: VG_(transfer) (&amd64->guest_ZMM13[4], buf, dir, size, mod); break;
   case 72: VG_(transfer) (&amd64->guest_ZMM14[4], buf, dir, size, mod); break;
   case 73: VG_(transfer) (&amd64->guest_ZMM15[4], buf, dir, size, mod); break;
   /* xmm16-31 */
   case 74: VG_(transfer) (&amd64->guest_ZMM16[0], buf, dir, size, mod); break;
   case 75: VG_(transfer) (&amd64->guest_ZMM17[0], buf, dir, size, mod); break;
   case 76: VG_(transfer) (&amd64->guest_ZMM18[0], buf, dir, size, mod); break;
   case 77: VG_(transfer) (&amd64->guest_ZMM19[0], buf, dir, size, mod); break;
   case 78: VG_(transfer) (&amd64->guest_ZMM20[0], buf, dir, size, mod); break;
   case 79: VG_(transfer) (&amd64->guest_ZMM21[0], buf, dir, size, mod); break;
   case 80: VG_(transfer) (&amd64->guest_ZMM22[0], buf, dir, size, mod); break;
   case 81: VG_(transfer) (&amd64->guest_ZMM23[0], buf, dir, size, mod); break;
   case 82: VG_(transfer) (&amd64->guest_ZMM24[0], buf, dir, size, mod); break;
   case 83: VG_(transfer) (&amd64->guest_ZMM25[0], buf, dir, size, mod); break;
   case 84: VG_(transfer) (&amd64->guest_ZMM26[0], buf, dir, size, mod); break;
   case 85: VG_(transfer) (&amd64->guest_ZMM27[0], buf, dir, size, mod); break;
   case 86: VG_(transfer) (&amd64->guest_ZMM28[0], buf, dir, size, mod); break;
   case 87: VG_(transfer) (&amd64->guest_ZMM29[0], buf, dir, size, mod); break;
   case 88: VG_(transfer) (&amd64->guest_ZMM30[0], buf, dir, size, mod); break;
   case 89: VG_(transfer) (&amd64->guest_ZMM31[0], buf, dir, size, mod); break;
   /* ymm16-31 */
   case 90: VG_(transfer) (&amd64->guest_ZMM16[4], buf, dir, size, mod); break;
   case 91: VG_(transfer) (&amd64->guest_ZMM17[4], buf, dir, size, mod); break;
   case 92: VG_(transfer) (&amd64->guest_ZMM18[4], buf, dir, size, mod); break;
   case 93: VG_(transfer) (&amd64->guest_ZMM19[4], buf, dir, size, mod); break;
   case 94: VG_(transfer) (&amd64->guest_ZMM20[4], buf, dir, size, mod); break;
   case 95: VG_(transfer) (&amd64->guest_ZMM21[4], buf, dir, size, mod); break;
   case 96: VG_(transfer) (&amd64->guest_ZMM22[4], buf, dir, size, mod); break;
   case 97: VG_(transfer) (&amd64->guest_ZMM23[4], buf, dir, size, mod); break;
   case 98: VG_(transfer) (&amd64->guest_ZMM24[4], buf, dir, size, mod); break;
   case 99: VG_(transfer) (&amd64->guest_ZMM25[4], buf, dir, size, mod); break;
   case 100: VG_(transfer) (&amd64->guest_ZMM26[4], buf, dir, size, mod); break;
   case 101: VG_(transfer) (&amd64->guest_ZMM27[4], buf, dir, size, mod); break;
   case 102: VG_(transfer) (&amd64->guest_ZMM28[4], buf, dir, size, mod); break;
   case 103: VG_(transfer) (&amd64->guest_ZMM29[4], buf, dir, size, mod); break;
   case 104: VG_(transfer) (&amd64->guest_ZMM30[4], buf, dir, size, mod); break;
   case 105: VG_(transfer) (&amd64->guest_ZMM31[4], buf, dir, size, mod); break;
   /* k */
   case 106: VG_(transfer) (&amd64->guest_K0, buf, dir, size, mod); break;
   case 107: VG_(transfer) (&amd64->guest_K1, buf, dir, size, mod); break;
   case 108: VG_(transfer) (&amd64->guest_K2, buf, dir, size, mod); break;
   case 109: VG_(transfer) (&amd64->guest_K3, buf, dir, size, mod); break;
   case 110: VG_(transfer) (&amd64->guest_K4, buf, dir, size, mod); break;
   case 111: VG_(transfer) (&amd64->guest_K5, buf, dir, size, mod); break;
   case 112: VG_(transfer) (&amd64->guest_K6, buf, dir, size, mod); break;
   case 113: VG_(transfer) (&amd64->guest_K7, buf, dir, size, mod); break;
   /* zmm */
   case 114: VG_(transfer) (&amd64->guest_ZMM0[8], buf, dir, size, mod); break;
   case 115: VG_(transfer) (&amd64->guest_ZMM1[8], buf, dir, size, mod); break;
   case 116: VG_(transfer) (&amd64->guest_ZMM2[8], buf, dir, size, mod); break;
   case 117: VG_(transfer) (&amd64->guest_ZMM3[8], buf, dir, size, mod); break;
   case 118: VG_(transfer) (&amd64->guest_ZMM4[8], buf, dir, size, mod); break;
   case 119: VG_(transfer) (&amd64->guest_ZMM5[8], buf, dir, size, mod); break;
   case 120: VG_(transfer) (&amd64->guest_ZMM6[8], buf, dir, size, mod); break;
   case 121: VG_(transfer) (&amd64->guest_ZMM7[8], buf, dir, size, mod); break;
   case 122: VG_(transfer) (&amd64->guest_ZMM8[8], buf, dir, size, mod); break;
   case 123: VG_(transfer) (&amd64->guest_ZMM9[8], buf, dir, size, mod); break;
   case 124: VG_(transfer) (&amd64->guest_ZMM10[8], buf, dir, size, mod); break;
   case 125: VG_(transfer) (&amd64->guest_ZMM11[8], buf, dir, size, mod); break;
   case 126: VG_(transfer) (&amd64->guest_ZMM12[8], buf, dir, size, mod); break;
   case 127: VG_(transfer) (&amd64->guest_ZMM13[8], buf, dir, size, mod); break;
   case 128: VG_(transfer) (&amd64->guest_ZMM14[8], buf, dir, size, mod); break;
   case 129: VG_(transfer) (&amd64->guest_ZMM15[8], buf, dir, size, mod); break;
   case 130: VG_(transfer) (&amd64->guest_ZMM16[8], buf, dir, size, mod); break;
   case 131: VG_(transfer) (&amd64->guest_ZMM17[8], buf, dir, size, mod); break;
   case 132: VG_(transfer) (&amd64->guest_ZMM18[8], buf, dir, size, mod); break;
   case 133: VG_(transfer) (&amd64->guest_ZMM19[8], buf, dir, size, mod); break;
   case 134: VG_(transfer) (&amd64->guest_ZMM20[8], buf, dir, size, mod); break;
   case 135: VG_(transfer) (&amd64->guest_ZMM21[8], buf, dir, size, mod); break;
   case 136: VG_(transfer) (&amd64->guest_ZMM22[8], buf, dir, size, mod); break;
   case 137: VG_(transfer) (&amd64->guest_ZMM23[8], buf, dir, size, mod); break;
   case 138: VG_(transfer) (&amd64->guest_ZMM24[8], buf, dir, size, mod); break;
   case 139: VG_(transfer) (&amd64->guest_ZMM25[8], buf, dir, size, mod); break;
   case 140: VG_(transfer) (&amd64->guest_ZMM26[8], buf, dir, size, mod); break;
   case 141: VG_(transfer) (&amd64->guest_ZMM27[8], buf, dir, size, mod); break;
   case 142: VG_(transfer) (&amd64->guest_ZMM28[8], buf, dir, size, mod); break;
   case 143: VG_(transfer) (&amd64->guest_ZMM29[8], buf, dir, size, mod); break;
   case 144: VG_(transfer) (&amd64->guest_ZMM30[8], buf, dir, size, mod); break;
   case 145: VG_(transfer) (&amd64->guest_ZMM31[8], buf, dir, size, mod); break;
   default: vg_assert(0);
   }
}

static
Bool have_avx(void)
{
   VexArch va;
   VexArchInfo vai;
   VG_(machine_get_VexArchInfo) (&va, &vai);
   return ((vai.hwcaps & VEX_HWCAPS_AMD64_AVX) ? True : False);
}

static
Bool have_avx512(void) {
   VexArch va;
   VexArchInfo vai;
   VG_(machine_get_VexArchInfo) (&va, &vai);
   return ((vai.hwcaps & VEX_HWCAPS_AMD64_AVX512) ? True : False);
}

static
const char* target_xml (Bool shadow_mode)
{
   if (shadow_mode) {
#if defined(VGO_linux)
      if (have_avx())
         return "amd64-avx-linux-valgrind.xml";
      else
         return "amd64-linux-valgrind.xml";
#else
      if (have_avx())
         return "amd64-avx-coresse-valgrind.xml";
      else
         return "amd64-coresse-valgrind.xml";
#endif
   } else {
#if defined(VGO_linux)
      if (have_avx())
         return "amd64-avx-linux.xml";
      else
         return NULL;
#else
      if (have_avx())
         return "amd64-avx-coresse.xml";
      else
         return NULL;
#endif
   }  
}

static CORE_ADDR** target_get_dtv (ThreadState *tst)
{
   VexGuestAMD64State* amd64 = (VexGuestAMD64State*)&tst->arch.vex;
   return (CORE_ADDR**)((CORE_ADDR)amd64->guest_FS_CONST + 0x8);
}

static struct valgrind_target_ops low_target = {
   -1, // Must be computed at init time.
   regs,
   7, //RSP
   transfer_register,
   get_pc,
   set_pc,
   "amd64",
   target_xml,
   target_get_dtv
};

void amd64_init_architecture (struct valgrind_target_ops *target)
{
   *target = low_target;

   dyn_num_regs = max_num_regs;
   if (!have_avx512())
      dyn_num_regs -= 72; // remove the AVX-512 registers.
   if (!have_avx())
      dyn_num_regs -= 16; // remove the AVX "high" registers.

   dyn_num_regs = max_num_regs;
   target->num_regs = dyn_num_regs;
   set_register_cache (regs, dyn_num_regs);
   gdbserver_expedite_regs = expedite_regs;
}
