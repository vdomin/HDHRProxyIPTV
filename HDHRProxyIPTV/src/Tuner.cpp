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
#include "Tuner.h"

CTuner::CTuner()
{
	canal = 0;
	channelNotInMapList = 0;
	state = STANDBY; //Initial state will be STANDBY
	lock = new char[15];
	strcpy(lock, "none");
	ss = 0;
	snq = 0;
	seq = 0;
	bps = 0;
	pps = 0;
	program = 0;
	filter = (char*)malloc(MAX_SIZE_FILTER);
	strcpy(filter, "0x0000-0x1FFF");
	strcpy(target, "none");
	strcpy(channelmap, TUNERX_CHANNELMAP_VALUE);
	lockkey = 0;
	strcpy(IPLockkey, "");
	pidsToFiltering.Format(L"");
	transportTuner = new CTransport();
	
	m_cfgProxy = CConfigProxy::GetInstance();

	initiazedByCli = 0;
	timer1_secs = m_cfgProxy->getTimerResponseClient();

	m_traces = new CTrace();
}

CTuner::~CTuner()
{
	delete transportTuner;

	delete m_traces;
}

void CTuner::ChangeStateToStandby()
{
	canal = 0;

	state = STANDBY;

	strcpy(lock, NONE);
	ss = 0;
	snq = 0;
	seq = 0;
	bps = 0;
	pps = 0;
	program = 0;
	pidsToFiltering.Format(L"");
	strcpy(filter, "0x0000-0x1FFF");
	strcpy(target, "none");

	//Receiving treatment is stopped
	if (transportTuner->getPerformSend())
	{
		transportTuner->setPerformSend(0, tuner);
	}
}

void CTuner::ChangeStateToTunedChan(long chan)
{
	canal = chan;

	state = TUNED_CHAN;
	int ch = m_cfgProxy->ObtainIndexChannel(canal);

	strcpy(lock, lock_DEF);
	ss = m_cfgProxy->m_infoChannels[ch].signalStrength;
	snq = m_cfgProxy->m_infoChannels[ch].signalQuality;
	seq = m_cfgProxy->m_infoChannels[ch].symbolQuality;
	bps = m_cfgProxy->m_infoChannels[ch].networkRate;
	pps = m_cfgProxy->m_infoChannels[ch].networkRate;

	program = 0;
	pidsToFiltering.Format(L"");
	strcpy(filter, "0x0000-0x1FFF");
	strcpy(target, "none");
	transportTuner->setPerformSend(0, tuner);

	transportTuner->setPerformSend(0, tuner);
	transportTuner->ChangeFilterPIDsList(pidsToFiltering,
		m_cfgProxy->getInternalPIDFilteringOfChannel(canal),
		m_cfgProxy->getExternalPIDFilteringOfChannel(canal),
		canal);
}

void CTuner::ChangeStateToFilteringByProgram(int prog)
{
	if (prog != 0)
	{
		program = prog;

		pidsToFiltering = m_cfgProxy->findPidsOfProgram(canal, program);

		strcpy(filter, "0x0000-0x1FFF");
		strcpy(target, "none");

		if (transportTuner->getPerformSend())
			transportTuner->setPerformSend(0, tuner);
		transportTuner->ChangeFilterPIDsList(pidsToFiltering,
			m_cfgProxy->getInternalPIDFilteringOfChannel(canal),
			m_cfgProxy->getExternalPIDFilteringOfChannel(canal),
			canal);

		state = FILTERING;
	}
	else
	{
		program = 0;
		pidsToFiltering.Format(L"");
		strcpy(filter, "0x0000-0x1FFF");
		strcpy(target, "none");
		transportTuner->setPerformSend(0, tuner);
		if (canal == 0)
		{
			ChangeStateToStandby();
		}
		else
		{
			state = TUNED_CHAN;
		}
	}
}

