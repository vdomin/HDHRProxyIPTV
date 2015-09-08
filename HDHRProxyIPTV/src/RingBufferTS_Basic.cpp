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
#include "RingBufferTS_Basic.h"

CRingBufferTS_Basic::CRingBufferTS_Basic()
{
	m_cfgProxy = CConfigProxy::GetInstance();

	strcpy(m_buffer, "");
	m_posWrite = 0;
	m_posRead = 0;
	m_bufferSize = 78960;
	m_freeSpace = m_bufferSize;
	m_BusySpace = 0;
	m_pidsToFilterList.Format(L"");
	m_applyPidFiltering = 0;

	m_BusySpaceCopy = 0;
	m_posReadCopy = 0;

	strcpy(m_buffer_output, "");
	m_numTSPacketsOutput = 0;

	m_Traces = new CTrace();

	semaphoreTreatingPIDs = CreateSemaphore(
		NULL,           // default security attributes
		1,  // initial count
		1,  // maximum count
		NULL);          // unnamed semaphore

	for (int i = 0; i < 1024; i++)
		m_pidsList[i] = 0;

	m_filteredPacketsSent = 0;
	m_filteredOutPacketsDescarted = 0;
	m_nullPacketsSent = 0;
	m_receivedPackets = 0;
	m_timetoCheckAnalysis = 3000000000; //In nanoseconds = 3 seconds
	GetLocalTime(&m_timeLastAnalysis);
}

CRingBufferTS_Basic::~CRingBufferTS_Basic()
{
	if (m_Traces)
		delete m_Traces;
}

void CRingBufferTS_Basic::Initialize(CString pidsToFilterList)
{
	strcpy(m_buffer, "");
	m_posWrite = 0;
	m_posRead = 0;
	m_freeSpace = m_bufferSize;
	m_BusySpace = 0;

	m_BusySpaceCopy = 0;
	m_posReadCopy = 0;

	m_pidsToFilterList = pidsToFilterList;
}

int CRingBufferTS_Basic::Insert(char* data, int size)
{
	char log_output[1024];
	memset(log_output, 0, 1024);

	if (size > m_freeSpace)
	{
		if (m_BusySpace + size < m_bufferSize)
		{
			memcpy(&m_buffer[0], &m_buffer[m_posRead], m_BusySpace);

			m_posWrite = m_BusySpace - 1;
			m_posWrite = m_BusySpace;
			m_posRead = 0;
			m_freeSpace = m_bufferSize - m_BusySpace;

			m_posReadCopy = 0;
		}
		else
			return 0;

	}

	memcpy(&m_buffer[m_posWrite], data, size);

	m_posWrite += size;
	m_freeSpace -= size;
	m_BusySpace += size;

	m_BusySpaceCopy = m_BusySpace;
	m_receivedPackets += size / 188;

	if (m_Traces->IsLevelWriteable(LEVEL_TRZ_6))
	{
		_snprintf(log_output, 1024 - 2, "TRANSPORT  :: Saved at buffer:          %d bytes.\n", size);
		m_Traces->WriteTrace(log_output, LEVEL_TRZ_6);
	}

	return 1;
}

int CRingBufferTS_Basic::GetTSPacket(char* data)
{
	int size = 188;  //Size of a Tranport Stream packet: 188 bytes
	char log_output[1024];
	memset(log_output, 0, 1024);

	if (m_BusySpace > size)
	{
		//Initial data on buffer is sync, it can obtain a correct packet 188 bytes
		if (m_buffer[m_posRead] == 'G' && m_buffer[m_posRead + 188] == 'G')
		{
			GetData(data, size);
		}
		else
		{
			if (m_Traces->IsLevelWriteable(LEVEL_TRZ_3))
			{
				_snprintf(log_output, 1024 - 2, "TRANSPORT  :: HAVE TO Resynchronize. Read Position at buffer: %d; Length of dates at buffer: %d\n", m_posRead, m_BusySpace);
				m_Traces->WriteTrace(log_output, LEVEL_TRZ_3);
			}

			if (CheckValidTSPacket())
			{
				GetData(data, size);
				m_Traces->WriteTrace("TRANSPORT  :: Resynchronization OK of TS packets at buffer.\n", LEVEL_TRZ_2);
			}
			else
			{
				m_Traces->WriteTrace("TRANSPORT  :: Resynchronization of TS packets at buffer. NOT find sync sequence\n", LEVEL_TRZ_2);
				return 0;
			}
		}
	}
	else
		return 0;

	return size;
}

