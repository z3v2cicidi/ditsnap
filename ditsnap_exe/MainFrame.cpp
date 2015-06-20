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
	  dbTreeView_(CDbTreeView(tableController, tableModel)),
	  tableListView_(CTableListView(tableModel))
{
}

LRESULT CMainFrame::OnCreate(LPCREATESTRUCT lpcs)
{
	CreateSimpleStatusBar();
	UISetCheck(ID_VIEW_STATUS_BAR, 1);
	m_hWndClient = splitter_.Create(m_hWnd, rcDefault, nullptr,
	                                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	splitter_.SetSplitterExtendedStyle(0);
	dbTreeView_.Create(splitter_, rcDefault, nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TVS_HASLINES |
		TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE);
	splitter_.SetSplitterPane(SPLIT_PANE_LEFT, dbTreeView_);
	tableListView_.Create(splitter_, rcDefault, nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT |
		LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE);
	splitter_.SetSplitterPane(SPLIT_PANE_RIGHT, tableListView_);
	UpdateLayout();
	splitter_.SetSplitterPosPct(25);
	return 0;
}

void CMainFrame::OnFileExit(UINT uCode, int nID, HWND hwndCtrl)
{
	PostMessage(WM_CLOSE);
}

void CMainFrame::OnFileOpen(UINT uCode, int nID, HWND hwndCtrl)
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
}

void CMainFrame::OnViewStatusBar(UINT uCode, int nID, HWND hwndCtrl)
{
	BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
}

void CMainFrame::OnAppAbout(UINT uCode, int nID, HWND hwndCtrl)
{
	CAboutDlg dlg;
	dlg.DoModal();
}

void CMainFrame::OnFileSnapshot(UINT uCode, int nID, HWND hwndCtrl)
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
}

void CMainFrame::OnToolFilter(UINT uCode, int nID, HWND hwndCtrl)
{
	CFilterDialog filterDialog(&tableListView_);
	filterDialog.DoModal();
}
