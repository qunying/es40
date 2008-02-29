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
 * Contains the code for the CPU tracing engine.
 * This will become the debugging engine (interactive debugger) soon.
 *
 * $Id$
 *
 * X-1.34       Brian Wheeler                                   29-FEB-2008
 *      Add BREAKPOINT INSTRUCTION command to IDB.
 *
 * X-1.33	    Brian Wheeler    				                12-JAN-2008
 *	End run when Ctrl-C is received.
 *
 * X-1.32       Camiel Vanderhoeven                             02-JAN-2008
 *      Avoid compiler warnings
 *
 * X-1.31       Camiel Vanderhoeven                             10-DEC-2007
 *      Changes to make the TraceEngine work again after recent changes.
 *
 * X-1.30       Brian Wheeler                                   6-DEC-2007
 *      Bugfix in real_address.                   
 *
 * X-1.29       Camiel Vanderhoeven                             2-DEC-2007
 *      Changed the way translation buffers work. 
 *
 * X-1.27       Camiel Vanderhoeven                             1-DEC-2007
 *      Moved inclusion of StdAfx.h outside conditional block; necessary
 *      for using precompiled headers in Visual C++.
 *
 * X-1.26	    Brian Wheeler    				                22-NOV-2007
 *	Added LOADREG and LOADFPREG debugger commands.
 *
 * X-1.25	    Camiel Vanderhoeven				                7-APR-2007
 *	Added PCB to job context recognition.
 *
 * X-1.24       Camiel Vanderhoeven                             30-MAR-2007
 *      Added old changelog comments.
 *
 * X-1.23       Camiel Vanderhoeven                             14-MAR-2007
 *   a) bListing moved here from CAlphaCPU.
 *   b)	Translation from virtual to physical address moved to an inline
 *	function (code to do this was duplicated many times...)
 *
 * X-1.22       Camiel Vanderhoeven                             14-MAR-2007
 *      Added list_all method.
 *
 * X-1.21       Camiel Vanderhoeven                             12-MAR-2007
 *   a) Added support for TranslationBuffer debugging.
 *   b) Changed format of calls to CTranslationBuffer::convert_address.
 *
 * X-1.20	Camiel Vanderhoeven				9-MAR-2007
 *	Use procname from the structure rather than getting it from memory
 *	to print this to the trace files.
 *
 * X-1.19       Camiel Vanderhoeven                             8-MAR-2007
 *   a) get_fnc_name now requires CCPU * as an argument.
 *   b)	When tracing, try to convert virtual addresses to physical ones 
 *	through the translation buffers.
 *
 * X-1.18	Camiel Vanderhoeven				7-MAR-2007
 *   a)	Limit write_printable_s to 100 characters.
 *   b)	When tracing to an unknown (not in CSV file) function, treat all 
 *	arguments as if they were strings.
 *   c)	When dsplaying string arguments, try to translate a virtual address
 *	to a physical one through the translation buffer.
 *
 * X-1.17	Camiel Vanderhoeven				7-MAR-2007
 *	Fixed error in hiding functions (tracing).
 *
 * X-1.16       Camiel Vanderhoeven                             1-MAR-2007
 *      Made a couple of arguments const char *.
 *
 * X-1.15	Camiel Vanderhoeven				28-FEB-2007
 *	Added support for lockstep-mechanism.
 *
 * X-1.14	Camiel Vanderhoeven				26-FEB-2007
 *	Fixed some unsigned integer comparisons.
 *
 * X-1.13	Camiel Vanderhoeven				21-FEB-2007
 *	Allow user to terminate execution from the serial port.
 *
 * X-1.12       Camiel Vanderhoeven                             18-FEB-2007
 *      Added HASHING and DUMPREGS commands to the IDB.
 *
 * X-1.11       Camiel Vanderhoeven                             16-FEB-2007
 *   a)	Added the Interactive Debugger (IDB).
 *   b) Got rid of all #ifdef _WIN32's arount printf statements.
 *
 * X-1.10       Camiel Vanderhoeven                             12-FEB-2007
 *      Added comments.
 *
 * X-1.9        Camiel Vanderhoeven                             9-FEB-2007
 *      Debugging flqags (booleans) moved here from CAlphaCPU.
 *
 * X-1.8        Camiel Vanderhoeven                             7-FEB-2007
 *      Debugging functions are enabled only when compiling with -DIDB 
 *      (Interactive Debugger, a future feature).
 *
 * X-1.7	Camiel Vanderhoeven				3-FEB-2007
 *	Fixed error in trace_dev. String is no longer interpreted by printf.
 *
 * X-1.6	Camiel Vanderhoeven				3-FEB-2007
 *	Printable function moved to StdAfx.h.
 *
 * X-1.5        Brian Wheeler                                   3-FEB-2007
 *      Formatting.
 *
 * X-1.4	Brian Wheeler					3-FEB-2007
 *	Add definition of _strdup for Linux' benefit.
 *
 * X-1.3        Brian Wheeler                                   3-FEB-2007
 *      Scanf and printf statements made compatible with Linux/GCC/glibc.
 *      
 * X-1.2        Brian Wheeler                                   3-FEB-2007
 *      Includes are now case-correct (necessary on Linux)
 *
 * X-1.1        Camiel Vanderhoeven                             19-JAN-2007
 *      Initial version in CVS.
 *
 * \author Camiel Vanderhoeven (camiel@camicom.com / www.camicom.com)
 **/

#include "StdAfx.h"

#if defined(IDB)