int CRingBufferTS_Basic::GetData(char* data, int size)
{
	if (m_BusySpace > size)
	{
		memcpy(data, &m_buffer[m_posRead], size);
		m_posRead += size;
		m_BusySpace -= size;

		m_posReadCopy = m_posRead;
		m_BusySpaceCopy = m_BusySpace;

	}
	else
		return 0;

	return size;
}

int CRingBufferTS_Basic::CheckValidTSPacket()
{
	int pos = 0;
	char* findIniTS;
	char* eraseData = new char[m_BusySpace];
	strcpy(eraseData, "");
	int findSync = 0;
	int finishCheck = 0;
	char log_output[1024];
	memset(log_output, 0, 1024);

	while (!findSync && !finishCheck)
	{
		//G = 0x47 --> Initiation of packet of Tranports Stream
		findIniTS = strchr(&m_buffer[m_posRead], 'G');
		
		if (findIniTS == NULL)
		{
			m_Traces->WriteTrace("TRANSPORT  :: [Buffer] CheckValidTSPacket : Valid packet not found at buffer\n", LEVEL_TRZ_6);
			if (m_Traces->IsLevelWriteable(LEVEL_TRZ_3))
			{
				_snprintf(log_output, 1024 - 2, "TRANSPORT  :: Resynchronization: All bytes (%d) discarded\n", m_BusySpace);
				m_Traces->WriteTrace(log_output, LEVEL_TRZ_3);
			}
			GetData(eraseData, m_BusySpace);
			finishCheck = 1;
		}
		else
		{
			pos = findIniTS - &m_buffer[m_posRead];
			if (pos != 0)
			{
				if (m_Traces->IsLevelWriteable(LEVEL_TRZ_3))
				{
					_snprintf(log_output, 1024 - 2, "TRANSPORT  :: Resynchronization: %d bytes discarded\n", pos);
					m_Traces->WriteTrace(log_output, LEVEL_TRZ_3);
				}
				GetData(eraseData, pos);
			}

			if (m_buffer[m_posRead] == 'G' && m_buffer[m_posRead + 188] == 'G')
			{
				findSync = 1;
			}
		}
	}

	if (eraseData)
		delete[]eraseData;

	return findSync;
}

