// GUITest2.cpp : main source file for GUITest2.exe
//

#include "stdafx.h"
#include "resource.h"
#include "MainFrm.h"
#include "../Common/WinUtil.h"
#include "TableModel.h"
#include "TableController.h"

CAppModule _Module;

CMainFrame* MvcInit()
{
	ITableModel* tableModel = new CTableModel();
	ITableController* tableController = new CTableController(tableModel);
	return tableController->GetView();
}

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CMainFrame* wndMain = MvcInit();

	if (wndMain->CreateEx() == NULL)
	{
		ATLTRACE(_T("Main window creation failed!\n"));
		return 0;
	}

	wndMain->ShowWindow(nCmdShow);

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	// Only support Server OS.
	WinUtil::WinVersion winVersion = WinUtil::GetWinVersion();
	if (winVersion == WinUtil::WINVERSION_PRE_2000 || 
		winVersion == WinUtil::WINVERSION_XP || 
		winVersion == WinUtil::WINVERSION_VISTA)
	{
		::MessageBox(NULL, L"Ditsnap only runs on Windows Server.", L"Version Check", 0);
		return -1;
	}

	HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
