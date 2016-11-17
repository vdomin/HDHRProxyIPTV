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
#include "ServiceW.h"


CServiceW::CServiceW()
{
	//Initilization of variables
	m_HandleSCManager = NULL;
	m_HandleServiceW = NULL;
}

CServiceW::~CServiceW()
{
}

int CServiceW::InstallService()
{
	//Obtain current directory
	TCHAR sDirActual[200];
	GetCurrentDirectory(200, sDirActual);

	CloseHandles();

	m_HandleSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (m_HandleSCManager == NULL)
	{
		int err = GetLastError();
		return 0;
	}

	//Create service of Windows
	m_HandleServiceW = CreateService(
					m_HandleSCManager,
					_T(NAME_SERVICE),
					_T(DESC_SERVICE), // service name to display
					//DELETE | SERVICE_START | SERVICE_STOP | SERVICE_QUERY_STATUS,//SERVICE_ALL_ACCESS, // desired access
					SERVICE_ALL_ACCESS,
					SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS, // service type
					SERVICE_AUTO_START, // start type
					SERVICE_ERROR_NORMAL, // error control type
					sDirActual,//lpszBinaryPathName, // Opcional
					NULL, // no load ordering group
					NULL, // no tag identifier
					NULL, // no dependencies
					NULL, // LocalSystem account
					NULL); // no password

	if (m_HandleServiceW == NULL)
	{
		CloseHandles();
		return 0;
	}

	//Permissions
	AssignPermissionsService(_T(NAME_SERVICE), L"Everyone", TOKEN_ALL_ACCESS);
	AssignPermissionsService(_T(NAME_SERVICE), L"Todos", TOKEN_ALL_ACCESS);

	if (!StartServiceWin())
		return 0;

	CloseHandles();

	return 1;
}

void CServiceW::CloseHandles()
{
	if (m_HandleSCManager != NULL)
	{
		CloseServiceHandle(m_HandleSCManager);
		m_HandleSCManager = NULL;
	}

	if (m_HandleServiceW != NULL)
	{
		CloseServiceHandle(m_HandleServiceW);
		m_HandleServiceW = NULL;
	}
}

int CServiceW::ObtainStateService()
{
	SERVICE_STATUS servEstado;
	SC_HANDLE handleServ;
	int result = 0;

	SC_HANDLE handleMgr = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_QUERY_LOCK_STATUS | STANDARD_RIGHTS_READ); //SC_MANAGER_ALL_ACCESS);
	
	if (handleMgr == NULL)
		return 0;
	
	handleServ = OpenService(handleMgr, _T(NAME_SERVICE), SERVICE_QUERY_STATUS);
	if (handleServ == NULL)
	{
		CloseServiceHandle(handleMgr);
		return 0;
	}

	if (QueryServiceStatus(handleServ, &servEstado))
		result = servEstado.dwCurrentState; //There are defines for the states in file .h
	else
		result = 0;
	
	//Close the handles
	CloseServiceHandle(handleServ);
	CloseServiceHandle(handleMgr);
	return result;
}

int CServiceW::IsServiceInstalled()
{
	SC_HANDLE handleMgr;
	SC_HANDLE handleServ;
		
	CloseHandles();

	handleMgr = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_QUERY_LOCK_STATUS | STANDARD_RIGHTS_READ);
	
	if (handleMgr == NULL)
		return -1;
	
	handleServ = OpenService(handleMgr, _T(NAME_SERVICE), SERVICE_QUERY_STATUS);
	
	if (handleServ == NULL)
	{
		if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
			return 0;
		else
			return -1;
	}
	
	CloseHandles();
	
	//If it get here is that the service exists, it has accessed to it.
	return 1;
}

int CServiceW::StartServiceWin()
{
	char textErr[1024];
	memset(textErr, 0, 1024);
	CString sErr;

	//
	m_HandleSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_QUERY_LOCK_STATUS | STANDARD_RIGHTS_READ);
	if (m_HandleSCManager == NULL)
	{
		int err = GetLastError();
		_snprintf_s(textErr, sizeof(textErr) - 2, "Err %d", err);
		sErr.Format(L"Error %d",err);
		AfxMessageBox(sErr, MB_OK);
		ObtainError(err);
		return 0;
	}

	m_HandleServiceW = OpenService(m_HandleSCManager, _T(NAME_SERVICE), GENERIC_EXECUTE);
	if (m_HandleServiceW == NULL)
	{
		int err = GetLastError();
		AfxMessageBox(L"2", MB_OK);
		AfxMessageBox(err, MB_OK);
		ObtainError(err);
		return 0;
	}
	
	if (!StartService(m_HandleServiceW, 0, NULL))
	{
		int err = GetLastError();
		AfxMessageBox(sErr, MB_OK);
		return 0;
	}

	return 1;
}

int CServiceW::StopService()
{
	SC_HANDLE handleMgr;
	SC_HANDLE handleServ;
	SERVICE_STATUS stateServ;

	if (IsServiceInstalled() == 1)
	{
		//If exitst the service, it is stopped
		if (ObtainStateService() == SERVICE_RUNNING)
		{
			CloseHandles();
			
			handleMgr = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_QUERY_LOCK_STATUS | STANDARD_RIGHTS_READ); //SC_MANAGER_ALL_ACCESS);
			if (handleMgr == NULL)
				return 0;
			
			handleServ = OpenService(handleMgr, _T(NAME_SERVICE), SERVICE_QUERY_STATUS | SERVICE_STOP); //GENERIC_EXECUTE); //SERVICE_ALL_ACCESS);
			if (handleServ == NULL)
				return 0;

			//DWORD err = 0;
			if (!ControlService(handleServ, SERVICE_CONTROL_STOP, &stateServ))
				return 0;
			
			CloseServiceHandle(handleServ);
			m_HandleServiceW = NULL; //FOr no liberate it after
		
			CloseHandles();
		}
	}
	return 1;
}

