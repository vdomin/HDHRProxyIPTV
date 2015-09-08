/*
* DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS HEADER.
*
* Copyright (c) 2014-2015 Vanesa Dominguez
*
* Contributor(s): Daniel Soto
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
* USA
*
*/

#pragma once

#include "string.h"
#include "Trace.h"

/**** Class not in use for execution ****/
class CRingBufferTS
{
public:
	CTrace* m_Traces;
	unsigned char *m_buffer;	//Buffer of packets TS
	unsigned char *m_ReadPtr;	//Pointer to the position fto read data
	unsigned char *m_WritePtr;	//Pointer to de position to write data
	unsigned char *m_EndBuffer;	//End Position of the buffer
	
	int m_BufferSize;	//Size of buffer
	int m_MaxWrite;		//Maximum free size to write
	int m_MaxRead;		//Maximum size to read
	int m_dataInBuffer;

	CRingBufferTS(int bufferSize);
	~CRingBufferTS();

	int GetSize() { return m_BufferSize; }
	int GetFreeSize() { return m_MaxWrite; }
	int GetBusySize() { return m_MaxRead; }

	void Initialize();
	void UpdateStateBuffer();

	int Insert(char* data, int size);  /* return error if size>free_size */
	int GetDataFromBuffer(char* data, int len); /* return error if size>busy_size*/
	int GetTSPackets(char* data);  /* return 1-7 valids 188 bytes packets */

	int CheckValidTSPacket();  /* return 1 if one valid 188bytes packet is found inside the buffer */ /* Implies resync and delete invalid data */
	void TreatingHTTPMessage(char* data, int size);
};
