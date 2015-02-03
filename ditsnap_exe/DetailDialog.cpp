#include "stdafx.h"
#include "DetailDialog.h"
#include "../EseDataAccess/EseDataAccess.h"
#include "TableListView.h"

using namespace EseDataAccess;

BOOL CDetailDialog::PreTranslateMessage(MSG* pMsg)
{
	return CWindow::IsDialogMessage(pMsg);
}

BOOL CDetailDialog::OnIdle()
{
	return FALSE;
}

CDetailDialog::CDetailDialog(ITableModel* tableModel,
                             CTableListView* parent,
                             int rowIndex)
	: tableModel_(tableModel), parent_(parent), rowIndex_(rowIndex)
{
};

CDetailDialog::~CDetailDialog()
{
};

BOOL CDetailDialog::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
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
		tableModel_->Move(rowIndex_);
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

void CDetailDialog::OnDestroy()
{
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);
}

void CDetailDialog::OnOK(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	DestroyWindow();
}

void CDetailDialog::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	DestroyWindow();
}

void CDetailDialog::OnCheck(UINT uNotifyCode, int nID, CWindow wndCtl)
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
		wstring rdn(tableModel_->RetrieveColumnDataAsString(ATT_RDN));
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
			// list up all row
			for (uint columnIndex = 0; columnIndex < tableModel_->GetColumnCount(); ++columnIndex)
			{
				// Column Name
				wstring columnName(tableModel_->GetColumnName(columnIndex));

				// AD Simbol Name
				const wstring adName = parent_->GetAdNameFromColumnName(columnName);

				// Value (Support to display multi-valued column data)
				wstring columnValues;
				int numberOfColumnValue = tableModel_->CountColumnValue(columnIndex);
				for (int itagSequence = 1; itagSequence <= numberOfColumnValue; ++itagSequence)
				{
					wstring columnValue = tableModel_->RetrieveColumnDataAsString(columnIndex, itagSequence);
					columnValues += columnValue;
					if (numberOfColumnValue != itagSequence)
					{
						columnValues += L"; ";
					}
					else if (numberOfColumnValue > 1)
					{
						columnValues += L" (Multi-valued column)";
					}
				}

				if (0 == columnValues.size())
				{
					detailListView_.AddItem(columnIndex, 0, columnName.c_str());
					detailListView_.AddItem(columnIndex, 1, adName.c_str());
					detailListView_.AddItem(columnIndex, 2, L"<not set>");
				}
				else
				{
					detailListView_.AddItem(columnIndex, 0, columnName.c_str());
					detailListView_.AddItem(columnIndex, 1, adName.c_str());
					detailListView_.AddItem(columnIndex, 2, columnValues.c_str());
				}
			}
		}
		else
		{
			// list up only rows that have values
			int visibleColumnIndex = 0;

			for (uint columnIndex = 0; columnIndex < tableModel_->GetColumnCount(); ++columnIndex)
			{
				// Column Name
				wstring columnName(tableModel_->GetColumnName(columnIndex));

				// AD Simbol Name
				const wstring adName = parent_->GetAdNameFromColumnName(columnName);

				// Value (Support to display multi-valued column data)
				wstring columnValues;
				int numberOfColumnValue = tableModel_->CountColumnValue(columnIndex);
				for (int itagSequence = 1; itagSequence <= numberOfColumnValue; ++itagSequence)
				{
					wstring columnValue(tableModel_->RetrieveColumnDataAsString(columnIndex, itagSequence));
					columnValues += columnValue;
					if (numberOfColumnValue != itagSequence)
					{
						columnValues += L"; ";
					}
					else if (numberOfColumnValue > 1)
					{
						columnValues += L" (Multi-valued column)";
					}
				}

				if (0 != columnValues.length())
				{
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

LRESULT CDetailDialog::OnBnClickedButtonCopyall(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
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
