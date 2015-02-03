#include "stdafx.h"
#include "FilterDialog.h"
#include "resource.h"

CFilterDialog::CFilterDialog(CTableListView* mainListView)
	: mainListView_(mainListView)
{
};

BOOL CFilterDialog::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	CenterWindow();

	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR,
	                               GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);

	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR,
	                                    GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	checkBoxClassSchema_ = GetDlgItem(IDC_CHECK1);
	checkBoxAttributeSchema_ = GetDlgItem(IDC_CHECK2);
	checkBoxSubSchema_ = GetDlgItem(IDC_CHECK3);
	checkBoxDisplaySpecifier_ = GetDlgItem(IDC_CHECK4);
	checkBoxOthers_ = GetDlgItem(IDC_CHECK5);

	checkBoxClassSchema_.SetCheck(BST_CHECKED);
	checkBoxAttributeSchema_.SetCheck(BST_CHECKED);
	checkBoxSubSchema_.SetCheck(BST_CHECKED);
	checkBoxDisplaySpecifier_.SetCheck(BST_CHECKED);
	checkBoxOthers_.SetCheck(BST_CHECKED);

	return TRUE;
}

void CFilterDialog::OnOK(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	int filterFlag = checkBoxClassSchema_.GetCheck();
	filterFlag += checkBoxAttributeSchema_.GetCheck() << 1;
	filterFlag += checkBoxSubSchema_.GetCheck() << 2;
	filterFlag += checkBoxDisplaySpecifier_.GetCheck() << 3;
	filterFlag += checkBoxOthers_.GetCheck() << 4;

	mainListView_->FilterTable(filterFlag);
	EndDialog(nID);
	return;
}

void CFilterDialog::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	EndDialog(nID);
}
