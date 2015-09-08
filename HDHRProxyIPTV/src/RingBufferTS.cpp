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

#include "stdafx.h"
#include "RingBufferTS.h"

CRingBufferTS::CRingBufferTS(int bufferSize)
{
	m_BufferSize = bufferSize;
	m_buffer = new unsigned char[bufferSize];
	
	m_Traces = new CTrace();

	Initialize();
}

CRingBufferTS::~CRingBufferTS()
{
	delete m_buffer;

}

void CRingBufferTS::Initialize()
{
	memset(m_buffer, 0, m_BufferSize);
	
	m_EndBuffer = m_buffer + m_BufferSize;
	m_WritePtr = m_buffer;
	m_ReadPtr = m_buffer;
	m_MaxWrite = m_BufferSize;
	m_MaxRead = 0;
	m_dataInBuffer = 0;
}

void CRingBufferTS::UpdateStateBuffer()
{
	if (m_ReadPtr == m_EndBuffer)
		m_ReadPtr = m_buffer;

	if (m_WritePtr == m_EndBuffer)
		m_WritePtr = m_buffer;

	if (m_WritePtr == m_ReadPtr)
	{
		if (m_dataInBuffer > 0)
		{
			m_MaxWrite = 0;
			m_MaxRead = m_BufferSize;
		}
		else
		{
			m_MaxWrite = m_BufferSize;
			m_MaxRead = 0;
		}
	}
}

int CRingBufferTS::Insert(char* data, int size)
{
	int maxToEnd = 0;

	if (size > m_MaxWrite)
		return 0;

	//Case where received size is bigger than free size in buffer until the end of the buffer and and has to continue on the beginning of the buffer 
	if (m_WritePtr > m_ReadPtr && (m_EndBuffer - m_WritePtr) < size)
	{
		maxToEnd = m_EndBuffer - m_WritePtr;

		if (((maxToEnd)+(m_ReadPtr - m_buffer)) < size)
			return 0;

		memcpy(m_WritePtr, data, maxToEnd);
		m_WritePtr = m_buffer;
		memcpy(m_WritePtr, &data[maxToEnd], size - maxToEnd);
		m_WritePtr += (size - maxToEnd);
		m_dataInBuffer += size;
		m_MaxRead += size;
		m_MaxWrite -= size;

		UpdateStateBuffer();

		return 1;
	}
	//Case where the write pointer has reached the end of the buffer and returned to start, and not the reading pointer.
	//In principle with the first check size > m_MaxWrite it is done already suffice, but it becomes to check to make sure there is enough space in that case
	else if (m_WritePtr < m_ReadPtr && (m_ReadPtr - m_WritePtr) < size)
		return 0;

	memcpy(m_WritePtr, data, size);
	m_WritePtr += size;
	m_dataInBuffer += size;
	m_MaxRead += size;
	m_MaxWrite -= size;

	UpdateStateBuffer();

	return 1;
}

int CRingBufferTS::GetDataFromBuffer(char* data, int len)
{
	int maxLenToEnd = 0;

	if (len > m_MaxRead)
		return 0;

	maxLenToEnd = m_EndBuffer - m_ReadPtr;
	if (m_ReadPtr > m_WritePtr && maxLenToEnd < len)
	{
		memcpy(data, m_ReadPtr, maxLenToEnd);
		m_ReadPtr = m_buffer;
		memcpy(&data[maxLenToEnd], m_ReadPtr, len - maxLenToEnd);
		m_ReadPtr += (len - maxLenToEnd);
		
		m_dataInBuffer -= len;
		m_MaxRead -= len;
		m_MaxWrite += len;

		UpdateStateBuffer();

		return 1;
	}

	memcpy(data, m_ReadPtr, len);
	m_ReadPtr += len;
	m_dataInBuffer -= len;
	m_MaxRead -= len;
	m_MaxWrite += len;

	UpdateStateBuffer();

	return 1;
}