int CRingBufferTS_Basic::GetMultipleTSPacket(char* data, int numMaxPackets)
{
	int numTSPackets = 0;
	char packet[1316];
	strcpy(packet, "");
	char log_output[1024];
	memset(log_output, 0, 1024);
	int findNumPacketToSend = 0;
	int endNotFound = 0;
	int numPacksToEliminate = 0;
	int i = 0;

	m_Traces->WriteTrace("TRANSPORT  :: **** START TREATMENT OF GETTING Packets of buffer to send ****\n", LEVEL_TRZ_6);

	//Not necessary apply internal PID Filtering
	if (!m_applyPidFiltering || !m_pidsToFilterList.Compare(L""))
	{
		for (i = 0; i < NUM_PACKETS_TO_SEND; i++)
		{
			if (GetTSPacket(packet))
			{
				//Not apply PID Filtering. It sends all packets
				memcpy(&data[i * 188], packet, 188);
				numTSPackets++;

			}
			else
				i = NUM_PACKETS_TO_SEND;
		}
		
		m_filteredPacketsSent += numTSPackets;

		if (m_Traces->IsLevelWriteable(LEVEL_TRZ_6))
		{
			_snprintf(log_output, 1024 - 2, "TRANSPORT  :: Read from buffer to send: %d bytes. %d packets. More Info Buffer: Read Position[%d]; Write Position[%d]\n",
				numTSPackets * 188, numTSPackets, m_posRead, m_posWrite);
			m_Traces->WriteTrace(log_output, LEVEL_TRZ_6);
		}
	}
	else
	{
		while (!findNumPacketToSend && !endNotFound)
		{
			if (GetTSPacket(packet))
			{
				if (PIDFiltering(packet))
				{
					memcpy(&m_buffer_output[m_numTSPacketsOutput * 188], packet, 188);
					m_numTSPacketsOutput++;

					if (m_Traces->IsLevelWriteable(LEVEL_TRZ_6))
					{
						_snprintf(log_output, 1024 - 2, "TRANSPORT  :: Add TS packet to output buffer. Number of packet: %d\n", m_numTSPacketsOutput);
						m_Traces->WriteTrace(log_output, LEVEL_TRZ_6);
					}

					//Substract m_maxTimeToPacket to m_maxTimeToSendDgram
					SubstractTimeToPacket();

					if (m_numTSPacketsOutput == NUM_PACKETS_TO_SEND)
					{
						findNumPacketToSend = 1;
						numTSPackets = m_numTSPacketsOutput;
						m_numTSPacketsOutput = 0;
						memcpy(data, m_buffer_output, MAX_SIZE_DATAGRAM_TO_SEND);
						strcpy(m_buffer_output, "");

						GetLocalTime(&m_timeLastSending);

						m_Traces->WriteTrace("TRANSPORT  :: 7 TS packets (PIDs) at output buffer to send\n", LEVEL_TRZ_6);

						if (m_Traces->IsLevelWriteable(LEVEL_TRZ_6))
						{
							_snprintf(log_output, 1024 - 2, "TRANSPORT  :: (PID filtering) Read from buffer to send: %d bytes. %d packets. More Info Buffer: Read Position[%d]; Write Position[%d]\n",
								numTSPackets * 188, numTSPackets, m_posRead, m_posWrite);
							m_Traces->WriteTrace(log_output, LEVEL_TRZ_6);
						}

						m_filteredPacketsSent += numTSPackets;

					}
				}
				else
					m_filteredOutPacketsDescarted++;
			}
			else
			{
				endNotFound = 1;

				m_Traces->WriteTrace("There are not more packet with de ordered pids at input buffer. Check max time to send datagram to the client\n", LEVEL_TRZ_6);

				//Comprove if the time to send has passed, and then have to add padding and send
				if (CheckTimeToSend())
				{
					memcpy(data, m_buffer_output, m_numTSPacketsOutput * 188);
					numTSPackets = m_numTSPacketsOutput;
					m_numTSPacketsOutput = 0;

					m_filteredPacketsSent += numTSPackets;

					GetLocalTime(&m_timeLastSending);

					if (m_Traces->IsLevelWriteable(LEVEL_TRZ_6))
					{
						_snprintf(log_output, 1024 - 2, "TRANSPORT  :: Max time to send datagram. Send data in output buffer (%d TS packets)\n", numTSPackets);
						m_Traces->WriteTrace(log_output, LEVEL_TRZ_6);

						_snprintf(log_output, 1024 - 2, "TRANSPORT  :: (PID filtering; arrive max time to send) Read from buffer to send: %d bytes. %d packets. More Info Buffer: Read Position[%d]; Write Position[%d]\n",
							numTSPackets * 188, numTSPackets, m_posRead, m_posWrite);
						m_Traces->WriteTrace(log_output, LEVEL_TRZ_6);
					}
				}
				else
				{
					m_Traces->WriteTrace("Not arrive to Max time to send datagram. Not send packet yet.\n", LEVEL_TRZ_6);
					m_Traces->WriteTrace("TRANSPORT  :: **** END TREATMENT OF GETTING Packets of buffer to send ****\n", LEVEL_TRZ_6);
					return 0;
				}
			}
		}
	}


#ifdef AddPaddingToTSPacket
	if (numTSPackets != 0 && numTSPackets < NUM_PACKETS_TO_SEND)
	{
		m_nullPacketsSent += (NUM_PACKETS_TO_SEND - numTSPackets);

		char* nullPaddingPckt = new char[(NUM_PACKETS_TO_SEND - numTSPackets) * 188];
		strcpy(nullPaddingPckt, "");

		GenerateNullPaddingTSPackets(nullPaddingPckt, NUM_PACKETS_TO_SEND - numTSPackets);

		memcpy(&data[numTSPackets * 188], nullPaddingPckt, (NUM_PACKETS_TO_SEND - numTSPackets) * 188);

		if (m_Traces->IsLevelWriteable(LEVEL_TRZ_4))
		{
			_snprintf(log_output, 1024 - 2, "TRANSPORT  :: Adding %d null padding TS Packets to arrive at 7 packets to send.\n",
				NUM_PACKETS_TO_SEND - numTSPackets);
			m_Traces->WriteTrace(log_output, LEVEL_TRZ_4);
		}

		if (nullPaddingPckt)
			delete[] nullPaddingPckt;

		numTSPackets = NUM_PACKETS_TO_SEND;
	}
#endif

	if (m_Traces->IsLevelWriteable(LEVEL_TRZ_3))
		CheckTimeToAnalyzeData();

	m_Traces->WriteTrace("TRANSPORT  :: **** END TREATMENT OF GETTING Packets of buffer to send ****\n", LEVEL_TRZ_6);

	return numTSPackets * 188;
}