int CTuner::ChangeStateToStreaming(char* targ)
{
	if (strcmp(targ, "none") && strcmp(targ, "NONE"))
	{
		strcpy(target, targ);
		state = STREAMING;

		//Transport: Start reception and sending of TS /////////////////////////////////////////////
		transportTuner->AssignDataSend(target);
		int res = 0;

		transportTuner->ChangeFilterPIDsList(pidsToFiltering,
			m_cfgProxy->getInternalPIDFilteringOfChannel(canal),
			m_cfgProxy->getExternalPIDFilteringOfChannel(canal),
			canal);

		if (transportTuner->getState() == 0) //No initiated
		{
			res = transportTuner->InitilizeTransportStream(canal, pidsToFiltering);
		}
		else
		{
			res = transportTuner->ChangeSourceTS(canal, pidsToFiltering);
			if (res)
				transportTuner->setState(1);
		}
		if (res>0)
		{
			int ch = m_cfgProxy->ObtainIndexChannel(canal);
			transportTuner->setPerformSend(1, tuner);
			ss = m_cfgProxy->m_infoChannels[ch].signalStrength;
			snq = m_cfgProxy->m_infoChannels[ch].signalQuality;
			seq = m_cfgProxy->m_infoChannels[ch].symbolQuality;
			bps = m_cfgProxy->m_infoChannels[ch].networkRate;
			pps = m_cfgProxy->m_infoChannels[ch].networkRate;
			//pps = 0;
		}
		else if ((res == -1) || (res == -2))
		{
			ChangeStateToStandby();
		}
		else if (res == 0)
		{
			int ch = m_cfgProxy->ObtainIndexChannel(canal);
			transportTuner->setPerformSend(1, tuner);
			ss = m_cfgProxy->m_infoChannels[ch].signalStrength;
			snq = m_cfgProxy->m_infoChannels[ch].signalQuality;
			seq = m_cfgProxy->m_infoChannels[ch].symbolQuality;
			bps = 0;
			pps = 0;
		}
	}
	else
	{
		strcpy(target, "none");
		transportTuner->setPerformSend(0, tuner);
	}

	return state;
}

void CTuner::ChangeStateToFilteringByFilter(char* filt, char* ip)
{
	int lon = strlen(filt);

	if (lon == 0)
	{
		strcpy(filter, "0x0000-0x1FFF"); //Default value
		pidsToFiltering.Format(L"");
	}
	else
	{
		strncpy(filter, filt, MAX_SIZE_FILTER);

		if (strcmp(filter, "0x0000-0x1FFF"))
			pidsToFiltering = ConvertHexPidsListToNum(CString(filter));
		else
			pidsToFiltering.Format(L"");

		transportTuner->ChangeFilterPIDsList(pidsToFiltering,
			m_cfgProxy->getInternalPIDFilteringOfChannel(canal),
			m_cfgProxy->getExternalPIDFilteringOfChannel(canal),
			canal);
	}

	char* log_output = new char[3000];
	memset(log_output, 0, 3000);

	if (state == TUNED_CHAN)
	{
		state = FILTERING;
		if (m_traces->IsLevelWriteable(LEVEL_TRZ_2))
			_snprintf(log_output, 3000 - 2, "CONTROL    :: Change State of Tuner%d: FLTR [%s] : PID filtering: %s\n", tuner, ip, CStringA(pidsToFiltering));
	}
	else if (state == FILTERING
		|| state == STREAMING)
	{
		if (m_traces->IsLevelWriteable(LEVEL_TRZ_2))
			_snprintf(log_output, 3000 - 2, "CONTROL    :: Change PID filtering Tuner%d  [%s] : PID filtering: %s\n", tuner, ip, CStringA(pidsToFiltering));
	}

	if (m_traces->IsLevelWriteable(LEVEL_TRZ_2))
		m_traces->WriteTrace(log_output, LEVEL_TRZ_2);

	delete[]log_output;
}

void CTuner::ChangePIDsByFilterInStreaming(char* filt, char* ip)
{
	int lon = strlen(filt);

	if (lon == 0)
	{
		strcpy(filter, "0x0000-0x1FFF"); //Default value
		pidsToFiltering.Format(L"");
	}
	else
	{
		strncpy(filter, filt, MAX_SIZE_FILTER);

		if (strcmp(filter, "0x0000-0x1FFF"))
			pidsToFiltering = ConvertHexPidsListToNum(CString(filter));
		else
			pidsToFiltering.Format(L"");
	}

	transportTuner->ChangeFilterPIDsList(pidsToFiltering,
		m_cfgProxy->getInternalPIDFilteringOfChannel(canal),
		m_cfgProxy->getExternalPIDFilteringOfChannel(canal),
		canal);

	if (m_traces->IsLevelWriteable(LEVEL_TRZ_2))
	{
		char* log_output = new char[1024];
		memset(log_output, 0, 1024);
		_snprintf(log_output, 1024 - 2, "CONTROL    :: Change PIDs list Tuner%d      [%s] : State is STRM. Pids change by Filter: %s\n", tuner, ip, CStringA(pidsToFiltering));

		m_traces->WriteTrace(log_output, LEVEL_TRZ_2);

		delete[]log_output;
	}

	if (m_cfgProxy->getExternalPIDFilteringOfChannel(canal))
		transportTuner->SendGetHTTPRequest();
}

