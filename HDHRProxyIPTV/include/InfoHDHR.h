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
#include "hdhomerun_os_windows.h"
#include "Transport.h"

typedef struct
{
	int tipoMsg;		//Type of message received by HDHR client
	char *RequestMsg;//Request from HDHR client
	char *peticionMsg; //Type of received message
	int setMsg;			//Indicator of received message type set
	char *setValue;		//Content of received message type set
	char unknownMsg[50];	//received message is unknown
	long numMsg;		//Numbering of received message
	int numTuner;
	uint32_t seqUpgrade;
	int upgradeMsg;
	uint32_t  IDLockkeyReceived; // ID de received lock (lockkey)
} InfoMessageHDHR;
