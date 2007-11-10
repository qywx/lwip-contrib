lwIP for Win32

***WARNING***
The current CVS code of this port isn't much tested.
***WARNING***

This is a quickly hacked port and example project of the lwIP library to
Win32/MSVC.

It doesn't (yet?) include support for slipif, ppp or pppoe. This is simply
because none of the active developers using this port are using these interfaces
right now.

To get this compiling, you have to set a couple of environment variables:
- LWIP_SRC: points to the src subdirectory in the main lwip tree
- PCAP_DIR: points to the WinPcap Developer's Packs (containing 'include' and 'lib')

You also will have to copy the file 'lwipcfg_msvc.h.example' to
'lwipcfg_msvc.h' and modify to suit your needs (WinPcap adapter number,
IP configuration).


Included in the proj/msvc6 directory is the network interface driver using
the winpcap library.

There is no more documentation yet. Try to figure it out yourself.

This is provided as is, it's just a hack to test some stuff, no serious
implementation.

Florian Schulze (florian.proff.schulze@gmx.net)
Simon Goldschmidt

lwIP: http://www.sics.se/~adam/lwip/
WinPCap: http://netgroup-serv.polito.it/winpcap/