void CTuner::ChangePIDsByProgramInStreaming(int prog, char* ip)
{
	if (prog != 0)
	{
		program = prog;
		pidsToFiltering = m_cfgProxy->findPidsOfProgram(canal, program);
		strcpy(filter, "0x0000-0x1FFF");
	}
	else
	{
		program = 0;
		pidsToFiltering.Format(L"");
		strcpy(filter, "0x0000-0x1FFF");
	}

	transportTuner->ChangeFilterPIDsList(pidsToFiltering,
		m_cfgProxy->getInternalPIDFilteringOfChannel(canal),
		m_cfgProxy->getExternalPIDFilteringOfChannel(canal),
		canal);

	if (m_traces->IsLevelWriteable(LEVEL_TRZ_2))
	{
		char log_output[1024];
		memset(log_output, 0, 1024);
		_snprintf(log_output, 1024 - 2, "CONTROL    :: Change PIDs list Tuner%d      [%s] : State is STRM. Pids change by Program: %s\n", tuner, ip, CStringA(pidsToFiltering));
		m_traces->WriteTrace(log_output, LEVEL_TRZ_2);
	}
	
	if (m_cfgProxy->getExternalPIDFilteringOfChannel(canal))
		transportTuner->SendGetHTTPRequest();
}

CString CTuner::ConvertHexPidsListToNum(CString filterpids)
{
	CString numericPids;
	numericPids.Format(L"");

	if (!m_traces->IsPrintable((char*)CStringA(filterpids).GetString()))
	{
		m_traces->WriteTrace("CONTROL    :: warning in CTuner::ConvertHexPidsListToNum() because list is binary!\n", LEVEL_TRZ_4);
		return numericPids;
	}

	int index = 0, index1 = 0, index2 = 0, index3 = 0, indexFin = 0, index1Fin = 0;
	int pid1 = 0, pid2 = 0;
	CString pid;

	index = filterpids.Find(L" ", 0);
	index1 = filterpids.Find(L"-", 0);

	if ((index != -1 || index1 != -1))
	{
		if ((index < index1 && index != -1 && index1 != -1)
			|| (index != -1 && index1 == -1))
		{
			pid.Format(L"%d", strtol(CStringA(filterpids.Left(index)), NULL, 16));

			if (!pid.Compare(L"8192"))
			{
				strcpy(filter, "0x0000-0x1FFF");
				numericPids.Format(L"");
				m_traces->WriteTrace("CONTROL    :: Has arrived a the PID 0x2000 (8192 in decimal) to filter. Configure list of PIDs to \"0x0000-0x1FFF\", desactive PID filtering.\n", LEVEL_TRZ_2);
				return numericPids;
			}

			numericPids.Append(pid);

			index3 = index;
			indexFin = index;
			index = filterpids.Find(L" ", index + 1);
		}
		else if ((index1 < index && index != -1 && index1 != -1)
			|| (index == -1 && index1 != -1))
		{
			index2 = filterpids.Find(L" ", index1 + 1);
			pid1 = strtol(CStringA(filterpids.Left(index)), NULL, 16);
			pid2 = strtol(CStringA(filterpids.Mid(index1 + 1, index2 - index1 - 1)), NULL, 16);
			for (int i = pid1; i <= pid2; i++)
			{
				if (!numericPids.Compare(L""))
					pid.Format(L"%d", i);
				else
					pid.Format(L",%d", i);

				if (!pid.Compare(L"8192") || !pid.Compare(L",8192"))
				{
					strcpy(filter, "0x0000-0x1FFF");
					numericPids.Format(L"");
					m_traces->WriteTrace("CONTROL    :: Has arrived a the PID 0x2000 (8192 in decimal) to filter. Configure list of PIDs to \"0x0000-0x1FFF\", desactive PID filtering.\n", LEVEL_TRZ_2);
					return numericPids;
				}

				numericPids.Append(pid);
			}
			index3 = index2;
			index1Fin = index1;
			index1 = filterpids.Find(L"-", index2 + 1);
		}

		while (index != -1 || index1 != -1)
		{
			if (index3 == index)
				index = filterpids.Find(L" ", index + 1);

			if ((index < index1 && index != -1 && index1 != -1)
				|| (index != -1 && index1 == -1))
			{
				pid.Format(L",%d", strtol(CStringA(filterpids.Mid(index3 + 1, index - index3 - 1)), NULL, 16));

				if (!pid.Compare(L",8192"))
				{
					strcpy(filter, "0x0000-0x1FFF");
					numericPids.Format(L"");
					m_traces->WriteTrace("CONTROL    :: Has arrived a the PID 0x2000 (8192 in decimal) to filter. Configure list of PIDs to \"0x0000-0x1FFF\", desactive PID filtering.\n", LEVEL_TRZ_2);
					return numericPids;
				}

				numericPids.Append(pid);

				index3 = index;
				indexFin = index;
				index = filterpids.Find(L" ", index + 1);
			}
			else if ((index1 < index && index != -1 && index1 != -1)
				|| (index == -1 && index1 != -1))
			{
				index2 = filterpids.Find(L" ", index1 + 1);
				if (index2 == -1)
					index2 = filterpids.GetLength() - 1;

				pid1 = strtol(CStringA(filterpids.Mid(index3 + 1, index1 - index3 - 1)), NULL, 16);
				pid2 = strtol(CStringA(filterpids.Mid(index1 + 1, index2 - index1)), NULL, 16);
				for (int i = pid1; i <= pid2; i++)
				{
					pid.Format(L",%d", i);

					if (!pid.Compare(L",8192"))
					{
						strcpy(filter, "0x0000-0x1FFF");
						numericPids.Format(L"");
						m_traces->WriteTrace("CONTROL    :: Has arrived a the PID 0x2000 (8192 in decimal) to filter. Configure list of PIDs to \"0x0000-0x1FFF\", desactive PID filtering.\n", LEVEL_TRZ_2);
						return numericPids;
					}
					numericPids.Append(pid);
				}

				index3 = index2;
				index1Fin = index1;
				index1 = filterpids.Find(L"-", index2 + 1);
			}
		}

		if (indexFin > index1Fin)
		{
			pid.Format(L",%d", strtol(CStringA(filterpids.Right(filterpids.GetLength() - indexFin - 1)), NULL, 16));

			if (!pid.Compare(L",8192"))
			{
				strcpy(filter, "0x0000-0x1FFF");
				numericPids.Format(L"");
				m_traces->WriteTrace("CONTROL    :: Has arrived a the PID 0x2000 (8192 in decimal) to filter. Configure list of PIDs to \"0x0000-0x1FFF\", desactive PID filtering.\n", LEVEL_TRZ_2);
				return numericPids;
			}

			numericPids.Append(pid);
		}
	}

	return numericPids;
}


void CTuner::SetTimers()
{
	GetLocalTime(&timer1);

	initiazedByCli = 1;
}

//If the timer expires (timeout) returns 1; else returns 0
int CTuner::CheckTimer()
{
	if (!initiazedByCli)
		return 0;

	SYSTEMTIME curr;
	GetLocalTime(&curr);

	_int64 sec = CompareSystemTime(timer1, curr);
	int seconds = (int)sec;

	if (seconds >= timer1_secs)
	{
		initiazedByCli = 0;

		if (state != STANDBY)
		{
			ChangeStateToStandby();
			return 1;
		}
		else
			return 0;
	}

	return 0;
}

//Returns differences in seconds.
_int64 CTuner::CompareSystemTime(SYSTEMTIME st1, SYSTEMTIME st2)
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

	res = (ft2.ul.QuadPart - ft1.ul.QuadPart) * 100 /1000000000; //nanoseconds

	return res;
}