#include "TraceEngine.h"
#include "AlphaCPU.h"
#include "System.h"
#include "DPR.h"
#include "Flash.h"
#include "lockstep.h"
#include <signal.h>

CTraceEngine * trc;

inline void write_printable_s(char * dest, char * org)
{
  int cnt = 100;
  while (*org && cnt--)
    {
      *(dest++) = printable(*(org++));
    }
  *dest='\0';
}

inline u64 real_address(u64 address, CAlphaCPU * c, bool bIBOX)
{
  bool b;
  u64 a;

  if (bIBOX && (address&1))
    return address & X64(fffffffffffffffc);

  if (!(c->virt2phys(address,&a,ACCESS_READ | NO_CHECK | FAKE,&b,0)))
    return a & (bIBOX?X64(fffffffffffffffc):X64(ffffffffffffffff));

  return ((address&X64(fffffffff0000000)) ==X64(0000000020000000))?
     address-X64(000000001fe00000):
      (((address&X64(fffffffff0000000))== X64(0000000010000000))?
       address-X64(000000000fffe000):address) & (bIBOX?X64(fffffffffffffffc):X64(ffffffffffffffff));
}

CTraceEngine::CTraceEngine(CSystem * sys)
{
  int i;
  iNumFunctions = 0;
  iNumPRBRs = 0;
  cSystem = sys;
  for (i=0;i<4;i++)
    {
      asCPUs[0].last_prbr = -1;
    }
  current_trace_file = stdout;
  bBreakPoint = false;
}

CTraceEngine::~CTraceEngine(void)
{
  int i;
  for (i=0;i<iNumPRBRs;i++)
    fclose(asPRBRs[i].f);
}

void CTraceEngine::trace(CAlphaCPU * cpu, u64 f, u64 t, bool down, bool up, const char * x, int y)
{
  if (      (t == f+4)  
	 || (t == f) )
    return;

  int p = get_prbr(cpu->get_prbr(),cpu->get_hwpcb());
  int o = asCPUs[cpu->get_cpuid()].last_prbr;
  
  if (p != o)
    {
      if (o != -1)
	{
	  fprintf(asPRBRs[o].f, "\n==>   Switch to PRBR %08" LL "x %08" LL "x (%s)\n",   asPRBRs[p].prbr, asPRBRs[p].hwpcb, asPRBRs[p].procname);
	  fprintf(asPRBRs[p].f,   "        This is PRBR %08" LL "x %08" LL "x (%s)\n",   asPRBRs[p].prbr, asPRBRs[p].hwpcb, asPRBRs[p].procname);
	  fprintf(asPRBRs[p].f,   "<== Switch from PRBR %08" LL "x %08" LL "x (%s)\n\n", asPRBRs[o].prbr, asPRBRs[o].hwpcb, asPRBRs[o].procname);
	}
      asCPUs[cpu->get_cpuid()].last_prbr = p;
    }

  if (asPRBRs[p].trc_waitfor)
    {
      if ((t&~X64(3))==asPRBRs[p].trc_waitfor)
	asPRBRs[p].trc_waitfor = 0;
      return;
    }

  u64 pc_f = real_address(f,cpu,true);
  u64 pc_t = real_address(f,cpu,true);
  
  int oldlvl = asPRBRs[p].trclvl;
  int i, j;

  if (up)
    {
      for (i=oldlvl-1; i>=0;i--)
	{
	  if (   (asPRBRs[p].trcadd[i] == (pc_t-4))
		 || (asPRBRs[p].trcadd[i] == (pc_t))   )
	    {
	      asPRBRs[p].trclvl = i;
	      if (asPRBRs[p].trchide > i)
		asPRBRs[p].trchide = -1;
			
	      if(asPRBRs[p].trchide != -1)
		return;

	      for (j=0;j<oldlvl;j++)
		fprintf(asPRBRs[p].f," ");
	      fprintf(asPRBRs[p].f,"%016" LL "x(%08" LL "x) ($r0 = %" LL "x)\n", f, pc_f, cpu->get_r(0,true));
			
	      for (j=0;j<asPRBRs[p].trclvl;j++)
		fprintf(asPRBRs[p].f," ");
				
	      fprintf(asPRBRs[p].f,"%016" LL "x(%08" LL "x) <--\n", t, pc_t);
	      return;
	    }
	}
    }

  if(asPRBRs[p].trchide != -1)
    return;
 
  if (!down)
    {
      trace_br(cpu,f,t);
      return;
    }
	
  if (oldlvl<700)
    asPRBRs[p].trclvl = oldlvl + 1;
  asPRBRs[p].trcadd[oldlvl] = pc_f;

  if (x)
    {
      for (i=0;i<oldlvl;i++)
	fprintf(asPRBRs[p].f," ");
      fprintf(asPRBRs[p].f,x,y);
      fprintf(asPRBRs[p].f,"\n");
    }
	
  for (i=0;i<oldlvl;i++)
    fprintf(asPRBRs[p].f," ");
  fprintf(asPRBRs[p].f,"%016" LL "x(%08" LL "x) -->\n", f, pc_f);

  for(i=0;i<asPRBRs[p].trclvl;i++)
    fprintf(asPRBRs[p].f," ");
	
  for (i=0;i<iNumFunctions;i++)
    {
      if (asFunctions[i].address == pc_t)
	{
	  fprintf(asPRBRs[p].f,"%016" LL "x(%s)", t, asFunctions[i].fn_name);
	  write_arglist(cpu,asPRBRs[p].f,asFunctions[i].fn_arglist);
	  fprintf(asPRBRs[p].f,"\n");
	  if (asFunctions[i].step_over)
	    asPRBRs[p].trchide = asPRBRs[p].trclvl;
	  return;
	}
    }

  fprintf(asPRBRs[p].f,"%016" LL "x(%08" LL "x)", t, pc_t);
  write_arglist(cpu,asPRBRs[p].f,"(%s|16%, %s|17%, %s|18%, %s|19%)");
  fprintf(asPRBRs[p].f,"\n");
}

