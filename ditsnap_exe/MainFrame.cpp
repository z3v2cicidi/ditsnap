#include "stdafx.h"
#include "MainFrame.h"
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
	  tableListView_(CTableListView(tableController, tableModel))
{
}

LRESULT CMainFrame::OnCreate(LPCREATESTRUCT lpcs)
{
	CreateSimpleStatusBar();
	UISetCheck(ID_VIEW_STATUS_BAR, 1);

	m_hWndClient = splitter_.Create(m_hWnd, rcDefault, nullptr,
	                                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

	pane_.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON);
	pane_.Create(splitter_);

	dbTreeView_.Create(pane_, rcDefault, nullptr,
	                   WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TVS_HASLINES |
	                   TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE);
	dbTreeView_.SetFont(AtlGetDefaultGuiFont());
	pane_.SetClient(dbTreeView_);

	tableListView_.Create(splitter_, rcDefault, nullptr,
	                      WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT |
	                      LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE);
	tableListView_.SetFont(AtlGetDefaultGuiFont());

	splitter_.SetSplitterPanes(pane_, tableListView_);
	UpdateLayout();
	splitter_.SetSplitterPosPct(25);
	return 0;
}

LRESULT CMainFrame::OnFileExit(UINT uCode, int nID, HWND hwndCtrl)
{
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT CMainFrame::OnFileOpen(UINT uCode, int nID, HWND hwndCtrl)
{
	CFileDialog fileDialog(TRUE, _T("txt"), nullptr, OFN_HIDEREADONLY | OFN_CREATEPROMPT,
	                           _T("dit file (*.dit)\0*.dit\0all file (*.*)\0*.*\0\0"));

	wchar_t moduleName[MAX_PATH];
	CPath modulePath;
	if (::GetModuleFileName(nullptr, moduleName, sizeof(moduleName)) > 0)
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
		catch (EseException& e)
		{
			CString errorMessage;
			errorMessage.Format(L"Error Code : %d\n%s", e.GetErrorCode(), e.GetErrorMessage().c_str());
			MessageBox(errorMessage, L"Ditsnap", MB_ICONWARNING | MB_OK);
		}
	}
	return 0;
}

LRESULT CMainFrame::OnViewStatusBar(UINT uCode, int nID, HWND hwndCtrl)
{
	BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnAppAbout(UINT uCode, int nID, HWND hwndCtrl)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CMainFrame::OnFileSnapshot(UINT uCode, int nID, HWND hwndCtrl)
{
	CSnapshotWizard snapshotWizard;
	if (snapshotWizard.DoModal() == IDOK)
	{
		wchar_t* snapshotFilePath = snapshotWizard.GetSnapshotFilePath();
		if (nullptr != snapshotFilePath)
		{
			tableController_->OpenTable(snapshotFilePath);
		}
	}
	return 0;
}

LRESULT CMainFrame::OnToolFilter(UINT uCode, int nID, HWND hwndCtrl)
{
	if (nullptr != tableListView_)
	{
		CFilterDialog filterDialog(&tableListView_);
		filterDialog.DoModal();
	}
	return 0;
}