int  CRingBufferTS::GetTSPackets(char* data)
{
	if (m_MaxRead == 0) return 0;

	unsigned char* findIniTS;
	int indexINI = 0;
	int tamReadPcks = 188;
	int resync = 0;
	int numTSPackets = 0;
	int findSync = 0;
	int finishResync = 0;
	int temp = 0;
	int nbytes = 0;
	char log_output[1024];
	memset(log_output, 0, 1024);

	char *eraseData = (char*)malloc(m_MaxRead);

	strcpy(eraseData, "");

	strcpy(data, "");

	//G = 0x47 --> Start of packet of Tranports Stream
	findIniTS = (unsigned char*)strstr((const char*)m_ReadPtr, (const char*) "G");
	if (findIniTS == NULL)
	{
		if (m_ReadPtr > m_WritePtr && (m_EndBuffer - m_ReadPtr) < m_MaxRead)
		{
			GetDataFromBuffer(eraseData, m_EndBuffer - m_ReadPtr);
			findIniTS = (unsigned char*)strstr((const char*)m_ReadPtr, (const char*) "G");
			if (findIniTS == NULL)
			{
				if (m_Traces->IsLevelWriteable(LEVEL_TRZ_3))
				{
					_snprintf(log_output, sizeof(log_output) - 2, "TRANSPORT  :: Resynchronization: All bytes (%d) discarded [Start reading a packet of buffer (0)]\n", m_MaxRead);
					m_Traces->WriteTrace(log_output, LEVEL_TRZ_3);
				}

				GetDataFromBuffer(eraseData, m_MaxRead); //If no package is valid all content is removed
				if (eraseData)
					delete[]eraseData;
				return 0;
			}
			else
			{
				indexINI = findIniTS - m_ReadPtr;
				if (indexINI != 0)
				{
					GetDataFromBuffer(eraseData, indexINI);
					if (m_Traces->IsLevelWriteable(LEVEL_TRZ_3))
					{
						_snprintf(log_output, sizeof(log_output) - 2, "TRANSPORT  :: Resynchronization: %d bytes discarded [Start reading a packet of buffer (1)]\n", indexINI);
						m_Traces->WriteTrace(log_output, LEVEL_TRZ_3);
					}
				}
			}
		}
		else
		{
			if (m_Traces->IsLevelWriteable(LEVEL_TRZ_3))
			{
				_snprintf(log_output, sizeof(log_output) - 2, "TRANSPORT  :: Resynchronization: All bytes (%d) discarded [Start reading a packet of buffer (1)]\n", m_MaxRead);
				m_Traces->WriteTrace(log_output, LEVEL_TRZ_3);
			}
			GetDataFromBuffer(eraseData, m_MaxRead); //If no package is valid all content is removed
			if (eraseData)
				delete[]eraseData;
			return 0;
		}
	}
	indexINI = findIniTS - m_ReadPtr;

	if (indexINI != 0)
	{
		GetDataFromBuffer(eraseData, indexINI);

		if (m_Traces->IsLevelWriteable(LEVEL_TRZ_3))
		{
			_snprintf(log_output, sizeof(log_output) - 2, "TRANSPORT  :: Resynchronization: %d bytes discarded [Start reading a packet of buffer (2)]\n", indexINI);
			m_Traces->WriteTrace(log_output, LEVEL_TRZ_3);
		}
	}

	//Correct TS packets is obtained, validating the first sync byte(0x47 = 'G')
	for (int i = 0; i < 7; i++)
	{
		if (m_MaxRead >= tamReadPcks)
		{
			if (m_MaxRead == tamReadPcks)
			{
				tamReadPcks += 188;
				numTSPackets++;
				i = 7;
			}
			else if (m_WritePtr < m_ReadPtr)
			{
				if ((m_EndBuffer - m_ReadPtr) < tamReadPcks)
				{
					temp = tamReadPcks - (m_EndBuffer - m_ReadPtr);
					if (m_buffer[temp] == 'G')
					{
						tamReadPcks += 188;
						numTSPackets++;
					}
					else
					{
						resync = 1;
						i = 7;
					}
				}
				else if ((m_EndBuffer - m_ReadPtr) == tamReadPcks)
				{
					tamReadPcks += 188;
					numTSPackets++;
				}
				else if (m_ReadPtr[tamReadPcks] == 'G')
				{
					tamReadPcks += 188;
					numTSPackets++;
				}
				else
				{
					resync = 1;
					i = 7;
				}
			}
			else if (m_ReadPtr[tamReadPcks] == 'G')
			{
				tamReadPcks += 188;
				numTSPackets++;
			}
			else
			{
				resync = 1;
				i = 7;
			}
		}
		else // There is Not enough data in the buffer
		{
			i = 7;
		}
	}

	if (numTSPackets)
	{
		if (!GetDataFromBuffer(data, numTSPackets * 188))
			nbytes = 0;
		else
		{
			nbytes = numTSPackets * 188;
			if (m_Traces->IsLevelWriteable(LEVEL_TRZ_6))
			{
				_snprintf(log_output, sizeof(log_output) - 2, "TRANSPORT  :: Read %d packets. %d bytes.\n", numTSPackets, nbytes);
				m_Traces->WriteTrace(log_output, LEVEL_TRZ_6);
			}
		}
	}

	//Resynchronization
	if (resync) // Desynchronization it found. Have to find again the synchronization of packets
	{
		if (m_Traces->IsLevelWriteable(LEVEL_TRZ_3))
		{
			_snprintf(log_output, sizeof(log_output) - 2, "TRANSPORT  :: HAVE TO Resynchronize. Read Position at buffer: %d; Length of dates at buffer: %d(%d)\n", m_EndBuffer - m_ReadPtr, m_dataInBuffer, m_MaxRead);
			m_Traces->WriteTrace(log_output, LEVEL_TRZ_3);
		}
		if (CheckValidTSPacket())
		{
			m_Traces->WriteTrace("TRANSPORT  :: Resynchronization OK of TS packets at buffer.\n", LEVEL_TRZ_6);
			m_Traces->WriteTrace("TRANSPORT  :: Resynchronization OK of TS packets at buffer.\n", LEVEL_TRZ_2);
		}
		else
		{
			m_Traces->WriteTrace("TRANSPORT  :: Resynchronization of TS packets at buffer. NOT find sync sequence\n", LEVEL_TRZ_6);
			m_Traces->WriteTrace("TRANSPORT  :: Resynchronization of TS packets at buffer. NOT find sync sequence\n", LEVEL_TRZ_2);
		}
	}

	delete[]eraseData;

	return nbytes;
}

