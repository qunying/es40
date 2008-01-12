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
 * Contains code for the disk base class.
 *
 * $Id$
 *
 * X-1.10       Camiel Vanderhoeven                             12-JAN-2008
 *      Include SCSI engine, because this is common to both SCSI and ATAPI
 *      devices.
 *
 * X-1.9        Camiel Vanderhoeven                             09-JAN-2008
 *      Save disk state to state file.
 *
 * X-1.8        Camiel Vanderhoeven                             06-JAN-2008
 *      Set default blocksize to 2048 for cd-rom devices.
 *
 * X-1.7        Camiel Vanderhoeven                             06-JAN-2008
 *      Support changing the block size (required for SCSI, ATAPI).
 *
 * X-1.6        Camiel Vanderhoeven                             02-JAN-2008
 *      Cleanup.
 *
 * X-1.5        Camiel Vanderhoeven                             29-DEC-2007
 *      Fix memory-leak.
 *
 * X-1.4        Camiel Vanderhoeven                             28-DEC-2007
 *      Throw exceptions rather than just exiting when errors occur.
 *
 * X-1.3        Camiel Vanderhoeven                             28-DEC-2007
 *      Keep the compiler happy.
 *
 * X-1.2        Brian Wheeler                                   16-DEC-2007
 *      Fixed case of StdAfx.h.
 *
 * X-1.1        Camiel Vanderhoeven                             12-DEC-2007
 *      Initial version in CVS.
 **/

#include "StdAfx.h" 
#include "Disk.h"

/**
 * \brief Constructor.
 **/
CDisk::CDisk(CConfigurator * cfg, CSystem * sys, CDiskController * ctrl, int idebus, int idedev) : CSystemComponent(cfg, sys)
{
  char * a;
  char * b;
  char * c;
  char * d;

  myCfg = cfg;
  myCtrl = ctrl;
  myBus = idebus;
  myDev = idedev;
  atapi_mode = false;

  a = myCfg->get_myName();
  b = myCfg->get_myValue();
  c = myCfg->get_myParent()->get_myName();
  d = myCfg->get_myParent()->get_myValue();

  free(devid_string); // we override the default to include the controller.
  CHECK_ALLOCATION(devid_string = (char*) malloc(strlen(a)+strlen(b)+strlen(c)+strlen(d)+6));
  sprintf(devid_string,"%s(%s).%s(%s)",c,d,a,b);

  serial_number = myCfg->get_text_value("serial_num", "ES40EM00000");
  revision_number = myCfg->get_text_value("rev_num", "0.0");
  read_only = myCfg->get_bool_value("read_only");
  is_cdrom = myCfg->get_bool_value("cdrom");

  state.block_size = is_cdrom?2048:512;

  myCtrl->register_disk(this,myBus,myDev);
}

/**
 * \brief Destructor.
 **/
CDisk::~CDisk(void)
{
  free(devid_string);
}

/**
 * \Calculate the number of cylinders to report.
 **/
void CDisk::calc_cylinders()
{
  cylinders = byte_size/state.block_size/sectors/heads;

  off_t_large chs_size = sectors*cylinders*heads*state.block_size;
  if (chs_size<byte_size)
    cylinders++;
}

/**
 * \brief Called when this device is selected.
 *
 * Set status fields up to begin a new SCSI command sequence
 * and set the SCSI bus phase to Message Out.
 **/
void CDisk::scsi_select_me(int bus)
{
  state.scsi.msgo.written = 0;
  state.scsi.msgi.available = 0;
  state.scsi.msgi.read = 0;
  state.scsi.cmd.written = 0;
  state.scsi.dati.available = 0;
  state.scsi.dati.read = 0;
  state.scsi.dato.expected = 0;
  state.scsi.dato.written = 0;
  state.scsi.stat.available = 0;
  state.scsi.stat.read = 0;
  state.scsi.lun_selected = false;
  //state.scsi.disconnect_priv = false;
  //state.scsi.will_disconnect = false;
  //state.scsi.disconnected = false;
  if (atapi_mode)
    scsi_set_phase(bus, SCSI_PHASE_COMMAND);
  else
    scsi_set_phase(bus, SCSI_PHASE_MSG_OUT);
}

static u32 disk_magic1 = 0xD15D15D1;
static u32 disk_magic2 = 0x15D15D5;

/**
 * Save state to a Virtual Machine State file.
 **/

int CDisk::SaveState(FILE *f)
{
  long ss = sizeof(state);

  fwrite(&disk_magic1,sizeof(u32),1,f);
  fwrite(&ss,sizeof(long),1,f);
  fwrite(&state,sizeof(state),1,f);
  fwrite(&disk_magic2,sizeof(u32),1,f);
  printf("%s: %d bytes saved.\n",devid_string,ss);
  return 0;
}

/**
 * Restore state from a Virtual Machine State file.
 **/

int CDisk::RestoreState(FILE *f)
{
  long ss;
  u32 m1;
  u32 m2;
  size_t r;

  r = fread(&m1,sizeof(u32),1,f);
  if (r!=1)
  {
    printf("%s: unexpected end of file!\n",devid_string);
    return -1;
  }
  if (m1 != disk_magic1)
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
  if (m2 != disk_magic2)
  {
    printf("%s: MAGIC 1 does not match!\n",devid_string);
    return -1;
  }

  calc_cylinders(); // state.block_size may have changed.

  printf("%s: %d bytes restored.\n",devid_string,ss);
  return 0;
}