void CTraceEngine::trace_br(CAlphaCPU * cpu, u64 f, u64 t)
{
  int p, o;
  u64 pc_f = real_address(f, cpu, true);
  u64 pc_t = real_address(t, cpu, true);

  p = get_prbr(cpu->get_prbr(),cpu->get_hwpcb());
  o = asCPUs[cpu->get_cpuid()].last_prbr;

  if (asPRBRs[p].trc_waitfor)
    {
      if ((t&~X64(3))==asPRBRs[p].trc_waitfor)
	asPRBRs[p].trc_waitfor = 0;
      return;
    }

  if (asPRBRs[p].trchide != -1)
    return;
    
  if (p!= o)
    {
      if (o != -1)
	{
	  fprintf(asPRBRs[o].f, "\n==>   Switch to PRBR %08" LL "x %08" LL "x (%s)\n",   asPRBRs[p].prbr, asPRBRs[p].hwpcb, asPRBRs[p].procname);
	  fprintf(asPRBRs[p].f,   "        This is PRBR %08" LL "x %08" LL "x (%s)\n",   asPRBRs[p].prbr, asPRBRs[p].hwpcb, asPRBRs[p].procname);
	  fprintf(asPRBRs[p].f,   "<== Switch from PRBR %08" LL "x %08" LL "x (%s)\n\n", asPRBRs[o].prbr, asPRBRs[o].hwpcb, asPRBRs[o].procname);
	}
      asCPUs[cpu->get_cpuid()].last_prbr = p;
    }

  int i;

  if (   (pc_t > pc_f + 4) 
	 || (pc_t < pc_f))
    {
      for (i=0;i<asPRBRs[p].trclvl;i++)
	fprintf(asPRBRs[p].f," ");
      fprintf(asPRBRs[p].f,"%016" LL "x(%08" LL "x) --+\n", f, pc_f);
      for(i=0;i<asPRBRs[p].trclvl;i++)
	fprintf(asPRBRs[p].f," ");

      for (i=0;i<iNumFunctions;i++)
	if (asFunctions[i].address == pc_t)
	  {
	    fprintf(asPRBRs[p].f,"%016" LL "x(%s)", t, asFunctions[i].fn_name);
	    write_arglist(cpu,asPRBRs[p].f,asFunctions[i].fn_arglist);
	    fprintf(asPRBRs[p].f," <-+\n");
	    if (asFunctions[i].step_over)
	      asPRBRs[p].trchide = asPRBRs[p].trclvl;
	    return;
	  }

      fprintf(asPRBRs[p].f,"%016" LL "x(%08" LL "x) <-+\n", t, pc_t);
    }
}

void CTraceEngine::add_function(u64 address, char * fn_name, char * fn_arglist, bool step_over)
{
  asFunctions[iNumFunctions].address = (u32)address&~3;
  asFunctions[iNumFunctions].fn_name = _strdup(fn_name);
  asFunctions[iNumFunctions].fn_arglist = _strdup(fn_arglist);
  asFunctions[iNumFunctions].step_over = step_over;
  iNumFunctions++;
}

void CTraceEngine::set_waitfor(CAlphaCPU * cpu, u64 address)
{
  int p;
  p = get_prbr(cpu->get_prbr(),cpu->get_hwpcb());

  if(asPRBRs[p].trc_waitfor == 0)
    asPRBRs[p].trc_waitfor = address & ~X64(3);
}
bool CTraceEngine::get_fnc_name(CAlphaCPU * c, u64 address, char ** p_fn_name)
{
  int i;

  u64 a = real_address(address, c, true);

  for (i=0;i<iNumFunctions;i++)
    {
      if (asFunctions[i].address == a)
	{
	  *p_fn_name = asFunctions[i].fn_name;
	  return true;
	}
    }
  *p_fn_name = (char *)0;
  return false;
}

int CTraceEngine::get_prbr(u64 prbr, u64 hwpcb)
{
  int i;
  char filename[100];

  for (i=0;i<iNumPRBRs;i++)
    {
      if ((asPRBRs[i].prbr == prbr) && (asPRBRs[i].hwpcb == hwpcb))
	{
	  if (asPRBRs[i].f == current_trace_file)
	    return i;
	  if (!strncmp(asPRBRs[i].procname,cSystem->PtrToMem(prbr+0x154),20))
	    {
	      current_trace_file = asPRBRs[i].f;
	      return i;
	    }
	  fclose(asPRBRs[i].f);
	  break;
	}
    }

  if (i == iNumPRBRs)
    {
      asPRBRs[i].generation = 0;
      iNumPRBRs++;
    }
  asPRBRs[i].generation++;

  asPRBRs[i].prbr = prbr;
  asPRBRs[i].hwpcb = hwpcb;
  if (prbr > 0 && prbr < (X64(1)<<cSystem->get_memory_bits()))
    strncpy(asPRBRs[i].procname, cSystem->PtrToMem(prbr+0x154), 20);
  else
    strcpy(asPRBRs[i].procname,"");
  sprintf(filename,"trace_%08" LL "x_%08" LL "x_%02d_%s.trc",prbr,hwpcb,asPRBRs[i].generation,asPRBRs[i].procname);
  asPRBRs[i].f = fopen(filename,"w");
  if (asPRBRs[i].f==0)
    printf("Failed to open file!!\n");
  asPRBRs[i].trclvl = 0;
  asPRBRs[i].trchide = -1;
  asPRBRs[i].trc_waitfor = 0;
  current_trace_file = asPRBRs[i].f;
  printf("Add PRBR: %08" LL "x_%08" LL "x\n",prbr,hwpcb);
  return i;
}

