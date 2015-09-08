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
#include "Transport.h"

DWORD WINAPI Listening__ThreadTransport(void* p)
{
	((CTransport*)p)->ListeningThreadTransport();
	return 0;
}

DWORD CTransport::ListeningThreadTransport()
{
	while (1)
	{
		if (getErrConnectionIni())
		{
			ReconnectHTTP();
		}
		else
		{
			if (getPerformSend() && !m_refreshFailedConnHTTP)
				TreatReceivedDataTS();
		}
	}
	return 1;
}

int CTransport::StartThreadTransport()
{
	char log_output[1024];
	memset(log_output, 0, 1024);

	//Creation of Thread for listen and send of Transport Stream
	hThreadTransport = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Listening__ThreadTransport, this, 0, 0);
	SetThreadPriority(hThreadTransport, THREAD_PRIORITY_HIGHEST);

	_snprintf(log_output, 1024 - 2, "TRANSPORT  :: Create Transport thread %d\n", hThreadTransport);
	m_Traces->WriteTrace(log_output, LEVEL_TRZ_4);

	return 1;
}

void CTransport::StopThreadTransport()
{
	if (hThreadTransport != 0)
	{
		TerminateThread(hThreadTransport, 1002);
		hThreadTransport = 0;
	}
}

CTransport::CTransport()
{
	m_errConnectionIni = 0;
	receivingDataHTTP = 0;
	m_Traces = new CTrace();
	m_state = 0;  //0 : Uninitiated ; 1 : Initiated
	m_PerformSend = 0;

	m_applyExtPidFiltering = 0;

	m_typeTransportOutput = UDP_TS; //By default will be UDP protocol of output.
	m_typeTransportInput = 0;  //By default UDP, for initialize variable

	cfgProxy = CConfigProxy::GetInstance();

	strcpy(m_ipSend, "");
	m_portSend = 0;

	m_dataGETHTTP.Format(L"");

	hThreadTransport = 0;

	m_failedConnectHTTP = 0;
	m_refreshFailedConnHTTP = 0;

#ifdef UseRingBuffer
	m_ringBuffer = new CRingBufferTS(26320); //20
#endif

#ifdef UseBasicRingBuffer
	m_basicRingBuffer = new CRingBufferTS_Basic();
#endif

}

CTransport::~CTransport()
{
	if (getState() == 1)
	{
		if (m_typeTransportInput == UDP_TS)
			StopTransportStreamUDP();
		else if (m_typeTransportInput == HTTP_TS)
			StopTransportStreamHTTP();
		setState(0);
	}

	TerminateThread(hThreadTransport, 1002);

	delete m_Traces;
	m_Traces = NULL;

#ifdef UseRingBuffer
	if (m_ringBuffer)
		delete m_ringBuffer;
#endif

#ifdef UseBasicRingBuffer
	if (m_basicRingBuffer)
		delete m_basicRingBuffer;
#endif
}