/**
 * \brief Return the number of bytes expected or available.
 *
 * Return the number of bytes we still expect to receive
 * from the initiator, or still have available for the
 * initiator, in the current SCSI phase.
 *
 * For an overview of data transfer during a SCSI bus phase,
 * see SCSIDevice::scsi_xfer_ptr.
 **/

size_t CDisk::scsi_expected_xfer_me(int bus)
{
  switch (scsi_get_phase(0))
  {
  case SCSI_PHASE_DATA_OUT:
    return state.scsi.dato.expected - state.scsi.dato.written;
  case SCSI_PHASE_DATA_IN:
    return state.scsi.dati.available - state.scsi.dati.read;
  case SCSI_PHASE_COMMAND:
    return 256 - state.scsi.cmd.written;
  case SCSI_PHASE_STATUS:
    return state.scsi.stat.available - state.scsi.stat.read;
  case SCSI_PHASE_MSG_OUT:
    return 256 - state.scsi.msgo.written;
  case SCSI_PHASE_MSG_IN:
    return state.scsi.msgi.available - state.scsi.msgi.read;
  default:
    printf("transfer requested in phase %d\n",scsi_get_phase(0));
    FAILURE("invalid SCSI phase");
  }
}

/**
 * \brief Return a pointer where the initiator can read or write data.
 *
 * Return a pointer to where the initiator can read or write
 * (the remainder of) our data in the current SCSI phase.
 *
 * For an overview of data transfer during a SCSI bus phase,
 * see SCSIDevice::scsi_xfer_ptr.
 **/

void * CDisk::scsi_xfer_ptr_me(int bus, size_t bytes)
{
  void * res = 0;

  switch (scsi_get_phase(0))
  {
  case SCSI_PHASE_DATA_OUT:
    res = &(state.scsi.dato.data[state.scsi.dato.written]);
    state.scsi.dato.written += bytes;
    break;

  case SCSI_PHASE_DATA_IN:
    res = &(state.scsi.dati.data[state.scsi.dati.read]);
    state.scsi.dati.read += bytes;
    break;

  case SCSI_PHASE_COMMAND:
    res = &(state.scsi.cmd.data[state.scsi.cmd.written]);
    state.scsi.cmd.written += bytes;
    break;

  case SCSI_PHASE_STATUS:
    res = &(state.scsi.stat.data[state.scsi.stat.read]);
    state.scsi.stat.read += bytes;
    break;

  case SCSI_PHASE_MSG_OUT:
    res = &(state.scsi.msgo.data[state.scsi.msgo.written]);
    state.scsi.msgo.written += bytes;
    break;

  case SCSI_PHASE_MSG_IN:
    //if (PT.reselected)
    //{
    //  retval = 0x80; // identify
    //  break;
    //}

    //if (PT.disconnected)
    //{
    //  if (!PT.dati_ptr)
    //    retval = 0x04; // disconnect
    //  else
    //  {
    //    if (state.scsi.msgi.read==0)
    //    {
    //      retval = 0x02; // save data pointer
    //      state.scsi.msgi.read=1;
    //    }
    //    else if (state.scsi.msgi.read==1)
    //    {
    //      retval = 0x04; // disconnect
    //      state.scsi.msgi.read=0;
    //    }
    //  }
    //  break;
    //}
    res = &(state.scsi.msgi.data[state.scsi.msgi.read]);
    state.scsi.msgi.read += bytes;
    break;

  default:
    printf("transfer requested in phase %d\n",scsi_get_phase(0));
    FAILURE("invalid SCSI phase");
  }

  return res;
}

/**
 * \brief Process data written or read.
 *
 * Determine what action (if any) should be taken after a
 * transfer, and what the next SCSI bus phase should be.
 *
 * For an overview of data transfer during a SCSI bus phase,
 * see SCSIDevice::scsi_xfer_ptr.
 *
 * \todo Handle disconnect/reconnect properly.
 **/
