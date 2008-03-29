define(ES_ASK_Q, 
  while true; do
    echo -n " $1? [$2]: "
    read ac_tmp 
    if test "X$ac_tmp" = "X"; then
      ac_tmp="$2"
    fi
    if test "$ac_tmp" = "y" -o "$ac_tmp" = "ye" -o "$ac_tmp" = "yes"; then
      ifelse($3, , :, $3)
      break
    fi
    if test "$ac_tmp" = "n" -o "$ac_tmp" = "no"; then
      ifelse($4, , :, $4)
      break
    fi
    echo "Invalid value; please enter yes or no"
  done
)

define(ES_ASK_DEBUG_Q,
  ES_ASK_Q(Do you want to enable $1 debugging, no, dbg="yes", dbg="no")
  if test "$dbg" = "yes"; then
    cat >>src/config_debug.h <<EOF

// Enable $1 debugging
#define DEBUG_$2 1
EOF
  else
    cat >>src/config_debug.h <<EOF

// Disable $1 debugging
//#define DEBUG_$2 1 
EOF
  fi
)

define(ES_DEBUG_Q, 
  cat >src/config_debug.h <<EOF
// This file contains the debug configuration options.
// This file was generated by configure_1.sh
EOF
  ES_ASK_Q(Do you want the defaults for all options, yes, defopt="yes", defopt="no")
  if test "$defopt" = "no"; then
    ES_ASK_Q(Do you want to show the cycle counter, yes, cyc="yes", cyc="no")
    if test "$cyc" = "no"; then
      cat >>src/config_debug.h <<EOF

// Disable the cycle counter
#define HIDE_COUNTER 1
EOF
    else
      cat >>src/config_debug.h <<EOF

// Enable the cycle counter
//#define HIDE_COUNTER 1
EOF
    fi
    ES_ASK_DEBUG_Q(VGA, VGA)
    ES_ASK_DEBUG_Q(Serial Port, SERIAL)
    ES_ASK_Q(Do you want to enable one or more IDE debugging options, no, ide="yes", ide="no")
    if test "$ide" = "yes"; then
      ES_ASK_DEBUG_Q(all IDE, IDE)
      if test "$dbg" = "no"; then
        ES_ASK_DEBUG_Q(IDE Busmaster, IDE_BUSMASTER)
        ES_ASK_DEBUG_Q(IDE Command, IDE_COMMAND)
        ES_ASK_DEBUG_Q(IDE DMA, IDE_DMA)
        ES_ASK_DEBUG_Q(IDE Interrupt, IDE_INTERRUPT)
        ES_ASK_DEBUG_Q(IDE Command Register, IDE_REG_COMMAND)
        ES_ASK_DEBUG_Q(IDE Control Register, IDE_REG_CONTROL)
        ES_ASK_DEBUG_Q(IDE ATAPI Packet, IDE_PACKET)
        ES_ASK_DEBUG_Q(IDE Thread, IDE_THREADS)
        ES_ASK_DEBUG_Q(IDE Mutexes, IDE_LOCKS)
      fi
    fi
    ES_ASK_DEBUG_Q(unknown memory access, UNKMEM)
    ES_ASK_DEBUG_Q(PCI, PCI)
    ES_ASK_DEBUG_Q(Translationbuffer, TB)
    ES_ASK_DEBUG_Q(I/O Port Access, PORTACCESS)
    ES_ASK_DEBUG_Q(SCSI, SCSI)
    ES_ASK_DEBUG_Q(Keyboard, KBD)
    ES_ASK_DEBUG_Q(Programmable Interrupt Controller (PIC), PIC)
    ES_ASK_DEBUG_Q(Printer port, LPT)
    ES_ASK_DEBUG_Q(USB Controller, USB)
    ES_ASK_DEBUG_Q(Symbios SCSI Controller, SYM)
    ES_ASK_DEBUG_Q(DMA Controller, DMA)
    ES_ASK_DEBUG_Q(backtrace on SIGSEGV, BACKTRACE)
    ES_ASK_DEBUG_Q(mutex, LOCKS)
  fi
)
#! /bin/sh
#
# Configure debugging options for es40.
#
echo "This is the debug-options configuration script for the ES40 emulator"
echo "If you don't want any debugging options enabled, answer YES to the"
echo "following question"
echo ""
ES_DEBUG_Q
