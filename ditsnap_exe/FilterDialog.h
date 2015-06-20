#pragma once
#include "TableListView.h"
#include "resource.h"

class CFilterDialog : public CDialogImpl<CFilterDialog>
{
public:
	enum
	{
		IDD = IDD_FILTER_DIALOG
	};

	BEGIN_MSG_MAP(CFilterDialog)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_ID_HANDLER_EX(IDOK, OnOK)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
	END_MSG_MAP()

	CFilterDialog(CTableListView* mainListView);

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnOK(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);

private:
	CTableListView* mainListView_;
	CButton checkBoxClassSchema_;
	CButton checkBoxAttributeSchema_;
	CButton checkBoxSubSchema_;
	CButton checkBoxDisplaySpecifier_;
	CButton checkBoxOthers_;

	DISALLOW_COPY_AND_ASSIGN(CFilterDialog);
};