void CDisk::scsi_xfer_done_me(int bus)
{
  int res;
  int newphase = scsi_get_phase(0);

  switch (scsi_get_phase(0))
  {
  case SCSI_PHASE_DATA_OUT:
    if (state.scsi.dato.written < state.scsi.dato.expected)
      break;

    res = do_scsi_command();
    if (res == 2)
      FAILURE("do_command returned 2 after DATA OUT phase");

    if (state.scsi.dati.available)
      newphase = SCSI_PHASE_DATA_IN;
    else
      newphase = SCSI_PHASE_STATUS;
    break;

  case SCSI_PHASE_DATA_IN:
    if (state.scsi.dati.read < state.scsi.dati.available)
      break;

    newphase = SCSI_PHASE_STATUS;
    break;

  case SCSI_PHASE_COMMAND:
    res = do_scsi_command();
    if (res == 2)
      newphase = SCSI_PHASE_DATA_OUT;
    else if (state.scsi.dati.available)
      newphase = SCSI_PHASE_DATA_IN;
    else
      newphase = SCSI_PHASE_STATUS;
    break;

  case SCSI_PHASE_STATUS:
    if (state.scsi.stat.read < state.scsi.stat.available)
      break;

    if (atapi_mode)
    {
      scsi_free(0);
      return;
    }

    newphase = SCSI_PHASE_MSG_IN;
    break;

  case SCSI_PHASE_MSG_OUT:
    newphase = do_scsi_message(); // command
    break;

  case SCSI_PHASE_MSG_IN:
    //if (state.scsi.reselected)
    //{
    //  state.scsi.reselected = false;
    //  newphase = state.scsi.disconnect_phase;
    //}
    //else if (state.scsi.disconnected)
    //{
    //  if (!state.scsi.msgi.read)
    //    newphase = -1;
    //}
    if (state.scsi.msgi.read < state.scsi.msgi.available)
      break;

    if (state.scsi.cmd.written)
    {
      scsi_free(0);
      return;
    }
    else
        newphase = SCSI_PHASE_COMMAND;
    break;

  default:
    printf("transfer requested in phase %d\n",scsi_get_phase(0));
    FAILURE("invalid SCSI phase");
  }

  // if data in and can disconnect...
  //if (state.phase!=7 && newphase==1 && PT.will_disconnect && !PT.disconnected)
  //{
  //  //printf("SYM: Disconnecting now...\n");
  //  PT.disconnected = true;
  //  PT.disconnect_phase = newphase;
  //  newphase = 7; // msg in
  //}

  if (newphase != scsi_get_phase(0))
  {
//    if (newphase==-1)
//    {
//      //printf("SYM: Disconnect. Timer started!\n");
//      // disconnect. generate interrupt?
//      state.disconnected = 20;
//    }
    scsi_set_phase(0, newphase);
  }
  //getchar();
}

// SCSI commands: 

#define	SCSICMD_TEST_UNIT_READY		  0x00	
#define	SCSICMD_REQUEST_SENSE		  0x03	
#define	SCSICMD_INQUIRY			      0x12	

#define	SCSICMD_READ			      0x08
#define	SCSICMD_READ_10			      0x28
#define SCSICMD_READ_12               0xA8
#define SCSICMD_READ_16               0x88
#define SCSICMD_READ_32               0x7F
#define SCSICMD_READ_LONG             0x3E
#define SCSICMD_READ_CD               0xBE

#define	SCSICMD_WRITE			      0x0A
#define	SCSICMD_WRITE_10		      0x2A
#define SCSICMD_WRITE_12              0xAA
#define SCSICMD_WRITE_LONG            0x3F

#define	SCSICMD_MODE_SELECT		      0x15
#define	SCSICMD_MODE_SENSE		      0x1a
#define	SCSICMD_START_STOP_UNIT		  0x1b
#define	SCSICMD_PREVENT_ALLOW_REMOVE  0x1e
#define	SCSICMD_MODE_SENSE_10		  0x5a

#define	SCSICMD_SYNCHRONIZE_CACHE	  0x35

//  SCSI block device commands:  
#define	SCSIBLOCKCMD_READ_CAPACITY	  0x25

//  SCSI CD-ROM commands:  
#define	SCSICDROM_READ_SUBCHANNEL	  0x42
#define	SCSICDROM_READ_TOC		      0x43
#define	SCSICDROM_READ_DISCINFO		  0x51
#define	SCSICDROM_READ_TRACKINFO	  0x52


//  SCSI tape commands:  
#define	SCSICMD_REWIND			      0x01
#define	SCSICMD_READ_BLOCK_LIMITS	  0x05
#define	SCSICMD_SPACE			      0x11

// SCSI mode pages:
#define SCSIMP_VENDOR                 0x00
#define SCSIMP_READ_WRITE_ERRREC      0x01
#define SCSIMP_DISCONNECT_RECONNECT   0x02
#define SCSIMP_FORMAT_PARAMS          0x03
#define SCSIMP_RIGID_GEOMETRY         0x04
#define SCSIMP_FLEX_PARAMS            0x05
#define SCSIMP_CDROM_CAP              0x2A

/**
 * \brief Handle a SCSI command.
 *
 * Called when a SCSI command has been received. We parse
 * the command, and set up the state for the data in or
 * data out phases.
 *
 * If a data out phase is required, we return the value 2
 * to indicate this. In that case, do_scsi_command will be
 * called again once the data out has been received from
 * the initiator.
 **/
