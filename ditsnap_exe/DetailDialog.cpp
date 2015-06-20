#include "stdafx.h"
#include "DetailDialog.h"
#include "../EseDataAccess/EseDataAccess.h"
#include "TableListView.h"

using namespace EseDataAccess;

CDetailDialog::CDetailDialog(EseDbManager* eseDbManager,
                             CTableListView* parent,
                             int rowIndex)
	: eseDbManager_(eseDbManager), parent_(parent), rowIndex_(rowIndex)
{
};

CDetailDialog::~CDetailDialog()
{
};

LRESULT CDetailDialog::OnInitDialog(HWND hWnd, LPARAM lParam)
{
	CenterWindow();
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR,
	                                            GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR,
	                                                 GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);
	detailListView_ = GetDlgItem(IDC_LIST1);
	checkBox_ = GetDlgItem(IDC_CHECK1);
	detailListView_.SetExtendedListViewStyle(LVS_EX_INFOTIP | LVS_EX_FULLROWSELECT);
	CRect rcList;
	detailListView_.GetWindowRect(rcList);
	int nScrollWidth = GetSystemMetrics(SM_CXVSCROLL);
	int n3DEdge = GetSystemMetrics(SM_CXEDGE);
	detailListView_.InsertColumn(0, L"Column name", LVCFMT_LEFT, 100, -1);
	detailListView_.InsertColumn(1, L"AD Simbol name", LVCFMT_LEFT, 200, -1);
	detailListView_.InsertColumn(2, L"Value", LVCFMT_LEFT,
	                             rcList.Width() - 300 - nScrollWidth - n3DEdge * 2, -1);
	checkBox_.SetCheck(1);

	try
	{
		eseDbManager_->Move(rowIndex_);
	}
	catch (EseException& e)
	{
		CString errorMessage;
		errorMessage.Format(L"Error Code : %d\n%s", e.GetErrorCode(), e.GetErrorMessage().c_str());
		MessageBox(errorMessage, L"Ditsnap", MB_ICONWARNING | MB_OK);
	}

	SetupTopLabel();
	SetupListItems();
	return TRUE;
}

void CDetailDialog::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	DestroyWindow();
}

void CDetailDialog::OnShowAllCheckBoxToggled(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	SetupListItems();
	return;
}

void CDetailDialog::SetupTopLabel()
{
	// ATTm589825 indicates RDN.@
	const int ATT_RDN = parent_->GetColumnIdFromColumnName(L"ATTm589825");
	auto rdnLabel = GetDlgItem(IDC_RDN);

	try
	{
		wstring rdn(eseDbManager_->RetrieveColumnDataAsString(ATT_RDN));
		rdnLabel.SetWindowTextW(rdn.c_str());
	}
	catch (EseException& e)
	{
		CString errorMessage;
		errorMessage.Format(L"Error Code : %d\n%s", e.GetErrorCode(), e.GetErrorMessage().c_str());
		MessageBox(errorMessage, L"Ditsnap", MB_ICONWARNING | MB_OK);
	}
	return;
}

void CDetailDialog::SetupListItems()
{
	try
	{
		detailListView_.DeleteAllItems();
		if (!checkBox_.GetCheck())
		{
			for (uint columnIndex = 0; columnIndex < eseDbManager_->GetColumnCount(); ++columnIndex)
			{
				wstring columnValues = GetColumnValueString(columnIndex);
				wstring columnName(eseDbManager_->GetColumnName(columnIndex));
				const wstring adName = parent_->GetAdNameFromColumnName(columnName);
				detailListView_.AddItem(columnIndex, 0, columnName.c_str());
				detailListView_.AddItem(columnIndex, 1, adName.c_str());
				if (0 == columnValues.size())
				{
					detailListView_.AddItem(columnIndex, 2, L"<not set>");
				}
				else
				{
					detailListView_.AddItem(columnIndex, 2, columnValues.c_str());
				}
			}
		}
		else
		{
			int visibleColumnIndex = 0;
			for (uint columnIndex = 0; columnIndex < eseDbManager_->GetColumnCount(); ++columnIndex)
			{
				wstring columnValues = GetColumnValueString(columnIndex);
				if (0 != columnValues.size())
				{
					wstring columnName(eseDbManager_->GetColumnName(columnIndex));
					const wstring adName = parent_->GetAdNameFromColumnName(columnName);
					detailListView_.AddItem(visibleColumnIndex, 0, columnName.c_str());
					detailListView_.AddItem(visibleColumnIndex, 1, adName.c_str());
					detailListView_.AddItem(visibleColumnIndex, 2, columnValues.c_str());
					++visibleColumnIndex;
				}
			}
		}
	}
	catch (EseException& e)
	{
		CString errorMessage;
		errorMessage.Format(L"Error Code : %d\n%s", e.GetErrorCode(), e.GetErrorMessage().c_str());
		MessageBox(errorMessage, L"Ditsnap", MB_ICONWARNING | MB_OK);
	}
	return;
}

wstring CDetailDialog::GetColumnValueString(uint columnIndex)
{
	wstring columnValues;
	int numberOfColumnValue = eseDbManager_->CountColumnValue(columnIndex);
	for (int itagSequence = 1; itagSequence <= numberOfColumnValue; ++itagSequence)
	{
		wstring columnValue = eseDbManager_->RetrieveColumnDataAsString(columnIndex, itagSequence);
		columnValues += columnValue;
		if (numberOfColumnValue != itagSequence)
		{
			columnValues += L"; ";
		}
	}

	return columnValues;
}

LRESULT CDetailDialog::OnCopyAllButtonClicked(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	CString copyText;
	for (int i = 0; i < detailListView_.GetItemCount(); ++i)
	{
		CString s;
		detailListView_.GetItemText(i, 0, s);
		copyText.Append(s);

		CString temp;
		detailListView_.GetItemText(i, 1, temp);
		if (temp.IsEmpty())
		{
			s.Format(L": ");
		}
		else
		{
			s.Format(L" ( %s ): ", temp);
		}
		copyText.Append(s);

		detailListView_.GetItemText(i, 2, s);
		copyText.Append(s);
		copyText.Append(L"\r\n");
	}

	if (!OpenClipboard())
	{
		MessageBox(L"Cannot open clipboard.", L"Error");
		return -1;
	}
	int bufSize = (copyText.GetLength() + 1) * sizeof(wchar_t);
	HGLOBAL hBuf = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, bufSize);
	wchar_t* pBuf = (wchar_t*) GlobalLock(hBuf);
	memcpy(pBuf, (LPCTSTR) copyText, bufSize);
	GlobalUnlock(hBuf);

	if (!EmptyClipboard())
	{
		MessageBox(L"Cannot empty clipboard.", L"Error");
		return -1;
	}

	if (nullptr == SetClipboardData(CF_UNICODETEXT, hBuf))
	{
		CloseClipboard();
		return 1;
	}
	CloseClipboard();

	return 0;
}
