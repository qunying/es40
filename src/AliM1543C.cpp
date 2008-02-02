/* ES40 emulator.
 * Copyright (C) 2007-2008 by the ES40 Emulator Project
 *
 * Website: http://sourceforge.net/projects/es40
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
 * Contains the code for the emulated Ali M1543C chipset devices.
 *
 * $Id$
 *
 * X-1.52       Brian Wheeler                                   02-FEB-2008
 *      Completed LPT support so it works with FreeBSD as a guest OS.
 *
 * X-1.51       Brian wheeler                                   15-JAN-2008
 *      When a keyboard self-test command is received, and the queue is
 *      not empty, the queue is cleared so the 0x55 that's sent back
 *      will be the first thing in line. Makes the keyboard initialize
 *      a little better with SRM. 
 *
 * X-1.50       Camiel Vanderhoeven                             08-JAN-2008
 *      Comments.
 *
 * X-1.49       Camiel Vanderhoeven                             02-JAN-2008
 *      Comments; moved keyboard status register bits to a "status" struct.
 *
 * X-1.48       Camiel Vanderhoeven                             30-DEC-2007
 *      Print file id on initialization.
 *
 * X-1.47       Camiel Vanderhoeven                             30-DEC-2007
 *      Comments.
 *
 * X-1.46       Camiel Vanderhoeven                             29-DEC-2007
 *      Avoid referencing uninitialized data.
 *
 * X-1.45       Camiel Vanderhoeven                             28-DEC-2007
 *      Throw exceptions rather than just exiting when errors occur.
 *
 * X-1.44       Camiel Vanderhoeven                             28-DEC-2007
 *      Keep the compiler happy.
 *
 * X-1.43        Camiel Vanderhoeven                             19-DEC-2007
 *      Commented out message on PIC de-assertion.
 *
 * X-1.42        Brian wheeler                                   17-DEC-2007
 *      Better DMA support.      
 *
 * X-1.41        Camiel Vanderhoeven                             17-DEC-2007
 *      SaveState file format 2.1
 *
 * X-1.40       Brian Wheeler                                   11-DEC-2007
 *      Improved timer logic (again).
 *
 * X-1.39       Brian Wheeler                                   10-DEC-2007
 *      Improved timer logic.
 *
 * X-1.38       Camiel Vanderhoeven                             10-DEC-2007
 *      Added config item for vga_console.
 *
 * X-1.37       Camiel Vanderhoeven                             10-DEC-2007
 *      Use configurator; move IDE and USB to their own classes.
 *
 * X-1.36       Camiel Vanderhoeven                             7-DEC-2007
 *      Made keyboard messages conditional; add busmaster_status; add
 *      pic_edge_level.
 *
 * X-1.35       Camiel Vanderhoeven                             7-DEC-2007
 *      Generate keyboard interrupts when needed.
 *
 * X-1.34       Camiel Vanderhoeven                             6-DEC-2007
 *      Corrected bug regarding make/break key settings.
 *
 * X-1.33       Camiel Vanderhoeven                             6-DEC-2007
 *      Changed keyboard implementation (with thanks to the Bochs project!!)
 *
 * X-1.32       Brian Wheeler                                   2-DEC-2007
 *      Timing / floppy tweak for Linux/BSD guests.
 *
 * X-1.31       Brian Wheeler                                   1-DEC-2007
 *      Added console support (using SDL library), corrected timer
 *      behavior for Linux/BSD as a guest OS.
 *
 * X-1.30       Camiel Vanderhoeven                             1-DEC-2007
 *      Use correct interrupt for secondary IDE controller.
 *
 * X-1.29       Camiel Vanderhoeven                             17-NOV-2007
 *      Use CHECK_ALLOCATION.
 *
 * X-1.28       Eduardo Marcelo Serrat                          31-OCT-2007
 *      Corrected IDE interface revision level.
 *
 * X-1.27       Camiel Vanderhoeven                             18-APR-2007
 *      On a big-endian system, the LBA address for a read or write action
 *      was byte-swapped. Fixed this.
 *
 * X-1.26       Camiel Vanderhoeven                             17-APR-2007
 *      Removed debugging messages.
 *
 * X-1.25       Camiel Vanderhoeven                             16-APR-2007
 *      Added ResetPCI()
 *
 * X-1.24       Camiel Vanderhoeven                             11-APR-2007
 *      Moved all data that should be saved to a state file to a structure
 *      "state".
 *
 * X-1.23	    Camiel Vanderhoeven				                3-APR-2007
 *	    Fixed wrong IDE configuration mask (address ranges masked were too 
 *	    short, leading to overlapping memory regions.)	
 *
 * X-1.22	    Camiel Vanderhoeven				                1-APR-2007
 *	    Uncommented the IDE debugging statements.
 *
 * X-1.21       Camiel Vanderhoeven                             31-MAR-2007
 *      Added old changelog comments.
 *
 * X-1.20	    Camiel Vanderhoeven				                30-MAR-2007
 *	    Unintentional CVS commit / version number increase.
 *
 * X-1.19	    Camiel Vanderhoeven				                27-MAR-2007
 *       a) When DEBUG_PIC is defined, generate more debugging messages.
 *       b)	When an interrupt originates from the cascaded interrupt controller,
 *	    the interrupt vector from the cascaded controller is returned.
 *       c)	When interrupts are ended on the cascaded controller, and no 
 *	    interrupts are left on that controller, the cascade interrupt (2)
 *	    on the primary controller is ended as well. I'M NOT COMPLETELY SURE
 *	    IF THIS IS CORRECT, but what goes on in OpenVMS seems to imply this.
 *       d) When the system state is saved to a vms file, and then restored, the
 *	    ide_status may be 0xb9, this bug has not been found yet, but as a 
 *	    workaround, we detect the value 0xb9, and replace it with 0x40.
 *       e) Changed the values for cylinders/heads/sectors on the IDE identify
 *	    command, because it looks like OpenVMS' DQDRIVER doesn't like it if
 *	    the number of sectors is greater than 63.
 *       f) All IDE commands generate an interrupt upon completion.
 *       g) IDE command SET TRANSLATION (0x91) is recognized, but has no effect.
 *	    This is allright, as long as OpenVMS NEVER DOES CHS-mode access to 
 *	    the disk.
 *
 * X-1.18	    Camiel Vanderhoeven				                26-MAR-2007
 *       a) Specific-EOI's (end-of-interrupt) now only end the interrupt they 
 *	    are meant for.
 *   b) When DEBUG_PIC is defined, debugging messages for the interrupt 
 *	controllers are output to the console, same with DEBUG_IDE and the
 *	IDE controller.
 *   c) If IDE registers for a non-existing drive are read, 0xff is returned.
 *   d) Generate an interrupt when a sector is read or written from a disk.
 *
 * X-1.17	Camiel Vanderhoeven				1-MAR-2007
 *   a) Accesses to IDE-configuration space are byte-swapped on a big-endian 
 *	architecture. This is done through the endian_bits macro.
 *   b) Access to the IDE databuffers (16-bit transfers) are byte-swapped on 
 *	a big-endian architecture. This is done through the endian_16 macro.
 *
 * X-1.16	Camiel Vanderhoeven				20-FEB-2007
 *	Write sectors to disk when the IDE WRITE command (0x30) is executed.
 *
 * X-1.15	Brian Wheeler					20-FEB-2007
 *	Information about IDE disks is now kept in the ide_info structure.
 *
 * X-1.14	Camiel Vanderhoeven				16-FEB-2007
 *   a) This is now a slow-clocked device.
 *   b) Removed ifdef _WIN32 from printf statements.
 *
 * X-1.13	Brian Wheeler					13-FEB-2007
 *      Corrected some typecasts in printf statements.
 *
 * X-1.12	Camiel Vanderhoeven				12-FEB-2007
 *	Added comments.
 *
 * X-1.11       Camiel Vanderhoeven                             9-FEB-2007
 *      Replaced f_ variables with ide_ members.
 *
 * X-1.10       Camiel Vanderhoeven                             9-FEB-2007
 *      Only open an IDE disk image, if there is a filename.
 *
 * X-1.9 	Brian Wheeler					7-FEB-2007
 *	Load disk images according to the configuration file.
 *
 * X-1.8	Camiel Vanderhoeven				7-FEB-2007
 *   a)	Removed a lot of pointless messages.
 *   b)	Calls to trace_dev now use the TRC_DEVx macro's.
 *
 * X-1.7	Camiel Vanderhoeven				3-FEB-2007
 *      Removed last conditional for supporting another system than an ES40
 *      (ifdef DS15)
 *
 * X-1.6        Brian Wheeler                                   3-FEB-2007
 *      Formatting.
 *
 * X-1.5	Brian Wheeler					3-FEB-2007
 *	Fixed some problems with sprintf statements.
 *
 * X-1.4	Brian Wheeler					3-FEB-2007
 *	Space for 4 disks in f_img.
 *
 * X-1.3        Brian Wheeler                                   3-FEB-2007
 *      Scanf, printf and 64-bit literals made compatible with 
 *	Linux/GCC/glibc.
 *      
 * X-1.2        Brian Wheeler                                   3-FEB-2007
 *      Includes are now case-correct (necessary on Linux)
 *
 * X-1.1        Camiel Vanderhoeven                             19-JAN-2007
 *      Initial version in CVS.
 *
 * \author Camiel Vanderhoeven (camiel@camicom.com / http://www.camicom.com)
 **/

#include "StdAfx.h"
#include "AliM1543C.h"
#include "System.h"

#include <math.h>

#include "gui/scancodes.h"
#include "gui/keymap.h"

#ifdef DEBUG_PIC
bool pic_messages = false;
#endif

/* Timer Calibration: Instructions per Microsecond (assuming 1 clock = 1 instruction) */
#define IPus 847