int CDisk::do_scsi_command()
{
  unsigned int retlen;
  int q;
  int pagecode;
  u32 ofs;

  printf("%s: %d-byte command ",devid_string,state.scsi.cmd.written);
  for (unsigned int x=0;x<state.scsi.cmd.written;x++) printf("%02x ",state.scsi.cmd.data[x]);
  printf("\n");

  if (state.scsi.cmd.written<1)
    return 0;

  if (state.scsi.cmd.data[1] & 0xe0)
  {
    printf("%s: LUN selected...\n",devid_string);
    state.scsi.lun_selected = true;
  }

  if (state.scsi.lun_selected && state.scsi.cmd.data[0] != SCSICMD_INQUIRY && state.scsi.cmd.data[0] != SCSICMD_REQUEST_SENSE)
  {
    printf("%s: LUN not supported!\n",devid_string);
    //printf(">");
    //getchar();
  }

  switch(state.scsi.cmd.data[0])
  {
  case SCSICMD_TEST_UNIT_READY:
    //printf("%s: TEST UNIT READY.\n",devid_string);
	if (state.scsi.cmd.data[1] != 0x00) 
    {
      printf("%s: Don't know how to handle TEST UNIT READY with cmd[1]=0x%02x.\n", devid_string, state.scsi.cmd.data[1]);
      break;
	}
    
    state.scsi.stat.available = 1;
    state.scsi.stat.data[0] = 0;
    state.scsi.stat.read = 0;
    state.scsi.msgi.available = 1;
    state.scsi.msgi.data[0] = 0;
    state.scsi.msgi.read = 0;
    break;

  case SCSICMD_INQUIRY:
    {
      //printf("SYM.%d: INQUIRY.\n",GET_DEST());
	  if ((state.scsi.cmd.data[1] & 0x1e) != 0x00) 
      {
        printf("%s: Don't know how to handle INQUIRY with cmd[1]=0x%02x.\n", devid_string, state.scsi.cmd.data[1]);
        //printf(">");
        //getchar();
        break;
	  }
      u8 qual_dev = state.scsi.lun_selected ? 0x7F : (cdrom() ? 0x05 : 0x00);

      retlen = state.scsi.cmd.data[4];
      state.scsi.dati.data[0] = qual_dev; // device type

      if (state.scsi.cmd.data[1] & 0x01)
      {
        // Vital Product Data
        if (state.scsi.cmd.data[2] == 0x80)
        {
          char serial_number[20];
          sprintf(serial_number,"SRL%04x",scsi_initiator_id[0]*0x0101);
          // unit serial number page
          state.scsi.dati.data[1] = 0x80; // page code: 0x80
          state.scsi.dati.data[2] = 0x00; // reserved
          state.scsi.dati.data[3] = (u8)strlen(serial_number);
          memcpy(&state.scsi.dati.data[4],serial_number,strlen(serial_number));
        }
        else
        {
          //printf("Don't know format for vital product data page %02x!!\n",state.scsi.cmd.data[2]);
          //printf(">");
          //getchar();
          state.scsi.dati.data[1] = state.scsi.cmd.data[2]; // page code
          state.scsi.dati.data[2] = 0x00; // reserved
        }
      }
      else
      {
        //  Return values: 
        if (retlen < 36) {
	        printf("%s: SCSI inquiry len=%i, <36!\n", devid_string, retlen);
	        retlen = 36;
        }
        state.scsi.dati.data[1] = 0; // not removable;
        state.scsi.dati.data[2] = 0x02; // ANSI scsi 2
        state.scsi.dati.data[3] = 0x02; // response format
        state.scsi.dati.data[4] = 32; // additional length
        state.scsi.dati.data[5] = 0; // reserved
        state.scsi.dati.data[6] = 0x04; // reserved
        state.scsi.dati.data[7] = 0x60; // capabilities
    //                        vendor  model           rev.
        memcpy(&(state.scsi.dati.data[8]),"DEC     RZ58     (C) DEC2000",28);

        //  Some data is different for CD-ROM drives:  
        if (cdrom()) {
          state.scsi.dati.data[1] = 0x80;  //  0x80 = removable  
    //                           vendor  model           rev.
	      memcpy(&(state.scsi.dati.data[8]), "DEC     RRD42   (C) DEC 4.5d",28);
        }
      }

      state.scsi.dati.read = 0;
      state.scsi.dati.available = retlen;

      state.scsi.stat.available = 1;
      state.scsi.stat.data[0] = 0;
      state.scsi.stat.read = 0;
      state.scsi.msgi.available = 1;
      state.scsi.msgi.data[0] = 0;
      state.scsi.msgi.read = 0;
    }
    break;

  case SCSICMD_MODE_SENSE:
  case SCSICMD_MODE_SENSE_10:	
    //printf("SYM.%d: MODE SENSE.\n",GET_DEST());
	if (state.scsi.cmd.data[0] == SCSICMD_MODE_SENSE)
    {
      q = 4; 
      retlen = state.scsi.cmd.data[4];
    }
    else
    {
      q = 8;
	  retlen = state.scsi.cmd.data[7] * 256 + state.scsi.cmd.data[8];
	}
    
	if ((state.scsi.cmd.data[2] & 0xc0) != 0)
    {
      printf(" mode sense, cmd[2] = 0x%02x.\n", state.scsi.cmd.data[2]);
      throw((int)1);
    }

	//  Return data:  

    state.scsi.dati.available = retlen;	//  Restore size.  

	pagecode = state.scsi.cmd.data[2] & 0x3f;

	//printf("[ MODE SENSE pagecode=%i ]\n", pagecode);

	//  4 bytes of header for 6-byte command,
	//  8 bytes of header for 10-byte command.  
    state.scsi.dati.data[0] = retlen;	//  0: mode data length  
    state.scsi.dati.data[1] = cdrom() ? 0x05 : 0x00; //  1: medium type  
	state.scsi.dati.data[2] = 0x00;	//  device specific parameter  
	state.scsi.dati.data[3] = 8 * 1;	//  block descriptor length: 1 page (?)  

	state.scsi.dati.data[q+0] = 0x00;	//  density code  
	state.scsi.dati.data[q+1] = 0;	//  nr of blocks, high  
	state.scsi.dati.data[q+2] = 0;	//  nr of blocks, mid  
	state.scsi.dati.data[q+3] = 0;	//  nr of blocks, low 
	state.scsi.dati.data[q+4] = 0x00;	//  reserved  
    state.scsi.dati.data[q+5] = (u8)(get_block_size() >> 16) & 255;
	state.scsi.dati.data[q+6] = (u8)(get_block_size() >>  8) & 255;
	state.scsi.dati.data[q+7] = (u8)(get_block_size() >>  0) & 255;
	q += 8;

    state.scsi.stat.available = 1;
    state.scsi.stat.data[0] = 0;
    state.scsi.stat.read = 0;
    state.scsi.msgi.available = 1;
    state.scsi.msgi.data[0] = 0;
    state.scsi.msgi.read = 0;

	//  descriptors, 8 bytes (each)  

	//  page, n bytes (each)  
	switch (pagecode) {
	case SCSIMP_VENDOR: // vendor specific
		//  TODO: Nothing here?  
		break;
	case SCSIMP_READ_WRITE_ERRREC:		//  read-write error recovery page  
		state.scsi.dati.data[q + 0] = pagecode;
		state.scsi.dati.data[q + 1] = 10;
		break;
	case SCSIMP_FORMAT_PARAMS:		//  format device page  
		state.scsi.dati.data[q + 0] = pagecode;
		state.scsi.dati.data[q + 1] = 22;

		//  10,11 = sectors per track  
		state.scsi.dati.data[q + 10] = 0;
        state.scsi.dati.data[q + 11] = (u8)get_sectors();

		///  12,13 = physical sector size 
		state.scsi.dati.data[q + 12] = (u8)(get_block_size() >> 8) & 255;
		state.scsi.dati.data[q + 13] = (u8)(get_block_size() >> 0) & 255;
		break;
	case SCSIMP_RIGID_GEOMETRY:		//  rigid disk geometry page  
		state.scsi.dati.data[q + 0] = pagecode;
		state.scsi.dati.data[q + 1] = 22;
        state.scsi.dati.data[q + 2] = (u8)(get_cylinders() >> 16) & 255;
		state.scsi.dati.data[q + 3] = (u8)(get_cylinders() >> 8) & 255;
        state.scsi.dati.data[q + 4] = (u8)get_cylinders() & 255;
        state.scsi.dati.data[q + 5] = (u8)get_heads();

        //rpms
		state.scsi.dati.data[q + 20] = (7200 >> 8) & 255;
		state.scsi.dati.data[q + 21] = 7200 & 255;
		break;
	case SCSIMP_FLEX_PARAMS:		//  flexible disk page  
		state.scsi.dati.data[q + 0] = pagecode;
		state.scsi.dati.data[q + 1] = 0x1e;

		//  2,3 = transfer rate  
		state.scsi.dati.data[q + 2] = ((5000) >> 8) & 255;
		state.scsi.dati.data[q + 3] = (5000) & 255;

		state.scsi.dati.data[q + 4] = (u8)get_heads();
		state.scsi.dati.data[q + 5] = (u8)get_sectors();

		//  6,7 = data bytes per sector  
		state.scsi.dati.data[q + 6] = (u8)(get_block_size() >> 8) & 255;
		state.scsi.dati.data[q + 7] = (u8)(get_block_size() >> 0) & 255;

		state.scsi.dati.data[q + 8] = (u8)(get_cylinders() >> 8) & 255;
		state.scsi.dati.data[q + 9] = (u8)get_cylinders() & 255;

        //rpms
		state.scsi.dati.data[q + 28] = (7200 >> 8) & 255;
		state.scsi.dati.data[q + 29] = 7200 & 255;
		break;
    case SCSIMP_CDROM_CAP: // CD-ROM capabilities
      state.scsi.dati.data[q + 0] = pagecode;
      state.scsi.dati.data[q + 1] = 0x14; // length
      state.scsi.dati.data[q + 2] = 0x03; // read CD-R/CD-RW
      state.scsi.dati.data[q + 3] = 0x00; // no write
      state.scsi.dati.data[q + 4] = 0x00; // dvd/audio capabilities
      state.scsi.dati.data[q + 5] = 0x00; // cd-da capabilities
      state.scsi.dati.data[q + 6] = state.scsi.locked?0x23:0x21; // tray-loader
      state.scsi.dati.data[q + 7] = 0x00;
      state.scsi.dati.data[q + 8] = (u8)(2800 >> 8); // max read speed in kBps (2.8Mbps = 16x)
      state.scsi.dati.data[q + 9] = (u8)(2800 >> 0);
      state.scsi.dati.data[q + 10] = (u8)(0 >> 8); // number of volume levels
      state.scsi.dati.data[q + 11] = (u8)(0 >> 0);
      state.scsi.dati.data[q + 12] = (u8)(64 >> 8);  // buffer size in KBytes
      state.scsi.dati.data[q + 13] = (u8)(64 >> 0);
      state.scsi.dati.data[q + 14] = (u8)(2800 >> 8); // current read speed
      state.scsi.dati.data[q + 15] = (u8)(2800 >> 0);
      state.scsi.dati.data[q + 16] = 0; // reserved
      state.scsi.dati.data[q + 17] = 0; // digital output format
      state.scsi.dati.data[q + 18] = (u8)(0 >> 8); // max write speed
      state.scsi.dati.data[q + 19] = (u8)(0 >> 0);
      state.scsi.dati.data[q + 20] = (u8)(0 >> 8); // current write speed
      state.scsi.dati.data[q + 21] = (u8)(0 >> 0);
      break;
	default:
		printf("[ MODE_SENSE for page %i is not yet implemented! ]\n", pagecode);
        throw((int)1);
	}
	break;

  case SCSICMD_PREVENT_ALLOW_REMOVE:
    if (state.scsi.cmd.data[4] & 1)
    {
      state.scsi.locked = true;
    //  printf("SYM.%d: PREVENT MEDIA REMOVAL.\n",GET_DEST());
    }
    else
    {
      state.scsi.locked = false;
    //  printf("SYM.%d: ALLOW MEDIA REMOVAL.\n",GET_DEST());
    }
    
    state.scsi.stat.available = 1;
    state.scsi.stat.data[0] = 0;
    state.scsi.stat.read = 0;
    state.scsi.msgi.available = 1;
    state.scsi.msgi.data[0] = 0;
    state.scsi.msgi.read = 0;
    break;

  case SCSICMD_MODE_SELECT:
    // get data out first...
    state.scsi.dato.expected = 12;
    if (state.scsi.dato.written < state.scsi.dato.expected)
      return 2;

    //printf("%s: MODE SELECT.\n",devid_string);

    if (   state.scsi.cmd.written == 6 
        && state.scsi.dato.written == 12 
        && state.scsi.dato.data[0] == 0x00 // data length
        //&& state.scsi.dato.data[1] == 0x05 // medium type - ignore
        && state.scsi.dato.data[2] == 0x00 // dev. specific
        && state.scsi.dato.data[3] == 0x08 // block descriptor length
        && state.scsi.dato.data[4] == 0x00 // density code
        && state.scsi.dato.data[5] == 0x00 // all blocks
        && state.scsi.dato.data[6] == 0x00 // all blocks
        && state.scsi.dato.data[7] == 0x00 // all blocks
        && state.scsi.dato.data[8] == 0x00) // reserved
    {
      set_block_size( (state.scsi.dato.data[9]<<16) | (state.scsi.dato.data[10]<<8) | state.scsi.dato.data[11] );
      //printf("%s: Block size set to %d.\n",devid_string,get_block_size());
    }
    else
    {
	  unsigned int x;
      printf("%s: MODE SELECT ignored.\nCommand: ",devid_string);
      for(x=0; x<state.scsi.cmd.written; x++) printf("%02x ",state.scsi.cmd.data[x]);
      printf("\nData: ");
      for(x=0; x<state.scsi.dato.written; x++) printf("%02x ",state.scsi.dato.data[x]);
      printf("\nThis might be an attempt to change our blocksize or something like that...\nPlease check the above data, then press enter.\n>");
      getchar();
    }

    // ignore it...

    state.scsi.stat.available = 1;
    state.scsi.stat.data[0] = 0;
    state.scsi.stat.read = 0;
    state.scsi.msgi.available = 1;
    state.scsi.msgi.data[0] = 0;
    state.scsi.msgi.read = 0;
	break;

  case SCSIBLOCKCMD_READ_CAPACITY:
    //printf("%s: READ CAPACITY.\n",devid_string);
	if (state.scsi.cmd.data[8] & 1) 
    {
      printf("SYM: Don't know how to handle READ CAPACITY with PMI bit set.\n");
      break;
	}

    state.scsi.dati.data[0] = (u8)((get_lba_size()) >> 24) & 255;
	state.scsi.dati.data[1] = (u8)((get_lba_size()) >> 16) & 255;
	state.scsi.dati.data[2] = (u8)((get_lba_size()) >>  8) & 255;
	state.scsi.dati.data[3] = (u8)((get_lba_size()) >>  0) & 255;

	state.scsi.dati.data[4] = (u8)(get_block_size() >> 24) & 255;
	state.scsi.dati.data[5] = (u8)(get_block_size() >> 16) & 255;
	state.scsi.dati.data[6] = (u8)(get_block_size() >>  8) & 255;
	state.scsi.dati.data[7] = (u8)(get_block_size() >>  0) & 255;

    state.scsi.dati.available = 8;

    state.scsi.stat.available = 1;
    state.scsi.stat.data[0] = 0;
    state.scsi.stat.read = 0;
    state.scsi.msgi.available = 1;
    state.scsi.msgi.data[0] = 0;
    state.scsi.msgi.read = 0;
	break;

  case SCSICMD_READ:
  case SCSICMD_READ_10:
  case SCSICMD_READ_12:
  case SCSICMD_READ_CD:
    printf("%s: READ.\n", devid_string);
    //if (state.scsi.disconnect_priv)
    //{
    //  //printf("%s: Will disconnect before returning read data.\n", devid_string);
    //  state.scsi.will_disconnect = true;
    //}
    if (state.scsi.cmd.data[0] == SCSICMD_READ)
    {
	  //  bits 4..0 of cmd[1], and cmd[2] and cmd[3]
	  //  hold the logical block address.
	  //
	  //  cmd[4] holds the number of logical blocks
	  //  to transfer. (Special case if the value is
	  //  0, actually means 256.)
	   
	  ofs = ((state.scsi.cmd.data[1] & 0x1f) << 16) + (state.scsi.cmd.data[2] << 8) + state.scsi.cmd.data[3];
	  retlen = state.scsi.cmd.data[4];
	  if (retlen == 0)
		retlen = 256;
	} 
    else if (state.scsi.cmd.data[0] = SCSICMD_READ_10)
    {
      //  cmd[2..5] hold the logical block address.
	  //  cmd[7..8] holds the number of logical
	   
	  ofs    = (state.scsi.cmd.data[2] << 24) + (state.scsi.cmd.data[3] << 16)
             + (state.scsi.cmd.data[4] <<  8) +  state.scsi.cmd.data[5];
      retlen = (state.scsi.cmd.data[7] <<  8) +  state.scsi.cmd.data[8];
	}
    else if (state.scsi.cmd.data[0] = SCSICMD_READ_10)
    {
      //  cmd[2..5] hold the logical block address.
	  //  cmd[6..9] holds the number of logical
	   
	  ofs    = (state.scsi.cmd.data[2] << 24) + (state.scsi.cmd.data[3] << 16)
             + (state.scsi.cmd.data[4] <<  8) +  state.scsi.cmd.data[5];
      retlen = (state.scsi.cmd.data[6] << 24) + (state.scsi.cmd.data[7] << 16)
             + (state.scsi.cmd.data[8] <<  8) +  state.scsi.cmd.data[9];
    }
    else if (state.scsi.cmd.data[0] = SCSICMD_READ_CD)
    {
      if (state.scsi.cmd.data[9] != 0x10)
      {
        printf("READ CD issued with data type %02x.\n",state.scsi.cmd.data[9]);
        FAILURE("data type not recognized");
      }
      //  cmd[2..5] hold the logical block address.
	  //  cmd[6..8] holds the number of logical blocks to transfer.
	   
	  ofs    = (state.scsi.cmd.data[2] << 24) + (state.scsi.cmd.data[3] << 16)
             + (state.scsi.cmd.data[4] <<  8) +  state.scsi.cmd.data[5];
      retlen = (state.scsi.cmd.data[6] << 16) + (state.scsi.cmd.data[7] << 8)
             + state.scsi.cmd.data[8];
    }

    state.scsi.stat.available = 1;
    state.scsi.stat.data[0] = 0;
    state.scsi.stat.read = 0;
    state.scsi.msgi.available = 1;
    state.scsi.msgi.data[0] = 0;
    state.scsi.msgi.read = 0;

    // Within bounds? 
    if ((ofs+retlen) > get_lba_size())
    {
      state.scsi.stat.data[0] = 0x02; // check condition
      break;
    }

	//  Return data:  
    seek_block(ofs);
    read_blocks(state.scsi.dati.data, retlen);
    state.scsi.dati.available = retlen * get_block_size();

    printf("%s: READ  ofs=%d size=%d\n", devid_string, ofs, retlen);
    //getchar();
	break;

  case SCSICMD_WRITE:
  case SCSICMD_WRITE_10:
    //printf("SYM.%d: WRITE.\n",GET_DEST());
    if (state.scsi.cmd.data[0] == SCSICMD_WRITE)
    {
	  //  bits 4..0 of cmd[1], and cmd[2] and cmd[3]
	  //  hold the logical block address.
	  //
	  //  cmd[4] holds the number of logical blocks
	  //  to transfer. (Special case if the value is
	  //  0, actually means 256.)

      ofs = ((state.scsi.cmd.data[1] & 0x1f) << 16) + (state.scsi.cmd.data[2] << 8) + state.scsi.cmd.data[3];
	  retlen = state.scsi.cmd.data[4];
	  if (retlen == 0)
		retlen = 256;
	} 
    else 
    {

      //  cmd[2..5] hold the logical block address.
	  //  cmd[7..8] holds the number of logical blocks
      //  to transfer.
	  
	  ofs = (state.scsi.cmd.data[2] << 24) + (state.scsi.cmd.data[3] << 16) + (state.scsi.cmd.data[4] << 8) + state.scsi.cmd.data[5];
      retlen = (state.scsi.cmd.data[7] << 8) + state.scsi.cmd.data[8];
	}

    state.scsi.stat.available = 1;
    state.scsi.stat.data[0] = 0;
    state.scsi.stat.read = 0;
    state.scsi.msgi.available = 1;
    state.scsi.msgi.data[0] = 0;
    state.scsi.msgi.read = 0;

    // Within bounds? 
    if (((ofs+retlen)) > get_lba_size())
    {
      state.scsi.stat.data[0] = 0x02; // check condition
      break;
    }

    state.scsi.dato.expected = retlen * get_block_size();

    if (state.scsi.dato.written < state.scsi.dato.expected)
      return 2;

	//  Write data
    seek_block(ofs);
    write_blocks(state.scsi.dato.data, retlen);

	printf("%s WRITE  ofs=%d size=%d\n", devid_string, ofs, retlen);
    //getchar();
	break;

  case SCSICMD_SYNCHRONIZE_CACHE:
    //printf("SYM.%d: SYNCHRONIZE CACHE.\n",GET_DEST());
    
    state.scsi.stat.available = 1;
    state.scsi.stat.data[0] = 0;
    state.scsi.stat.read = 0;
    state.scsi.msgi.available = 1;
    state.scsi.msgi.data[0] = 0;
    state.scsi.msgi.read = 0;
    break;

  case SCSICDROM_READ_TOC:
    //printf("SYM.%d: CDROM READ TOC.\n",GET_DEST());

    retlen = state.scsi.cmd.data[7]*256 + state.scsi.cmd.data[8];

    state.scsi.dati.available = retlen;
    state.scsi.dati.read = 0;
    retlen -=2;
    if (retlen>10) 
      retlen = 10; 
    else 
      retlen = 2;
    
    state.scsi.dati.data[0] = (retlen>>8) & 0xff;
    state.scsi.dati.data[1] = (retlen>>0) & 0xff;
    state.scsi.dati.data[2] = 1; // first track
    state.scsi.dati.data[3] = 1; // second track

    if (retlen==10)
    {
      state.scsi.dati.data[4] = 0;
      state.scsi.dati.data[2] = state.scsi.cmd.data[6];
      if (state.scsi.cmd.data[6]==1)
        printf("%s: Don't know how to return info on CDROM track %02x.\n",devid_string,state.scsi.cmd.data[6]);
      else if (state.scsi.cmd.data[6] == 0xAA)
        printf("%s: Don't know how to return info on CDROM leadout track %02x.\n",devid_string,state.scsi.cmd.data[6]);
      else
        printf("%s: Unknown CDROM track %02x.\n",devid_string,state.scsi.cmd.data[6]);
    }
    
    state.scsi.stat.available = 1;
    state.scsi.stat.data[0] = 0;
    state.scsi.stat.read = 0;
    state.scsi.msgi.available = 1;
    state.scsi.msgi.data[0] = 0;
    state.scsi.msgi.read = 0;
    break;

  default:
    printf("%s: Unknown SCSI command 0x%02x.\n",devid_string,state.scsi.cmd.data[0]);
    //printf(">");
    //getchar();
    throw((int)1);
  }
  return 0;
}

