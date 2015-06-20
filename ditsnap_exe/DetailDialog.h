#pragma once
#include "resource.h"
#include "Interfaces.h"

namespace EseDataAccess
{
	class EseTable;
}

class CTableListView;
class EseDbManager;

class CDetailDialog : public CDialogImpl<CDetailDialog>
{
public:
	enum { IDD = IDD_DETAIL_DIALOG };

	BEGIN_MSG_MAP_EX(CDetailDialog)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER_EX(IDC_CHECK1, OnShowAllCheckBoxToggled);
		COMMAND_HANDLER_EX(IDC_BUTTON_COPYALL, BN_CLICKED, OnCopyAllButtonClicked)
	END_MSG_MAP()

	CDetailDialog(EseDbManager* eseDbManager, CTableListView* paBrent, int rowIndex);
	~CDetailDialog();
	LRESULT OnInitDialog(HWND hWnd, LPARAM lParam);
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnShowAllCheckBoxToggled(UINT uNotifyCode, int nID, CWindow wndCtl);
	LRESULT OnCopyAllButtonClicked(UINT uNotifyCode, int nID, CWindow wndCtl);

private:
	EseDbManager* eseDbManager_;
	CTableListView* parent_;
	CListViewCtrl detailListView_;
	CButton checkBox_;
	EseDataAccess::EseTable* eseTable_;
	int rowIndex_;

	void SetupTopLabel();
	void SetupListItems();
	wstring GetColumnValueString(uint columnIndex);

	DISALLOW_COPY_AND_ASSIGN(CDetailDialog);
};
