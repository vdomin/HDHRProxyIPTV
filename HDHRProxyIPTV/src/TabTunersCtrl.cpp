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
#include "TabTunersCtrl.h"

CTabTunersCtrl::CTabTunersCtrl()
{
	m_DialogID = IDD_TUNERS_DATA;
	m_Dialog = new CPanelInfoTunerDlg();
}

CTabTunersCtrl::~CTabTunersCtrl()
{

}

//This function creates the Dialog boxes once
void CTabTunersCtrl::InitDialogs(int numTabs)
{
	m_Dialog->Create(m_DialogID, GetParent());
	m_numTabs = numTabs;

	CString tuner(L"");
	for (int i = 0; i < m_numTabs; i++)
	{
		tuner.Format(L"Tuner%d", i);
		InsertItem(i, (LPCTSTR)tuner);
	}
}

//Selection change event for the class derived from CTabCtrl
void CTabTunersCtrl::OnSelchange(NMHDR* pNMHDR, LRESULT* pResult)
{
	ActivateTabDialogs();
	*pResult = 0;
}

void CTabTunersCtrl::ActivateTabDialogs()
{
	int nSel = GetCurSel();

	CRect l_rectClient;
	CRect l_rectWnd;

	GetClientRect(l_rectClient);
	AdjustRect(FALSE, l_rectClient);
	GetWindowRect(l_rectWnd);
	GetParent()->ScreenToClient(l_rectWnd);
	l_rectClient.OffsetRect(l_rectWnd.left, l_rectWnd.top);

	m_Dialog->SetWindowPos(&wndTop, l_rectClient.left, l_rectClient.top, l_rectClient.Width(), l_rectClient.Height(), SWP_HIDEWINDOW);

	m_Dialog->SetWindowPos(&wndTop, l_rectClient.left, l_rectClient.top, l_rectClient.Width(), l_rectClient.Height(), SWP_SHOWWINDOW);

	m_Dialog->ShowWindow(SW_SHOW);

}

void CTabTunersCtrl::UpdateDataTuner()
{
	m_Dialog->UpdateDataTuner();
}