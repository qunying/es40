/** ES40 emulator.
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
 * 
 * SERIAL.CPP contains the code for the emulated Serial Port devices.
 *
 **/

#include "StdAfx.h"
#include "Serial.h"
#include "System.h"
#include "AliM1543C.h"

#include <stdlib.h>
#include <ctype.h>

#ifdef _WIN32
#include "windows/telnet.h"
#include <process.h>

CRITICAL_SECTION critSection;
#else
#include <arpa/inet.h>
#include <arpa/telnet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#define RECV_TICKS 100000

#endif



extern CAliM1543C * ali;


bool bStopping = false;
int  iCounter  = 0;
bool bStop     = false;

#define FIFO_SIZE 1024

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSerial::CSerial(CSystem * c, int number) : CSystemComponent(c)
{
  int base = atoi(c->GetConfig("serial.base","8000"));
  c->RegisterMemory (this, 0, X64(00000801fc0003f8) - (0x100*number), 8);
#ifdef _WIN32
  InitializeCriticalSection( &critSection );
  cTelnet = new CTelnet(base+number, this);

  // start the server
  if ( !cTelnet->start() )
    return;

  // start the client
  char s[100];
  sprintf(s,"telnet://localhost:%d/",base+number);
  _spawnl(P_NOWAIT,"putty.exe","putty.exe",s,NULL);

  // wait until connection
  for (;!cTelnet->getLoggedOn();) ;

  printf("%%SRL-I-INIT: Serial Interface %d emulator initialized.\n",number);
  printf("%%SRL-I-ADDRESS: Serial Interface %d on telnet port %d.\n",number,number+base);

  sprintf(s,"This is serial port #%d on AlphaSim\r\n",number);
  cTelnet->write(s);

#else
  c->RegisterClock(this);
	
  struct sockaddr_in Address;
  socklen_t nAddressSize=sizeof(struct sockaddr_in);

  listenSocket = socket(AF_INET,SOCK_STREAM,0);
  Address.sin_addr.s_addr=INADDR_ANY;
  Address.sin_port=htons(base+number);
  Address.sin_family=AF_INET;

  int optval = 1;
  setsockopt(listenSocket,SOL_SOCKET,SO_REUSEADDR, &optval,sizeof(optval));
  bind(listenSocket,(struct sockaddr *)&Address,sizeof(Address));
  listen(listenSocket,5);

  // start the server
  printf("%%SRL-I-WAIT: Waiting for connection on port %d.\n",number+base);
  connectSocket = accept(listenSocket,(struct sockaddr*)&Address,&nAddressSize);

  printf("%%SRL-I-INIT: Serial Interface %d emulator initialized.\n",number);
  printf("%%SRL-I-ADDRESS: Serial Interface %d on telnet port %d.\n",number,number+base);

  // Send some control characters to the telnet client to handle 
  // character-at-a-time mode.  
  char *telnet_options="%c%c%c";
  char buffer[8];

  sprintf(buffer,telnet_options,IAC,DO,TELOPT_ECHO);
  this->write(buffer);

  sprintf(buffer,telnet_options,IAC,DO,TELOPT_NAWS);
  write(buffer);

  sprintf(buffer,telnet_options,IAC,DO,TELOPT_LFLOW);
  this->write(buffer);

  sprintf(buffer,telnet_options,IAC,WILL,TELOPT_ECHO);
  this->write(buffer);

  sprintf(buffer,telnet_options,IAC,WILL,TELOPT_SGA);
  this->write(buffer);

  char s[100];
  sprintf(s,"This is serial port #%d on AlphaSim\r\n",number);
  this->write(s);

  serial_cycles = 0;
#endif

  iNumber = number;

  rcvW = 0;
  rcvR = 0;

  bLCR = 0x00;
  bLSR = 0x60; // THRE, TSRE
  bMSR = 0x30; // CTS, DSR
  bIIR = 0x01; // no interrupt
}

CSerial::~CSerial()
{
#ifdef _WIN32
  cTelnet->stop();
  free(cTelnet);
#endif
}