int CTransport::InitilizeTransportStream(int channel, CString pidsToFilterList)
{
	char log_output[5500];
	memset(log_output, 0, 5500);

	int res = 1;

	m_PosChannelInMapList = cfgProxy->ObtainIndexChannel(channel);

	//If the channel is not in the List Mapping it can not start Transport Stream as there are no data

	if (m_PosChannelInMapList == -1)
	{
		m_Traces->WriteTrace("TRANSPORT  :: Could be not initiate TRANSPORT phase. Channel not in MAPPING LIST.\n", ERR);
		return -1;
	}

	CString prot = cfgProxy->m_infoChannels[m_PosChannelInMapList].Protocol;
	
	if (!prot.Compare(L"UDP"))
	{
		m_typeTransportInput = UDP_TS;
		if (!cfgProxy->m_infoChannels[m_PosChannelInMapList].UDPsource.Compare(L""))
		{
			if (m_Traces->IsLevelWriteable(LEVEL_TRZ_1))
			{
				_snprintf(log_output, 5500 - 2, "TRANSPORT  :: Channel not tuned. Not defined UDPsource information in MappingList for channel %ld.\n", channel);
				m_Traces->WriteTrace(log_output, LEVEL_TRZ_1);
			}
			return -2;
		}
	}
	else if (!prot.Compare(L"HTTP"))
	{
		m_typeTransportInput = HTTP_TS;
		if (!cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet.Compare(L""))
		{
			if (m_Traces->IsLevelWriteable(LEVEL_TRZ_1))
			{
				_snprintf(log_output, 5500 - 2, "TRANSPORT  :: Channel not tuned. Not defined URLGet information in MappingList for channel %ld.\n", channel);
				m_Traces->WriteTrace(log_output, LEVEL_TRZ_1);
			}
			return -2;
		}
	}
	else
	{
		if (m_Traces->IsLevelWriteable(LEVEL_TRZ_1))
		{
			_snprintf(log_output, 5500 - 2, "TRANSPORT  :: Channel not tuned. Not defined Protocol information (HTTP/UDP) in MappingList for channel %ld.\n", channel);
			m_Traces->WriteTrace(log_output, LEVEL_TRZ_1);
		}
		return -2;
	}
	
	if (((!prot.Compare(L"HTTP")) &&
		((!cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet.Compare(L"none"))
		|| (!cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet.Compare(L"NONE"))
		|| (!cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet.Compare(L""))))
		||
		((!prot.Compare(L"UDP")) &&
		((!cfgProxy->m_infoChannels[m_PosChannelInMapList].UDPsource.Compare(L"none"))
		|| (!cfgProxy->m_infoChannels[m_PosChannelInMapList].UDPsource.Compare(L"NONE"))
		|| (!cfgProxy->m_infoChannels[m_PosChannelInMapList].UDPsource.Compare(L""))))
		)
	{
		if (m_Traces->IsLevelWriteable(LEVEL_TRZ_1))
		{
			_snprintf(log_output, 5500 - 2, "TRANSPORT  :: Channel not tuned. Not defined all Protocol information in MappingList for channel %ld.\n", channel);
			m_Traces->WriteTrace(log_output, LEVEL_TRZ_1);
		}
		return 0;
	}

	if (m_typeTransportInput == UDP_TS)
	{
		if (InitializeTransportStreamUDP())
		{
			m_PerformSend = 1;

			m_Traces->WriteTrace("TRANSPORT  :: HDHR server ready to receive Transport Stream Packages over UDP\n", LEVEL_TRZ_2);

			if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
			{
				_snprintf(log_output, 5500 - 2, "TRANSPORT  :: Listening by [UDP] %s:%d. Sending to [UDP] %s:%d\n", CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].ipUDP), cfgProxy->m_infoChannels[m_PosChannelInMapList].puertoUDP, m_ipSend, m_portSend);
				m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
			}
		}
		else
		{
			m_Traces->WriteTrace("TRANSPORT  :: Could be not initiate TRANSPORT phase (Protocol UDP)\n", ERR);
			if (m_Traces->IsLevelWriteable(ERR))
			{
				_snprintf(log_output, 5500 - 2, "TRANSPORT  :: ERR: [UDP] In: %s:%d ; Out: %s:%d\n", CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].ipUDP), cfgProxy->m_infoChannels[m_PosChannelInMapList].puertoUDP, m_ipSend, m_portSend);
				m_Traces->WriteTrace(log_output, ERR);
			}
			return 0;
		}
	}
	else if (m_typeTransportInput == HTTP_TS)
	{
		if (InitilizeTransportStreamHTTP())
		{
#ifdef UseRingBuffer
			m_ringBuffer->Initialize();
#endif

#ifdef UseBasicRingBuffer
			m_basicRingBuffer->Initialize(pidsToFilterList);
			m_basicRingBuffer->setTuner(m_tuner);
#endif
			m_PerformSend = 1;

			m_Traces->WriteTrace("TRANSPORT  :: HDHR server ready to receive Transport Stream Packages over HTTP\n", LEVEL_TRZ_2);

			if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
			{
				_snprintf(log_output, 5500 - 2, "TRANSPORT  :: Listening by [HTTP] http://%s:%d/%s. Sending to [UDP] %s:%d\n", CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].ipHTTP), cfgProxy->m_infoChannels[m_PosChannelInMapList].puertoHTTP, CStringA(m_dataGETHTTP), m_ipSend, m_portSend);
				m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
			}
		}
		else
		{
			m_Traces->WriteTrace("TRANSPORT  :: Could be not initiate TRANSPORT phase (Protocol HTTP)\n", LEVEL_TRZ_1);

			_snprintf(log_output, 5500 - 2, "TRANSPORT  :: ERR: [HTTP] In: http://%s:%d/%s ; Out: %s:%d\n", CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].ipHTTP), cfgProxy->m_infoChannels[m_PosChannelInMapList].puertoHTTP, CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].datosGETHTTP), m_ipSend, m_portSend);
			m_Traces->WriteTrace(log_output, ERR);
			m_errConnectionIni = 1;
			res = 0;
		}
	}

	if (hThreadTransport == 0)
	{
		StartThreadTransport();
	}

	setState(1);

	return res;
}

int CTransport::InitializeTransportStreamUDP()
{
	int res;
	WSADATA WsaDat;

	//Initilize the use of WinSock
	res = WSAStartup(MAKEWORD(2, 2), &WsaDat);
	if (res)	//If err is 0, ko
	{
		m_Traces->WriteTrace("TRANSPORT  :: Error initializing use of WinSock.\n", ERR);
		return 0;
	}

	//Initilize Socket UDP
	m_socketUDP = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (m_socketUDP == INVALID_SOCKET)
	{
		m_Traces->WriteTrace("TRANSPORT  :: Error in creation of UDP Socket\n", ERR);
		return 0;
	}

	//If the protocol of input is HTTP, only it needs to create the UDP socket for to forward
	//by him the TS received for HTTP, no need to perform the following steps of this function
	if (m_typeTransportInput == HTTP_TS)
		return 1;

	//Especific of server Socket

	//Information of the socket
	SOCKADDR_IN servInf;
	servInf.sin_family = AF_INET;
	if (!strcmp(CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].ipUDP), "ANY") ||
		!strcmp(CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].ipUDP), "127.0.0.1"))
		servInf.sin_addr.s_addr = htonl(INADDR_ANY);
	else
	{
		servInf.sin_addr.s_addr = inet_addr(CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].ipUDP));
	}

	servInf.sin_port = htons(cfgProxy->m_infoChannels[m_PosChannelInMapList].puertoUDP);  //For the server socket it is the port of the server (from where listen)

	//Associate a local direction to the socket
	res = bind(m_socketUDP, (SOCKADDR*)(&servInf), sizeof(servInf));
	if (res)
	{
		res = WSAGetLastError();
		closesocket(m_socketUDP);
		char * log_output = new char[1024];
		memset(log_output, 0, 1024);

		_snprintf(log_output, 1024 - 2, "TRANSPORT  :: Has not been able to associate (bind) socket direction. Error %d\n", res);
		m_Traces->WriteTrace(log_output, ERR);
		delete[] log_output;
		return 0;
	}

	return 1;
}

