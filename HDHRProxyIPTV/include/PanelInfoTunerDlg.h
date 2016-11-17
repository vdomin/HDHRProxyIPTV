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
#include "afxdialogex.h"
#include "resource.h"
#include "ConfigProxy.h"

class CPanelInfoTunerDlg :	public CDialog
{
public:
	CPanelInfoTunerDlg();
	
	~CPanelInfoTunerDlg();

	// Dialog Data
	enum { IDD = IDD_TUNERS_DATA };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

public:
	CConfigProxy *m_CfgProxy;
	CEdit m_stateCli;
	CEdit m_channel;
	CEdit m_pids;
	CEdit m_TargetCli;
	CEdit m_ProtTSCli;
	CEdit m_HTTPTSCli;
	CEdit m_lockkey;
	CEdit m_program;
	CEdit m_pidsProgram;
	CEdit m_readbuffer;
	CEdit m_ringbuffer;

	void UpdateDataTuner();
};