u32 ali_cfg_data[64] = {
/*00*/  0x153310b9, // CFID: vendor + device
/*04*/  0x0200000f, // CFCS: command + status
/*08*/  0x060100c3, // CFRV: class + revision
/*0c*/  0x00000000, // CFLT: latency timer + cache line size
/*10*/  0x00000000, // BAR0: 
/*14*/  0x00000000, // BAR1: 
/*18*/  0x00000000, // BAR2: 
/*1c*/  0x00000000, // BAR3: 
/*20*/  0x00000000, // BAR4: 
/*24*/  0x00000000, // BAR5: 
/*28*/  0x00000000, // CCIC: CardBus
/*2c*/  0x00000000, // CSID: subsystem + vendor
/*30*/  0x00000000, // BAR6: expansion rom base
/*34*/  0x00000000, // CCAP: capabilities pointer
/*38*/  0x00000000,
/*3c*/  0x00000000, // CFIT: interrupt configuration
        0,0,0,0,0,
/*54*/  0x00000200, //                             
        0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

u32 ali_cfg_mask[64] = {
/*00*/  0x00000000, // CFID: vendor + device
/*04*/  0x00000000, // CFCS: command + status
/*08*/  0x00000000, // CFRV: class + revision
/*0c*/  0x00000000, // CFLT: latency timer + cache line size
/*10*/  0x00000000, // BAR0
/*14*/  0x00000000, // BAR1: CBMA
/*18*/  0x00000000, // BAR2: 
/*1c*/  0x00000000, // BAR3: 
/*20*/  0x00000000, // BAR4: 
/*24*/  0x00000000, // BAR5: 
/*28*/  0x00000000, // CCIC: CardBus
/*2c*/  0x00000000, // CSID: subsystem + vendor
/*30*/  0x00000000, // BAR6: expansion rom base
/*34*/  0x00000000, // CCAP: capabilities pointer
/*38*/  0x00000000,
/*3c*/  0x00000000, // CFIT: interrupt configuration
/*40*/  0xffcfff7f,
/*44*/  0xff00cbdf,
/*48*/  0xffffffff,
/*4c*/  0x000000ff,
/*50*/  0xffff8fff,
/*54*/  0xf0ffff00,
/*58*/  0x030f0d7f,
        0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

/**
 * Constructor.
 **/

CAliM1543C::CAliM1543C(CConfigurator * cfg, CSystem * c, int pcibus, int pcidev): CPCIDevice(cfg,c,pcibus,pcidev)
{
  if (theAli != 0)
    FAILURE("More than one Ali!!");
  theAli = this;

  add_function(0, ali_cfg_data, ali_cfg_mask);

  int i;
  char * filename;

  add_legacy_io(1,0x61,1);
  add_legacy_io(28,0x60,1);
  add_legacy_io(29,0x64,1);

  kbd_resetinternals(1);

  state.kbd_internal_buffer.led_status = 0;
  state.kbd_internal_buffer.scanning_enabled = 1;

  state.mouse_internal_buffer.num_elements = 0;
  for (i=0; i<BX_MOUSE_BUFF_SIZE; i++)
    state.mouse_internal_buffer.buffer[i] = 0;
  state.mouse_internal_buffer.head = 0;

  state.kbd_controller.status.pare = 0;
  state.kbd_controller.status.tim  = 0;
  state.kbd_controller.status.auxb = 0;
  state.kbd_controller.status.keyl = 1;
  state.kbd_controller.status.c_d  = 1;
  state.kbd_controller.status.sysf = 0;
  state.kbd_controller.status.inpb = 0;
  state.kbd_controller.status.outb = 0;

  state.kbd_controller.kbd_clock_enabled = 1;
  state.kbd_controller.aux_clock_enabled = 0;
  state.kbd_controller.allow_irq1 = 1;
  state.kbd_controller.allow_irq12 = 1;
  state.kbd_controller.kbd_output_buffer = 0;
  state.kbd_controller.aux_output_buffer = 0;
  state.kbd_controller.last_comm = 0;
  state.kbd_controller.expecting_port60h = 0;
  state.kbd_controller.irq1_requested = 0;
  state.kbd_controller.irq12_requested = 0;
  state.kbd_controller.expecting_mouse_parameter = 0;
  state.kbd_controller.bat_in_progress = 0;
  state.kbd_controller.scancodes_translate = 1;

  state.kbd_controller.timer_pending = 0;

  // Mouse initialization stuff
  state.mouse.captured        = myCfg->get_bool_value("mouse.enabled",true);
  state.mouse.sample_rate     = 100; // reports per second
  state.mouse.resolution_cpmm = 4;   // 4 counts per millimeter
  state.mouse.scaling         = 1;   /* 1:1 (default) */
  state.mouse.mode            = MOUSE_MODE_RESET;
  state.mouse.enable          = 0;
  state.mouse.delayed_dx      = 0;
  state.mouse.delayed_dy      = 0;
  state.mouse.delayed_dz      = 0;
  state.mouse.im_request      = 0; // wheel mouse mode request
  state.mouse.im_mode         = 0; // wheel mouse mode

  for (i=0; i<BX_KBD_CONTROLLER_QSIZE; i++)
    state.kbd_controller_Q[i] = 0;
  state.kbd_controller_Qsize = 0;
  state.kbd_controller_Qsource = 0;


  state.reg_61 = 0;

  add_legacy_io(2,0x70,4);
  c->RegisterMemory(this, 2, X64(00000801fc000070), 4);
  for (i=0;i<4;i++)
    state.toy_access_ports[i] = 0;
  for (i=0;i<256;i++)
    state.toy_stored_data[i] = 0;
  
  state.toy_stored_data[0x17] = myCfg->get_bool_value("vga_console")?1:0;

  ResetPCI();

  // PIT Setup
  add_legacy_io(6,0x40,4);
  for (i=0;i<3;i++)
    state.pit_status[i]=0x40; // invalid/null counter

  for(i=0;i<9;i++)
    state.pit_counter[i] = 0;

  c->RegisterClock(this, true);

  add_legacy_io(7,0x20,2);
  add_legacy_io(8,0xa0,2);
  add_legacy_io(30,0x4d0,2);

  // odd one, byte read in PCI IACK (interrupt acknowledge) cycle. Interrupt vector.
  c->RegisterMemory(this, 20, X64(00000801f8000000), 1);

  for(i=0;i<2;i++)
    {
      state.pic_mode[i] = 0;
      state.pic_intvec[i] = 0;
      state.pic_mask[i] = 0;
      state.pic_asserted[i] = 0;
    }

  // DMA Setup
  add_legacy_io(12,0x00,16); // dma 0-3
  add_legacy_io(13,0xc0,32); // dma 4-7
  add_legacy_io(33,0x80,16); // dma 0-7 (memory base low page register)
  add_legacy_io(34,0x480,16); // dma 0-7 (memory base high page register)
  for(i=0;i<4;i++) {
    state.dma_channel[i].lobyte=true;
  }

  // Initialize parallel port
  add_legacy_io(27,0x3bc,4);
  filename=myCfg->get_text_value("lpt.outfile");
  if(filename) {
    lpt=fopen(filename,"ab");
  } else {
    lpt=NULL;
  }
  lpt_reset();

  printf("%s: $Id$\n",devid_string);
}

/**
 * Destructor.
 **/

CAliM1543C::~CAliM1543C()
{
  if(lpt)
    fclose(lpt);
}

/**
 * Read (byte,word,longword) from one of the legacy ranges. Only byte-accesses are supported.
 *
 * Ranges are:
 *  - 1. I/O port 61h
 *  - 2. I/O ports 70h-73h (time-of-year clock)
 *  - 6. I/O ports 40h-43h (programmable interrupt timer)
 *  - 7. I/O ports 20h-21h (primary programmable interrupt controller)
 *  - 8. I/O ports a0h-a1h (secondary (cascaded) programmable interrupt controller)
 *  - 12. I/O ports 00h-0fh (primary DMA controller)
 *  - 13. I/O ports c0h-dfh (secondary DMA controller)
 *  - 20. PCI IACK address (interrupt vector)
 *  - 27. I/O ports 3bch-3bfh (parallel port)
 *  - 28. I/O port 60h (keyboard controller)
 *  - 29. I/O port 64h (keyboard controller)
 *  - 30. I/O ports 4d0h-4d1h (edge/level register of programmable interrupt controller)
 *  - 33. I/O ports 80h-8fh (DMA controller memory base low page register)
 *  - 34. I/O ports 480h-48fh (DMA controller memory base high page register)
 *  .
 **/

u32 CAliM1543C::ReadMem_Legacy(int index, u32 address, int dsize)
{
  if (dsize!=8 && index !=20) // when interrupt vector is read, dsize doesn't matter.
  {
    printf("%s: DSize %d seen reading from legacy memory range # %d at address %02x\n", devid_string, dsize, index, address);
    FAILURE("Unsupported dsize");
  }
  int channel = 0;
  switch(index)
    {
    case 1:
	  return reg_61_read();
    case 2:
      return toy_read(address);
    case 6:
      return pit_read(address);
    case 8:
      channel = 1;
    case 7:
      return pic_read(channel, address);
    case 20:
      return pic_read_vector();
    case 30:
      return pic_read_edge_level(address);
    case 13:
      channel = 1;
      address >>= 1;
    case 12:
      return dma_read(channel, address);
    case 33:
      return dma_read(3,address);
    case 34:
      return dma_read(4,address);
    case 27:
      return lpt_read(address);
    case 28:
      return kbd_60_read();
    case 29:
      return kbd_64_read();
    }

  return 0;
}

/**
 * Write (byte,word,longword) to one of the legacy ranges. Only byte-accesses are supported.
 *
 * Ranges are:
 *  - 1. I/O port 61h
 *  - 2. I/O ports 70h-73h (time-of-year clock)
 *  - 6. I/O ports 40h-43h (programmable interrupt timer)
 *  - 7. I/O ports 20h-21h (primary programmable interrupt controller)
 *  - 8. I/O ports a0h-a1h (secondary (cascaded) programmable interrupt controller)
 *  - 12. I/O ports 00h-0fh (primary DMA controller)
 *  - 13. I/O ports c0h-dfh (secondary DMA controller)
 *  - 20. PCI IACK address (interrupt vector)
 *  - 27. I/O ports 3bch-3bfh (parallel port)
 *  - 28. I/O port 60h (keyboard controller)
 *  - 29. I/O port 64h (keyboard controller)
 *  - 30. I/O ports 4d0h-4d1h (edge/level register of programmable interrupt controller)
 *  - 33. I/O ports 80h-8fh (DMA controller memory base low page register)
 *  - 34. I/O ports 480h-48fh (DMA controller memory base high page register)
 *  .
 **/

void CAliM1543C::WriteMem_Legacy(int index, u32 address, int dsize, u32 data)
{
  if (dsize!=8)
  {
    printf("%s: DSize %d seen writing to legacy memory range # %d at address %02x\n", devid_string, dsize, index, address);
    FAILURE("Unsupported dsize");
  }
  int channel = 0;
  switch(index)
    {
    case 1:
	  reg_61_write((u8)data);
      return;
    case 2:
      toy_write(address, (u8)data);
      return;
    case 6:
      pit_write(address, (u8) data);
      return;
    case 8:
      channel = 1;
    case 7:
      pic_write(channel, address, (u8) data);
      return;
    case 30:
      pic_write_edge_level(address, (u8) data);
      return;
    case 13:
      channel = 1;
      address >>= 1;
    case 12:
      dma_write(channel, address, (u8) data);
      return;
    case 33:
      dma_write(3,address,(u8) data);
      return;
    case 34:
      dma_write(4,address,(u8)data);
      return;
    case 27:
      lpt_write(address,(u8)data);
      return;
    case 28:
      kbd_60_write((u8)data);
      return;
    case 29:
      kbd_64_write((u8)data);
      return;
    }
}

/**
 * Enqueue scancode for a keypress or key-release. Used by the GUI implementation
 * to send keypresses to the keyboard controller.
 **/

void CAliM1543C::kbd_gen_scancode(u32 key)
{
  unsigned char *scancode;
  u8  i;

#if defined(DEBUG_KBD)
  printf("gen_scancode(): %s %s  \n", bx_keymap.getBXKeyName(key), (key >> 31)?"released":"pressed");
  if (!state.kbd_controller.scancodes_translate)
    BX_DEBUG(("keyboard: gen_scancode with scancode_translate cleared"));
#endif

  // Ignore scancode if keyboard clock is driven low
  if (state.kbd_controller.kbd_clock_enabled==0)
    return;

  // Ignore scancode if scanning is disabled
  if (state.kbd_internal_buffer.scanning_enabled==0)
    return;

  // Switch between make and break code
  if (key & BX_KEY_RELEASED)
    scancode=(unsigned char *)scancodes[(key&0xFF)][state.kbd_controller.current_scancodes_set].brek;
  else
    scancode=(unsigned char *)scancodes[(key&0xFF)][state.kbd_controller.current_scancodes_set].make;

  if (state.kbd_controller.scancodes_translate) 
  {
    // Translate before send
    u8 escaped=0x00;

    for (i=0; i<strlen((const char *)scancode); i++) 
    {
      if (scancode[i] == 0xF0)
      {
        escaped=0x80;
      }
      else 
      {
#if defined(DEBUG_KBD)
	    printf("gen_scancode(): writing translated %02x   \n",translation8042[scancode[i] ] | escaped);
#endif
        kbd_enQ(translation8042[scancode[i]] | escaped);
        escaped=0x00;
      }
    }
  } 
  else 
  {
    // Send raw data
    for (i=0; i<strlen((const char *)scancode); i++) {
#if defined(DEBUG_KBD)
      printf("gen_scancode(): writing raw %02x",scancode[i]);
#endif
      kbd_enQ(scancode[i]);
    }
  }
}

/**
 * Read port 61h (speaker/ miscellaneous).
 *
 * BDW:
 * This may need some expansion to help with timer delays.  It looks like
 * the 8254 flips bits on occasion, and the linux kernel (at least) uses
 *   do {
 *     count++;
 *   } while ((inb(0x61) & 0x20) == 0 && count < TIMEOUT_COUNT);
 * to calibrate the cpu clock.
 * 
 * Every 1500 reads the bit gets flipped so maybe the timing will
 * seem reasonable to the OS.
 */

u8 CAliM1543C::reg_61_read()
{
#if 0
  static long read_count = 0;
  if(!(state.reg_61 & 0x20)) {
    if (read_count % 1500 == 0)
      state.reg_61 |= 0x20;
  } else {
    state.reg_61 &= ~0x20;
  }
  read_count++;
#else
  state.reg_61 &= ~0x20;
  state.reg_61 |= (state.pit_status[2] & 0x80) >> 2;
#endif
  return state.reg_61;
}

/**
 * Write port 61h (speaker/ miscellaneous).
 **/

void CAliM1543C::reg_61_write(u8 data)
{
  state.reg_61 = (state.reg_61 & 0xf0) | (((u8)data) & 0x0f);
}

/**
 * Read time-of-year clock ports (70h-73h).
 **/

u8 CAliM1543C::toy_read(u32 address)
{
  //printf("%%ALI-I-READTOY: read port %02x: 0x%02x\n", (u32)(0x70 + address), state.toy_access_ports[address]);
  return (u8)state.toy_access_ports[address];
}

/**
 * Write time-of-year clock ports (70h-73h). On a write to port 0, recalculate clock values.
 **/

void CAliM1543C::toy_write(u32 address, u8 data)
{
  time_t ltime;
  struct tm stime;
  static long read_count = 0;
  static long hold_count = 0;

  //printf("%%ALI-I-WRITETOY: write port %02x: 0x%02x\n", (u32)(0x70 + address), data);

  state.toy_access_ports[address] = (u8)data;

  switch (address)
  {
  case 0:
    if ((data&0x7f)<14)
    {
	  state.toy_stored_data[0x0d] = 0x80; // data is geldig!
	  // update clock.......
	  time (&ltime);
	  gmtime_s(&stime,&ltime);
	  if (state.toy_stored_data[0x0b] & 4)
      {
        // binary
        state.toy_stored_data[0] = (u8)(stime.tm_sec);
	    state.toy_stored_data[2] = (u8)(stime.tm_min);
	    if (state.toy_stored_data[0x0b] & 2)		  // 24-hour
		  state.toy_stored_data[4] = (u8)(stime.tm_hour);
	    else                                  		  // 12-hour
		  state.toy_stored_data[4] = (u8)(((stime.tm_hour/12)?0x80:0) | (stime.tm_hour%12));
        state.toy_stored_data[6] = (u8)(stime.tm_wday + 1);
        state.toy_stored_data[7] = (u8)(stime.tm_mday);
	    state.toy_stored_data[8] = (u8)(stime.tm_mon + 1);
	    state.toy_stored_data[9] = (u8)(stime.tm_year % 100);  
	  }
	  else
      {
	    // BCD
	    state.toy_stored_data[0] = (u8)(((stime.tm_sec/10)<<4) | (stime.tm_sec%10));
	    state.toy_stored_data[2] = (u8)(((stime.tm_min/10)<<4) | (stime.tm_min%10));
	    if (state.toy_stored_data[0x0b] & 2)		  // 24-hour
		  state.toy_stored_data[4] = (u8)(((stime.tm_hour/10)<<4) | (stime.tm_hour%10));
        else                                  		  // 12-hour
		  state.toy_stored_data[4] = (u8)(((stime.tm_hour/12)?0x80:0) | (((stime.tm_hour%12)/10)<<4) | ((stime.tm_hour%12)%10));
	    state.toy_stored_data[6] = (u8)(stime.tm_wday + 1);
	    state.toy_stored_data[7] = (u8)(((stime.tm_mday/10)<<4) | (stime.tm_mday%10));
	    state.toy_stored_data[8] = (u8)((((stime.tm_mon+1)/10)<<4) | ((stime.tm_mon+1)%10));
	    state.toy_stored_data[9] = (u8)((((stime.tm_year%100)/10)<<4) | ((stime.tm_year%100)%10));
      }

	  // Debian Linux wants something out of 0x0a.  It gets initialized
	  // with 0x26, by the SRM
	  // Ah, here's something from the linux kernel:
	  //# /********************************************************
	  //# * register details
	  //# ********************************************************/
	  //# #define RTC_FREQ_SELECT RTC_REG_A
	  //#
	  //# /* update-in-progress - set to "1" 244 microsecs before RTC goes off the bus,
	  //# * reset after update (may take 1.984ms @ 32768Hz RefClock) is complete,
	  //# * totalling to a max high interval of 2.228 ms.
	  //# */
	  //# # define RTC_UIP 0x80
	  //# # define RTC_DIV_CTL 0x70
	  //# /* divider control: refclock values 4.194 / 1.049 MHz / 32.768 kHz */
	  //# # define RTC_REF_CLCK_4MHZ 0x00
	  //# # define RTC_REF_CLCK_1MHZ 0x10
	  //# # define RTC_REF_CLCK_32KHZ 0x20
	  //# /* 2 values for divider stage reset, others for "testing purposes only" */
	  //# # define RTC_DIV_RESET1 0x60
	  //# # define RTC_DIV_RESET2 0x70
	  //# /* Periodic intr. / Square wave rate select. 0=none, 1=32.8kHz,... 15=2Hz */
	  //# # define RTC_RATE_SELECT 0x0F
	  //#
	  
	  // The SRM-init value of 0x26 means:
	  //  xtal speed 32.768KHz  (standard)
	  //  periodic interrupt rate divisor of 32 = interrupt every 976.562 ms (1024Hz clock)
	  
	  if(state.toy_stored_data[0x0a] & 0x80) 
      {
	    // Once the UIP line goes high, we have to stay high for 2228us.
	    hold_count--;
	    if(hold_count==0) 
        {
	      state.toy_stored_data[0x0a] &= ~0x80;
	      read_count=0;
	    }
	  } 
      else 
      {
	    // UIP isn't high, so if we're looping and waiting for it to go, it
	    // will take 1,000,000/(IPus*3) reads for a 3 instruction loop.
	    // If it happens to be a one time read, it'll only throw our calculations
	    // off a tiny bit, and they'll be re-synced on the next read-loop.
	    read_count++;
	    if(read_count > 1000000/(IPus*3))    // 3541 @ 847IPus
        {
	      state.toy_stored_data[0x0a] |= 0x80;
	      hold_count=(2228/(IPus*3))+1; // .876 @ 847IPus, so we add one.
	    }
	  }
	  	  
	  //# /****************************************************/
	  //# #define RTC_CONTROL RTC_REG_B
	  //# # define RTC_SET 0x80 /* disable updates for clock setting */
	  //# # define RTC_PIE 0x40 /* periodic interrupt enable */
	  //# # define RTC_AIE 0x20 /* alarm interrupt enable */
	  //# # define RTC_UIE 0x10 /* update-finished interrupt enable */
	  //# # define RTC_SQWE 0x08 /* enable square-wave output */
	  //# # define RTC_DM_BINARY 0x04 /* all time/date values are BCD if clear */
	  //# # define RTC_24H 0x02 /* 24 hour mode - else hours bit 7 means pm */
	  //# # define RTC_DST_EN 0x01 /* auto switch DST - works f. USA only */
	  //#
	  // this is set (by the srm?) to 0x0e = SQWE | DM_BINARY | 24H
	  // linux sets the PIE bit later.
    
	  //# /***********************************************************/
	  //# #define RTC_INTR_FLAGS RTC_REG_C
	  //# /* caution - cleared by read */
	  //# # define RTC_IRQF 0x80 /* any of the following 3 is active */
	  //# # define RTC_PF 0x40
	  //# # define RTC_AF 0x20
	  //# # define RTC_UF 0x10
	  //#
	}
    state.toy_access_ports[1] = state.toy_stored_data[data & 0x7f];

    // register C is cleared after a read, and we don't care if its a write
    if(data== 0x0c) 
	  state.toy_stored_data[data & 0x7f]=0;
    break;
  case 1:
    if(state.toy_access_ports[0]==0x0b && data & 0x040) // If we're writing to register B, we make register C look like it fired.
	  state.toy_stored_data[0x0c]=0xf0;
    state.toy_stored_data[state.toy_access_ports[0] & 0x7f] = (u8)data;
    break;
  case 2:
    state.toy_access_ports[3] = state.toy_stored_data[0x80 + (data & 0x7f)];
    break;
  case 3:
    state.toy_stored_data[0x80 + (state.toy_access_ports[2] & 0x7f)] = (u8)data;
    break;
  }
}

/**
 * Read from the programmable interrupt timer ports (40h-43h)
 *
 * BDW:
 * Here's the PIT Traffic during SRM and Linux Startup:
 *
 * SRM
 * PIT Write:  3, 36  = counter 0, load lsb + msb, mode 3
 * PIT Write:  0, 00  
 * PIT Write:  0, 00  = 65536 = 18.2 Hz = timer interrupt.
 * PIT Write:  3, 54  = counter 1, msb only, mode 2
 * PIT Write:  1, 12  = 0x1200 = memory refresh?
 * PIT Write:  3, b6  = counter 2, load lsb + msb, mode 3
 * PIT Write:  3, 00  
 * PIT Write:  0, 00  
 * PIT Write:  0, 00  
 * 
 * Linux Startup
 * PIT Write:  3, b0  = counter 2, load lsb+msb, mode 0
 * PIT Write:  2, a4  
 * PIT Write:  2, ec  = eca4
 * PIT Write:  3, 36  = counter 0, load lsb+msb, mode 3
 * PIT Write:  0, 00  
 * PIT Write:  0, 00  = 65536
 * PIT Write:  3, b6  = counter 2, load lsb+msb, mode 3
 * PIT Write:  2, 31  
 * PIT Write:  2, 13  = 1331
 **/

u8 CAliM1543C::pit_read(u32 address)
{
  //printf("PIT Read: %02" LL "x \n",address);
  u8 data;
  data = 0;
  return data;
}

/**
 * Write to the programmable interrupt timer ports (40h-43h)
 **/

void CAliM1543C::pit_write(u32 address, u8 data)
{
  //printf("PIT Write: %02" LL "x, %02x \n",address,data);
  if(address==3) { // control
    if(data != 0) {
      state.pit_status[address]=data; // last command seen.
      if((data & 0xc0)>>6 != 3) {
	state.pit_status[(data & 0xc0)>>6]=data & 0x3f;
	state.pit_mode[(data & 0xc0)>>6]=(data & 0x30) >> 4;
      } else { // readback command 8254 only
	state.pit_status[address]=0xc0;  // bogus :)
      }
    }
  } else { // a counter
    switch(state.pit_mode[address]) {
    case 0:
      break;
    case 1:
    case 3:
      state.pit_counter[address] = (state.pit_counter[address] & 0xff) | data<<8;
      state.pit_counter[address + PIT_OFFSET_MAX]=state.pit_counter[address];
      if(state.pit_mode[address]==3) {
	state.pit_mode[address]=2;
      } else
	state.pit_status[address] &= ~0xc0; // no longer high, counter valid.
      break;
    case 2:
      state.pit_counter[address] = (state.pit_counter[address] & 0xff00) | data;

      // two bytes were written with 0x00, so its really 0x10000
      if((state.pit_status[address] & 0x30) >> 4==3 && state.pit_counter[address]==0) {
	state.pit_counter[address]=65536;
      }
      state.pit_counter[address + PIT_OFFSET_MAX]=state.pit_counter[address];
      state.pit_status[address] &= ~0xc0; // no longer high, counter valid.
      break;
    }
  }
}

#define PIT_FACTOR 5000
#define PIT_DEC(p) p=(p<PIT_FACTOR?0:p-PIT_FACTOR);

/**
 * Handle the PIT interrupt.
 *
 *  - counter 0 is the 18.2Hz time counter.
 *  - counter 1 is the ram refresh, we don't care.
 *  - counter 2 is the speaker and/or generic timer
 *  .
 **/

void CAliM1543C::pit_clock() {
  int i;
  for(i=0;i<3;i++) {
    // decrement the counter.
    if(state.pit_status[i] & 0x40)
      continue;
    PIT_DEC(state.pit_counter[i]);
    switch((state.pit_status[i] & 0x0e)>>1) {
    case 0: // interrupt at terminal
      if(!state.pit_counter[i]) {
	state.pit_status[i] |= 0xc0;  // out pin high, no count set.
      }
      break;
    case 3: // square wave generator
      if(!state.pit_counter[i]) {
	if(state.pit_status[i] & 0x80) {
	  state.pit_status[i] &= ~0x80; // lower output;
	} else {
	  state.pit_status[i] |= 0x80; // raise output
	  if(i==0) {
	    pic_interrupt(0,0); // counter 0 is tied to irq 0.
	    //printf("Generating timer interrupt.\n");
	  }
	}
	state.pit_counter[i]=state.pit_counter[i+PIT_OFFSET_MAX];
      }
      // decrement again, since we want a half-wide square wave.
      PIT_DEC(state.pit_counter[i]);
      break;
    default:
      break;  // we don't care to handle it.
    }
  }
}

#define PIT_RATIO 1

/**
 * Handle all events that need to be handled on a clock-driven basis.
 *
 * This is a slow-clocked device, which means this DoClock isn't called as often as the CPU's DoClock.
 * Do the following:
 *  - Generate the 1024 Hz clock interrupt at IRQ_H 2 on the CPU's.
 *  - Handle keyboard clock.
 *  - Handle PIT clock.
 *  .
 **/

int CAliM1543C::DoClock()
{
  static int interrupt_factor=0;
  cSystem->interrupt(-1, true);

  kbd_clock();
#if 0
  // this isn't ready yet.
  if(state.toy_stored_data[0x0b] & 0x40 && interrupt_factor++ > 10000) 
  {
    //printf("Issuing RTC Interrupt\n");
    pic_interrupt(1,0);
    state.toy_stored_data[0x0c] |= 0xf0; // mark that the interrupt has happened.
    interrupt_factor=0;
  }
#endif

  static int pit_counter = 0;
  if(pit_counter++ >= PIT_RATIO)
  {
    pit_counter = 0;
    pit_clock();
  }

  return 0;
}

#define PIC_STD 0
#define PIC_INIT_0 1
#define PIC_INIT_1 2
#define PIC_INIT_2 3

/**
 * Read a byte from one of the programmable interrupt controller's registers.
 **/

u8 CAliM1543C::pic_read(int index, u32 address)
{
  u8 data;

  data = 0;

  if (address == 1) 
    data = state.pic_mask[index];

#ifdef DEBUG_PIC
  if (pic_messages) printf("%%PIC-I-READ: read %02x from port %" LL "d on PIC %d\n",data,address,index);
#endif

  return data;
}

/**
 * Read a byte from the edge/level register of one of the programmable interrupt controllers.
 **/

u8 CAliM1543C::pic_read_edge_level(int index)
{
  return state.pic_edge_level[index];
}

/**
 * Read the interrupt vector during a PCI IACK cycle.
 **/

u8 CAliM1543C::pic_read_vector()
{
  if (state.pic_asserted[0] & 1)
    return state.pic_intvec[0];
  if (state.pic_asserted[0] & 2)
    return state.pic_intvec[0]+1;
  if (state.pic_asserted[0] & 4)
  {
    if (state.pic_asserted[1] & 1)
      return state.pic_intvec[1];
    if (state.pic_asserted[1] & 2)
      return state.pic_intvec[1]+1;
    if (state.pic_asserted[1] & 4)
      return state.pic_intvec[1]+2;
    if (state.pic_asserted[1] & 8)
      return state.pic_intvec[1]+3;
    if (state.pic_asserted[1] & 16)
      return state.pic_intvec[1]+4;
    if (state.pic_asserted[1] & 32)
      return state.pic_intvec[1]+5;
    if (state.pic_asserted[1] & 64)
      return state.pic_intvec[1]+6;
    if (state.pic_asserted[1] & 128)
      return state.pic_intvec[1]+7;
  }
  if (state.pic_asserted[0] & 8)
    return state.pic_intvec[0]+3;
  if (state.pic_asserted[0] & 16)
    return state.pic_intvec[0]+4;
  if (state.pic_asserted[0] & 32)
    return state.pic_intvec[0]+5;
  if (state.pic_asserted[0] & 64)
    return state.pic_intvec[0]+6;
  if (state.pic_asserted[0] & 128)
    return state.pic_intvec[0]+7;
  return 0;
}

/**
 * Write a byte to one of the programmable interrupt controller's registers.
 **/

void CAliM1543C::pic_write(int index, u32 address, u8 data)
{
  int level;
  int op;
#ifdef DEBUG_PIC
  if (pic_messages) printf("%%PIC-I-WRITE: write %02x to port %" LL "d on PIC %d\n",data,address,index);
#endif

  switch(address)
    {
    case 0:
      if (data & 0x10)
	state.pic_mode[index] = PIC_INIT_0;
      else
	state.pic_mode[index] = PIC_STD;
      if (data & 0x08)
	{
	  // OCW3
	}
      else
	{
	  // OCW2
	  op = (data>>5) & 7;
	  level = data & 7;
	  switch(op)
	    {
	    case 1:
	      //non-specific EOI
	      state.pic_asserted[index] = 0;
	      //
	      if (index==1)
	        state.pic_asserted[0] &= ~(1<<2);
	      //
	      if (!state.pic_asserted[0])
		cSystem->interrupt(55,false);
#ifdef DEBUG_PIC
	      pic_messages = false;
#endif
	      break;
	    case 3:
	      // specific EOI
	      state.pic_asserted[index] &= ~(1<<level);
	      //
	      if ((index==1) && (!state.pic_asserted[1]))
	        state.pic_asserted[0] &= ~(1<<2);
	      //
	      if (!state.pic_asserted[0])
		cSystem->interrupt(55,false);
#ifdef DEBUG_PIC
	      pic_messages = false;
#endif
	      break;				
	    }
	}
      return;
    case 1:
      switch(state.pic_mode[index])
	{
	case PIC_INIT_0:
	  state.pic_intvec[index] = (u8)data & 0xf8;
	  state.pic_mode[index] = PIC_INIT_1;
	  return;
	case PIC_INIT_1:
	  state.pic_mode[index] = PIC_INIT_2;
	  return;
	case PIC_INIT_2:
	  state.pic_mode[index] = PIC_STD;
	  return;
	case PIC_STD:
	  state.pic_mask[index] = data;
	  state.pic_asserted[index] &= ~data;
	  return;
	}
    }
}

/**
 * Write a byte to the edge/level register of one of the programmable interrupt controllers.
 **/

void CAliM1543C::pic_write_edge_level(int index, u8 data)
{
  state.pic_edge_level[index] = data;
}

#define DEBUG_EXPR (index!=0 || (intno != 0 && intno > 4))

/**
 * Assert an interrupt on one of the programmable interrupt controllers.
 **/

void CAliM1543C::pic_interrupt(int index, int intno)
{
#ifdef DEBUG_PIC
  if (DEBUG_EXPR) 
  {
    printf("%%PIC-I-INCOMING: Interrupt %d incomming on PIC %d",intno,index);
    pic_messages = true;
  }
#endif

  // do we have this interrupt enabled?
  if (state.pic_mask[index] & (1<<intno))
  {
#ifdef DEBUG_PIC
  if (DEBUG_EXPR)     printf(" (masked)\n");
  pic_messages = false;
#endif
    return;
  }

  if (state.pic_asserted[index] & (1<<intno))
  {
#ifdef DEBUG_PIC
  if (DEBUG_EXPR)     printf(" (already asserted)\n");
#endif
    return;
  }

#ifdef DEBUG_PIC
  if (DEBUG_EXPR)   printf("\n");
#endif

  state.pic_asserted[index] |= (1<<intno);
	
  if (index==1)
    pic_interrupt(0,2);	// cascade

  if (index==0)
    cSystem->interrupt(55,true);
}

/**
 * De-assert an interrupt on one of the programmable interrupt controllers.
 **/

void CAliM1543C::pic_deassert(int index, int intno)
{
  if (!(state.pic_asserted[index] & (1<<intno)))
    return;

//  printf("De-asserting %d,%d\n",index,intno);
  state.pic_asserted[index] &= !(1<<intno);
  if (index==1 && state.pic_asserted[1]==0)
    pic_deassert(0,2); // cascade

  if (index==0 && state.pic_asserted[0]==0)
    cSystem->interrupt(55,false);
}

/**
 * Read a byte from the dma controller.
 * Always returns 0.
 **/

u8 CAliM1543C::dma_read(int index, u32 address)
{
  u8 data;
  printf("DMA Read: %d,%x \n",index,address);
  data = 0;

  return data;
}

/**
 * Write a byte to the dma controller.
 * Not functional.
 **/

void CAliM1543C::dma_write(int index, u32 address, u8 data)
{
  printf("DMA Write: %x,%x,%x \n",index,address,data);
  int num;

  switch(index) {
  case 0: // 0x00 -> 0x0f
  case 1: // 0xc0 -> 0xdf
    switch(address) {
    case 0x00: // base address
    case 0x02: 
    case 0x04:
    case 0x06:
      num = (address/2)+(index*4);
      if(state.dma_channel[num].lobyte) {
      state.dma_channel[num].base = data;
      state.dma_channel[num].lobyte=false;
      } else {
      state.dma_channel[num].base |= (data << 8);
      state.dma_channel[num].current=state.dma_channel[num].base;
      state.dma_channel[num].lobyte=true;
      }
      break;
      
    case 0x01: // word count
    case 0x03:
    case 0x05:
    case 0x07:
      num = ((address-1)/2)+(index*4);
      if(state.dma_channel[num].lobyte) {
      state.dma_channel[num].count = data;
      state.dma_channel[num].lobyte=false;
      } else {
      state.dma_channel[num].count |= (data << 8);
      state.dma_channel[num].lobyte=true;
      }
      break;

    case 0x08: // command register (2)
      /*
      Bit(s)  Description     (Table P0002)
      7      DACK sense active high
      6      DREQ sense active high
      5      =1 extended write selection
        =0 late write selection
      4      rotating priority instead of fixed priority
      3      compressed timing (two clocks instead of four per transfer)
        =1 normal timing (default)
        =0 compressed timing
      2      =1 enable controller
        =0 enable memory-to-memory
      1-0    channel number
       */
      /* we'll actually do the DMA here. */

      break;
    case 0x09: // write request register (3)
      /*
      Bit(s)  Description     (Table P0003)
      7-3    reserved (0)
      2      =0 clear request bit
        =1 set request bit
      1-0    channel number
        00 channel 0 select
        01 channel 1 select
        10 channel 2 select
        11 channel 3 select
       */
      state.dma_controller[index].writereq=data;
      break;
    case 0x0a: // mask register (4)
      /*
      Bit(s)  Description     (Table P0004)
      7-3    reserved (0)
      2      =0 clear mask bit
        =1 set mask bit
      1-0    channel number
        00 channel 0 select
        01 channel 1 select
        10 channel 2 select
        11 channel 3 select
       */
      state.dma_controller[index].mask=data;
      break;
    case 0x0b: // mode register (5)
      /*
Bit(s)  Description     (Table P0005)
 7-6    transfer mode
        00 demand mode
        01 single mode
        10 block mode
        11 cascade mode
 5      direction
        =0 increment address after each transfer
        =1 decrement address
 3-2    operation
        00 verify operation
        01 write to memory
        10 read from memory
        11 reserved
 1-0    channel number
        00 channel 0 select
        01 channel 1 select
        10 channel 2 select
        11 channel 3 select
       */
      state.dma_controller[index].mode=data;
      break;
    case 0x0c: // clear byte pointer flip-flop register
      break;
    case 0x0d: // dma channel master clear register
      printf("DMA-I-RESET: DMA %d reset.",index);
      break;
    case 0x0e: // clear mask register
      break;
    case 0x0f: // write mask register (6)
      /*      
Bit(s)  Description     (Table P0006)
 7-4    reserved
 3      channel 3 mask bit
 2      channel 2 mask bit
 1      channel 1 mask bit
 0      channel 0 mask bit
Note:   each mask bit is automatically set when the corresponding channel
          reaches terminal count or an extenal EOP sigmal is received
      */
      break;
    }
    break;
  case 2:
    // dma base low page register
    switch(address) {
    case 1:
      num=2;
      break;
    case 2:
      num=3;
      break;
    case 3:
      num=1;
      break;
    case 7:
      num=0;
      break;
    case 9:
      num=6;
      break;
    case 0xa:
      num=7;
      break;
    case 0xb:
      num=5;
      break;
    default:
      printf("DMA Unknown low page register: %x\n",address);
      return;
    }
    state.dma_channel[num].pagebase = (state.dma_channel[num].pagebase & 0xff00) | data;
    break;
  case 3:
    // dma base high page register
    switch(address) {
    case 1:
      num=2;
      break;
    case 2:
      num=3;
      break;
    case 3:
      num=1;
      break;
    case 7:
      num=0;
      break;
    case 9:
      num=6;
      break;
    case 0xa:
      num=7;
      break;
    case 0xb:
      num=5;
      break;
    default:
      printf("DMA Unknown high page register: %x\n",address);
      return;
    }
    state.dma_channel[num].pagebase = (state.dma_channel[num].pagebase & 0xff) | (data<<8);
    break;
  }
}

/**
 * Make sure a clock interrupt is generated on the next clock.
 * used for debugging, or to speed tings up when software is waiting for a clock tick.
 **/

void CAliM1543C::instant_tick()
{
  DoClock();
}

static u32 ali_magic1 = 0xA111543C;
static u32 ali_magic2 = 0xC345111A;

/**
 * Save state to a Virtual Machine State file.
 **/

int CAliM1543C::SaveState(FILE *f)
{
  long ss = sizeof(state);
  int res;

  if (res = CPCIDevice::SaveState(f))
    return res;

  fwrite(&ali_magic1,sizeof(u32),1,f);
  fwrite(&ss,sizeof(long),1,f);
  fwrite(&state,sizeof(state),1,f);
  fwrite(&ali_magic2,sizeof(u32),1,f);
  printf("%s: %d bytes saved.\n",devid_string,ss);
  return 0;
}

/**
 * Restore state from a Virtual Machine State file.
 **/

int CAliM1543C::RestoreState(FILE *f)
{
  long ss;
  u32 m1;
  u32 m2;
  int res;
  size_t r;

  if (res = CPCIDevice::RestoreState(f))
    return res;

  r = fread(&m1,sizeof(u32),1,f);
  if (r!=1)
  {
    printf("%s: unexpected end of file!\n",devid_string);
    return -1;
  }
  if (m1 != ali_magic1)
  {
    printf("%s: MAGIC 1 does not match!\n",devid_string);
    return -1;
  }

  fread(&ss,sizeof(long),1,f);
  if (r!=1)
  {
    printf("%s: unexpected end of file!\n",devid_string);
    return -1;
  }
  if (ss != sizeof(state))
  {
    printf("%s: STRUCT SIZE does not match!\n",devid_string);
    return -1;
  }

  fread(&state,sizeof(state),1,f);
  if (r!=1)
  {
    printf("%s: unexpected end of file!\n",devid_string);
    return -1;
  }

  r = fread(&m2,sizeof(u32),1,f);
  if (r!=1)
  {
    printf("%s: unexpected end of file!\n",devid_string);
    return -1;
  }
  if (m2 != ali_magic2)
  {
    printf("%s: MAGIC 1 does not match!\n",devid_string);
    return -1;
  }

  printf("%s: %d bytes restored.\n",devid_string,ss);
  return 0;
}

 /**
 * Parallel Port information:
 * address 0 (R/W):  data pins.  On read, the last byte written is returned.
 *
 *
 * address 1 (R): status register
 * \code
 *   1 0 0 0 0 0 00 <-- default
 *   ^ ^ ^ ^ ^ ^ ^
 *   | | | | | | +- Undefined
 *   | | | | | +--- IRQ (undefined?)
 *   | | | | +----- printer has error condition
 *   | | | +------- printer is not selected.
 *   | | +--------- printer has paper (online)
 *   | +----------- printer is asserting 'ack'
 *   +------------- printer busy (active low).
 * \endcode
 *
 * address 2 (R/W): control register.
 * \code
 *   00 0 0 1 0 1 1  <-- default
 *   ^  ^ ^ ^ ^ ^ ^
 *   |  | | | | | +-- Strobe (active low)
 *   |  | | | | +---- Auto feed (active low)
 *   |  | | | +------ Initialize
 *   |  | | +-------- Select (active low)
 *   |  | +---------- Interrupt Control
 *   |  +------------ Bidirectional control (unimplemented)
 *   +--------------- Unused
 * \endcode
 **/

void CAliM1543C::lpt_reset() {
  state.lpt_data = ~0;
  state.lpt_status = 0xd8; // busy, ack, online, error
  state.lpt_control = 0x0c; // select, init
  state.lpt_init = false;
}

/**
 * Read a byte from one of the parallel port controller's registers.
 **/

u8 CAliM1543C::lpt_read(u32 address) {
  u8 data = 0;
  switch(address) {
  case 0:
    data=state.lpt_data;
    break;
  case 1:
    data = state.lpt_status;
    if((state.lpt_status & 0x80)==0 && (state.lpt_control & 0x01)==0) {
      if(state.lpt_status & 0x40) { // test ack
  	    state.lpt_status &= ~ 0x40; // turn off ack
      } else {
	    state.lpt_status |= 0x40; // set ack.
	    state.lpt_status |= 0x80; // set (not) busy.
      }
    }
    break;
  case 2:
    data = state.lpt_control;
  }
#ifdef DEBUG_LPT
  printf("%%LPT-I-READ: port %d = %x\n",address,data);
#endif

  return data;
}

/**
 * Write a byte to one of the parallel port controller's registers.
 **/

void CAliM1543C::lpt_write(u32 address, u8 data) {
#ifdef DEBUG_LPT
  printf("%%LPT-I-WRITE: port %d = %x\n",address,data);
#endif
  switch(address) {
  case 0:
    state.lpt_data=data;
    break;
  case 1:
    break;
  case 2:
    if((data & 0x04)==0) {
      state.lpt_init=true;
      state.lpt_status = 0xd8;
    } else {
      if(data & 0x08) { // select bit
	    if(data & 0x01) { // strobe?
	      state.lpt_status &= ~0x80;  // we're busy
	      // do the write!
	      if(lpt && state.lpt_init)
	        fputc(state.lpt_data,lpt);
	      if(state.lpt_control & 0x10) {
	        pic_interrupt(0,7);
	      }
	    } else {
	      // ?
	    }
      }
    }
    state.lpt_control=data;
  }
}

/**
 * Reset keyboard internals.
 **/

void CAliM1543C::kbd_resetinternals(bool powerup)
{
  state.kbd_internal_buffer.num_elements = 0;
  for (int i=0; i<BX_KBD_ELEMENTS; i++)
    state.kbd_internal_buffer.buffer[i] = 0;
  state.kbd_internal_buffer.head = 0;

  state.kbd_internal_buffer.expecting_typematic = 0;
  state.kbd_internal_buffer.expecting_make_break = 0;

  // Default scancode set is mf2 (translation is controlled by the 8042)
  state.kbd_controller.expecting_scancodes_set = 0;
  state.kbd_controller.current_scancodes_set = 1;
  
  if (powerup) {
    state.kbd_internal_buffer.expecting_led_write = 0;
    state.kbd_internal_buffer.delay = 1; // 500 mS
    state.kbd_internal_buffer.repeat_rate = 0x0b; // 10.9 chars/sec
  }
}

/**
 * Enqueue a byte (scancode) into the keyboard buffer.
 **/

void CAliM1543C::kbd_enQ(u8 scancode)
{
  int tail;

#if defined(DEBUG_KBD)
  printf("kbd_enQ(0x%02x)", (unsigned) scancode);
#endif

  if (state.kbd_internal_buffer.num_elements >= BX_KBD_ELEMENTS) {
    printf("internal keyboard buffer full, ignoring scancode.(%02x)  \n",
      (unsigned) scancode);
    return;
  }

  /* enqueue scancode in multibyte internal keyboard buffer */
#if defined(DEBUG_KBD)
  BX_DEBUG(("kbd_enQ: putting scancode 0x%02x in internal buffer",
      (unsigned) scancode));
#endif
  tail = (state.kbd_internal_buffer.head + state.kbd_internal_buffer.num_elements) %
   BX_KBD_ELEMENTS;
  state.kbd_internal_buffer.buffer[tail] = scancode;
  state.kbd_internal_buffer.num_elements++;

  if (!state.kbd_controller.status.outb && state.kbd_controller.kbd_clock_enabled) {
    state.kbd_controller.timer_pending = 1;
    return;
  }
}

/**
 * Read a byte from keyboard port 60.
 **/

u8 CAliM1543C::kbd_60_read()
{
  u8 val;
/* output buffer */
    if (state.kbd_controller.status.auxb) { /* mouse byte available */
      val = state.kbd_controller.aux_output_buffer;
      state.kbd_controller.aux_output_buffer = 0;
      state.kbd_controller.status.outb = 0;
      state.kbd_controller.status.auxb = 0;
      state.kbd_controller.irq12_requested = 0;

      if (state.kbd_controller_Qsize) {
        unsigned i;
        state.kbd_controller.aux_output_buffer = state.kbd_controller_Q[0];
        state.kbd_controller.status.outb = 1;
        state.kbd_controller.status.auxb = 1;
        if (state.kbd_controller.allow_irq12)
          state.kbd_controller.irq12_requested = 1;
        for (i=0; i<state.kbd_controller_Qsize-1; i++) {
          // move Q elements towards head of queue by one
          state.kbd_controller_Q[i] = state.kbd_controller_Q[i+1];
        }
        state.kbd_controller_Qsize--;
      }

      //DEV_pic_lower_irq(12);
      state.kbd_controller.timer_pending = 1;
#if defined(DEBUG_KBD)
      BX_DEBUG(("[mouse] read from 0x60 returns 0x%02x", val));
#endif
      return val;
    }
    else if (state.kbd_controller.status.outb) { /* kbd byte available */
      val = state.kbd_controller.kbd_output_buffer;
      state.kbd_controller.status.outb = 0;
      state.kbd_controller.status.auxb = 0;
      state.kbd_controller.irq1_requested = 0;
      state.kbd_controller.bat_in_progress = 0;

      if (state.kbd_controller_Qsize) {
        unsigned i;
        state.kbd_controller.aux_output_buffer = state.kbd_controller_Q[0];
        state.kbd_controller.status.outb = 1;
        state.kbd_controller.status.auxb = 1;
        if (state.kbd_controller.allow_irq1)
          state.kbd_controller.irq1_requested = 1;
        for (i=0; i<state.kbd_controller_Qsize-1; i++) {
          // move Q elements towards head of queue by one
          state.kbd_controller_Q[i] = state.kbd_controller_Q[i+1];
        }
#if defined(DEBUG_KBD)
	BX_DEBUG(("s.controller_Qsize: %02X",state.kbd_controller_Qsize));
#endif
        state.kbd_controller_Qsize--;
      }

//      DEV_pic_lower_irq(1);
      state.kbd_controller.timer_pending = 1;
#if defined(DEBUG_KBD)
      BX_DEBUG(("READ(60) = %02x", (unsigned) val));
#endif
      return val;
    }
    else {
#if defined(DEBUG_KBD)
      BX_DEBUG(("num_elements = %d", state.kbd_internal_buffer.num_elements));
      BX_DEBUG(("read from port 60h with outb empty"));
#endif
      return state.kbd_controller.kbd_output_buffer;
    }
}

/**
 * Read a byte from keyboard port 64
 **/

u8 CAliM1543C::kbd_64_read()
{
  u8 val;
/* status register */
    val = (state.kbd_controller.status.pare << 7)  |
          (state.kbd_controller.status.tim  << 6)  |
          (state.kbd_controller.status.auxb << 5)  |
          (state.kbd_controller.status.keyl << 4)  |
          (state.kbd_controller.status.c_d  << 3)  |
          (state.kbd_controller.status.sysf << 2)  |
          (state.kbd_controller.status.inpb << 1)  |
          (state.kbd_controller.status.outb << 0);
    state.kbd_controller.status.tim = 0;
    return val;
}

/**
 * Write a byte to keyboard port 60.
 **/

void CAliM1543C::kbd_60_write(u8 value)
{
// input buffer
      // if expecting data byte from command last sent to port 64h
      if (state.kbd_controller.expecting_port60h) {
        state.kbd_controller.expecting_port60h = 0;
        // data byte written last to 0x60
        state.kbd_controller.status.c_d = 0;
#if defined(DEBUG_KBD)
        if (state.kbd_controller.status.inpb)
          printf("write to port 60h, not ready for write   \n");
#endif
        switch (state.kbd_controller.last_comm) {
          case 0x60: // write command byte
            {
            bool scan_convert, disable_keyboard,
                    disable_aux;

            scan_convert = (value >> 6) & 0x01;
            disable_aux      = (value >> 5) & 0x01;
            disable_keyboard = (value >> 4) & 0x01;
            state.kbd_controller.status.sysf = (value >> 2) & 0x01;
            state.kbd_controller.allow_irq1  = (value >> 0) & 0x01;
            state.kbd_controller.allow_irq12 = (value >> 1) & 0x01;
            set_kbd_clock_enable(!disable_keyboard);
            set_aux_clock_enable(!disable_aux);
            if (state.kbd_controller.allow_irq12 && state.kbd_controller.status.auxb)
              state.kbd_controller.irq12_requested = 1;
            else if (state.kbd_controller.allow_irq1  && state.kbd_controller.status.outb)
              state.kbd_controller.irq1_requested = 1;

#if defined(DEBUG_KBD)
            BX_DEBUG(( " allow_irq12 set to %u",
              (unsigned) state.kbd_controller.allow_irq12));
            if (!scan_convert)
              BX_INFO(("keyboard: scan convert turned off"));
#endif
	    // (mch) NT needs this
	    state.kbd_controller.scancodes_translate = scan_convert;
            }
            break;
          case 0xd1: // write output port
#if defined(DEBUG_KBD)
            BX_DEBUG(("write output port with value %02xh", (unsigned) value));
	    BX_DEBUG(("write output port : %sable A20",(value & 0x02)?"en":"dis"));
//            BX_SET_ENABLE_A20((value & 0x02) != 0);
            if (!(value & 0x01))
              BX_INFO(("write output port : processor reset requested!"));
//              bx_pc_system.Reset( BX_RESET_SOFTWARE);
#endif
            break;
          case 0xd4: // Write to mouse
            // I don't think this enables the AUX clock
            //set_aux_clock_enable(1); // enable aux clock line
            kbd_ctrl_to_mouse(value);
            // ??? should I reset to previous value of aux enable?
            break;

          case 0xd3: // write mouse output buffer
            // Queue in mouse output buffer
            kbd_controller_enQ(value, 1);
            break;

	  case 0xd2:
	    // Queue in keyboard output buffer
	    kbd_controller_enQ(value, 0);
	    break;

          default:
            printf("=== unsupported write to port 60h(lastcomm=%02x): %02x   \n",
              (unsigned) state.kbd_controller.last_comm, (unsigned) value);

          }
      } else {
        // data byte written last to 0x60
        state.kbd_controller.status.c_d = 0;
        state.kbd_controller.expecting_port60h = 0;
        /* pass byte to keyboard */
        /* ??? should conditionally pass to mouse device here ??? */
        if (state.kbd_controller.kbd_clock_enabled==0) {
          set_kbd_clock_enable(1);
        }
        kbd_ctrl_to_kbd(value);
      }
      kbd_clock();
}

/**
 * Write a byte to keyboard port 64.
 **/

void CAliM1543C::kbd_64_write(u8 value)
{
  static int kbd_initialized=0;
  u8 command_byte;

// control register
      // command byte written last to 0x64
      state.kbd_controller.status.c_d = 1;
      state.kbd_controller.last_comm = value;
      // most commands NOT expecting port60 write next
      state.kbd_controller.expecting_port60h = 0;

      switch (value) {
        case 0x20: // get keyboard command byte
#if defined(DEBUG_KBD)
          BX_DEBUG(("get keyboard command byte"));
#endif
          // controller output buffer must be empty
          if (state.kbd_controller.status.outb) {
#if defined(DEBUG_KBD)
            BX_ERROR(("kbd: OUTB set and command 0x%02x encountered", value));
#endif
            break;
          }
          command_byte =
            (state.kbd_controller.scancodes_translate << 6) |
            ((!state.kbd_controller.aux_clock_enabled) << 5) |
            ((!state.kbd_controller.kbd_clock_enabled) << 4) |
            (0 << 3) |
            (state.kbd_controller.status.sysf << 2) |
            (state.kbd_controller.allow_irq12 << 1) |
            (state.kbd_controller.allow_irq1  << 0);
          kbd_controller_enQ(command_byte, 0);
          break;
        case 0x60: // write command byte
#if defined(DEBUG_KBD)
          BX_DEBUG(("write command byte"));
#endif
          // following byte written to port 60h is command byte
          state.kbd_controller.expecting_port60h = 1;
          break;

        case 0xa0:
#if defined(DEBUG_KBD)
          BX_DEBUG(("keyboard BIOS name not supported"));
#endif
          break;

        case 0xa1:
#if defined(DEBUG_KBD)
          BX_DEBUG(("keyboard BIOS version not supported"));
#endif
          break;

        case 0xa7: // disable the aux device
          set_aux_clock_enable(0);
#if defined(DEBUG_KBD)
          BX_DEBUG(("aux device disabled"));
#endif
          break;
        case 0xa8: // enable the aux device
          set_aux_clock_enable(1);
#if defined(DEBUG_KBD)
          BX_DEBUG(("aux device enabled"));
#endif
          break;
        case 0xa9: // Test Mouse Port
          // controller output buffer must be empty
          if (state.kbd_controller.status.outb) {
            printf("kbd: OUTB set and command 0x%02x encountered", value);
            break;
          }
          kbd_controller_enQ(0x00, 0); // no errors detected
          break;
        case 0xaa: // motherboard controller self test
#if defined(DEBUG_KBD)
          BX_DEBUG(("Self Test"));
#endif
	  if (kbd_initialized == 0) {
	    state.kbd_controller_Qsize = 0;
	    state.kbd_controller.status.outb = 0;
	    kbd_initialized++;
	  }
          // controller output buffer must be empty
          if (state.kbd_controller.status.outb) {
            printf("kbd: OUTB set and command 0x%02x encountered", value);
            //break;
	        // drain the queue?
	        state.kbd_internal_buffer.head = 0;
	        state.kbd_internal_buffer.num_elements = 0;
	        state.kbd_controller.status.outb = 0;
          }
	  // (mch) Why is this commented out??? Enabling
          state.kbd_controller.status.sysf = 1; // self test complete
          kbd_controller_enQ(0x55, 0); // controller OK
          break;
        case 0xab: // Interface Test
          // controller output buffer must be empty
          if (state.kbd_controller.status.outb) {
            printf("kbd: OUTB set and command 0x%02x encountered", value);
            break;
          }
          kbd_controller_enQ(0x00, 0);
          break;
        case 0xad: // disable keyboard
          set_kbd_clock_enable(0);
#if defined(DEBUG_KBD)
          BX_DEBUG(("keyboard disabled"));
#endif
          break;
        case 0xae: // enable keyboard
          set_kbd_clock_enable(1);
#if defined(DEBUG_KBD)
          BX_DEBUG(("keyboard enabled"));
#endif
          break;
        case 0xaf: // get controller version
#if defined(DEBUG_KBD)
          BX_INFO(("'get controller version' not supported yet"));
#endif
          break;
        case 0xc0: // read input port
          // controller output buffer must be empty
          if (state.kbd_controller.status.outb) {
            BX_PANIC(("kbd: OUTB set and command 0x%02x encountered", value));
            break;
          }
          // keyboard not inhibited
          kbd_controller_enQ(0x80, 0);
          break;
        case 0xd0: // read output port: next byte read from port 60h
#if defined(DEBUG_KBD)
          BX_DEBUG(("io write to port 64h, command d0h (partial)"));
#endif
          // controller output buffer must be empty
          if (state.kbd_controller.status.outb) {
            BX_PANIC(("kbd: OUTB set and command 0x%02x encountered", value));
            break;
          }
          kbd_controller_enQ(
              (state.kbd_controller.irq12_requested << 5) |
              (state.kbd_controller.irq1_requested << 4) |
//              (BX_GET_ENABLE_A20() << 1) |
              0x01, 0);
          break;

        case 0xd1: // write output port: next byte written to port 60h
#if defined(DEBUG_KBD)
          BX_DEBUG(("write output port"));
#endif
          // following byte to port 60h written to output port
          state.kbd_controller.expecting_port60h = 1;
          break;

        case 0xd3: // write mouse output buffer
	  //FIXME: Why was this a panic?
#if defined(DEBUG_KBD)
          BX_DEBUG(("io write 0x64: command = 0xD3(write mouse outb)"));
#endif
	  // following byte to port 60h written to output port as mouse write.
          state.kbd_controller.expecting_port60h = 1;
          break;

        case 0xd4: // write to mouse
#if defined(DEBUG_KBD)
          BX_DEBUG(("io write 0x64: command = 0xD4 (write to mouse)"));
#endif
          // following byte written to port 60h
          state.kbd_controller.expecting_port60h = 1;
          break;

        case 0xd2: // write keyboard output buffer
#if defined(DEBUG_KBD)
      BX_DEBUG(("io write 0x64: write keyboard output buffer"));
#endif
	  state.kbd_controller.expecting_port60h = 1;
	  break;
        case 0xdd: // Disable A20 Address Line
//	  BX_SET_ENABLE_A20(0);
	  break;
        case 0xdf: // Enable A20 Address Line
//	  BX_SET_ENABLE_A20(1);
	  break;
        case 0xc1: // Continuous Input Port Poll, Low
        case 0xc2: // Continuous Input Port Poll, High
        case 0xe0: // Read Test Inputs
          BX_PANIC(("io write 0x64: command = %02xh", (unsigned) value));
          break;

        case 0xfe: // System (cpu?) Reset, transition to real mode
#if defined(DEBUG_KBD)
          BX_INFO(("io write 0x64: command 0xfe: reset cpu"));
#endif
//          bx_pc_system.Reset(BX_RESET_SOFTWARE);
          break;

        default:
          if (value==0xff || (value>=0xf0 && value<=0xfd)) {
            /* useless pulse output bit commands ??? */
#if defined(DEBUG_KBD)
            BX_DEBUG(("io write to port 64h, useless command %02x",
                (unsigned) value));
#endif
            return;
          }
          BX_ERROR(("unsupported io write to keyboard port 64, value = %x",
            (unsigned) value));
          break;
      }
      kbd_clock();
}

/**
 * Enqueue a byte into one of the keyboard controller's output buffers.
 **/

void CAliM1543C::kbd_controller_enQ(u8 data, unsigned source)
{
  // source is 0 for keyboard, 1 for mouse

#if defined(DEBUG_KBD)
  BX_DEBUG(("kbd_controller_enQ(%02x) source=%02x", (unsigned) data,source));
#endif

  // see if we need to Q this byte from the controller
  // remember this includes mouse bytes.
  if (state.kbd_controller.status.outb) {
    if (state.kbd_controller_Qsize >= BX_KBD_CONTROLLER_QSIZE)
      FAILURE("controller_enq(): controller_Q full!");
    state.kbd_controller_Q[state.kbd_controller_Qsize++] = data;
    state.kbd_controller_Qsource = source;
    return;
  }

  // the Q is empty
  if (source == 0) { // keyboard
    state.kbd_controller.kbd_output_buffer = data;
    state.kbd_controller.status.outb = 1;
    state.kbd_controller.status.auxb = 0;
    state.kbd_controller.status.inpb = 0;
    if (state.kbd_controller.allow_irq1)
      state.kbd_controller.irq1_requested = 1;
  } else { // mouse
    state.kbd_controller.aux_output_buffer = data;
    state.kbd_controller.status.outb = 1;
    state.kbd_controller.status.auxb = 1;
    state.kbd_controller.status.inpb = 0;
    if (state.kbd_controller.allow_irq12)
      state.kbd_controller.irq12_requested = 1;
  }
}

/**
 * Enable or disable the keyboard clock.
 **/

void CAliM1543C::set_kbd_clock_enable(u8   value)
{
  bool prev_kbd_clock_enabled;

  if (value==0) 
  {
    state.kbd_controller.kbd_clock_enabled = 0;
  } 
  else 
  {
    /* is another byte waiting to be sent from the keyboard ? */
    prev_kbd_clock_enabled = state.kbd_controller.kbd_clock_enabled;
    state.kbd_controller.kbd_clock_enabled = 1;

    if (prev_kbd_clock_enabled==0 && state.kbd_controller.status.outb==0)
      state.kbd_controller.timer_pending = 1;
  }
}

/**
 * Enable or disable the mouse clock.
 **/

void CAliM1543C::set_aux_clock_enable(u8   value)
{
  bool prev_aux_clock_enabled;

#if defined(DEBUG_KBD)
  BX_DEBUG(("set_aux_clock_enable(%u)", (unsigned) value));
#endif
  if (value==0) 
  {
    state.kbd_controller.aux_clock_enabled = 0;
  } 
  else 
  {
    /* is another byte waiting to be sent from the keyboard ? */
    prev_aux_clock_enabled = state.kbd_controller.aux_clock_enabled;
    state.kbd_controller.aux_clock_enabled = 1;
    if (prev_aux_clock_enabled==0 && state.kbd_controller.status.outb==0)
      state.kbd_controller.timer_pending = 1;
  }
}

/**
 * Send a byte from controller to keyboard
 **/

void CAliM1543C::kbd_ctrl_to_kbd(u8 value)
{
#if defined(DEBUG_KBD)
  BX_DEBUG(("controller passed byte %02xh to keyboard", value));
#endif

  if (state.kbd_internal_buffer.expecting_make_break) {
    state.kbd_internal_buffer.expecting_make_break = 0;
#if defined(DEBUG_KBD)
    printf("setting key %x to make/break mode (unused)   \n", value);
#endif
    kbd_enQ(0xFA); // send ACK
    return;
  }

  if (state.kbd_internal_buffer.expecting_typematic) {
    state.kbd_internal_buffer.expecting_typematic = 0;
    state.kbd_internal_buffer.delay = (value >> 5) & 0x03;
#if defined(DEBUG_KBD)
    switch (state.kbd_internal_buffer.delay) {
      case 0: BX_INFO(("setting delay to 250 mS (unused)")); break;
      case 1: BX_INFO(("setting delay to 500 mS (unused)")); break;
      case 2: BX_INFO(("setting delay to 750 mS (unused)")); break;
      case 3: BX_INFO(("setting delay to 1000 mS (unused)")); break;
    }
#endif
    state.kbd_internal_buffer.repeat_rate = value & 0x1f;
#if defined(DEBUG_KBD)
    double cps = 1 /((double)(8 + (value & 0x07)) * (double)exp(log((double)2) * (double)((value >> 3) & 0x03)) * 0.00417);
    BX_INFO(("setting repeat rate to %.1f cps (unused)", cps));
#endif
    kbd_enQ(0xFA); // send ACK
    return;
  }

  if (state.kbd_internal_buffer.expecting_led_write) {
    state.kbd_internal_buffer.expecting_led_write = 0;
    state.kbd_internal_buffer.led_status = value;
#if defined(DEBUG_KBD)
    BX_DEBUG(("LED status set to %02x",
      (unsigned) state.kbd_internal_buffer.led_status));
#endif
    kbd_enQ(0xFA); // send ACK %%%
    return;
  }

  if (state.kbd_controller.expecting_scancodes_set) {
    state.kbd_controller.expecting_scancodes_set = 0;
    if( value != 0 ) {
      if( value<4 ) {
        state.kbd_controller.current_scancodes_set = (value-1);
#if defined(DEBUG_KBD)
        BX_INFO(("Switched to scancode set %d",
          (unsigned) state.kbd_controller.current_scancodes_set + 1));
#endif
        kbd_enQ(0xFA);
      } else {
        BX_ERROR(("Received scancodes set out of range: %d", value ));
        kbd_enQ(0xFF); // send ERROR
      }
    } else {
      // Send ACK (SF patch #1159626)
      kbd_enQ(0xFA);
      // Send current scancodes set to port 0x60
      kbd_enQ(1 + (state.kbd_controller.current_scancodes_set)); 
    }
    return;
  }

  switch (value) {
    case 0x00: // ??? ignore and let OS timeout with no response
      kbd_enQ(0xFA); // send ACK %%%
      break;

    case 0x05: // ???
      // (mch) trying to get this to work...
      state.kbd_controller.status.sysf = 1;
      kbd_enQ_imm(0xfe);
      break;

    case 0xed: // LED Write
      state.kbd_internal_buffer.expecting_led_write = 1;
      kbd_enQ_imm(0xFA); // send ACK %%%
      break;

    case 0xee: // echo
      kbd_enQ(0xEE); // return same byte (EEh) as echo diagnostic
      break;

    case 0xf0: // Select alternate scan code set
      state.kbd_controller.expecting_scancodes_set = 1;
#if defined(DEBUG_KBD)
      BX_DEBUG(("Expecting scancode set info..."));
#endif
      kbd_enQ(0xFA); // send ACK
      break;

    case 0xf2:  // identify keyboard
#if defined(DEBUG_KBD)
      BX_INFO(("identify keyboard command received"));
#endif

      // XT sends nothing, AT sends ACK 
      // MFII with translation sends ACK+ABh+41h
      // MFII without translation sends ACK+ABh+83h
//      if (SIM->get_param_enum(BXPN_KBD_TYPE)->get() != BX_KBD_XT_TYPE) {
        kbd_enQ(0xFA); 
//        if (SIM->get_param_enum(BXPN_KBD_TYPE)->get() == BX_KBD_MF_TYPE) {
          kbd_enQ(0xAB);
          
          if(state.kbd_controller.scancodes_translate)
            kbd_enQ(0x41);
          else
            kbd_enQ(0x83);
  //      }
//      }
      break;

    case 0xf3:  // typematic info
      state.kbd_internal_buffer.expecting_typematic = 1;
#if defined(DEBUG_KBD)
      BX_INFO(("setting typematic info"));
#endif
      kbd_enQ(0xFA); // send ACK
      break;

    case 0xf4:  // enable keyboard
      state.kbd_internal_buffer.scanning_enabled = 1;
      kbd_enQ(0xFA); // send ACK
      break;

    case 0xf5:  // reset keyboard to power-up settings and disable scanning
      kbd_resetinternals(1);
      kbd_enQ(0xFA); // send ACK
      state.kbd_internal_buffer.scanning_enabled = 0;
#if defined(DEBUG_KBD)
      BX_INFO(("reset-disable command received"));
#endif
      break;

    case 0xf6:  // reset keyboard to power-up settings and enable scanning
      kbd_resetinternals(1);
      kbd_enQ(0xFA); // send ACK
      state.kbd_internal_buffer.scanning_enabled = 1;
#if defined(DEBUG_KBD)
      BX_INFO(("reset-enable command received"));
#endif
      break;

    case 0xfc:  // PS/2 Set Key Type to Make/Break
      state.kbd_internal_buffer.expecting_make_break = 1;
      kbd_enQ(0xFA); /* send ACK */
      break;

    case 0xfe:  // resend. aiiee.
      BX_PANIC(("got 0xFE (resend)"));
      break;

    case 0xff:  // reset: internal keyboard reset and afterwards the BAT
#if defined(DEBUG_KBD)
      BX_DEBUG(("reset command received"));
#endif
      kbd_resetinternals(1);
      kbd_enQ(0xFA); // send ACK
      state.kbd_controller.bat_in_progress = 1;
      kbd_enQ(0xAA); // BAT test passed
      break;

    case 0xd3:
      kbd_enQ(0xfa);
      break;

    case 0xf7:  // PS/2 Set All Keys To Typematic
    case 0xf8:  // PS/2 Set All Keys to Make/Break
    case 0xf9:  // PS/2 PS/2 Set All Keys to Make
    case 0xfa:  // PS/2 Set All Keys to Typematic Make/Break
    case 0xfb:  // PS/2 Set Key Type to Typematic
    case 0xfd:  // PS/2 Set Key Type to Make
    default:
      BX_ERROR(("kbd_ctrl_to_kbd(): got value of 0x%02x", value));
      kbd_enQ(0xFE); /* send NACK */
      break;
  }
}

/**
 * enqueue scancode in multibyte internal keyboard buffer 
 **/

void CAliM1543C::kbd_enQ_imm(u8 val)
{
  int tail;

  if (state.kbd_internal_buffer.num_elements >= BX_KBD_ELEMENTS) 
  {
    BX_PANIC(("internal keyboard buffer full (imm)"));
    return;
  }

  tail = (state.kbd_internal_buffer.head + state.kbd_internal_buffer.num_elements) %
    BX_KBD_ELEMENTS;

  state.kbd_controller.kbd_output_buffer = val;
  state.kbd_controller.status.outb = 1;

  if (state.kbd_controller.allow_irq1)
    state.kbd_controller.irq1_requested = 1;
}

/**
 * Send a byte from controller to mouse
 **/

void CAliM1543C::kbd_ctrl_to_mouse(u8 value)
{
#if defined(DEBUG_KBD)
  BX_DEBUG(("MOUSE: kbd_ctrl_to_mouse(%02xh)", (unsigned) value));
  BX_DEBUG(("  enable = %u", (unsigned) state.mouse.enable));
  BX_DEBUG(("  allow_irq12 = %u",
    (unsigned) state.kbd_controller.allow_irq12));
  BX_DEBUG(("  aux_clock_enabled = %u",
    (unsigned) state.kbd_controller.aux_clock_enabled));
#endif
  // an ACK (0xFA) is always the first response to any valid input
  // received from the system other than Set-Wrap-Mode & Resend-Command

  if (state.kbd_controller.expecting_mouse_parameter) {
    state.kbd_controller.expecting_mouse_parameter = 0;
    switch (state.kbd_controller.last_mouse_command) {
      case 0xf3: // Set Mouse Sample Rate
        state.mouse.sample_rate = value;
#if defined(DEBUG_KBD)
        BX_DEBUG(("[mouse] Sampling rate set: %d Hz", value));
#endif
        if ((value == 200) && (!state.mouse.im_request)) {
          state.mouse.im_request = 1;
        } else if ((value == 100) && (state.mouse.im_request == 1)) {
          state.mouse.im_request = 2;
        } else if ((value == 80) && (state.mouse.im_request == 2)) {
#if defined(DEBUG_KBD)
          BX_INFO(("wheel mouse mode enabled"));
#endif
          state.mouse.im_mode = 1;
          state.mouse.im_request = 0;
        } else {
          state.mouse.im_request = 0;
        }
        kbd_controller_enQ(0xFA, 1); // ack
        break;

      case 0xe8: // Set Mouse Resolution
        switch (value) {
          case 0:
            state.mouse.resolution_cpmm = 1;
            break;
          case 1:
            state.mouse.resolution_cpmm = 2;
            break;
          case 2:
            state.mouse.resolution_cpmm = 4;
            break;
          case 3:
            state.mouse.resolution_cpmm = 8;
            break;
          default:
            BX_PANIC(("[mouse] Unknown resolution %d", value));
            break;
        }
#if defined(DEBUG_KBD)
        BX_DEBUG(("[mouse] Resolution set to %d counts per mm",
          state.mouse.resolution_cpmm));
#endif
        kbd_controller_enQ(0xFA, 1); // ack
        break;

      default:
        BX_PANIC(("MOUSE: unknown last command (%02xh)", (unsigned) state.kbd_controller.last_mouse_command));
    }
  } else {
    state.kbd_controller.expecting_mouse_parameter = 0;
    state.kbd_controller.last_mouse_command = value;

    // test for wrap mode first
    if (state.mouse.mode == MOUSE_MODE_WRAP) {
      // if not a reset command or reset wrap mode
      // then just echo the byte.
      if ((value != 0xff) && (value != 0xec)) {
//        if (bx_dbg.mouse)
#if defined(DEBUG_KBD)
          BX_INFO(("[mouse] wrap mode: Ignoring command %0X02.",value));
#endif
        kbd_controller_enQ(value,1);
        // bail out
        return;
      }
    }
    switch (value) {
      case 0xe6: // Set Mouse Scaling to 1:1
        kbd_controller_enQ(0xFA, 1); // ACK
        state.mouse.scaling = 2;
#if defined(DEBUG_KBD)
        BX_DEBUG(("[mouse] Scaling set to 1:1"));
#endif
        break;

      case 0xe7: // Set Mouse Scaling to 2:1
        kbd_controller_enQ(0xFA, 1); // ACK
        state.mouse.scaling         = 2;
#if defined(DEBUG_KBD)
        BX_DEBUG(("[mouse] Scaling set to 2:1"));
#endif
        break;

      case 0xe8: // Set Mouse Resolution
        kbd_controller_enQ(0xFA, 1); // ACK
        state.kbd_controller.expecting_mouse_parameter = 1;
        break;

      case 0xea: // Set Stream Mode
//        if (bx_dbg.mouse)
#if defined(DEBUG_KBD)
          BX_INFO(("[mouse] Mouse stream mode on."));
#endif
        state.mouse.mode = MOUSE_MODE_STREAM;
        kbd_controller_enQ(0xFA, 1); // ACK
        break;

      case 0xec: // Reset Wrap Mode
        // unless we are in wrap mode ignore the command
        if (state.mouse.mode == MOUSE_MODE_WRAP) {
//          if (bx_dbg.mouse)
#if defined(DEBUG_KBD)
            BX_INFO(("[mouse] Mouse wrap mode off."));
#endif
          // restore previous mode except disable stream mode reporting.
          // ### TODO disabling reporting in stream mode
          state.mouse.mode = state.mouse.saved_mode;
          kbd_controller_enQ(0xFA, 1); // ACK
        }
        break;
      case 0xee: // Set Wrap Mode
        // ### TODO flush output queue.
        // ### TODO disable interrupts if in stream mode.
//        if (bx_dbg.mouse)
#if defined(DEBUG_KBD)
          BX_INFO(("[mouse] Mouse wrap mode on."));
#endif
        state.mouse.saved_mode = state.mouse.mode;
        state.mouse.mode = MOUSE_MODE_WRAP;
        kbd_controller_enQ(0xFA, 1); // ACK
        break;

      case 0xf0: // Set Remote Mode (polling mode, i.e. not stream mode.)
//        if (bx_dbg.mouse)
#if defined(DEBUG_KBD)
          BX_INFO(("[mouse] Mouse remote mode on."));
#endif
        // ### TODO should we flush/discard/ignore any already queued packets?
        state.mouse.mode = MOUSE_MODE_REMOTE;
        kbd_controller_enQ(0xFA, 1); // ACK
        break;

      case 0xf2: // Read Device Type
        kbd_controller_enQ(0xFA, 1); // ACK
        if (state.mouse.im_mode)
          kbd_controller_enQ(0x03, 1); // Device ID (wheel z-mouse)
        else
          kbd_controller_enQ(0x00, 1); // Device ID (standard)
#if defined(DEBUG_KBD)
        BX_DEBUG(("[mouse] Read mouse ID"));
#endif
        break;

      case 0xf3: // Set Mouse Sample Rate (sample rate written to port 60h)
        kbd_controller_enQ(0xFA, 1); // ACK
        state.kbd_controller.expecting_mouse_parameter = 1;
        break;

      case 0xf4: // Enable (in stream mode)
        state.mouse.enable = 1;
        kbd_controller_enQ(0xFA, 1); // ACK
#if defined(DEBUG_KBD)
        BX_DEBUG(("[mouse] Mouse enabled (stream mode)"));
#endif
        break;

      case 0xf5: // Disable (in stream mode)
        state.mouse.enable = 0;
        kbd_controller_enQ(0xFA, 1); // ACK
#if defined(DEBUG_KBD)
        BX_DEBUG(("[mouse] Mouse disabled (stream mode)"));
#endif
        break;

      case 0xf6: // Set Defaults
        state.mouse.sample_rate     = 100; /* reports per second (default) */
        state.mouse.resolution_cpmm = 4; /* 4 counts per millimeter (default) */
        state.mouse.scaling         = 1;   /* 1:1 (default) */
        state.mouse.enable          = 0;
        state.mouse.mode            = MOUSE_MODE_STREAM;
        kbd_controller_enQ(0xFA, 1); // ACK
#if defined(DEBUG_KBD)
        BX_DEBUG(("[mouse] Set Defaults"));
#endif
        break;

      case 0xff: // Reset
        state.mouse.sample_rate     = 100; /* reports per second (default) */
        state.mouse.resolution_cpmm = 4; /* 4 counts per millimeter (default) */
        state.mouse.scaling         = 1;   /* 1:1 (default) */
        state.mouse.mode            = MOUSE_MODE_RESET;
        state.mouse.enable          = 0;
#if defined(DEBUG_KBD)
        if (state.mouse.im_mode)
          BX_INFO(("wheel mouse mode disabled"));
#endif
        state.mouse.im_mode         = 0;
        kbd_controller_enQ(0xFA, 1); // ACK
        kbd_controller_enQ(0xAA, 1); // completion code
        kbd_controller_enQ(0x00, 1); // ID code (standard after reset)
#if defined(DEBUG_KBD)
        BX_DEBUG(("[mouse] Mouse reset"));
#endif
        break;

      case 0xe9: // Get mouse information
        // should we ack here? (mch): Yes
        kbd_controller_enQ(0xFA, 1); // ACK
        kbd_controller_enQ(state.mouse.get_status_byte(), 1); // status
        kbd_controller_enQ(state.mouse.get_resolution_byte(), 1); // resolution
        kbd_controller_enQ(state.mouse.sample_rate, 1); // sample rate
#if defined(DEBUG_KBD)
        BX_DEBUG(("[mouse] Get mouse information"));
#endif
        break;

      case 0xeb: // Read Data (send a packet when in Remote Mode)
        kbd_controller_enQ(0xFA, 1); // ACK
        // perhaps we should be adding some movement here.
        mouse_enQ_packet(((state.mouse.button_status & 0x0f) | 0x08),
          0x00, 0x00, 0x00); // bit3 of first byte always set
        //assumed we really aren't in polling mode, a rather odd assumption.
#if defined(DEBUG_KBD)
        BX_ERROR(("[mouse] Warning: Read Data command partially supported."));
#endif
        break;

      case 0xbb: // OS/2 Warp 3 uses this command
#if defined(DEBUG_KBD)
       BX_ERROR(("[mouse] ignoring 0xbb command"));
#endif
       break;

      default:
        BX_ERROR(("[mouse] kbd_ctrl_to_mouse(): got value of 0x%02x", value));
        kbd_controller_enQ(0xFE, 1); /* send NACK */
    }
  }
}

/**
 * Put a (3/4 byte) mouse packet into the mouse buffer.
 **/

bool CAliM1543C::mouse_enQ_packet(u8 b1, u8 b2, u8 b3, u8 b4)
{
  int bytes = 3;
  if (state.mouse.im_mode) bytes = 4;

  if ((state.mouse_internal_buffer.num_elements + bytes) >= BX_MOUSE_BUFF_SIZE) 
  {
    return(0); /* buffer doesn't have the space */
  }

  mouse_enQ(b1);
  mouse_enQ(b2);
  mouse_enQ(b3);
  if (state.mouse.im_mode) 
    mouse_enQ(b4);

  return(1);
}

/**
 * Put a byte into the mouse buffer.
 **/

void CAliM1543C::mouse_enQ(u8 mouse_data)
{
  int tail;

#if defined(DEBUG_KBD)
  BX_DEBUG(("mouse_enQ(%02x)", (unsigned) mouse_data));
#endif

  if (state.mouse_internal_buffer.num_elements >= BX_MOUSE_BUFF_SIZE) {
    BX_ERROR(("[mouse] internal mouse buffer full, ignoring mouse data.(%02x)",
      (unsigned) mouse_data));
    return;
  }

  /* enqueue mouse data in multibyte internal mouse buffer */
  tail = (state.mouse_internal_buffer.head + state.mouse_internal_buffer.num_elements) %
   BX_MOUSE_BUFF_SIZE;
  state.mouse_internal_buffer.buffer[tail] = mouse_data;
  state.mouse_internal_buffer.num_elements++;

  if (!state.kbd_controller.status.outb && state.kbd_controller.aux_clock_enabled) {
    state.kbd_controller.timer_pending = 1;
    return;
  }
}

/**
 * Determine what IRQ's need to be asserted.
 **/

unsigned CAliM1543C::kbd_periodic()
{
  u8   retval;

  retval = (state.kbd_controller.irq1_requested << 0)
         | (state.kbd_controller.irq12_requested << 1);
  state.kbd_controller.irq1_requested = 0;
  state.kbd_controller.irq12_requested = 0;

  if (state.kbd_controller.timer_pending == 0) {
    return(retval);
  }

  if (1 >= state.kbd_controller.timer_pending) {
    state.kbd_controller.timer_pending = 0;
  } else {
    state.kbd_controller.timer_pending--;
    return(retval);
  }

  if (state.kbd_controller.status.outb) {
    return(retval);
  }

  /* nothing in outb, look for possible data xfer from keyboard or mouse */
  if (state.kbd_internal_buffer.num_elements &&
      (state.kbd_controller.kbd_clock_enabled || state.kbd_controller.bat_in_progress)) {
#if defined(DEBUG_KBD)
    BX_DEBUG(("service_keyboard: key in internal buffer waiting"));
#endif
    state.kbd_controller.kbd_output_buffer =
      state.kbd_internal_buffer.buffer[state.kbd_internal_buffer.head];
    state.kbd_controller.status.outb = 1;
    // commented out since this would override the current state of the
    // mouse buffer flag - no bug seen - just seems wrong (das)
    //    state.kbd_controller.auxb = 0;
    state.kbd_internal_buffer.head = (state.kbd_internal_buffer.head + 1) %
      BX_KBD_ELEMENTS;
    state.kbd_internal_buffer.num_elements--;
    if (state.kbd_controller.allow_irq1)
      state.kbd_controller.irq1_requested = 1;
  } else { 
    create_mouse_packet(0);
    if (state.kbd_controller.aux_clock_enabled && state.mouse_internal_buffer.num_elements) {
#if defined(DEBUG_KBD)
      BX_DEBUG(("service_keyboard: key(from mouse) in internal buffer waiting"));
#endif
      state.kbd_controller.aux_output_buffer =
	state.mouse_internal_buffer.buffer[state.mouse_internal_buffer.head];

      state.kbd_controller.status.outb = 1;
      state.kbd_controller.status.auxb = 1;
      state.mouse_internal_buffer.head = (state.mouse_internal_buffer.head + 1) %
	BX_MOUSE_BUFF_SIZE;
      state.mouse_internal_buffer.num_elements--;
      if (state.kbd_controller.allow_irq12)
	state.kbd_controller.irq12_requested = 1;
    } 
#if defined(DEBUG_KBD)
    else {
      BX_DEBUG(("service_keyboard(): no keys waiting"));
    }
#endif
  }
  return(retval);
}

/**
 * Create a mouse packet and send it to the controller if needed.
 **/

void CAliM1543C::create_mouse_packet(bool force_enq)
{
  u8 b1, b2, b3, b4;

  if(state.mouse_internal_buffer.num_elements && !force_enq)
    return;

  s16 delta_x = state.mouse.delayed_dx;
  s16 delta_y = state.mouse.delayed_dy;
  u8 button_state=state.mouse.button_status | 0x08;

  if(!force_enq && !delta_x && !delta_y) {
    return;
  }

  if(delta_x>254) delta_x=254;
  if(delta_x<-254) delta_x=-254;
  if(delta_y>254) delta_y=254;
  if(delta_y<-254) delta_y=-254;

  b1 = (button_state & 0x0f) | 0x08; // bit3 always set

  if ( (delta_x>=0) && (delta_x<=255) ) {
    b2 = (u8) delta_x;
    state.mouse.delayed_dx-=delta_x;
  }
  else if (delta_x > 255) {
    b2 = (u8) 0xff;
    state.mouse.delayed_dx-=255;
  }
  else if (delta_x >= -256) {
    b2 = (u8) delta_x;
    b1 |= 0x10;
    state.mouse.delayed_dx-=delta_x;
  }
  else {
    b2 = (u8) 0x00;
    b1 |= 0x10;
    state.mouse.delayed_dx+=256;
  }

  if ( (delta_y>=0) && (delta_y<=255) ) {
    b3 = (u8) delta_y;
    state.mouse.delayed_dy-=delta_y;
  }
  else if (delta_y > 255) {
    b3 = (u8) 0xff;
    state.mouse.delayed_dy-=255;
  }
  else if (delta_y >= -256) {
    b3 = (u8) delta_y;
    b1 |= 0x20;
    state.mouse.delayed_dy-=delta_y;
  }
  else {
    b3 = (u8) 0x00;
    b1 |= 0x20;
    state.mouse.delayed_dy+=256;
  }

  b4 = (u8) -state.mouse.delayed_dz;

  mouse_enQ_packet(b1, b2, b3, b4);
}

/**
 * Keyboard clock. Handle events on a clocked basis.
 *
 * Do the following:
 *  - Let the GUI (if available) handle any pending events.
 *  - Check if interrupts need to be asserted.
 *  - Assert interrupts as needed.
 *  .
 **/

void CAliM1543C::kbd_clock()
{
  unsigned retval;

  if (bx_gui)
    bx_gui->handle_events();

  retval=kbd_periodic();

  if(retval&0x01)
    pic_interrupt(0,1);
  if(retval&0x02)
    pic_interrupt(1,4);
}

CAliM1543C * theAli = 0;