void CTransport::TreatReceivedDataTS()
{
	int attemptsReconn = 0;
	int reconnect = 0;
	int err = 0;
	char log_output[1024];
	memset(log_output, 0, 1024);

	if (m_typeTransportInput == UDP_TS)
		TreatReceivedDataUDP();
	else if (m_typeTransportInput == HTTP_TS)
	{
#ifdef UseRingBuffer 
		TreatReceivedDataHTTP();
#else
#ifdef UseBasicRingBuffer
		if (TreatReceivedDataHTTP_BasicBuffer() == -1) //Close
		{
			if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
			{
				_snprintf(log_output, 1024 - 2, "TRANSPORT  :: [Tuner %d] Try to reconnect connection HTTP.\n", m_tuner);
				m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
			}

			while (attemptsReconn < 50 && !reconnect)
			{
				SendNullPackets();
				reconnect = ReconnectConnectionHTTP();
				attemptsReconn++;
			}

			if (!reconnect)
			{
				if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
				{
					_snprintf(log_output, 1024 - 2, "TRANSPORT  :: [Tuner %d] Not possible to reconnect connection HTTP.\n", m_tuner);
					m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
				}
				m_failedConnectHTTP = 1;
				m_refreshFailedConnHTTP = 1;
				setPerformSend(0, m_tuner);
			}
			else
			{
				if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
				{
					_snprintf(log_output, 1024 - 2, "TRANSPORT  :: [Tuner %d] Connection HTTP is reconnected.\n", m_tuner);
					m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
				}
			}
		}
#else
		TreatReceivedDataHTTP_SinRBuffer();
#endif
#endif

	}
}

int CTransport::TreatReceivedDataUDP()
{
	if (!m_PerformSend)
		return 1;

	int res;
	char log_output[1024];
	memset(log_output, 0, 1024);
	char *RecvBuf = new char[1316];
	int BufLen = 1316;
	sockaddr_in SenderAddr;
	int SenderAddrSize = sizeof(SenderAddr);

	//Returns the number of bytes received, if not is error
	res = recvfrom(m_socketUDP, RecvBuf, BufLen, 0, (SOCKADDR *)& SenderAddr, &SenderAddrSize);
	if (res == SOCKET_ERROR) {
			if (WSAGetLastError() != 10054)
			{
				_snprintf(log_output, 1024 - 2, "TRANSPORT  :: Error %d at UDP data reception\n", WSAGetLastError());
				m_Traces->WriteTrace(log_output, ERR);
				delete[] RecvBuf;
				return 0;
			}
	}
	else
	{
		//Send Datagram:
		sockaddr_in RecvAddr;
		RecvAddr.sin_family = AF_INET;
		RecvAddr.sin_port = htons(m_portSend);
		RecvAddr.sin_addr.s_addr = inet_addr(m_ipSend);

		res = sendto(m_socketUDP,
			RecvBuf,
			BufLen,
			0, (SOCKADDR *)& RecvAddr, sizeof(RecvAddr));

		if (res == SOCKET_ERROR) {
			_snprintf(log_output, 1024 - 2, "TRANSPORT  :: Error %d sending UDP data.\n", WSAGetLastError());
			m_Traces->WriteTrace(log_output, ERR);
			delete[] RecvBuf;
			return 0;
		}
	}

	return 1;
}

void CTransport::StopTransportStreamUDP()
{
	m_PerformSend = 0;

	if (getState() == 1)
	{
		shutdown(m_socketUDP, SD_BOTH);
		closesocket(m_socketUDP);
	}
}

void CTransport::AssignDataSend(char* target)
{
	int infoProt = 0;
	CString aux(target);
	CString ip;
	CString port;

	int index = aux.Find(L":", 0);
	ip = aux.Left(index);
	ip.MakeUpper();

	if (!ip.Compare(L"UDP"))
	{
		m_typeTransportOutput = UDP_TS;
		infoProt = 1;
	}
	else if (!ip.Compare(L"RTP"))
	{
		m_typeTransportOutput = RTP_TS;
		infoProt = 1;
	}
	
	if (infoProt)
	{
		//format: udp_o_rtp://ip:port
		index = index + 3;
		int index2 = aux.Find(L":", index + 1);
		ip = aux.Mid(index, index2 - index);
		port = aux.Mid(index2 + 1, aux.GetLength()- index2);
	}
	else
	{
		//format: ip:port (TSReader)
		port = aux.Right(aux.GetLength() - index-1);
		m_typeTransportOutput = UDP_TS;  //If  the transport protocol is not informed, it will be by default UDP
	}

	CStringA ip2(ip);
	CStringA port2(port);

	strcpy(m_ipSend, ip2);
	m_portSend = atoi(port2);
	
	if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
	{
		char log_output[1000];
		if (m_typeTransportOutput == RTP_TS)
			_snprintf(log_output, 1024 - 2, "TRANSPORT  :: Sending to [RTP] %s:%d\n", m_ipSend, m_portSend);
		else
			_snprintf(log_output, 1024 - 2, "TRANSPORT  :: Sending to [UDP] %s:%d\n", m_ipSend, m_portSend);

		m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
	}
}

