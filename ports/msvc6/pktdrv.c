/*
 * Copyright (c) 2001,2002 Florian Schulze.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the authors nor the names of the contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * pktif.c - This file is part of lwIPtest
 *
 ****************************************************************************
 *
 * This file is derived from an example in lwIP with the following license:
 *
 * Copyright (c) 2001, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#define WIN32_LEAN_AND_MEAN
/* get the windows definitions of the following 4 functions out of the way */
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <packet32.h>
#include <ntddndis.h>

LPADAPTER  lpAdapter;
LPPACKET   lpPacket;
char buffer[256000];  // buffer to hold the data coming from the driver
unsigned char *cur_packet;
int cur_length;
unsigned char ethaddr[6];

/*-----------------------------------------------------------------------------------*/
int init_adapter(int adapter_num)
{
  #define Max_Num_Adapter 10

  void *AdapterList[Max_Num_Adapter];

	int i;
	DWORD dwVersion;
	DWORD dwWindowsMajorVersion;

	//unicode strings (winnt)
	wchar_t 	AdapterName[8192]; // string that contains a list of the network adapters
	wchar_t 	*temp,*temp1;

	//ascii strings (win95)
	char		AdapterNamea[8192]; // string that contains a list of the network adapters
	char		*tempa,*temp1a;

	int			AdapterNum=0;
	ULONG		AdapterLength;

  PPACKET_OID_DATA ppacket_oid_data;

	// obtain the name of the adapters installed on this machine
	AdapterLength=4096;

  memset(AdapterList,0,sizeof(AdapterList));

  i=0;

	// the data returned by PacketGetAdapterNames is different in Win95 and in WinNT.
	// We have to check the os on which we are running
	dwVersion=GetVersion();
	dwWindowsMajorVersion =  (DWORD)(LOBYTE(LOWORD(dwVersion)));
	if (!(dwVersion >= 0x80000000 && dwWindowsMajorVersion >= 4))
	{  // Windows NT
		if (PacketGetAdapterNames((char *)AdapterName,&AdapterLength)==FALSE){
			printf("Unable to retrieve the list of the adapters!\n");
			return -1;
		}
		temp=AdapterName;
		temp1=AdapterName;
		while ((*temp!='\0')||(*(temp-1)!='\0'))
		{
			if (*temp=='\0')
			{
				AdapterList[i] = temp1;
				temp1=temp+1;
				i++;
		}

		temp++;
		}

		AdapterNum=i;
		for (i=0; i<AdapterNum; i++)
			wprintf(L"%2i: %s\n", i, AdapterList[i]);
	}

	else	//windows 95
	{
		if (PacketGetAdapterNames(AdapterNamea,&AdapterLength)==FALSE){
			printf("Unable to retrieve the list of the adapters!\n");
			return -1;
		}
		tempa=AdapterNamea;
		temp1a=AdapterNamea;

		while ((*tempa!='\0')||(*(tempa-1)!='\0'))
		{
			if (*tempa=='\0')
			{
				AdapterList[i] = temp1a;
				temp1a=tempa+1;
				i++;
			}
			tempa++;
		}

		AdapterNum=i;
		AdapterNum=i;
		for (i=0; i<AdapterNum; i++)
			printf("%2i: %s", i, AdapterList[i]);
	}

  if (AdapterNum<=0)
    return -1;
  if (adapter_num < 0)
	  return -1;
  if (adapter_num >= AdapterNum)
	  return -1;
  ppacket_oid_data=malloc(sizeof(PACKET_OID_DATA)+6);
  lpAdapter=PacketOpenAdapter(AdapterList[adapter_num]);
	if (!lpAdapter || (lpAdapter->hFile == INVALID_HANDLE_VALUE))
	  return -1;
  ppacket_oid_data->Oid=OID_802_3_PERMANENT_ADDRESS;
  ppacket_oid_data->Length=6;
  if (!PacketRequest(lpAdapter,FALSE,ppacket_oid_data))
		return -1;
  memcpy(&ethaddr,ppacket_oid_data->Data,6);
  free(ppacket_oid_data);
  printf("MAC: %2X%2X%2X%2X%2X%2X\n", ethaddr[0], ethaddr[1], ethaddr[2], ethaddr[3], ethaddr[4], ethaddr[5]);
	PacketSetBuff(lpAdapter,512000);
	PacketSetReadTimeout(lpAdapter,1);
	PacketSetHwFilter(lpAdapter,NDIS_PACKET_TYPE_ALL_LOCAL);
	if ((lpPacket = PacketAllocatePacket())==NULL){
		return (-1);
	}
	PacketInitPacket(lpPacket,(char*)buffer,256000);

	return 0;
}

void shutdown_adapter(void)
{
	PacketFreePacket(lpPacket);
	PacketCloseAdapter(lpAdapter);
}

int packet_send(void *buffer, int len)
{
	LPPACKET lpPacket;

	if ((lpPacket = PacketAllocatePacket())==NULL)
 		return -1;
	PacketInitPacket(lpPacket,buffer,len);
	if (!PacketSendPacket(lpAdapter,lpPacket,TRUE))
		return -1;
	PacketFreePacket(lpPacket);

	return 0;
}

extern void process_input(void);

static void ProcessPackets(LPPACKET lpPacket)
{
	ULONG	ulLines, ulBytesReceived;
	char	*base;
	char	*buf;
	u_int off=0;
	u_int tlen,tlen1;
	struct bpf_hdr *hdr;

	ulBytesReceived = lpPacket->ulBytesReceived;

	buf = lpPacket->Buffer;

	off=0;

	while (off<ulBytesReceived)
  {	
		//if (kbhit())return;
		hdr=(struct bpf_hdr *)(buf+off);
		tlen1=hdr->bh_datalen;
		cur_length=tlen1;
		tlen=hdr->bh_caplen;
		off+=hdr->bh_hdrlen;

		ulLines = (tlen + 15) / 16;
		if (ulLines > 5) ulLines=5;

		base =(char*)(buf+off);
		cur_packet=base;
		off=Packet_WORDALIGN(off+tlen);

		process_input();
	}
}

void update_adapter(void)
{
  if (PacketReceivePacket(lpAdapter,lpPacket,TRUE)==TRUE)
    ProcessPackets(lpPacket);
  cur_length=0;
  cur_packet=NULL;
}