u64 CSerial::ReadMem(int index, u64 address, int dsize)
{
  dsize;
  index;
  char trcbuffer[1000];

  u8 d;

  switch (address)
    {
    case 0:						// data buffer
      if (bLCR & 0x80)
        {
	  return bBRB_LSB;
        }
      else
        {
	  if (rcvR != rcvW)
	    {   
	      bRDR = rcvBuffer[rcvR];
	      rcvR++;
	      if (rcvR == FIFO_SIZE)
		rcvR = 0;
	      if(isprint(bRDR)) {
		sprintf(trcbuffer,"Read character %02x (%c) on serial port %d\n",bRDR,bRDR,iNumber);
	      } else {
		sprintf(trcbuffer,"Read character %02x () on serial port %d\n",bRDR,iNumber);
	      }
	      cSystem->trace->trace_dev(trcbuffer);
	    }
	  else
            {
	      sprintf(trcbuffer,"Read past FIFO on serial port %d\n",iNumber);
	      cSystem->trace->trace_dev(trcbuffer);
            }
	  return bRDR;
        }
    case 1:
      if (bLCR & 0x80)
        {
	  return bBRB_MSB;
        }
      else
        {
	  return bIER;
        }
    case 2:						//interrupt cause
      d = bIIR;
      bIIR = 0x01;
      return d;
    case 3:
      return bLCR;
    case 4:
      return bMCR;
    case 5:						//serialization state
      if (rcvR != rcvW)
	bLSR = 0x61; // THRE, TSRE, RxRD
      else
	bLSR = 0x60; // THRE, TSRE
      return bLSR;
    case 6:
      return bMSR;
    default:
      return bSPR;
    }
}

void CSerial::WriteMem(int index, u64 address, int dsize, u64 data)
{
  dsize;
  index;

  u8 d;
  char s[5];
  d = (u8)data;
  char trcbuffer[500];

  switch (address)
    {
    case 0:						// data buffer
      if (bLCR & 0x80)
        {
	  bBRB_LSB = d;
        }
      else
        {
	  sprintf(s,"%c",d);
#ifdef _WIN32
	  cTelnet->write(s);
#else
	  write(s);
#endif
	  if(isprint(d)) {
	    sprintf(trcbuffer,"Write character %02x (%c) on serial port %d\n",d,d,iNumber);
	  } else {
	    sprintf(trcbuffer,"Write character %02x () on serial port %d\n",d,iNumber);
	  }
	  cSystem->trace->trace_dev(trcbuffer);
	  if (bIER & 0x2)
            {
	      bIIR = (bIIR>0x02)?bIIR:0x02;
	      ali->pic_interrupt(0, 4 - iNumber);
            }
        }
      break;
    case 1:
      if (bLCR & 0x80)
        {
	  bBRB_MSB = d;
        }
      else
        {
	  bIER = d;
	  bIIR = 0x01;
	  if (bIER & 0x2)
            {
	      bIIR = (bIIR>0x02)?bIIR:0x02;
	      ali->pic_interrupt(0, 4 - iNumber);
            }
        }
      break;
    case 2:			
      bFCR = d;
      break;
    case 3:
      bLCR = d;
      break;
    case 4:
      bMCR = d;
      break;
    default:
      bSPR = d;
    }
}

void CSerial::write(char *s)
{
#ifdef _WIN32
  cTelnet->write(s);
#else
  send(connectSocket,s,strlen(s)+1,0);
#endif
}

void CSerial::receive(const char* data)
{
  char * x;

  x = (char *) data;

  while (*x)
    {
      //		if (	(rcvW==rcvR-1)					// overflow...
      //			||  ((rcvR==0) && (rcvW==3)))
      //			break;
      rcvBuffer[rcvW++] = *x;
      if (rcvW == FIFO_SIZE)
	rcvW = 0;
      x++;
      if (bIER & 0x1)
        {
	  bIIR = (bIIR>0x04)?bIIR:0x04;
	  ali->pic_interrupt(0, 4 - iNumber);
        }
    }
}

#ifndef _WIN32

void CSerial::DoClock() {
  fd_set readset;
  char buffer[FIFO_SIZE+1];
  ssize_t size;
  struct timeval tv;

  serial_cycles++;
  if(serial_cycles >= RECV_TICKS) {
    FD_ZERO(&readset);
    FD_SET(connectSocket,&readset);
    tv.tv_sec=0;
    tv.tv_usec=0;
    if(select(connectSocket+1,&readset,NULL,NULL,&tv) > 0) {
      size = read(connectSocket,&buffer,FIFO_SIZE);
      buffer[size+1]=0; // force null termination.
      this->receive((const char*)&buffer);
    }
    serial_cycles=0;
  }
}


#endif