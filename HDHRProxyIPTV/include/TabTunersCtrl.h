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

#include "afxcmn.h"
#include "PanelInfoTunerDlg.h"

class CTabTunersCtrl :
	public CTabCtrl
{
public:
	CTabTunersCtrl();
	~CTabTunersCtrl();

	int m_numTabs;
	//Array to hold the list of dialog boxes/tab pages for CTabCtrl
	int m_DialogID;

	//CDialog Array Variable to hold the dialogs
	CPanelInfoTunerDlg *m_Dialog;

	//Function to Create the dialog boxes during startup
	void InitDialogs(int numTabs);

	//Function to activate the tab dialog boxes
	void ActivateTabDialogs();

	void UpdateDataTuner();

	afx_msg void OnSelchange(NMHDR* pNMHDR, LRESULT* pResult);
};