int CTransport::InitilizeTransportStreamHTTP()
{
	WSADATA wsa;

	m_errConnectionIni = 0;
	m_failedConnectHTTP = 0;
	m_refreshFailedConnHTTP = 0;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		m_Traces->WriteTrace("TRANSPORT  :: Error initializing use of WinSock [HTTP].\n", ERR);
		return 0;
	}

	//Initilize UDP Socket to resend by its the received TS from HTTP
	if (!InitializeTransportStreamUDP())
		return 0;

	//Initilize Socket for HTTP. It will be a TCP socket
	
	if ((m_socketHTTP = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		m_Traces->WriteTrace("TRANSPORT  :: Error in creation of HTTP(TCP) Socket\n", ERR);
		return 0;
	}

	struct sockaddr_in addr;
	addr.sin_addr.s_addr = inet_addr(CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].ipHTTP));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(cfgProxy->m_infoChannels[m_PosChannelInMapList].puertoHTTP);

	//Connect to remote server
	if (connect(m_socketHTTP, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		int err = WSAGetLastError();
		char log_output[1024];
		memset(log_output, 0, 1024);
		_snprintf(log_output, 1024 - 2, "TRANSPORT  :: [Tuner%d] Can not connect to http://%s:%d. Error code %d\n", m_tuner, CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].ipHTTP), cfgProxy->m_infoChannels[m_PosChannelInMapList].puertoHTTP, err);
		m_Traces->WriteTrace(log_output, ERR);
		shutdown(m_socketHTTP, SD_BOTH);
		closesocket(m_socketHTTP);
		m_socketHTTP = 0;
		return 0;
	}

	return 1;
}

int CTransport::TreatReceivedDataHTTP_SinRBuffer()
{
	int recv_size = 1316;
	char server_reply[1316];
	char *msg = new char[5200];
	memset(msg, 0, 5200);

	//To send Datagrama:
	sockaddr_in RecvAddr;
	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(m_portSend);
	RecvAddr.sin_addr.s_addr = inet_addr(m_ipSend);

	_snprintf(msg, 5200 - 2, "GET /%s HTTP/1.1\r\n\r\n", CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].datosGETHTTP));

	if (send(m_socketHTTP, msg, strlen(msg), 0) < 0)
	{
		return 0;
	}
	delete[]msg;

	while (getPerformSend()){
		//Receive a reply from the server
		recv_size = 1316;
		strcpy(server_reply, "");
		if ((recv_size = recv(m_socketHTTP, server_reply, recv_size, 0)) == SOCKET_ERROR)
		{
			return 0;
		}

		receivingDataHTTP = 1;

		//Send Datagram:
		int res = sendto(m_socketUDP,
			server_reply,
			recv_size,
			0, (SOCKADDR *)& RecvAddr, sizeof(RecvAddr));

		if (res == SOCKET_ERROR) {
			return 0;
		}
	}

	return 1;
}

int CTransport::SendGetHTTPRequest()
{
	char log_output[1024];
	memset(log_output, 0, 1024);
	char *msg = new char[5200];
	memset(msg, 0, 5200);
	_snprintf(msg, 5200 - 2, "GET /%s HTTP/1.1\r\n\r\n", CStringA(m_dataGETHTTP));

	if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
	{
		_snprintf(log_output, 1024 - 2, "TRANSPORT  :: Resending Request HTTP message(GET): \"GET /%s HTTP/1.1\"\n", CStringA(m_dataGETHTTP));
		m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
	}

	if (send(m_socketHTTP, msg, strlen(msg), 0) < 0)
	{
		return 0;
	}
	delete[]msg;

	return 1;
}