int CRingBufferTS::CheckValidTSPacket()
{
	if (m_MaxRead == 0) return 0;

	unsigned char* findIniTS;
	char* eraseData = new char[m_MaxRead];
	strcpy(eraseData, "");
	int indexINI = 0;
	int tamReadPcks = 188;
	int findSync = 0;
	int finishResync = 0;
	char log_output[1024];
	memset(log_output, 0, 1024);

	findIniTS = (unsigned char*)strstr((const char*)m_ReadPtr, (const char*) "G");
	if (findIniTS == NULL)
	{
		if (m_ReadPtr > m_WritePtr && (m_EndBuffer - m_ReadPtr) < m_MaxRead)
		{
			GetDataFromBuffer(eraseData, m_EndBuffer - m_ReadPtr);
			findIniTS = (unsigned char*)strstr((const char*)m_ReadPtr, (const char*) "G");
			if (findIniTS == NULL)
			{
				m_Traces->WriteTrace("TRANSPORT  :: [Buffer] CheckValidTSPacket : Valid packet not found at buffer\n", LEVEL_TRZ_6);

				if (m_Traces->IsLevelWriteable(LEVEL_TRZ_3))
				{
					_snprintf(log_output, sizeof(log_output) - 2, "TRANSPORT  :: Resynchronization: All bytes (%d) discarded\n", m_MaxRead);
					m_Traces->WriteTrace(log_output, LEVEL_TRZ_3);
				}
				GetDataFromBuffer(eraseData, m_MaxRead); //If no package is valid all content is removed

				if (eraseData)
					delete[]eraseData;
				return 0;
			}
			else
			{
				indexINI = findIniTS - m_ReadPtr;
				if (indexINI != 0)
				{
					GetDataFromBuffer(eraseData, indexINI);
					if (m_Traces->IsLevelWriteable(LEVEL_TRZ_3))
					{
						_snprintf(log_output, sizeof(log_output) - 2, "TRANSPORT  :: Resynchronization: %d bytes discarded\n", indexINI);
						m_Traces->WriteTrace(log_output, LEVEL_TRZ_3);
					}
				}
			}
		}
		else
		{
			m_Traces->WriteTrace("TRANSPORT  :: [Buffer] CheckValidTSPacket : Valid packet not found at buffer\n", LEVEL_TRZ_6);

			if (m_Traces->IsLevelWriteable(LEVEL_TRZ_3))
			{
				_snprintf(log_output, sizeof(log_output) - 2, "TRANSPORT  :: Resynchronization: All bytes (%d) discarded\n", m_MaxRead);
				m_Traces->WriteTrace(log_output, LEVEL_TRZ_3);
			}

			GetDataFromBuffer(eraseData, m_MaxRead); //If no package is valid all content is removed

			if (eraseData)
				delete[]eraseData;
			return 0;
		}
	}
	else
	{
		indexINI = findIniTS - m_ReadPtr;

		if (indexINI != 0)
		{
			GetDataFromBuffer(eraseData, indexINI);

			if (m_Traces->IsLevelWriteable(LEVEL_TRZ_3))
			{
				_snprintf(log_output, sizeof(log_output) - 2, "TRANSPORT  :: Resynchronization: %d bytes discarded\n", indexINI);
				m_Traces->WriteTrace(log_output, LEVEL_TRZ_3);
			}
		}

		tamReadPcks = 188 + 188; //2 TS packets paquetes TS consecutives
		while (!findSync && !finishResync)
		{
			if (m_MaxRead >= tamReadPcks)
			{
				//Synchronization
				if (m_ReadPtr > m_WritePtr && m_EndBuffer - m_ReadPtr < m_MaxRead)
				{
					if (m_buffer[188 - (m_EndBuffer - m_ReadPtr)] == 'G')
						findSync = 1;
				}
				else if (m_ReadPtr[188] == 'G')
				{
					findSync = 1;
				}
				else
				{
					indexINI = findIniTS - m_ReadPtr;
					if (indexINI != m_BufferSize - 1)
					{
						findIniTS = (unsigned char*)strstr((const char*)m_ReadPtr + 1, (const char*) "G");
						if (findIniTS == NULL)
						{
							if (m_ReadPtr > m_WritePtr && (m_EndBuffer - m_ReadPtr) < m_MaxRead)
							{
								GetDataFromBuffer(eraseData, m_EndBuffer - m_ReadPtr);
								findIniTS = (unsigned char*)strstr((const char*)m_ReadPtr, (const char*) "G");
								if (findIniTS == NULL)
								{
									m_Traces->WriteTrace("TRANSPORT  :: [Buffer] CheckValidTSPacket : Valid packet not found at buffer\n", LEVEL_TRZ_6);
									if (m_Traces->IsLevelWriteable(LEVEL_TRZ_3))
									{
										_snprintf(log_output, sizeof(log_output) - 2, "TRANSPORT  :: Resynchronization: All bytes (%d) discarded\n", m_MaxRead);
										m_Traces->WriteTrace(log_output, LEVEL_TRZ_3);
									}

									GetDataFromBuffer(eraseData, m_MaxRead); //If no package is valid all content is removed
									finishResync = 1;
								}
								else
								{
									indexINI = findIniTS - m_ReadPtr;
									if (indexINI != 0)
									{
										GetDataFromBuffer(eraseData, indexINI);
										if (m_Traces->IsLevelWriteable(LEVEL_TRZ_3))
										{
											_snprintf(log_output, sizeof(log_output) - 2, "TRANSPORT  :: Resynchronization: %d bytes discarded\n", indexINI);
											m_Traces->WriteTrace(log_output, LEVEL_TRZ_3);
										}
									}
								}
							}
							else
							{
								m_Traces->WriteTrace("TRANSPORT  :: [Buffer] CheckValidTSPacket : Valid packet not found at buffer\n", LEVEL_TRZ_6);

								if (m_Traces->IsLevelWriteable(LEVEL_TRZ_3))
								{
									_snprintf(log_output, sizeof(log_output) - 2, "TRANSPORT  :: Resynchronization: All bytes (%d) discarded\n", m_MaxRead);
									m_Traces->WriteTrace(log_output, LEVEL_TRZ_3);
								}
								GetDataFromBuffer(eraseData, m_MaxRead); //If no package is valid all content is removed

								finishResync = 1;
							}
						}
						else
						{
							indexINI = findIniTS - m_ReadPtr;
							if (indexINI != 0)
							{
								GetDataFromBuffer(eraseData, indexINI);
								if (m_Traces->IsLevelWriteable(LEVEL_TRZ_3))
								{
									_snprintf(log_output, sizeof(log_output) - 2, "TRANSPORT  :: Resynchronization: %d bytes discarded\n", indexINI);
									m_Traces->WriteTrace(log_output, LEVEL_TRZ_3);
								}
							}
						}
					}
					else
					{
						finishResync = 1;
					}
				}
			}
			else // There is Not enough data in the buffer
			{
				finishResync = 1;
			}
		}
	}

	if (eraseData)
		delete []eraseData;

	if (findSync)
		return 1;
	else
		return 0;
}