void CTraceEngine::write_arglist(CAlphaCPU * c, FILE * fl, char * a)
{
  char o[500];
  char * op = o;
  char * ap = a;
  char f[20];
  char * fp;
  char * rp;
  int r;
  u64 value;

  while (*ap)
    {
      if (*ap=='%')
	{
	  ap++;
	  fp = f;
	  *(fp++) = '%';
	  while (*ap != '%')
	    *(fp++) = *(ap++);
	  ap++;
	  *fp = '\0';
	  // now we have a formatter in f.
	  rp = strchr(f,'|');
	  *(rp++) = '\0';
	  // and the register in rp;
	  r = atoi(rp);
	  value = c->get_r(r,true);
	  if (!strcmp(f,"%s"))
	    {
	      sprintf(op,"%" LL "x (",value);
	      while (*op)
		op++;
	      value = real_address(value, c, false);
	      if ((value > 0) && (value < (X64(1)<<cSystem->get_memory_bits())))
		write_printable_s(op, cSystem->PtrToMem(value));
	      else
		sprintf(op,"INVPTR");
	      while (*op)
		op++;
	      *(op++) = ')';
	      *(op)='\0';
	    }
	  else if (!strcmp(f,"%c"))
	    sprintf(op,"%02" LL "x (%c)",value,printable((char)value));
	  else if (!strcmp(f,"%d"))
	    sprintf(op,"%" LL "d",value);
	  else if (!strcmp(f,"%x"))
	    sprintf(op,"%" LL "x",value);
	  else if (!strcmp(f,"%0x"))
	    sprintf(op,"%016" LL "x",value);
	  else if (!strcmp(f,"%016x"))
	    sprintf(op,"%016" LL "x",value);
	  else if (!strcmp(f,"%08x"))
	    sprintf(op,"%08" LL "x",value);
	  else if (!strcmp(f,"%04x"))
	    sprintf(op,"%04" LL "x",value);
	  else if (!strcmp(f,"%02x"))
	    sprintf(op,"%02" LL "x",value);
	  else
	    sprintf(op,f,value);
	  while(*op) 
	    op++;
	}
      else
	{
	  *(op++) = *(ap++);
	}
    }
  *op = '\0';

  fprintf(fl,"%s",o);
}

void CTraceEngine::read_procfile(char *filename)
{
  FILE * f;
  u64 address;
  char linebuffer[1000];
  char * fn_name;
  char * fn_args;
  char * sov;
  int step_over;
  int result;
  int i = 0;

  f = fopen(filename,"r");

  if (f)
    {
      while (fscanf(f,"%[^\n] ",linebuffer) != EOF)
	{
	  address = X64(0);
	  fn_name = strchr(linebuffer,';');
	  if (fn_name)
            {
	      *fn_name = '\0';
	      fn_name++;
	      result = sscanf(linebuffer,"%" LL "x",&address);
	      if ((result == 1) && address)
                {
		  fn_args = strchr(fn_name,';');
		  if (fn_args)
                    {
		      *fn_args = '\0';
		      fn_args++;
		      sov = strchr(fn_args,';');
		      if (sov)
                        {
			  *sov = '\0';
			  sov++;
			  result = sscanf(sov,"%d",&step_over);
			  if (result==1)
                            {
			      add_function(address,fn_name,fn_args,step_over?true:false);
			      i++;
                            }
                        }
                        
                    }
                }

            }
        }
      fclose(f);
      printf("%%IDB-I-RDTRC : Read %d entries from trace-file %s\n",i,filename);
    }

}

void CTraceEngine::trace_dev(const char * text)
{
  fprintf(current_trace_file, "%s", text);
}

FILE * CTraceEngine::trace_file()
{
  return current_trace_file;
}

void CTraceEngine::run_script(char * filename)
{
  char s[100][100];
  int i;

#if !defined(LS_SLAVE)

  char c = '\0';
  int j;
  FILE * f = NULL;
  
  if (filename)
  {
     f = fopen(filename,"r");
     if (!f)
     {
       printf("%%IDB-F-NOLOAD: File %s could not be opened.\n",filename);
       return;
     }
  }
  else
  {
    printf("This is the ES40 interactive debugger. To start non-interactively, run es40,\n");
    printf("Or run this executable (es40_idb) with a last argument of @<script-file>\n");
    f = stdin;
  }
#endif

  for (;;)
  {
#if !defined(LS_SLAVE)
    if (filename)
    {
      if (feof(f))
	break;
    }
    else
    {
      printf("IDB %016" LL "x %c>",theSystem->get_cpu(0)->get_clean_pc(), (theSystem->get_cpu(0)->get_pc()&X64(1))?'P':'-');
    }
#endif

    for (i=0; i<100; )
    {
#if defined(LS_SLAVE)
      lockstep_receive(s[i],100);
      if (!strcmp(s[i],"TERM"))
        break;
#else 
      bool u = false;
      for (j=0; j<100; )
      {
  	  fscanf(f,"%c",&c);
        if (c != '\n' && c!= '\r' && c!= ' ' && c!= '\t')
	{
	  s[i][j++] = c;
	  u = true;
	}
	if (c == ' ' || c == '\t' || c== '\n')
	  break;
      }
      s[i][j] = '\0';

      if (u) 
#endif
      {
#if defined(LS_MASTER)
        lockstep_send(s[i]);
#endif
        i++;
      }
#if !defined(LS_SLAVE)
      if (c == '\n')
      {
#if defined(LS_MASTER)
        lockstep_send("TERM");
#endif
        break;
      }
#endif
    }
    s[i][0] = '\0';
    if (parse(s))
      break;
  }
#if !defined(LS_SLAVE)
  if (filename)
    fclose(f);
#endif
}