int CTransport::TreatReceivedDataHTTP_BasicBuffer()
{
	int recv_size = 1316;

	if (!m_basicRingBuffer->getapplyPidFiltering() || !m_basicRingBuffer->getPidsTOFilter().Compare(L""))
		recv_size = 1316;
	else
		recv_size = 1316*4;
	int size = recv_size;

	char *msg = new char[5200];
	memset(msg, 0, 5200);
	char* log_output = new char[5500];
	memset(log_output, 0, 5500);
	char server_reply[1316*4];
	char dataR[1316*4];
	int numMaxRecv = 10;
	int numRecvs = 0;
	int nbytes = 0;
	int inicio = 0;
	int sincronizado = 0;
	int tamSend = 0;
	int resync = 0;
	int validatedPck = 0;
	int isSync = 0;
	int notstream = 0;
	int res = 0;
	char splitPacket[1316];
	strcpy(splitPacket, "");
	int countTosplit = 0;
	int sendRefresh8192 = 0;
	int err = 0;

	sockaddr_in RecvAddr;
	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(m_portSend);
	RecvAddr.sin_addr.s_addr = inet_addr(m_ipSend);

	_snprintf(msg, 5200 - 2, "GET /%s HTTP/1.1\r\n\r\n", CStringA(m_dataGETHTTP));
	
	if (m_Traces->IsLevelWriteable(LEVEL_TRZ_3))
	{
		_snprintf(log_output, 5500 - 2, "TRANSPORT  :: Sending Request HTTP message(GET): \"GET /%s HTTP/1.1\"\n", CStringA(m_dataGETHTTP));
		m_Traces->WriteTrace(log_output, LEVEL_TRZ_3);
	}

	if (send(m_socketHTTP, msg, strlen(msg), 0) < 0)
	{
		return 0;
	}
	delete[]msg;

	//Send an initial UDP datagram (7 TS packets with null padding). It's like say Ok to client.
	char* nullPaddingPckt = new char[MAX_SIZE_DATAGRAM_TO_SEND];
	strcpy(nullPaddingPckt, "");
	m_basicRingBuffer->GenerateNullPaddingTSPackets(nullPaddingPckt, NUM_PACKETS_TO_SEND);

	res = sendto(m_socketUDP,
		nullPaddingPckt,
		MAX_SIZE_DATAGRAM_TO_SEND,
		0, (SOCKADDR *)& RecvAddr, sizeof(RecvAddr));

	if (res == SOCKET_ERROR) {
		char txtlog[1024];
		memset(txtlog, 0, 1024);
		_snprintf(txtlog, 1024 - 2, "TRANSPORT  :: Error sending initial packet to UDP with padding %s:%d\n", m_ipSend, m_portSend);
		m_Traces->WriteTrace(txtlog, ERR);

		return 0;
	}

	if (m_Traces->IsLevelWriteable(LEVEL_TRZ_4))
	{
		_snprintf(log_output, 5500 - 2, "TRANSPORT  :: Send an initial UDP datagram with %d null padding packets to client.\n", NUM_PACKETS_TO_SEND);
		m_Traces->WriteTrace(log_output, LEVEL_TRZ_4);
	}

	delete[]nullPaddingPckt;

	//Begin to receive and send TS packets
	while (getPerformSend()){
		numRecvs = 0;

		numMaxRecv = 10;
		
		while (numRecvs < numMaxRecv)	//The first time it receives more than one followed messages and they are saved at buffer
		{
			recv_size = size;
			strcpy(server_reply, "");

			if ((recv_size = recv(m_socketHTTP, server_reply, recv_size, 0)) == SOCKET_ERROR)
			{
				err = WSAGetLastError();
				if (err == WSAECONNRESET || err == 0) //The connection is closed
				{
					shutdown(m_socketHTTP, SD_BOTH);
					closesocket(m_socketHTTP);
					return -1;
				}

				return 0;
			}

			if (recv_size == 0)
			{
				receivingDataHTTP = 0;

				if (notstream < 3)  //The message will appear initially if there is no flow
				{
					m_Traces->WriteTrace("TRANSPORT:: Not receiving HTTP Transport Stream.\n", LEVEL_TRZ_3);
					notstream++;
				}
			}
			else
			{
				receivingDataHTTP = 1;

				if (strstr(server_reply, "HTTP/1.0 200 OK") == NULL && strstr(server_reply, "HTTP/1.1 200 OK") == NULL)
				{
					if (notstream != 0)
					{
						notstream = 0;
						m_Traces->WriteTrace("TRANSPORT:: Receiving HTTP Transport Stream.\n", LEVEL_TRZ_3);
					}

					if (m_Traces->IsLevelWriteable(LEVEL_TRZ_6))
					{
						_snprintf(log_output, 5500 - 2, "TRANSPORT  :: Received from Socket:     %d bytes.\n", recv_size);
						m_Traces->WriteTrace(log_output, LEVEL_TRZ_6);
					}

					m_basicRingBuffer->Insert(server_reply, recv_size);
				}
				else
				{
					m_basicRingBuffer->TreatingHTTPMessage(server_reply, recv_size);

					continue;
				}
			}

			if (!inicio)
			{
				numRecvs++;
				if (numRecvs == numMaxRecv - 1)
				{
					inicio = 1;
					//Validate the first TS packet. Check bytes sync (0x47). Check after to have received several message for if it is not is full (completo) in the fist
					if (m_basicRingBuffer->CheckValidTSPacket())
						m_Traces->WriteTrace("TRANSPORT  :: Start receiving HTTP Transport Stream. Find synchronization Sync byte 0x47\n", LEVEL_TRZ_2);
					else
						m_Traces->WriteTrace("TRANSPORT:: (ERR) First TS packet is not correct, have not synchronization Sync byte 0x47. It will be resynchronized\n", LEVEL_TRZ_2);
					
				}
			}
			else
			{
				numRecvs = numMaxRecv;
			}
		}

		if (m_basicRingBuffer->GetBusySpaceBuf())
		{
			strcpy(dataR, "");

#ifdef SplitSendTSPacket
			countTosplit++;
			if (countTosplit == 40)
			{
				countTosplit = 0;
				//Send Datagram:

				tamSend = m_basicRingBuffer->GetMultipleTSPacket(dataR, 3);
				if (tamSend)
				{

					res = sendto(m_socketUDP,
						dataR,
						tamSend,
						0, (SOCKADDR *)& RecvAddr, sizeof(RecvAddr));

					if (res == SOCKET_ERROR) {
						char txt[1024];
						memset(txt, 0, 1024);
						_snprintf(txt, 1024-2, "TRANSPORT  :: Error sending packet to UDP %s:%d\n", m_ipSend, m_portSend);
						m_Traces->WriteTrace(txt, ERR);

						return 0;
					}
					if (m_Traces->IsLevelWriteable(LEVEL_TRZ_6))
					{
						_snprintf(log_output, 1024-2, "TRANSPORT  :: Sent to client:           %d bytes.\n", tamSend);
						m_Traces->WriteTrace(log_output, LEVEL_TRZ_6);
					}

					tamSend = m_basicRingBuffer->GetMultipleTSPacket(dataR, 4);

					res = sendto(m_socketUDP,
						dataR,
						tamSend,
						0, (SOCKADDR *)& RecvAddr, sizeof(RecvAddr));

					if (res == SOCKET_ERROR) {
						char txt[1024];
						memset(txt, 0, 1024);
						_snprintf(txt, 1024-2, "TRANSPORT  :: Error sending packet to UDP %s:%d\n", m_ipSend, m_portSend);
						m_Traces->WriteTrace(txt, ERR);

						return 0;
					}

					if (m_Traces->IsLevelWriteable(LEVEL_TRZ_6))
					{
						_snprintf(log_output, 1024-2, "TRANSPORT  :: Sent to client:           %d bytes.\n", tamSend);
						m_Traces->WriteTrace(log_output, LEVEL_TRZ_6);
					}

					continue;
				}
			}
#endif

			tamSend = m_basicRingBuffer->GetMultipleTSPacket(dataR, 7);

			if (strlen(dataR) && tamSend)
			{

				//Send UDP Datagram:

				res = sendto(m_socketUDP,
					dataR,
					tamSend,
					0, (SOCKADDR *)& RecvAddr, sizeof(RecvAddr));
				
				if (res == SOCKET_ERROR) {
					char txt[1024];
					memset(txt, 0, 1024);
					_snprintf(txt, 1024 - 2, "TRANSPORT  :: Error sending packet to UDP %s:%d\n", m_ipSend, m_portSend);
					m_Traces->WriteTrace(txt, ERR);

					return 0;
				}
				if (m_Traces->IsLevelWriteable(LEVEL_TRZ_6))
				{
					_snprintf(log_output, 5500 - 2, "TRANSPORT  :: Sent to client:           %d bytes.\n", tamSend);
					m_Traces->WriteTrace(log_output, LEVEL_TRZ_6);
				}
			}
		}
	}

	delete[]log_output;

	return 1;
}

