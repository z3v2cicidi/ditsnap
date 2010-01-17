#include "stdafx.h"
#include "MainFrm.h"
#include "SnapshotWizard.h"
#include "AboutDlg.h"
#include "FilterDialog.h"
#include "../EseDataAccess/EseDataAccess.h"

using namespace EseDataAccess;

CMainFrame::CMainFrame(ITableController* tableController, 
					   ITableModel* tableModel)
	: tableController_(tableController), 
	  tableModel_(tableModel),
	  dbTreeView_(CDbTreeView(tableController, tableModel)),
	  tableListView_(CTableListView(tableController, tableModel)) {}

CMainFrame::~CMainFrame() {}

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, 
							 WPARAM /*wParam*/, 
							 LPARAM /*lParam*/, 
							 BOOL& /*bHandled*/)
{
	CreateSimpleStatusBar();
	UISetCheck(ID_VIEW_STATUS_BAR, 1);

	m_hWndClient = splitter_.Create(m_hWnd, rcDefault, NULL, 
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

	pane_.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON);
	pane_.Create(splitter_);

	dbTreeView_.Create(pane_, rcDefault, NULL, 
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TVS_HASLINES |
		TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE);
	dbTreeView_.SetFont(AtlGetDefaultGuiFont());
	pane_.SetClient(dbTreeView_);

	tableListView_.Create(splitter_, rcDefault, NULL, 
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT | 
		LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE);
	tableListView_.SetFont(AtlGetDefaultGuiFont());

	splitter_.SetSplitterPanes(pane_, tableListView_);
	UpdateLayout();
	splitter_.SetSplitterPosPct(25);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	return 0;
}

LRESULT CMainFrame::OnDestroy(UINT /*uMsg*/, 
							  WPARAM /*wParam*/, 
							  LPARAM /*lParam*/, 
							  BOOL& bHandled)
{
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);
	bHandled = FALSE;
	return 1;
}

LRESULT CMainFrame::OnFileExit(WORD /*wNotifyCode*/, 
							   WORD /*wID*/, 
							   HWND /*hWndCtl*/, 
							   BOOL& /*bHandled*/)
{
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT CMainFrame::OnFileOpen(WORD /*wNotifyCode*/, 
							   WORD /*wID*/, 
							   HWND /*hWndCtl*/, 
							   BOOL& /*bHandled*/)
{
	CFileDialog fileDialog(TRUE, _T("txt"), NULL, OFN_HIDEREADONLY | OFN_CREATEPROMPT,
				_T("dit file (*.dit)\0*.dit\0all file (*.*)\0*.*\0\0"));

	wchar_t moduleName[MAX_PATH];
	CPath modulePath;
	if (::GetModuleFileName(NULL, moduleName, sizeof(moduleName)) > 0)
	{
		modulePath = CPath(moduleName);
		if (modulePath.RemoveFileSpec())
			fileDialog.m_ofn.lpstrInitialDir = static_cast<LPCWSTR>(modulePath.m_strPath);
	}
	if (fileDialog.DoModal() == IDOK)
	{
		try
		{
			tableController_->OpenTable(fileDialog.m_szFileName);
		}
		catch(EseException& e)
		{
			CString errorMessage;
			errorMessage.Format(L"Error Code : %d\n%s", e.GetErrorCode(), e.GetErrorMessage().c_str());
			MessageBox( errorMessage, L"Ditsnap", MB_ICONWARNING | MB_OK);
		}
	}
	return 0;
}

LRESULT CMainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, 
									WORD /*wID*/, 
									HWND /*hWndCtl*/, 
									BOOL& /*bHandled*/)
{
	BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnAppAbout(WORD /*wNotifyCode*/, 
							   WORD /*wID*/, 
							   HWND /*hWndCtl*/, 
							   BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CMainFrame::OnFileSnapshot(WORD /*wNotifyCode*/, 
								   WORD /*wID*/, 
								   HWND /*hWndCtl*/, 
								   BOOL& /*bHandled*/)
{
	CSnapshotWizard snapshotWizard;
	if (snapshotWizard.DoModal() == IDOK)
	{
		wchar_t* snapshotFilePath = snapshotWizard.GetSnapshotFilePath();
		if (NULL != snapshotFilePath)
		{
			tableController_->OpenTable(snapshotFilePath);
		}
	}
	return 0;
}
LRESULT CMainFrame::OnToolFilter(WORD /*wNotifyCode*/, 
								 WORD /*wID*/, 
								 HWND /*hWndCtl*/, 
								 BOOL& /*bHandled*/)
{
	if (NULL != tableListView_)
	{
		CFilterDialog filterDialog(&tableListView_);
		filterDialog.DoModal();
	}
	return 0;
}