int CTraceEngine::parse(char command[100][100])
{
  int i = 0;
  int numargs;
  int result;
  int RunCycles;
  u64 iFrom;
  u64 iTo;
  u64 iJump;

  for (numargs=0; command[numargs][0] != '\0'; numargs++);

  if ((numargs>0) && (command[0][0]=='#'||command[0][0] ==';' || command[0][0] =='!' 
      || (command[0][0]=='/' && command[0][1]=='/')))
    //comment
    return 0;
  switch (numargs)
  {
  case 0:
    // empty command
    return 0;
  case 1:
    if (!strncasecmp(command[0],"EXIT",strlen(command[0])) ||
	!strncasecmp(command[0],"QUIT",strlen(command[0])))
      return 1;
    if (!strncasecmp(command[0],"HELP",strlen(command[0])) ||
	!strncasecmp(command[0],"?",strlen(command[0])))
    {
      printf("                                                                     \n");
      printf("Available commands:                                                  \n");
      printf("  HELP | ?                                                           \n");
      printf("  EXIT | QUIT                                                        \n");
      printf("  STEP                                                               \n");
      printf("  TRACE [ ON | OFF ]                                                 \n");
      printf("  HASHING [ ON | OFF ]                                               \n");
      printf("  BREAKPOINT [ OFF | [ > | < | = | INSTRUCTION ] <hex value> ]                         \n");
      printf("  DISASSEMBLE [ON | OFF ]	                                           \n");
#if defined(DEBUG_TB)
      printf("  TBDEBUG [ON | OFF ]	                                           \n");
#endif
      printf("  LIST [ ALL |<hex address> - <hex address> ]                        \n");
      printf("  RUN [ <max cycles> ]                                               \n");
      printf("  LOAD [ STATE | DPR | FLASH | CSV ] <file>                          \n");
      printf("  SAVE [ STATE | DPR | FLASH ] <file>                                \n");
      printf("  JUMP <hex address>                                                 \n");
      printf("  PAL [ ON | OFF ]                                                   \n"); 
      printf("  DUMPREGS                                                           \n");
      printf("  LOADREG <register> <value>                                         \n");
      printf("  LOADFPREG <register> <value>                                       \n");
      printf("  @<script-file>                                                     \n");
      printf("  # | // | ; | ! <comment>                                           \n");
      printf("                                                                     \n");
      printf("The words of each command can be abbreviated, e.g. B or BRE for      \n");
      printf("BREAKPOINT; S F for save flash.                                      \n");
      printf("                                                                     \n");
      return 0;
    }
    if (!strncasecmp(command[0],"STEP",strlen(command[0])))
    {
      printf("%%IDB-I-SSTEP : Single step.\n");
      theSystem->SingleStep();
      return 0;
    }
    if (!strncasecmp(command[0],"DUMPREGS",strlen(command[0])))
    {
      printf("\n==================== SYSTEM STATE =======================\n");
      printf("PC: %" LL "x\n", theSystem->get_cpu(0)->get_pc());
      printf("Physical PC: %" LL "x\n",theSystem->get_cpu(0)->get_current_pc_physical());
      printf("Instruction count: %" LL "d\n",theSystem->get_cpu(0)->get_instruction_count());
      printf("\n==================== REGISTER VALUES ====================\n");
      for(i=0;i<32;i++)
      {
	if (i<10) printf("R");
	printf("%d:%016" LL "x", i, theSystem->get_cpu(0)->get_r(i,false));
	if (i%4==3) printf("\n"); else printf(" ");
      }
      printf("\n");
      for(i=4;i<8;i++)
      {
	if (i<10) printf("S");
	printf("%d:%016" LL "x", i, theSystem->get_cpu(0)->get_r(i+32,false));
	if (i%4==3) printf("\n"); else printf(" ");
      }
      for(i=20;i<24;i++)
      {
	if (i<10) printf("S");
	printf("%d:%016" LL "x", i, theSystem->get_cpu(0)->get_r(i+32,false));
	if (i%4==3) printf("\n"); else printf(" ");
      }
      printf("\n");
      for(i=0;i<32;i++)
      {
	if (i<10) printf("F");
	printf("%d:%016" LL "x", i, theSystem->get_cpu(0)->get_f(i));
	if (i%4==3) printf("\n"); else printf(" ");
      }
      printf("=========================================================\n");
      return 0;
    }
    if (!strncasecmp(command[0],"RUN",strlen(command[0])))
    {
      if (!bBreakPoint)
      {
        printf("%%IDB-F-NOBRKP: No breakpoint set, press Ctrl-C to end run.\n");
        /* catch CTRL-C and shutdown gracefully */
       extern int got_sigint;
       void sigint_handler(int);
       signal(SIGINT,&sigint_handler);
       while(!got_sigint) {
         theSystem->SingleStep();
       }
       got_sigint=0;
       return 0;
      }
      printf("%%IDB-I-RUNBPT: Running until breakpoint found.\n");
      switch (iBreakPointMode)
      {
      case -1:
	for (i=0;;i++)
	{
	  if (theSystem->SingleStep())
	  {
	    printf("%%IDB-I-ABORT : Abort run requested (probably from serial port)\n");
	    break;
	  }
	  if (theSystem->get_cpu(0)->get_clean_pc() < iBreakPoint)
	  {
            printf("%%IDB-I-BRKPT : Breakpoint encountered.\n");
	    break;
	  }
	}
	break;
      case 0:
	for(i=0;;i++)
	{
	  if (theSystem->SingleStep())
	  {
	    printf("%%IDB-I-ABORT : Abort run requested (probably from serial port)\n");
	    break;
	  }
	  if (theSystem->get_cpu(0)->get_clean_pc() == iBreakPoint)
	  {
            printf("%%IDB-I-BRKPT : Breakpoint encountered.\n");
	    break;
	  }
	}
	break;
      case 1:
	for(i=0;;i++)
	{
	  if (theSystem->SingleStep())
	  {
	    printf("%%IDB-I-ABORT : Abort run requested (probably from serial port)\n");
	    break;
	  }
	  if (theSystem->get_cpu(0)->get_clean_pc() > iBreakPoint)
	  {
            printf("%%IDB-I-BRKPT : Breakpoint encountered.\n");
	    break;
	  }
	}
	break;
      case 2:
	// break when the instruction matches.  Great for stopping when
	// pal_halt is called!
	extern int got_sigint;
	void sigint_handler(int);
	signal(SIGINT,&sigint_handler);
	while(1) 
	{
	  theSystem->SingleStep();
	  if(theSystem->get_cpu(0)->get_last_instruction() == iBreakPointInstruction || got_sigint) 
	  {
	    printf("%%IDB-I-BRKPT: Found trapped instruction or control-c\n");
	    break;
	  }
	}
	got_sigint=0;
	break;
      default:
	break;
      }
      printf("%%IDB-I-ENDRUN: End of run.\n");
      return 0;
    }
#if !defined(IDB) || !defined(LS_SLAVE)
    if (command[0][0]=='@')
    {
      run_script(command[0] + 1);
      return 0;
    }
#endif // !defined(IDB) || !defined(LS_SLAVE)
    break;
  case 2:
    if (!strncasecmp(command[0],"LIST",strlen(command[0])) && !strncasecmp(command[1],"ALL",strlen(command[1])))
      {
	list_all();
	return 0;
      }
    if (!strncasecmp(command[0],"TRACE",strlen(command[0])))
    {
      if (!strcasecmp(command[1],"ON"))
      {
	printf("%%IDB-I-TRCON : Tracing enabled.\n");
	bTrace = true;
	return 0;
      }
      if (!strcasecmp(command[1],"OFF"))
      {
	printf("%%IDB-I-TRCOFF: Tracing disabled.\n");
	bTrace =false;
	return 0;
      }
    }
#if defined(DEBUG_TB)
    if (!strncasecmp(command[0],"TBDEBUG",strlen(command[0])))
    {
      if (!strcasecmp(command[1],"ON"))
      {
	printf("%%IDB-I-TBDON : Translation Buffer Debugging enabled.\n");
	bTB_Debug = true;
	return 0;
      }
      if (!strcasecmp(command[1],"OFF"))
      {
	printf("%%IDB-I-TBDOFF: Translation Buffer Debugging disabled.\n");
	bTB_Debug =false;
	return 0;
      }
    }
#endif
    if (!strncasecmp(command[0],"HASHING",strlen(command[0])))
    {
      if (!strcasecmp(command[1],"ON"))
      {
	printf("%%IDB-I-HSHON : Hashing enabled.\n");
	bHashing = true;
	return 0;
      }
      if (!strcasecmp(command[1],"OFF"))
      {
	printf("%%IDB-I-HSHOFF: Hashing disabled.\n");
	bHashing = false;
	return 0;
      }
    }
    if (!strncasecmp(command[0],"PAL",strlen(command[0])))
    {
      if (!strcasecmp(command[1],"ON"))
      {
	printf("%%IDB-I-PALON : PALmode enabled.\n");
	theSystem->get_cpu(0)->set_pc(theSystem->get_cpu(0)->get_clean_pc() + 1);
	return 0;
      }
      if (!strcasecmp(command[1],"OFF"))
      {
	printf("%%IDB-I-PALOFF: PALmode disabled.\n");
	theSystem->get_cpu(0)->set_pc(theSystem->get_cpu(0)->get_clean_pc());
	return 0;
      }
    }
    if (!strncasecmp(command[0],"DISASSEMBLE",strlen(command[0])))
    {
      if (!strcasecmp(command[1],"ON"))
      {
	printf("%%IDB-I-DISON : Disassembling enabled.\n");
	bDisassemble = true;
	return 0;
      }
      if (!strcasecmp(command[1],"OFF"))
      {
	printf("%%IDB-I-DISOFF: Disassembling disabled.\n");
	bDisassemble = false;
	return 0;
      }
    }
    if (!strncasecmp(command[0],"BREAKPOINT",strlen(command[0])))
    {
      if (!strncasecmp(command[1],"OFF",strlen(command[1])))
      {
	printf("%%IDB-I-BRKOFF: Breakpoint disabled.\n");
	bBreakPoint = false;
	return 0;
      }
    }
    if (!strncasecmp(command[0],"RUN",strlen(command[0])))
    {
      result = sscanf(command[1],"%d",&RunCycles);
      if (result != 1)
      {
	printf("%%IDB-F-INVVAL: Invalid decimal value.\n");
	return 0;
      }
      if (bBreakPoint)
      {
        printf("%%IDB-I-RUNCBP: Running until breakpoint found or max cycles reached.\n");
        switch (iBreakPointMode)
	{
        case -1:
	  for (i=0;i<RunCycles;i++)
	  {
	    if (theSystem->SingleStep())
	    {
	      printf("%%IDB-I-ABORT : Abort run requested (probably from serial port)\n");
	      break;
	    }
 	    if (theSystem->get_cpu(0)->get_clean_pc() < iBreakPoint)
	    {
              printf("%%IDB-I-BRKPT : Breakpoint encountered.\n");
	      break;
	    }
	  }
	  break;
        case 0:
	  for(i=0;i<RunCycles;i++)
	  {
	    if (theSystem->SingleStep())
	    {
	      printf("%%IDB-I-ABORT : Abort run requested (probably from serial port)\n");
	      break;
	    }
	    if (theSystem->get_cpu(0)->get_clean_pc() == iBreakPoint)
	    {
              printf("%%IDB-I-BRKPT : Breakpoint encountered.\n");
	      break;
	    }
	  }
	  break;
        case 1:
	  for(i=0;i<RunCycles;i++)
	  {
	    if (theSystem->SingleStep())
	    {
	      printf("%%IDB-I-ABORT : Abort run requested (probably from serial port)\n");
	      break;
	    }
	    if (theSystem->get_cpu(0)->get_clean_pc() > iBreakPoint)
	    {
              printf("%%IDB-I-BRKPT : Breakpoint encountered.\n");
	      break;
	    }
	  }
	  break;
	case 2:
	  for(i=0;i<RunCycles;i++)
	  {
	    if (theSystem->SingleStep())
	    {
	      printf("%%IDB-I-ABORT : Abort run requested (probably from serial port)\n");
	      break;
	    }
	    if(theSystem->get_cpu(0)->get_last_instruction() == iBreakPointInstruction) 
	    {
	      printf("%%IDB-I-BRKPT: Found trapped instruction\n");
	      break;
	    }
	  }
	  break;
        default:
	  break;
	}
      }
      else
      {
        printf("%%IDB-I-RUNCYC: Running until max cycles reached.\n");
	extern int got_sigint;
	void sigint_handler(int);
	signal(SIGINT,&sigint_handler);
	for(i=0;i<RunCycles;i++) 
	{
	  if (theSystem->SingleStep())
	  {
	    printf("%%IDB-I-ABORT : Abort run requested (probably from serial port)\n");
	    break;
	  }
	  if(got_sigint) 
	    break;
	}
	got_sigint=0;
      }
      printf("%%IDB-I-ENDRUN: End of run.\n");
      return 0;
    }
    if (!strncasecmp(command[0],"JUMP",strlen(command[0])))
    {
      result = sscanf(command[1],"%" LL "x",&iJump);
      if (result != 1)
      {
	printf("%%IDB-F-INVVAL: Invalid hexadecimal value.\n");
	return 0;
      }
      if (iJump&X64(3))
      {
	printf("%%IDB-F-ALGVAL: Value not aligned on a 4-byte bounday.\n");
	return 0;
      }
      printf("%%IDB-I-JUMPTO: Jumping.\n");
      theSystem->get_cpu(0)->set_pc(iJump + (theSystem->get_cpu(0)->get_pc() & X64(1)));
      return 0;
    }
    break;
  case 3:
    if (!strncasecmp(command[0],"BREAKPOINT",strlen(command[0])))
    {
      if (!strcmp(command[1],"=") ||
	  !strcmp(command[1],">") ||
	  !strcmp(command[1],"<"))
      {
	result = sscanf(command[2],"%" LL "x",&iBreakPoint);
	if (result != 1)
	{
	  printf("%%IDB-F-INVVAL: Invalid hexadecimal value.\n");
	  bBreakPoint = false;
	  return 0;
	}
	if (iBreakPoint&X64(3))
	{
	  printf("%%IDB-F-ALGVAL: Value not aligned on a 4-byte bounday.\n");
	  bBreakPoint = false;
	  return 0;
	}
	switch (command[1][0])
	{
	case '=':
	  iBreakPointMode = 0;
	  break;
	case '>':
	  iBreakPointMode = 1;
	  break;
	case '<':
	  iBreakPointMode = -1;
	  break;
	}
	printf("%%IDB-I-BRKSET: Breakpoint set when PC %c %016" LL "x.\n",command[1][0],iBreakPoint);
	bBreakPoint = true;
	return 0;
      } 
      else 
      {
	if(!strncasecmp(command[1],"INSTRUCTION",strlen(command[1])))
	{
	  result = sscanf(command[2],"%" LL "x",&iBreakPointInstruction);
	  if (result != 1)
	  {
	    printf("%%IDB-F-INVVAL: Invalid hexadecimal value.\n");
	    bBreakPoint = false;
	    return 0;
	  }
	  printf("%%IDB-I-BRKSET: Breakpoint set when instruction %08x is executed.\n",iBreakPointInstruction);
	  bBreakPoint = true;
	  iBreakPointMode = 2;
	  return 0;
	}
      }
    }
    if (!strncasecmp(command[0],"LOAD",strlen(command[0])))
    {
      if (!strncasecmp(command[1],"CSV",strlen(command[1])))
      {
        read_procfile(command[2]);
	return 0;
      }
      if (!strncasecmp(command[1],"STATE",strlen(command[1])))
      {
	theSystem->RestoreState(command[2]);
	return 0;
      }
      if (!strncasecmp(command[1],"DPR",strlen(command[1])))
      {
	theDPR->RestoreStateF(command[2]);
	return 0;
      }
      if (!strncasecmp(command[1],"FLASH",strlen(command[1])))
      {
	theSROM->RestoreStateF(command[2]);
	return 0;
      }
    }
    if (!strncasecmp(command[0],"SAVE",strlen(command[0])))
    {
      if (!strncasecmp(command[1],"STATE",strlen(command[1])))
      {
	theSystem->SaveState(command[2]);
	return 0;
      }
      if (!strncasecmp(command[1],"DPR",strlen(command[1])))
      {
	theDPR->SaveStateF(command[2]);
	return 0;
      }
      if (!strncasecmp(command[1],"FLASH",strlen(command[1])))
      {
	theSROM->SaveStateF(command[2]);
	return 0;
      }
    }
    if (!strncasecmp(command[0],"LOADREG",strlen(command[0]))) {
      int reg;
      u64 value;     
      result = sscanf(command[1],"%d",&reg);
      if(result != 1 || reg > 31) {
	  printf("%%IDB-F-INVREG: Invalid register number.\n");
	  return 0;
      }
      result = sscanf(command[2],"%" LL "x",&value);
      if(result != 1) {
	  printf("%%IDB-F-INVVAL: Invalid hexadecimal value.\n");
	  return 0;
      }
      theSystem->get_cpu(0)->set_r(reg,value);
      return 0;
    }
    if (!strncasecmp(command[0],"LOADFPREG",strlen(command[0]))) {
      int reg;
      u64 value;     
      result = sscanf(command[1],"%d",&reg);
      if(result != 1 || reg > 31) {
	  printf("%%IDB-F-INVREG: Invalid register number.\n");
	  return 0;
      }
      result = sscanf(command[2],"%" LL "x",&value);
      if(result != 1) {
	  printf("%%IDB-F-INVVAL: Invalid hexadecimal value.\n");
	  return 0;
      }
      theSystem->get_cpu(0)->set_f(reg,value);
      return 0;
    }
    break;
  case 4:
    if (!strncasecmp(command[0],"LIST",strlen(command[0])) && !strcmp(command[2],"-"))
    {
      result = sscanf(command[1],"%" LL "x",&iFrom);
      if (result==1)
	result =  sscanf(command[3],"%" LL "x",&iTo);
      if (result != 1)
      {
        printf("%%IDB-F-INVVAL: Invalid hexadecimal value.\n");
	return 0;
      }
      if (iFrom&X64(3) || iTo&X64(3))
      {
        printf("%%IDB-F-ALGVAL: Value not aligned on a 4-byte bounday.\n");
	return 0;
      }
      if (iFrom > iTo)
      {
	printf("%%IDB-F-FRLTTO: From value exceeds to value.\n");
	return 0;
      }
      theSystem->get_cpu(0)->listing(iFrom,iTo);
      return 0;
    }
    break;
  default:
    break;
  }
  printf("%%IDB-F-SYNTAX: Syntax error. Type \"?\" or \"HELP\" for help.\n");
  return 0;
}

