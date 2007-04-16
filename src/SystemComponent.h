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
 * Contains definitions for the base class for devices that connect to the chipset.
 *
 * X-1.9        Camiel Vanderhoeven                             16-APR-2007
 *      Added ResetPCI()
 *
 * X-1.8        Camiel Vanderhoeven                             30-MAR-2007
 *      Added old changelog comments.
 *
 * X-1.7        Camiel Vanderhoeven                             16-FEB-2007
 *      DoClock returns 0.
 *
 * X-1.6        Brian Wheeler                                   13-FEB-2007
 *      Formatting.
 *
 * X-1.5        Camiel Vanderhoeven                             12-FEB-2007
 *      Member cSystem is protected now.
 *
 * X-1.4        Camiel Vanderhoeven                             12-FEB-2007
 *      Added comments.
 *
 * X-1.3        Camiel Vanderhoeven                             9-FEB-2007
 *      Added comments.
 *
 * X-1.2        Brian Wheeler                                   3-FEB-2007
 *      Formatting.
 *
 * X-1.1        Camiel Vanderhoeven                             19-JAN-2007
 *      Initial version in CVS.
 *
 * \author Camiel Vanderhoeven (camiel@camicom.com / http://www.camicom.com)
 **/

#if !defined(INCLUDED_SYSTEMCOMPONENT_H)
#define INCLUDED_SYSTEMCOMPONENT_H

#include "datatypes.h"

/**
 * Base class for devices.
 **/

class CSystemComponent  
{
 public:
  virtual void RestoreState(FILE * f);
  virtual void SaveState(FILE * f);

  CSystemComponent(class CSystem * system);
  virtual ~CSystemComponent();

  //=== abstract ===
  virtual u64 ReadMem(int index, u64 address, int dsize) {return 0;};
  virtual void WriteMem(int index, u64 address, int dsize, u64 data) {};
  virtual u8 ReadTIG(int index, int address) {return 0;};
  virtual void WriteTIG(int index, int address, u8 data) {};
  virtual int DoClock() {return 0;};
  virtual void ResetPCI() {};

 protected: 
  class CSystem * cSystem;
};

#endif // !defined(INCLUDED_SYSTEMCOMPONENT_H)