void CRingBufferTS::TreatingHTTPMessage(char* data, int size)
{
	char traza[1316];
	int indexINI = 0;
	char* findIniTS;

	if (strstr(data, "HTTP/1.0 200 OK") != NULL || strstr(data, "HTTP/1.1 200 OK") != NULL)
	{
		m_Traces->WriteTrace("TRANSPORT  :: Received response HTTP message.\n", LEVEL_TRZ_3);

		findIniTS = (char*)strstr((const char*)data, (const char*) "\r\n\r\n");
		if (findIniTS != NULL)
		{
			strcpy(traza, "TRANSPORT  :: Header of response HTTP message received:\n");
			indexINI = findIniTS - data;
			strncat(traza, data, indexINI);
			strcat(traza, "\n");
			m_Traces->WriteTrace(traza, LEVEL_TRZ_4);

			m_Traces->WriteTrace("Complete received package with HTTP header:\n", LEVEL_TRZ_5);
			strncpy(traza, data, size);
			m_Traces->WriteTrace(traza, LEVEL_TRZ_5);
			m_Traces->WriteTrace("\n", LEVEL_TRZ_5);

			if (size - (indexINI + 4) != 0)
			{
				Insert(&data[indexINI + 4], size - (indexINI + 4));
				if (!CheckValidTSPacket())
					m_Traces->WriteTrace("TRANSPORT  :: Content of response HTTP message have not synchronization.\n", LEVEL_TRZ_3);
				else
					m_Traces->WriteTrace("TRANSPORT  :: Content of response HTTP message have synchronization SYNC.\n", LEVEL_TRZ_3);
			}
			else
				m_Traces->WriteTrace("TRANSPORT  :: Response HTTP message doesn't have TS content. Not save data at buffer\n", LEVEL_TRZ_3);
		}
	}
}
