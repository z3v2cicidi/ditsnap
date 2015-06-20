#include "StdAfx.h"
#include "DbTreeView.h"
#include "../EseDataAccess/EseDataAccess.h"
#include "resource.h"

using namespace EseDataAccess;

CDbTreeView::CDbTreeView(EseDbManager* eseDbManager)
	: eseDbManager_(eseDbManager)
{
	eseDbManager_->RegisterDbObserver(this);
}

CDbTreeView::~CDbTreeView(void)
{
	eseDbManager_->RemoveDbObserver(this);
}

LRESULT CDbTreeView::OnTreeDoubleClick(LPNMHDR pnmh)
{
	UINT uFlag;
	CPoint pt = GetMessagePos();
	ScreenToClient(&pt);
	HTREEITEM hItem = HitTest(pt, &uFlag);
	if (hItem == nullptr || !(uFlag & TVHT_ONITEM))
	{
		return 0;
	}

	wchar_t tableName[1024];
	if (GetItemText(hItem, tableName, sizeof(tableName) / sizeof(tableName[0])))
	{
		try
		{
			eseDbManager_->SetTable(tableName);			
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

void CDbTreeView::LoadEseDbManager()
{
	DeleteAllItems();
	CImageList images;
	images.CreateFromImage(IDB_BITMAP1, 16, 0,
	                       RGB( 255, 0, 255 ), IMAGE_BITMAP, LR_CREATEDIBSECTION);
	this->SetImageList(images);

	HTREEITEM hRootItem = InsertItem(eseDbManager_->GetFilePath().c_str(), 0, 0, TVI_ROOT, TVI_LAST);
	if (hRootItem != nullptr)
	{
		SetItemData(hRootItem, reinterpret_cast<DWORD_PTR>(hRootItem));
	}
	try
	{
		vector<wstring> tableNames = eseDbManager_->GetTableNames();
		for (uint i = 0; i < tableNames.size(); ++i)
		{
			HTREEITEM hItem = InsertItem(tableNames[i].c_str(), 1, 1, hRootItem, TVI_LAST);
			if (hItem != nullptr)
			{
				SetItemData(hItem, reinterpret_cast<DWORD_PTR>(hItem));
				EnsureVisible(hItem);
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
