/* ES40 emulator.
 * Copyright (C) 2007 by Camiel Vanderhoeven
 *
 * Website: www.camicom.com
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
 * Contains the definitions for the emulated Ali M1543C chipset devices.
 *
 * X-1.15       Brian Wheeler                                   1-DEC-2007
 *      Added console support (using SDL library), corrected timer
 *      behavior for Linux/BSD as a guest OS.
 *
 * X-1.14       Camiel Vanderhoeven                             16-APR-2007
 *      Added ResetPCI()
 *
 * X-1.13       Camiel Vanderhoeven                             11-APR-2007
 *      Moved all data that should be saved to a state file to a structure
 *      "state".
 *
 * X-1.12       Camiel Vanderhoeven                             31-MAR-2007
 *      Added old changelog comments.
 *
 * X-1.11	Camiel Vanderhoeven				3-MAR-2007
 *	Added inline function get_ide_disk, which returns a file handle.
 *
 * X-1.10	Camiel Vanderhoeven				20-FEB-2007
 *	Added member variable to keep error status.
 *
 * X-1.9	Brian Wheeler					20-FEB-2007
 *	Information about IDE disks is now kept in the ide_info structure.
 *
 * X-1.8	Camiel Vanderhoeven				16-FEB-2007
 *	DoClock now returns int.
 *
 * X-1.7	Camiel Vanderhoeven				12-FEB-2007
 *	Formatting.
 *
 * X-1.6	Camiel Vanderhoeven				12-FEB-2007
 *	Added comments.
 *
 * X-1.5        Camiel Vanderhoeven                             9-FEB-2007
 *      Replaced f_ variables with ide_ members.
 *
 * X-1.4        Camiel Vanderhoeven                             9-FEB-2007
 *      Added comments.
 *
 * X-1.3        Brian Wheeler                                   3-FEB-2007
 *      Formatting.
 *
 * X-1.2        Brian Wheeler                                   3-FEB-2007
 *      Includes are now case-correct (necessary on Linux)
 *
 * X-1.1        Camiel Vanderhoeven                             19-JAN-2007
 *      Initial version in CVS.
 *
 * \author Camiel Vanderhoeven (camiel@camicom.com / http://www.camicom.com)
 **/

#if !defined(INCLUDED_ALIM1543C_H_)
#define INCLUDED_ALIM1543C_H

#include "SystemComponent.h"

/**
 * Disk information structure.
 **/

struct disk_info {
  FILE *handle;         /**< disk image handle. */
  char *filename;       /**< disk image filename. */
  int size;           /**< disk image size in 512-byte blocks */  
  int mode;           /**< disk image mode. */
};

/**
 * Emulated ALi M1543C multi-function device.
 * The ALi M1543C device provides i/o and glue logic support to the system: 
 * ISA, USB, IDE, DMA, Interrupt, Timer, TOY Clock. 
 *
 * Known shortcomings:
 *   - IDE
 *     - disk images are not checked for size, so size is not always correctly 
 *       reported.
 *     - IDE disks can be read but not written.
 *     .
 *   .
 **/

class CAliM1543C : public CSystemComponent  
{
 public:
  virtual void SaveState(FILE * f);
  virtual void RestoreState(FILE * f);
  void instant_tick();
  //	void interrupt(int number);
  virtual int DoClock();
  virtual void WriteMem(int index, u64 address, int dsize, u64 data);

  virtual u64 ReadMem(int index, u64 address, int dsize);
  CAliM1543C(class CSystem * c);
  virtual ~CAliM1543C();
  void pic_interrupt(int index, int intno);
  FILE * get_ide_disk(int controller, int drive);
  virtual void ResetPCI();

  void kb_enqueue(u8 data, int type);
  u8 kb_dequeue();
  bool kb_ready();


 private:

  // REGISTERS 60 & 64: KEYBOARD
  u8 kb_read(u64 address);
  void kb_write(u64 address, u8 data);

  // REGISTER 61 (NMI)
  u8 reg_61_read();
  void reg_61_write(u8 data);

  // REGISTERS 70 - 73: TOY
  u8 toy_read(u64 address);
  void toy_write(u64 address, u8 data);

  // ISA bridge
  u64 isa_config_read(u64 address, int dsize);
  void isa_config_write(u64 address, int dsize, u64 data);

  // Timer/Counter
  u8 pit_read(u64 address);
  void pit_write(u64 address, u8 data);

  // interrupt controller
  u8 pic_read(int index, u64 address);
  void pic_write(int index, u64 address, u8 data);
  u8 pic_read_vector();

  // IDE controller
  u64 ide_config_read(u64 address, int dsize);
  void ide_config_write(u64 address, int dsize, u64 data);
  u64 ide_command_read(int channel, u64 address);
  void ide_command_write(int channel, u64 address, u64 data);
  u64 ide_control_read(int channel, u64 address);
  void ide_control_write(int channel, u64 address, u64 data);
  u64 ide_busmaster_read(int channel, u64 address);
  void ide_busmaster_write(int channel, u64 address, u64 data);

  // USB host controller
  u64 usb_config_read(u64 address, int dsize);
  void usb_config_write(u64 address, int dsize, u64 data);

  // DMA controller
  u8 dma_read(int channel, u64 address);
  void dma_write(int channel, u64 address, u8 data);

#ifdef USE_CONSOLE
  u8 key2scan(int key);
#endif


  // The state structure contains all elements that need to be saved to the statefile.
  struct SAliM1543CState {
    // REGISTERS 60 & 64: KEYBOARD / MOUSE
    u8 kb_Input;
    u8 kb_Output;   	
    u8 kb_Status;   	
    u8 kb_Command;
    u8 kb_intState;
    u8 kb_buffer[32];
    u64 kb_head, kb_tail;
    u8 kb_dataPending;
    bool kb_mode;

    // REGISTER 61 (NMI)
    u8 reg_61;
    
    // REGISTERS 70 - 73: TOY
    u8 toy_stored_data[256];
    u8 toy_access_ports[4];

    // ISA bridge
    u8 isa_config_data[256];
    u8 isa_config_mask[256];

    // Timer/Counter
    bool pit_enable;

    // interrupt controller
    int pic_mode[2];
    u8 pic_intvec[2];
    u8 pic_mask[2];
    u8 pic_asserted[2];

    // IDE controller
    u8 ide_config_data[256];
    u8 ide_config_mask[256];
    u8 ide_command[2][8];
    u8 ide_status[2];
    u8 ide_error[2];
    u16 ide_data[2][256];
    int ide_data_ptr[2];
    bool ide_writing[2];
    bool ide_reading[2];
    int ide_sectors[2];
    int ide_selected[2];

    // USB host controller
    u8 usb_config_data[256];
    u8 usb_config_mask[256];
  } state;

  struct disk_info ide_info[2][2];
};

inline FILE * CAliM1543C::get_ide_disk(int controller, int drive)
{
  return ide_info[controller][drive].handle;
}

#endif // !defined(INCLUDED_ALIM1543C_H)