int CServiceW::DesInstallService()
{
	SC_HANDLE handleMgr;
	SC_HANDLE handleServ;

	CloseHandles();
	handleMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (handleMgr == NULL)
		return 0;

	handleServ = OpenService(handleMgr, _T(NAME_SERVICE), SERVICE_ALL_ACCESS);
	if (handleServ == NULL)
		return 0;

	if (DeleteService(handleServ) == 0)
	{
		CloseHandles();
		return 0;
	}

	CloseHandles();

	return 1;
}

void CServiceW::ObtainError(int error)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
		);

	CString er;
	er.Format(L"%s",(char*)lpMsgBuf);
	AfxMessageBox(er, MB_OK);
	

	LocalFree(lpMsgBuf);
}


int CServiceW::AssignPermissionsService(LPCTSTR servicio, LPTSTR usuario, DWORD permisos)
{
	EXPLICIT_ACCESS      ea;
	SECURITY_DESCRIPTOR  sd;
	PSECURITY_DESCRIPTOR psd = NULL;
	PACL                 pacl = NULL;
	PACL                 pNewAcl = NULL;
	BOOL                 bDaclPresent = FALSE;
	BOOL                 bDaclDefaulted = FALSE;
	DWORD                dwError = 0;
	DWORD                dwSize = 0;
	DWORD                dwBytesNeeded = 0;

	// Get a handle to the SCM database.
	m_HandleSCManager = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // ServicesActive database
		SC_MANAGER_ALL_ACCESS);  // full access rights

	if (NULL == m_HandleSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return 0;
	}

	// Get a handle to the service

	m_HandleServiceW = OpenService(
		m_HandleSCManager,              // SCManager database
		servicio,                 // name of service
		READ_CONTROL | WRITE_DAC); // access

	if (m_HandleServiceW == NULL)
	{
		printf("OpenService failed (%d)\n", GetLastError());
		CloseServiceHandle(m_HandleSCManager);
		return 0;
	}

	// Get the current security descriptor.

	if (!QueryServiceObjectSecurity(m_HandleServiceW,
		DACL_SECURITY_INFORMATION,
		&psd,           // using NULL does not work on all versions
		0,
		&dwBytesNeeded))
	{
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			dwSize = dwBytesNeeded;
			psd = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(),
				HEAP_ZERO_MEMORY, dwSize);
			if (psd == NULL)
			{
				// Note: HeapAlloc does not support GetLastError.
				printf("HeapAlloc failed\n");
				goto dacl_cleanup;
			}

			if (!QueryServiceObjectSecurity(m_HandleServiceW,
				DACL_SECURITY_INFORMATION, psd, dwSize, &dwBytesNeeded))
			{
				printf("QueryServiceObjectSecurity failed (%d)\n", GetLastError());
				goto dacl_cleanup;
			}
		}
		else
		{
			printf("QueryServiceObjectSecurity failed (%d)\n", GetLastError());
			goto dacl_cleanup;
		}
	}

	// Get the DACL.

	if (!GetSecurityDescriptorDacl(psd, &bDaclPresent, &pacl,
		&bDaclDefaulted))
	{
		printf("GetSecurityDescriptorDacl failed(%d)\n", GetLastError());
		goto dacl_cleanup;
	}

	// Build the ACE.

	BuildExplicitAccessWithName(&ea,
		usuario,
		permisos,/*TOKEN_ALL_ACCESS, SERVICE_START | SERVICE_STOP | READ_CONTROL | DELETE, etc*/
		SET_ACCESS, NO_INHERITANCE);

	dwError = SetEntriesInAcl(1, &ea, pacl, &pNewAcl);
	if (dwError != ERROR_SUCCESS)
	{
		printf("SetEntriesInAcl failed(%d)\n", dwError);
		goto dacl_cleanup;
	}

	// Initialize a new security descriptor.

	if (!InitializeSecurityDescriptor(&sd,
		SECURITY_DESCRIPTOR_REVISION))
	{
		printf("InitializeSecurityDescriptor failed(%d)\n", GetLastError());
		goto dacl_cleanup;
	}

	// Set the new DACL in the security descriptor.

	if (!SetSecurityDescriptorDacl(&sd, TRUE, pNewAcl, FALSE))
	{
		printf("SetSecurityDescriptorDacl failed(%d)\n", GetLastError());
		goto dacl_cleanup;
	}

	// Set the new DACL for the service object.

	if (!SetServiceObjectSecurity(m_HandleServiceW,
		DACL_SECURITY_INFORMATION, &sd))
	{
		printf("SetServiceObjectSecurity failed(%d)\n", GetLastError());
		goto dacl_cleanup;
	}
	else printf("Service DACL updated successfully\n");

	return 1;
dacl_cleanup:
	CloseServiceHandle(m_HandleSCManager);
	CloseServiceHandle(m_HandleServiceW);

	if (NULL != pNewAcl)
		LocalFree((HLOCAL)pNewAcl);
	if (NULL != psd)
		HeapFree(GetProcessHeap(), 0, (LPVOID)psd);

	return 1;
}