struct sRegion
{
  u64 from;
  u64 to;
  struct sRegion * pNext;
};

void CTraceEngine::list_all()
{
  struct sRegion * pR = NULL;
  struct sRegion ** ppN = &pR;
  struct sRegion *p = NULL;
  int f = 0;
  int t = 0;
  int ms   = 1<<(theSystem->get_memory_bits() - 3);
  u64 * pM = (u64*) theSystem->PtrToMem(0);
  

  for (;;)
  {
    while ((!pM[f] || pM[f] == X64(efefefefefefefef) || pM[f] == X64(ffffffffffffffff)) && f<ms)
    {
      f++;
      if (!(f&0x1ffff)) printf(".");
    }
    if (f>=ms)
      break;
    t = f;
    for(;;)
    {
      while (pM[t] && pM[t] != X64(efefefefefefefef) && pM[t] != X64(ffffffffffffffff) && t<ms)
      {
        t++;
        if (!(t&0x1ffff)) printf("x");
      }
      if (t+3<ms)
	if ((!pM[t+1] || pM[t+1] == X64(efefefefefefefef) || pM[t+1] == X64(ffffffffffffffff))
		&& (!pM[t+2] || pM[t+2] == X64(efefefefefefefef) || pM[t+2] == X64(ffffffffffffffff)) 
		&& (!pM[t+3] || pM[t+3] == X64(efefefefefefefef) || pM[t+3] == X64(ffffffffffffffff)))
	  break;
      if (t>=ms)
        break;
      t++;
      if (!(t&0x1ffff)) printf("x");
    }
    *ppN = new sRegion;
    (*ppN)->from = (u64)f*8;
    (*ppN)->to = (u64)((t-1)*8)+4;
    (*ppN)->pNext = NULL;
    ppN = &((*ppN)->pNext);
    f = t;
  }
  printf("\n");

  p = pR;

  while (p)
  {
    printf("\n======== DISASSEMBLING %08" LL "x TO %08" LL "x ========\n\n",p->from, p->to);
    theSystem->get_cpu(0)->listing(p->from,p->to);
    p = p->pNext;
  }
}

bool bTrace = false;
bool bDisassemble = false;
bool bHashing = false;
bool bListing = false;

#if defined(DEBUG_TB)
bool bTB_Debug = false;
#endif

#endif // IDB
