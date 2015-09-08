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
#include "io.h"
#include "stdio.h"
#include "ConfigProxy.h"
#include "ctype.h"
#include "string.h"

#define TRACE_FILE_NAME ".\\HDHRProxyIPTV.log"
#define NO_TRACE	0
#define LEVEL_TRZ_1	1
#define LEVEL_TRZ_2	2
#define LEVEL_TRZ_3	3
#define LEVEL_TRZ_4	4
#define LEVEL_TRZ_5	5	//Traces of flow http
#define LEVEL_TRZ_6	6	//Headers of TS packets (188) sent by the Client on HTTP Transport
#define ERR			7
#define WRNG		8  //WARNING

class CTrace
{
public:
	FILE *m_traceFile;
	int m_traceLevel;
	CConfigProxy* m_cfgProxy;

	CTrace();
	~CTrace();

	int getTraceLevel() { return m_traceLevel; }
	void setTraceLevel(int nivTrz) { m_traceLevel = nivTrz; }
	void InitializeTrace(int nivelTrz = 0);
	void StopTrace();
	char* ObtainCurrentDateTime();
	void WriteTrace(char *traza, int level);
	void WriteTraceForceLevel(char *traza, int level);
	void CleanTrace();
	int IsPrintable(char* cad);
	char* ConvertToHex(char* cad);
	char* ConvertHexToAscii(char* cad);
	char* ConvertToAHexWithoutSpaces(char* cad, int size);
	char* ConvertToAXHexWithoutSpaces(char* cad, int size);
	int IsLevelWriteable(int level);
};