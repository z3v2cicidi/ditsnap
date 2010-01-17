#pragma once
#include "resource.h"
#include "MVCInterfaces.h"

// Defined in EseDataAccess.h 
namespace EseDataAccess
{
	class EseTable;
}

// Defined in ListView.h
class CTableListView;


class CDetailDialog : public CDialogImpl<CDetailDialog>,
    public CMessageFilter, public CIdleHandler
{
public:
    enum { IDD = IDD_DETAIL_DIALOG };
    virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
        return CWindow::IsDialogMessage(pMsg);
    }
    virtual BOOL OnIdle(){ return FALSE; }

	// The ATL/WTL macros make the connection between event handlers and
	// window messages.
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

    void OnDestroy()
	{
        CMessageLoop* pLoop = _Module.GetMessageLoop();
        pLoop->RemoveMessageFilter(this);
        pLoop->RemoveIdleHandler(this);
    }
    
	void OnOK(UINT uNotifyCode, int nID, CWindow wndCtl)
	{ 
		DestroyWindow(); 
	}

    void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
	{ 
		DestroyWindow(); 
	}

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
