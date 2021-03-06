############################################################################
# arch/z80/src/Makefile
#
#   Copyright (C) 2007, 2008, 2011-2012 Gregory Nutt. All rights reserved.
#   Author: Gregory Nutt <gnutt@nuttx.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. Neither the name NuttX nor the names of its contributors may be
#    used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
############################################################################

# Makefile fragments

include $(TOPDIR)/Make.defs
include chip/Make.defs
include board/Make.defs

# Compiler-Dependent Make:  SDCC or ZiLOG ZDS-II

COMPILER = ${shell basename "$(CC)"}
ifeq ($(COMPILER),sdcc)

  # Check for SDCC native windows build

  ifeq ($(CONFIG_WINDOWS_NATIVE),y)

    # SDCC Windows native build

    include Makefile.sdccw
  else

    # SDCC in a POSIX environment (Linux, macOS, or Cygwin/MSYS)

    include Makefile.sdccl
  endif
else

  # Check for ZDSII native windows build

  ifeq ($(CONFIG_WINDOWS_NATIVE),y)

    # ZDSII Windows native build

    include Makefile.zdsiiw
  else

    # ZDSII in a POSIX environment (Cygwin/MSYS)

    include Makefile.zdsiil
  endif
endif