void CRingBufferTS_Basic::TreatingHTTPMessage(char* data, int size)
{
	char log_output[1316 * 4];
	int indexINI = 0;
	char* findIniTS;

	if (strstr(data, "HTTP/1.0 200 OK") != NULL || strstr(data, "HTTP/1.1 200 OK") != NULL)
	{
		m_Traces->WriteTrace("TRANSPORT  :: Received response HTTP message.\n", LEVEL_TRZ_3);
		
		//End of http header
		findIniTS = (char*)strstr((const char*)data, (const char*) "\r\n\r\n");
		if (findIniTS != NULL)
		{
			indexINI = findIniTS - data;
			if (m_Traces->IsLevelWriteable(LEVEL_TRZ_4))
			{
				strncpy(log_output, "TRANSPORT  :: Header of response HTTP message received:\n", 1316 * 4);
				strncat(log_output, data, indexINI);
				strcat(log_output, "\n");
				m_Traces->WriteTrace(log_output, LEVEL_TRZ_4);
			}
			m_Traces->WriteTrace("Complete received package with HTTP header:\n", LEVEL_TRZ_5);
			if (m_Traces->IsLevelWriteable(LEVEL_TRZ_5))
			{
				strncpy(log_output, data, size);
				m_Traces->WriteTrace(log_output, LEVEL_TRZ_5);
				m_Traces->WriteTrace("\n", LEVEL_TRZ_5);
			}
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

	//Initialize m_timeLastSending
	GetLocalTime(&m_timeLastSending);
}

void CRingBufferTS_Basic::GenerateNullPaddingTSPackets(char* data, int numPackets)
{
	char* datagram = new char[188];
	strcpy(datagram, "");
	int i = 0;

	//Generate a null padding TS packet 

	//SYNC byte (0x47 == 'G')
	datagram[0] = 'G'; 
	//2 bytes: PID Number. Value 0x1FFF -> Null Packet (used for fixed bandwidth padding)
	datagram[1] = '\x1f';
	datagram[2] = 'ÿ';
	//Special byte 0x1C. (It's Continuity Counter but I fix de value)
	datagram[3] = '\x1c';

	for (i = 4; i < 188; i++)
	{
		datagram[i] = 0;
	}

	//Return number of null packets required
	strcpy(data,"");
	for (i = 0; i < numPackets; i++)
	{
		memcpy(&data[i*188], datagram, 188);
	}

	delete[]datagram;
}

int CRingBufferTS_Basic::PIDFiltering(char* data)
{
	char log_output[1024];
	memset(log_output, 0, 1024);
	
	unsigned short pid = ObtainPID((const unsigned char *)data);

	if (pid > PID_MAX)
		return 0;

	if (IsPIDEnabled(pid))
	{
		if (m_Traces->IsLevelWriteable(LEVEL_TRZ_6))
		{
			_snprintf(log_output, 1024 - 2, "TRANSPORT  :: PID of packet to FILTER : %d (Add to output buffer)\n", pid);
			m_Traces->WriteTrace(log_output, LEVEL_TRZ_6);
		}
		return 1;
	}

	if (m_Traces->IsLevelWriteable(LEVEL_TRZ_6))
	{
		_snprintf(log_output, 1024 - 2, "TRANSPORT  :: PID of packet to DISCARD: %d\n", pid);
		m_Traces->WriteTrace(log_output, LEVEL_TRZ_6);
	}
	return 0;
}

void CRingBufferTS_Basic::SemaphoreWait()
{
	DWORD dwWaitResult;
	BOOL bContinue = TRUE;

	while (bContinue)
	{
		// Try to enter the semaphore gate.
		dwWaitResult = WaitForSingleObject(
			semaphoreTreatingPIDs,   // handle to semaphore
			0L);           // zero-second time-out interval

		switch (dwWaitResult)
		{
			// The semaphore object was signaled. Semaphore is free to continue
		case WAIT_OBJECT_0:
			bContinue = FALSE;
			break;
			// The semaphore was nonsignaled, so a time-out occurred. Not continue until semaphore was signaled (free)
		case WAIT_TIMEOUT:
			m_Traces->WriteTrace("DBG        :: SemaphoreWait : WAIT Semaphore\n", LEVEL_TRZ_4);
			break;
		default:
			m_Traces->WriteTrace("DBG        :: SemaphoreWait : Other result\n", LEVEL_TRZ_4);
			break;
		}

	}
}

void CRingBufferTS_Basic::SemaphoreSignal()
{
	// Release the semaphore when task is finished (when entry at this function)

	if (!ReleaseSemaphore(
		semaphoreTreatingPIDs,  // handle to semaphore
		1,            // increase count by one
		NULL))
	{
	}
}

//get:1 ; set:0
CString CRingBufferTS_Basic::GetSetPIFFilteringData(int getset, CString pidsToFilterList, int internalPFiltering)
{
	CString tmpPIDlist;
	tmpPIDlist.Format(L"");

	SemaphoreWait();

	if (getset)
		tmpPIDlist = m_pidsToFilterList;
	else
	{
		UpdatePIFFilteringData(pidsToFilterList, internalPFiltering);
	}

	SemaphoreSignal();

	return tmpPIDlist;
}

void CRingBufferTS_Basic::UpdatePIFFilteringData(CString pidsToFilterList, int internalPFiltering)
{
	setPidsToFilterList(pidsToFilterList);

	setApplyPidFiltering(internalPFiltering);
		

	for (int i = 0; i < 8192; i++)
		DisablePID(i);

	int index = 0, index2 = 0;
	index = pidsToFilterList.Find(L",", 0);
	if (index != -1)
	{
		EnablePID(atoi(CStringA(pidsToFilterList.Left(index))));
		index2 = pidsToFilterList.Find(L",", index + 1);

		while (index2 != -1)
		{
			EnablePID(atoi(CStringA(pidsToFilterList.Mid(index + 1, index2 - index - 1))));

			index = index2;
			index2 = pidsToFilterList.Find(L",", index + 1);
		}
		EnablePID(atoi(CStringA(pidsToFilterList.Right(pidsToFilterList.GetLength() - index - 1))));
	}

}

int CRingBufferTS_Basic::CheckTimeToSend()
{
	char log_output[1024];
	memset(log_output, 0, 1024);
	SYSTEMTIME curr;
	GetLocalTime(&curr);
	
	if (m_timeLastSending.wYear == 0) //Not yet initialize m_timeLastSending
		return 0;

	_int64 nanoSecs = CompareSystemTime(m_timeLastSending, curr);

	if (m_Traces->IsLevelWriteable(LEVEL_TRZ_6))
	{
		_snprintf(log_output, 1024 - 2, "TRANSPORT  :: (DBG) Check Time To Send: Nanoseconds passed: %I64d / MAX_TIME_TO_SEND: %d\n", nanoSecs, m_cfgProxy->getMaxTimeToSendDgram());
		m_Traces->WriteTrace(log_output, LEVEL_TRZ_6);
	}

	if (nanoSecs >= m_cfgProxy->getMaxTimeToSendDgram())
		return 1;
	
	return 0;
}

//Returns differences in nanoseconds.
_int64 CRingBufferTS_Basic::CompareSystemTime(SYSTEMTIME st1, SYSTEMTIME st2)
{
	__int64 res;
	timeunion ft1;
	timeunion ft2;
	SystemTimeToFileTime(&st1, &ft1.fileTime);
	SystemTimeToFileTime(&st2, &ft2.fileTime);

	ft1.ul.LowPart = ft1.fileTime.dwLowDateTime;
	ft1.ul.HighPart = ft1.fileTime.dwHighDateTime;
	
	ft2.ul.LowPart = ft2.fileTime.dwLowDateTime;
	ft2.ul.HighPart = ft2.fileTime.dwHighDateTime;

	//Values of FILETIME and QuadPart are expressed in 100 nano seconds 

/*		1 Minute = 60 Seconds
		1 Second = 1000 Miliseconds
		1 Milliseconds = 1000 Microseconds
		1 Microseconds = 1000 Nanoseconds
*/

//	__int64 res = (ft2.ul.QuadPart - ft1.ul.QuadPart) / 10000; // Convert to milliseconds
//	__int64 res = (ft2.ul.QuadPart - ft1.ul.QuadPart) * 100; // Convert to nanoseconds

	res = (ft2.ul.QuadPart - ft1.ul.QuadPart) * 100;

	return res;			
}

void CRingBufferTS_Basic::SubstractTimeToPacket()
{
	timeunion ft1;

	SystemTimeToFileTime(&m_timeLastSending, &ft1.fileTime);

	ft1.ul.LowPart = ft1.fileTime.dwLowDateTime;
	ft1.ul.HighPart = ft1.fileTime.dwHighDateTime;

	__int64 nanosecs = (ft1.ul.QuadPart * 100) - m_cfgProxy->getMaxTimeToPacket();
	ft1.ul.QuadPart = nanosecs / 100;

	memcpy(&ft1.fileTime, &ft1.ul, sizeof(ft1.fileTime));
	
	FileTimeToSystemTime(&ft1.fileTime, &m_timeLastSending);
}

inline
void CRingBufferTS_Basic::EnablePID(unsigned short pid)
{
	unsigned int index = pid / 8;
	unsigned int bit = pid % 8;
	m_pidsList[index] |= 1 << bit;
}

inline
void CRingBufferTS_Basic::DisablePID(unsigned short pid)
{
	unsigned int index = pid / 8;
	unsigned int bit = pid % 8;
	m_pidsList[index] &= ~(1 << bit);
}

inline
int CRingBufferTS_Basic::IsPIDEnabled(unsigned short pid)
{
	unsigned int index = pid / 8;
	unsigned int bit = pid % 8;
	return (m_pidsList[index] & (1 << bit)) ? 1 : 0;
}

inline
unsigned short CRingBufferTS_Basic::ObtainPID(const unsigned char * tspacket)
{
	return ((tspacket[1] & 0x1f) << 8) + tspacket[2];
}

int CRingBufferTS_Basic::CheckTimeToAnalyzeData()
{
	char trace[1024];
	memset(trace, 0, 1024);
	SYSTEMTIME curr;
	GetLocalTime(&curr);

	if (m_timeLastAnalysis.wYear == 0) //Not yet initialize m_timeLastSending
		return 0;

	_int64 nanoSecs = CompareSystemTime(m_timeLastAnalysis, curr);
	
	if (nanoSecs >= m_timetoCheckAnalysis)
	{
		if (!m_applyPidFiltering || !m_pidsToFilterList.Compare(L""))
		{
			if (m_receivedPackets < m_filteredPacketsSent)
			{
					m_receivedPackets = m_filteredPacketsSent;
			}
			_snprintf(trace, 1024 - 2, "TRANSPORT  :: ALL DATA [Tuner %d]: Packets: sent data %d, send padding %d, total received %d\n", m_tuner, m_filteredPacketsSent, m_nullPacketsSent, m_receivedPackets);
		}
		else
			_snprintf(trace, 1024 - 2, "TRANSPORT  :: PID FILTERING DATA [Tuner %d]: Packets: sent data %d, send padding %d, filtered out %d, total received %d\n", m_tuner, m_filteredPacketsSent, m_nullPacketsSent, /*m_filteredOutPacketsDescarted*/m_receivedPackets - m_filteredPacketsSent, m_receivedPackets);
		
		m_Traces->WriteTrace(trace, LEVEL_TRZ_3);

		m_filteredPacketsSent = 0;
		m_filteredOutPacketsDescarted = 0;
		m_nullPacketsSent = 0;
		m_receivedPackets = 0;

		GetLocalTime(&m_timeLastAnalysis);

		return 1;
	}

	return 0;
}
