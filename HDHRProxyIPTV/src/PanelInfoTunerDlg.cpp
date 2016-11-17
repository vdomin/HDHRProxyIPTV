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
#include "PanelInfoTunerDlg.h"


CPanelInfoTunerDlg::CPanelInfoTunerDlg()
{
	m_CfgProxy = CConfigProxy::GetInstance();

}


CPanelInfoTunerDlg::~CPanelInfoTunerDlg()
{
}

void CPanelInfoTunerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_ESTADO, m_stateCli);
	DDX_Control(pDX, IDC_EDIT_CHANNELFREQ, m_channel);
	DDX_Control(pDX, IDC_EDIT_PIDS, m_pids);
	DDX_Control(pDX, IDC_EDIT_TARGET, m_TargetCli);
	DDX_Control(pDX, IDC_EDIT_PROTOCOL, m_ProtTSCli);
	DDX_Control(pDX, IDC_EDIT_HTTP_TS, m_HTTPTSCli);
	DDX_Control(pDX, IDC_EDIT_LOCKKEY, m_lockkey);
	DDX_Control(pDX, IDC_EDIT_PROGRAM, m_program);
	DDX_Control(pDX, IDC_EDIT_PIDS_FILTERING, m_pidsProgram);
	DDX_Control(pDX, IDC_EDIT_IN_BUFFER, m_readbuffer);
	DDX_Control(pDX, IDC_EDIT_OUT_BUFFER, m_ringbuffer);
}

void CPanelInfoTunerDlg::UpdateDataTuner()
{
	m_stateCli.SetWindowText(m_CfgProxy->m_infoActualSelCli->state);
	CString ch;
	if (m_CfgProxy->m_infoActualSelCli->channel)
	{
		if (!m_CfgProxy->m_infoActualSelCli->channelNotInMapList)
			ch.Format(L"%ld", m_CfgProxy->m_infoActualSelCli->channel);
		else
			ch.Format(L"![%ld]", m_CfgProxy->m_infoActualSelCli->channel);
	}
	else
		ch.Format(L"");
	m_channel.SetWindowText(ch);
	m_pids.SetWindowText(m_CfgProxy->m_infoActualSelCli->pids);
	m_TargetCli.SetWindowText(m_CfgProxy->m_infoActualSelCli->target);
	m_lockkey.SetWindowText(m_CfgProxy->m_infoActualSelCli->lockkey);
	m_program.SetWindowText(m_CfgProxy->m_infoActualSelCli->program);
	m_pidsProgram.SetWindowText(m_CfgProxy->m_infoActualSelCli->pidsProgram);
	m_ProtTSCli.SetWindowText(m_CfgProxy->m_infoActualSelCli->protocolTS);
	if (!m_CfgProxy->m_infoActualSelCli->protocolTS.Compare(L"HTTP"))
		m_HTTPTSCli.SetWindowText(m_CfgProxy->m_infoActualSelCli->httpTS);
	else
		m_HTTPTSCli.SetWindowText(m_CfgProxy->m_infoActualSelCli->udpTS);
	m_readbuffer.SetWindowText(m_CfgProxy->m_infoActualSelCli->readbuffer);
	m_ringbuffer.SetWindowText(m_CfgProxy->m_infoActualSelCli->ringbuffer);

	m_HTTPTSCli.RedrawWindow();
	m_program.RedrawWindow();
	RedrawWindow();
}