int CTransport::TreatReceivedDataHTTP()
{
	char *msg = new char[5200];
	memset(msg, 0, 5200);
	char dataR[1316];
	char server_reply[1316];
	int recv_size = 1316;
	int numRecvs = 0;
	int numMaxRecv = 10;
	int nbytes = 0;
	int inicio = 0;
	int tamSend = 0;
	int notstream = 0;

	sockaddr_in RecvAddr;
	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(m_portSend);
	RecvAddr.sin_addr.s_addr = inet_addr(m_ipSend);


	_snprintf(msg, 5200 - 2, "GET /%s HTTP/1.1\r\n\r\n", CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].datosGETHTTP));
	
	m_Traces->WriteTrace("TRANSPORT  :: Sending Request HTTP message (GET)\n", LEVEL_TRZ_3);
	if (send(m_socketHTTP, msg, strlen(msg), 0) < 0)
	{
		return 0;
	}
	delete []msg;

	while (getPerformSend()){
		numRecvs = 0;
		numMaxRecv = 10;
		while (numRecvs < numMaxRecv)	//The fisrt time it receives several followed messages and they are saved at buffer
		{
			recv_size = 1316;
			strcpy(server_reply, "");

			if ((recv_size = recv(m_socketHTTP, server_reply, recv_size, 0)) == SOCKET_ERROR)
			{
				return 0;
			}

			if (recv_size == 0)
			{
				receivingDataHTTP = 0;
				if (notstream < 3)  //The message will appear initially if there is no flow
				{
					m_Traces->WriteTrace("TRANSPORT:: Not receiving HTTP Transport Stream.\n", LEVEL_TRZ_3);
					notstream++;
				}
			}
			else
			{
				receivingDataHTTP = 1;

				if (strstr(server_reply, "HTTP/1.0 200 OK") == NULL && strstr(server_reply, "HTTP/1.1 200 OK") == NULL)
				{
					if (notstream != 0)
					{
						notstream = 0;
						m_Traces->WriteTrace("TRANSPORT:: Receiving HTTP Transport Stream.\n", LEVEL_TRZ_3);
					}

					m_ringBuffer->Insert(server_reply, recv_size);
				}
				else
				{
					m_ringBuffer->TreatingHTTPMessage(server_reply, recv_size);
					continue;
				}
			}

			if (!inicio)
			{
				numRecvs++;
				if (numRecvs == numMaxRecv - 1)
				{
					inicio = 1;
					//Validate the first TS packet. Check bytes sync (0x47)
					if (m_ringBuffer->CheckValidTSPacket()) 
						m_Traces->WriteTrace("TRANSPORT  :: Start receiving HTTP Transport Stream. Find synchronization Sync byte 0x47\n", LEVEL_TRZ_2);
					else
						m_Traces->WriteTrace("TRANSPORT  :: (ERR) First TS packet is not correct, have not synchronization Sync byte 0x47. It will be resynchronized\n", LEVEL_TRZ_2);
				}
			}
			else
			{
				numRecvs = numMaxRecv;
			}
		}

		if (m_ringBuffer->GetBusySize())
		{
			strcpy(dataR, "");

			tamSend = m_ringBuffer->GetTSPackets(dataR);

			if (strlen(dataR) && tamSend)
			{
				//Send Datagram:

				int res = sendto(m_socketUDP,
					dataR,
					tamSend,
					0, (SOCKADDR *)& RecvAddr, sizeof(RecvAddr));

				if (res == SOCKET_ERROR) {
					char txt[1024];
					memset(txt, 0, 1024);
					_snprintf(txt, 1024 - 2, "TRANSPORT  :: Error sending packet to UDP %s:%d\n", m_ipSend, m_portSend);
					m_Traces->WriteTrace(txt, ERR);

					return 0;
				}
			}
		}
	}

	return 1;
}

