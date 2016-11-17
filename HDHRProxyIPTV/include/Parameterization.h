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

/* DISCOVERY */
#define	ID_DISP_SERV_HDHR	"0x1010C032"	//Default

/* CONTROL */
#define ERROR_MSG				0
#define ERROR_LOCKKEY_MSG		1
#define FEATURES_MSG			2
#define MODEL_MSG				3
#define TUNERX_STATUS_MSG		4
#define TUNERX_TARGET_MSG		5
#define TUNERX_STREAMINFO_MSG	6
#define TUNERX_CHANNELMAP_MSG	7
#define TUNERX_PROGRAM_MSG		8
#define SYS_HWMODEL_MSG			9
#define SYS_COPYRIGHT_MSG		10
#define HELP_MSG				11
#define VERSION_MSG				12
#define LINEUP_LOC_MSG			13
#define SYS_DEBUG_MSG			14
#define TUNERX_FILTER_MSG		15
#define TUNERX_LOCKKEY_MSG		16
#define TUNERX_DEBUG_MSG		17
#define TUNERX_CHANNEL_MSG		18
#define SYS_DVBC_MODULATION_MSG	19
#define HDHR_UPGRADE_MSG		20
#define HDHR_UPGRADE_FILE_MSG	21
#define SYS_RESTART_MSG			22

#define SYS_FEATURES		"/sys/features"
#define SYS_MODEL			"/sys/model"
#define TUNERX_STATUS		"/tuner%d/status"
#define TUNERX_TARGET		"/tuner%d/target"
#define TUNERX_STREAMINFO	"/tuner%d/streaminfo"
#define TUNERX_CHANNELMAP	"/tuner%d/channelmap"
#define TUNERX_PROGRAM		"/tuner%d/program"
#define SYS_HWMODEL			"/sys/hwmodel"
#define SYS_COPYRIGHT		"/sys/copyright"
#define HELP				"help"
#define	VERSION				"/sys/version"
#define LINEUP_LOC			"/lineup/location"
#define SYS_DEBUG			"/sys/debug"
#define TUNERX_FILTER		"/tuner%d/filter"
#define TUNERX_LOCKKEY		"/tuner%d/lockkey"
#define TUNERX_DEBUG		"/tuner%d/debug"
#define TUNERX_CHANNEL		"/tuner%d/channel"
#define SYS_DVBC_MODULATION	"/sys/dvbc_modulation"
#define SYS_RESTART			"/sys/restart"

#define ERROR_VALUE				"ERROR : Unknown message"
#define TUNERX_STATUS_VALUE		"ch=auto:%ld lock=%s ss=%d snq=%d seq=%d bps=%ld pps=%ld"
#define TUNERX_TARGET_VALUE		"none"
#define TUNERX_PROGRAM_VALUE	"%d"
#define TUNERX_LOCKKEY_VALUE	"none"
#define TUNERX_DEBUG_VALUE		"tun: ch=auto:%ld lock=%s:858000000 ss=%d snq=%d seq=%d dbg=8192-14030-1326/-38\ndev: bps = %ld resync = 0 overflow = 0\nts : bps = 0 te = 0 miss = 0 crc = 0\nnet : pps = 0 err = 0 stop = 0\n\n"
#define TUNERX_CHANNEL_VALUE	"auto:%ld"
//#define TUNERX_CHANNELMAP_VALUE "eu-bcast"
#define HELP_VALUE				"Supported configuration options:\n/ir/target <protocol>://<ip>:<port>\n/lineup/location <countrycode> : <postcode>\n/sys/copyright\n/sys/dvbc_modulation\n/sys/debug\n/sys/features\n/sys/hwmodel\n/sys/model\n/sys/restart <resource>\n/sys/version\n/tuner<n>/channel <modulation>:<freq | ch>\n/tuner<n>/channelmap <channelmap>\n/tuner<n>/debug\n/tuner<n>/filter \"0x<nnnn>-0x<nnnn> [...]\"\n/tuner<n>/lockkey\n/tuner<n>/program <program number>\n/tuner<n>/streaminfo\n/tuner<n>/status\n/tuner<n>/target <ip>:<port>\n"

//Default values to HDHR response wich are obtained of configuration file
#define VERSION_VALUE			"\"20150101\""
#define SYS_MODEL_VALUE			"\"hdhomerun_dvbt\""
#define SYS_HWMODEL_VALUE		"\"HDHR-EU\""
#define SYS_COPYRIGHT_VALUE		"\"HDHR Emulator, GPL.\""
#define SYS_DVBC_MODULATION_VALUE "\"qam64\""
#define SYS_CHANNEL_MAP_VALUE   "\"eu-bcast\""
#define SYS_FEATURES_VALUE		"\"[channelmap: eu-bcast eu-cable au-bcast au-cable tw-bcast tw-cable][modulation : t8qam64 t8qam16 t8qpsk t7qam64 t7qam16 t7qpsk t6qam64 t6qam16 t6qpsk a8qam256 - *a8qam128 - *a8qam64 - *a7qam256 - *a7qam128 - *a7qam64 - *a6qam256 - *a6qam128 - *a6qam64 - *][auto - modulation : auto auto8t auto7t auto6t auto8c auto7c auto6c]\""
#define SYS_DEBUG_VALUE			"\"[mem: nbm=104/100 qwm=6 npf=596 dmf=238][loop: pkt=0][t0: pt=9 cal=16589-12782 bsw=155/435][t1: pt=9 cal=16181-12777 bsw=155/435][eth: link=100f]\""
#define LINEUP_LOC_VALUE		"\"ES:00000\""

/* TRANSPORT */	
#define PUERTO_TRANSP_UDP	"1234"  //Port where it receives. Default value
#define PUERTO_TRANSP_RTP	1235

#define NONE		"none"
#define lock_DEF	"t8qam64"
#define ss_DEF		15//100 //76
#define snq_DEF		15//100 //73
#define seq_DEF		15//100
#define bps_DEF		1000000 //3478752
#define pps_DEF		100 //332
#define program_DEF	0

//States of Control phase, of the Tuners ("sintonizadores")
#define STANDBY		0
#define TUNED_CHAN	1
#define FILTERING	2
#define STREAMING	3
