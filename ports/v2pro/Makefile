###########################
#
# Makefile for lwIP Stack
#
##########################

COMPILER=powerpc-eabi-gcc
ARCHIVER=powerpc-eabi-ar
ARFLAGS=rs
CP=cp
COMPILERFLAGS=-Wall -gdwarf
EXTRA_COMPILER_FLAGS = -DIPv4 -fpack-struct -msdata=eabi -mcpu=403
LWIPDIR=../../../lwip/src
LIB=liblwip4.a
RELEASEDIR=../../../../../edk/projects/insight_v2pro/ppc405_i/lib
INCLUDEDIR=../../../../../edk/projects/insight_v2pro/ppc405_i/include
LEVEL=0

#XLIBINCLUDEDIR=../../ppc405_i/include
XLIBINCLUDEDIR=../../../../../edk/projects/insight_v2pro/ppc405_i/include
XDRIVERSINCLUDEDIR=../../../../../edk/projects/insight_v2pro/

INCLUDES= \
	-I$(LWIPDIR)/include \
   -I$(LWIPDIR)/arch/$(LWIPARCH)/include \
   -I$(LWIPDIR)/include/ipv4 \
   -I$(XLIBINCLUDEDIR) \
	-I./include \
   -I.

INCLUDEFILES=../../../lwip/src/include

# COREFILES, CORE4FILES: The minimum set of files needed for lwIP.
COREFILES=$(LWIPDIR)/core/mem.c \
          $(LWIPDIR)/core/memp.c \
          $(LWIPDIR)/core/netif.c \
          $(LWIPDIR)/core/pbuf.c \
          $(LWIPDIR)/core/stats.c \
          $(LWIPDIR)/core/sys.c \
          $(LWIPDIR)/core/tcp.c \
          $(LWIPDIR)/core/tcp_in.c \
          $(LWIPDIR)/core/tcp_out.c \
          $(LWIPDIR)/core/udp.c \

CORE4FILES=$(LWIPDIR)/core/ipv4/icmp.c \
           $(LWIPDIR)/core/ipv4/ip.c \
	        $(LWIPDIR)/core/inet.c \
           $(LWIPDIR)/core/ipv4/ip_addr.c

# NETIFFILES: Files implementing various generic network interface functions.
NETIFFILES=$(LWIPDIR)/netif/etharp.c

# ARCHFILES: Archiecture specific files.
ARCHFILES=$(wildcard *.c \
                     netif/*.c)

# LWIPFILES: All the above.
LWIPFILES=$(COREFILES) $(CORE4FILES) $(NETIFFILES) $(ARCHFILES)
LWIPFILESW=$(wildcard $(LWIPFILES))
LWIPOBJS=$(notdir $(LWIPFILESW:.c=.o))


help:
	@echo ""
	@echo "This Makefile generates a library for the v2pro port of lwIP."
	@echo ""
	@echo "   Below are the targets:"
	@echo ""
	@echo "      make libs"
	@echo "            This is the target that will compile all lwIP files, and"
	@echo "            build the library, $(LIB).  The final library will be"
	@echo "            copied to $(RELEASEDIR)."
	@echo ""
	@echo "      make include"
	@echo "            Copies the lwIP include files into the EDK directory,"
	@echo "            $(INCLUDEDIR)"
	@echo ""
	@echo "      make clean"
	@echo "            Removes all object files and the $(LIB) library from"
	@echo "            the local directory."
	@echo ""
	@echo "      make depend"
	@echo "            Generates the .depend file, which contains the"
	@echo "            dependancies needed to compile each .c file.  The"
	@echo "            .depend file will be generated automatically for"
	@echo "            other targets, so it is not nessessary to run"
	@echo "            make depend."
	@echo ""

%.o:
	@echo ""
	@echo "Building File: $@"
	@echo ""
	$(COMPILER) $(COMPILERFLAGS) $(EXTRA_COMPILER_FLAGS) $(INCLUDES) -c $(<:.o=.c)

all ipv4 libs: $(LIB)
.PHONY: all ipv4 libs

include: dummy
	${CP} -rf ${INCLUDEFILES} ${INCLUDEDIR}

dummy:
clean:
	@echo ""
	@echo "Cleaning up..."
	@echo ""
	@rm -f *.o *.a .depend* core

depend dep: .depend
	@exit

$(LIB): $(LWIPOBJS)
	@echo ""
	@echo "Building lwIP Library, $@"
	$(ARCHIVER) $(ARFLAGS) $@ $?
	@cp $@ $(RELEASEDIR)/

.depend: $(LWIPFILES) $(APPFILES)
	@echo ""
	@echo "Building dependancies..."
	$(COMPILER) $(COMPILERFLAGS) $(EXTRA_COMPILER_FLAGS) $(INCLUDES) \
               -MM $^ > .depend || rm -f .depend

ifneq ($(MAKECMDGOALS), clean)
ifneq ($(MAKECMDGOALS), depend)
ifneq ($(MAKECMDGOALS), dep)
ifneq ($(MAKECMDGOALS), )
ifneq ($(MAKECMDGOALS), help)
-include .depend
endif
endif
endif
endif
endif