void CTransport::StopTransportStreamHTTP()
{
	m_PerformSend = 0;

	if (getState() == 1)
	{
		m_basicRingBuffer->SemaphoreSignal();

		m_Traces->WriteTrace("TRANSPORT  :: Closing HTTP socket.\n", LEVEL_TRZ_5);

		shutdown(m_socketHTTP, SD_BOTH);
		closesocket(m_socketHTTP);
		m_socketHTTP = 0;

		shutdown(m_socketUDP, SD_BOTH);
		closesocket(m_socketUDP);
		m_socketUDP = 0;

		m_Traces->WriteTrace("DBG        :: End Closing HTTP socket.\n", LEVEL_TRZ_6);

#ifdef UseRingBuffer
		m_ringBuffer->Initialize();
#endif
	}
}

int CTransport::ChangeSourceTS(int channel, CString pidsToFilterList)
{
	int res;

	if (m_typeTransportInput == UDP_TS)
	{
		StopTransportStreamUDP();
	}
	else if (m_typeTransportInput == HTTP_TS)
	{
		StopTransportStreamHTTP();
	}

	res = InitilizeTransportStream(channel, pidsToFilterList);

	if (res <= 0)
	{
		if (res == -1)
		{
			m_Traces->WriteTrace("TRANSPORT  :: Could be not initiate TRANSPORT phase. Channel not in MAPPING LIST.\n", LEVEL_TRZ_1);
			return -1;
		}
		else
		{
			m_Traces->WriteTrace("TRANSPORT  :: Could be not initiate TRANSPORT phase\n", LEVEL_TRZ_1);
			if (res == -2)
				return-2;
			else
				return 0;
		}
	}

	return 1;
}

void CTransport::setPerformSend(int send, int tuner)
{
	int envio = m_PerformSend;
	
	m_PerformSend = send;

	char log_output[1024];
	memset(log_output, 0, 1024);

	if (envio == 1 && send == 0)
	{
		receivingDataHTTP = 0;

		//The sockets are closed
		if (m_typeTransportInput == UDP_TS)
		{
			StopTransportStreamUDP();
		}
		else if (m_typeTransportInput == HTTP_TS)
		{
			StopTransportStreamHTTP();
		}

		StopThreadTransport();

		if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
		{
			_snprintf(log_output, 1024 - 2, "TRANSPORT  :: Stop TS Packets Streaming (Forwading)\n"); //parada de reenvio de packetes
			m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
		}
	}
};

void CTransport::ChangeFilterPIDsList(CString pidsToFilterList, int internalPFiltering, int externalPFiltering, int channel)
{
	int index, index2;

	m_basicRingBuffer->GetSetPIFFilteringData(0, pidsToFilterList, internalPFiltering);
	setApplyExtPidFiltering(externalPFiltering);

	m_PosChannelInMapList = cfgProxy->ObtainIndexChannel(channel);

	if (m_PosChannelInMapList != -1)
	{
		//External PID Filtering

		m_dataGETHTTP = cfgProxy->m_infoChannels[m_PosChannelInMapList].datosGETHTTP;

		cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet_ExtPidFilt = cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet;

		if (externalPFiltering)
		{
			CString tmp;
			index = m_dataGETHTTP.Find(L"&pids=", 0);
			if (index != -1)
			{
				index2 = m_dataGETHTTP.Find(L"&", index+1);
				if (index2 == -1)
				{
					m_dataGETHTTP = m_dataGETHTTP.Left(index);
				}
				else
				{
					tmp = m_dataGETHTTP;
					m_dataGETHTTP = tmp.Left(index);
					m_dataGETHTTP.Append(tmp.Right(tmp.GetLength() - index2));
				}
			}

			index = cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet_ExtPidFilt.Find(L"&pids=", 0);
			if (index != -1)
			{
				index2 = cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet_ExtPidFilt.Find(L"&", index + 1);
				if (index2 == -1)
				{
					cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet_ExtPidFilt = cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet_ExtPidFilt.Left(index);
				}
				else
				{
					tmp = m_dataGETHTTP;
					cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet_ExtPidFilt = tmp.Left(index);
					cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet_ExtPidFilt.Append(tmp.Right(tmp.GetLength() - index2));
				}
			}


			if (!pidsToFilterList.Compare(L""))
			{
				m_dataGETHTTP.Append(L"&pids=all");
				cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet_ExtPidFilt.Append(L"&pids=all");
			}
			else
			{
				m_dataGETHTTP.Append(L"&pids=");
				m_dataGETHTTP.Append(pidsToFilterList);
				cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet_ExtPidFilt.Append(L"&pids=");
				cfgProxy->m_infoChannels[m_PosChannelInMapList].URLGet_ExtPidFilt.Append(pidsToFilterList);
			}
		}
	}
}

