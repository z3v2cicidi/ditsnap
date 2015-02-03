#pragma once
#include "resource.h"
#include "MVCInterfaces.h"

namespace EseDataAccess
{
	class EseTable;
}

class CTableListView;

class CDetailDialog : public CDialogImpl<CDetailDialog>,
                      public CMessageFilter, public CIdleHandler
{
public:
	enum
	{
		IDD = IDD_DETAIL_DIALOG
	};

	virtual BOOL PreTranslateMessage(MSG* pMsg) override;

	virtual BOOL OnIdle() override;

	BEGIN_MSG_MAP(CDetailDialog)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_DESTROY(OnDestroy)
		COMMAND_ID_HANDLER_EX(IDC_OK_BUTTON, OnOK)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER_EX(IDC_CHECK1, OnCheck);
		COMMAND_HANDLER(IDC_BUTTON_COPYALL, BN_CLICKED, OnBnClickedButtonCopyall)
		END_MSG_MAP()

	CDetailDialog(ITableModel* tableModel, CTableListView* parent, int rowIndex);
	~CDetailDialog();

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);

	void OnDestroy();

	void OnOK(UINT uNotifyCode, int nID, CWindow wndCtl);

	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);

	void OnCheck(UINT uNotifyCode, int nID, CWindow wndCtl);

private:
	ITableModel* tableModel_;
	CTableListView* parent_;
	CListViewCtrl detailListView_;
	CButton checkBox_;
	EseDataAccess::EseTable* eseTable_;
	int rowIndex_;

	void SetupTopLabel();
	void SetupListItems();
	LRESULT OnBnClickedButtonCopyall(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	DISALLOW_COPY_AND_ASSIGN(CDetailDialog);
};
