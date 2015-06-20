#pragma once
#include "resource.h"
#include "TableListView.h"
#include "DbTreeView.h"
#include "Interfaces.h"

class EseDbManager;

class CMainFrame : public CFrameWindowImpl<CMainFrame>, public CUpdateUI<CMainFrame>
{
public:
	DECLARE_FRAME_WND_CLASS(nullptr, IDR_MAINFRAME)

	BEGIN_UPDATE_UI_MAP(CMainFrame)
		UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP_EX(CMainFrame)
		MSG_WM_CREATE(OnCreate)
		COMMAND_ID_HANDLER_EX(ID_APP_EXIT, OnFileExit)
		COMMAND_ID_HANDLER_EX(ID_FILE_OPEN, OnFileOpen)
		COMMAND_ID_HANDLER_EX(ID_FILE_TAKESNAPSHOTANDOPEN, OnFileSnapshot)
		COMMAND_ID_HANDLER_EX(ID_VIEW_STATUS_BAR, OnViewStatusBar)
		COMMAND_ID_HANDLER_EX(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER_EX(ID_TOOL_FILTER, OnToolFilter)
		CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
		CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
		CHAIN_CLIENT_COMMANDS() // Command chains views
		REFLECT_NOTIFICATIONS() // Message Reflection
	END_MSG_MAP()

	CMainFrame(EseDbManager* eseDbManager);

	LRESULT OnCreate(LPCREATESTRUCT lpcs);
	void OnFileExit(UINT uCode, int nID, HWND hwndCtrl);
	void OnFileOpen(UINT uCode, int nID, HWND hwndCtrl);
	void OnFileSnapshot(UINT uCode, int nID, HWND hwndCtrl);
	void OnViewStatusBar(UINT uCode, int nID, HWND hwndCtrl);
	void OnAppAbout(UINT uCode, int nID, HWND hwndCtrl);
	void OnToolFilter(UINT uCode, int nID, HWND hwndCtrl);

private:
	CTableListView tableListView_;
	CDbTreeView dbTreeView_;
	CSplitterWindow splitter_;
	EseDbManager* eseDbManager_;

	DISALLOW_COPY_AND_ASSIGN(CMainFrame);
};