/**
 * \brief Handle a (series of) SCSI message(s).
 *
 * Called when one or more SCSI messages have been received.
 * We parse the message(s) and return what the next SCSI bus
 * phase should be.
 **/
int CDisk::do_scsi_message()
{
  unsigned int msg;
  unsigned int msglen;

  msg = 0;
  while (msg<state.scsi.msgo.written)
  {
    if (state.scsi.msgo.data[msg] & 0x80)
    {
      // identify
      printf("%s: MSG: identify",devid_string);
      if (state.scsi.msgo.data[msg] & 0x40)
      {
        printf(" w/disconnect priv");
//        state.scsi.disconnect_priv = true;
      }
      if (state.scsi.msgo.data[msg] & 0x07)
      {
      // LUN...
        printf(" for lun %d%",state.scsi.msgo.data[msg] & 0x07);
        state.scsi.lun_selected = true;
      }
      printf("\n");
      msg++;
    }
    else
    {
      switch (state.scsi.msgo.data[msg])
      {
      case 0x01:
        printf("%s: MSG: extended: ",devid_string);
        msglen = state.scsi.msgo.data[msg+1];
        msg += 2;
        switch (state.scsi.msgo.data[msg])
        {
        case 0x01:
	      {
            printf("SDTR.\n");
            state.scsi.msgi.available = msglen+2;
            state.scsi.msgi.data[0] = 0x01;
            state.scsi.msgi.data[1] = msglen;
            for (unsigned int x=0;x<msglen;x++)
              state.scsi.msgi.data[2+x] =state.scsi.msgo.data[msg+x];
	      }
          break;
        case 0x03:
		  {
            printf("WDTR.\n");
            state.scsi.msgi.available = msglen+2;
            state.scsi.msgi.data[0] = 0x01;
            state.scsi.msgi.data[1] = msglen;
            for (unsigned int x=0;x<msglen;x++)
              state.scsi.msgi.data[2+x] =state.scsi.msgo.data[msg+x];
		  }
          break;
        default:
          printf("%s: MSG: don't understand extended message %02x.\n",devid_string,state.scsi.msgo.data[msg]);
	      throw((int)1);
		}
        msg += msglen;
        break;
      default:
        printf("%s: MSG: don't understand message %02x.\n",devid_string,state.scsi.msgo.data[msg]);
	    throw((int)1);
      }
    }
  }

  // return next phase
  if (state.scsi.msgi.available)
    return SCSI_PHASE_MSG_IN;
  else
    return SCSI_PHASE_COMMAND;
}
