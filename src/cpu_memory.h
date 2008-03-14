/* ES40 emulator.
 * Copyright (C) 2007-2008 by the ES40 Emulator Project
 *
 * WWW    : http://sourceforge.net/projects/es40
 * E-mail : camiel@camicom.com
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 * Although this is not required, the author would appreciate being notified of, 
 * and receiving any modifications you may make to the source code that might serve
 * the general public.
 */

/**
 * \file 
 * Contains code macros for the processor memory load/store instructions.
 * Based on ARM chapter 4.2.
 *
 * $Id$
 *
 * X-1.9        Camiel Vanderhoeven                             14-MAR-2008
 *   1. More meaningful exceptions replace throwing (int) 1.
 *   2. U64 macro replaces X64 macro.
 *
 * X-1.8        Camiel Vanderhoeven                             05-MAR-2008
 *      Multi-threading version.
 *
 * X-1.7        Camiel Vanderhoeven                             25-JAN-2008
 *      Trap on unalogned memory access. The previous implementation where
 *      unaligned accesses were silently allowed could go wrong when page
 *      boundaries are crossed.
 *
 * X-1.6        Camiel Vanderhoeven                             18-JAN-2008
 *      Replaced sext_64 inlines with sext_u64_<bits> inlines for
 *      performance reasons (thanks to David Hittner for spotting this!);
 *
 * X-1.5        Camiel Vanderhoeven                             2-DEC-2007
 *      Changed the way translation buffers work. 
 *
 * X-1.4        Camiel Vanderhoeven                             11-APR-2007
 *      Moved all data that should be saved to a state file to a structure
 *      "state".
 *
 * X-1.3        Camiel Vanderhoeven                             30-MAR-2007
 *      Added old changelog comments.
 *
 * X-1.2        Camiel Vanderhoeven                             8-MAR-2007
 *      LDL and LDQ where the destination is R31 do not cause exceptions.
 *
 * X-1.1        Camiel Vanderhoeven                             18-FEB-2007
 *      File created. Contains code previously found in AlphaCPU.h
 **/

#define DO_LDA state.r[REG_1] = state.r[REG_2] + DISP_16;

#define DO_LDAH state.r[REG_1] = state.r[REG_2] + (DISP_16<<16);

#define DO_LDBU									\
	  DATA_PHYS_NT(state.r[REG_2] + DISP_16, ACCESS_READ);	\
	  state.r[REG_1] = READ_PHYS(8);

#define DO_LDL									\
	if (FREG_1 != 31) {							\
	  DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_READ, 3);	\
	  state.r[REG_1] = sext_u64_32(READ_PHYS(32)); }

#define DO_LDL_L								            \
	  DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_READ, 3);	\
      cSystem->cpu_lock(state.iProcNum,phys_address);       \
	  state.r[REG_1] = sext_u64_32(READ_PHYS(32));

#define DO_LDQ									\
	if (FREG_1 != 31) {							\
	  DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_READ, 7);	\
	  state.r[REG_1] = READ_PHYS(64); }

#define DO_LDQ_L								\
	  DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_READ, 7);	\
      cSystem->cpu_lock(state.iProcNum,phys_address);       \
	  state.r[REG_1] = READ_PHYS(64);

#define DO_LDQ_U									\
	  DATA_PHYS_NT((state.r[REG_2] + DISP_16)& ~U64(0x7), ACCESS_READ);	\
	  state.r[REG_1] = READ_PHYS(64);

#define DO_LDWU									\
	  DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_READ, 1);	\
	  state.r[REG_1] = READ_PHYS(16);

#define DO_STB									\
	  DATA_PHYS_NT(state.r[REG_2] + DISP_16, ACCESS_WRITE);	\
	  WRITE_PHYS(state.r[REG_1],8);

#define DO_STL									\
	  DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_WRITE, 3);	\
	  WRITE_PHYS(state.r[REG_1],32);

#define DO_STL_C								\
	  if (cSystem->cpu_unlock(state.iProcNum)) {				\
	      DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_WRITE, 3);	\
	      WRITE_PHYS(state.r[REG_1],32);						\
          state.r[REG_1] = 1;       \
	  }								\
        else                                                  \
          state.r[REG_1] = 0;

#define DO_STQ									\
	  DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_WRITE, 7);	\
	  WRITE_PHYS(state.r[REG_1],64);

#define DO_STQ_C								\
	  if (cSystem->cpu_unlock(state.iProcNum)) {				\
	      DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_WRITE, 7);	\
	      WRITE_PHYS(state.r[REG_1],64);						\
          state.r[REG_1] = 1;       \
	  }								\
        else                                                  \
          state.r[REG_1] = 0;

#define DO_STQ_U									\
	  DATA_PHYS_NT((state.r[REG_2] + DISP_16)& ~U64(0x7), ACCESS_WRITE);	\
	  WRITE_PHYS(state.r[REG_1],64);

#define DO_STW									\
	  DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_WRITE, 1);	\
	  WRITE_PHYS(state.r[REG_1],16);