int CTransport::ReconnectConnectionHTTP()
{
	char log_output[1024];
	memset(log_output, 0, 1024);

	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		_snprintf(log_output, 1024 - 2, "TRANSPORT  :: [Tuner %d] Error initializing use of WinSock [HTTP].\n", m_tuner);
		m_Traces->WriteTrace(log_output, ERR);
		return 0;
	}

	//Initilize Socket for HTTP. It will be a TCP socket

	if ((m_socketHTTP = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		_snprintf(log_output, 1024 - 2, "TRANSPORT  :: [Tuner %d] Error in creation of HTTP(TCP) Socket\n", m_tuner);
		m_Traces->WriteTrace(log_output, ERR);
		return 0;
	}

	struct sockaddr_in addr;
	addr.sin_addr.s_addr = inet_addr(CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].ipHTTP));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(cfgProxy->m_infoChannels[m_PosChannelInMapList].puertoHTTP);

	//ReConnect to remote server
	if (connect(m_socketHTTP, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		int err = WSAGetLastError();
		
		if (m_Traces->IsLevelWriteable(ERR))
		{
			_snprintf(log_output, 1024 - 2, "TRANSPORT  :: [Tuner %d] Can not reconnect to http://%s:%d. Error code %d\n", m_tuner, CStringA(cfgProxy->m_infoChannels[m_PosChannelInMapList].ipHTTP), cfgProxy->m_infoChannels[m_PosChannelInMapList].puertoHTTP, err);
			m_Traces->WriteTrace(log_output, ERR);
		}
		return 0;
	}

	m_failedConnectHTTP = 0;
	m_refreshFailedConnHTTP = 0;
	return 1;
}

int CTransport::SendNullPackets()
{
	char trace[1024];
	memset(trace, 0, 1024);
	int res = 0;
	//Send an UDP datagram (7 TS packets with null padding).
	char* nullPaddingPckt = new char[MAX_SIZE_DATAGRAM_TO_SEND];
	strcpy(nullPaddingPckt, "");
	m_basicRingBuffer->GenerateNullPaddingTSPackets(nullPaddingPckt, NUM_PACKETS_TO_SEND);

	sockaddr_in RecvAddr;
	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(m_portSend);
	RecvAddr.sin_addr.s_addr = inet_addr(m_ipSend);

	res = sendto(m_socketUDP,
		nullPaddingPckt,
		MAX_SIZE_DATAGRAM_TO_SEND,
		0, (SOCKADDR *)& RecvAddr, sizeof(RecvAddr));

	if (res == SOCKET_ERROR) {
		snprintf(trace, 1024-2, "TRANSPORT  :: [Tuner %d] Error sending null packet to UDP with padding %s:%d\n", m_tuner, m_ipSend, m_portSend);
		m_Traces->WriteTrace(trace, ERR);

		return 0;
	}
	snprintf(trace, 1024-2, "TRANSPORT  :: [Tuner %d] Send an UDP datagram with %d null padding packets to client.\n", m_tuner, NUM_PACKETS_TO_SEND);
	m_Traces->WriteTrace(trace, LEVEL_TRZ_4);

	delete[]nullPaddingPckt;

}

void CTransport::ReconnectHTTP()
{
	int attemptsReconn = 0;
	int reconnect = 0;
	char log_output[1024];
	memset(log_output, 0, 1024);

	shutdown(m_socketHTTP, SD_BOTH);
	closesocket(m_socketHTTP);

	if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
	{
		_snprintf(log_output, 1024 - 2, "TRANSPORT  :: [Tuner %d] Try to reconnect connection HTTP.\n", m_tuner);
		m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
	}

	while (attemptsReconn < 20 && !reconnect)
	{
		SendNullPackets();
		reconnect = ReconnectConnectionHTTP();
		attemptsReconn++;
	}

	if (!reconnect)
	{
		if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
		{
			_snprintf(log_output, 1024 - 2, "TRANSPORT  :: [Tuner %d] Not possible to reconnect connection HTTP.\n", m_tuner);
			m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
		}
		m_failedConnectHTTP = 1;
		m_refreshFailedConnHTTP = 1;
		setPerformSend(0, m_tuner);
	}
	else
	{
		if (m_Traces->IsLevelWriteable(LEVEL_TRZ_2))
		{
			_snprintf(log_output, 1024 - 2, "TRANSPORT  :: [Tuner %d] Connection HTTP is reconnected.\n", m_tuner);
			m_Traces->WriteTrace(log_output, LEVEL_TRZ_2);
		}
	}
	m_errConnectionIni = 0;
}