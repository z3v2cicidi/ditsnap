#include "stdafx.h"
#include "MainFrm.h"
#include "TableModel.h"
#include "TableController.h"

CAppModule _Module;

CMainFrame* MvcInit()
{
	ITableModel* tableModel = new CTableModel();
	ITableController* tableController = new CTableController(tableModel);
	return tableController->GetView();
}

int Run(LPTSTR /*lpstrCmdLine*/  = nullptr, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CMainFrame* wndMain = MvcInit();

	if (wndMain->CreateEx() == nullptr)
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
	HRESULT hRes = CoInitialize(nullptr);
	ATLASSERT(SUCCEEDED(hRes));
	::DefWindowProc(nullptr, 0, 0, 0L);
	AtlInitCommonControls(ICC_BAR_CLASSES);
	hRes = _Module.Init(nullptr, hInstance);
	ATLASSERT(SUCCEEDED(hRes));
	int nRet = Run(lpstrCmdLine, nCmdShow);
	_Module.Term();
	CoUninitialize();
	return nRet;
}
