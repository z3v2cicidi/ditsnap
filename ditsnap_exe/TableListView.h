#pragma once
#include "resource.h"
#include "MVCInterfaces.h"

// Defined in DetailDialog.h
class CDetailDialog;

class CTableListView : public CWindowImpl<CTableListView, CListViewCtrl>, 
					   ITableObserver,
					   IDbObserver
{
public:
	// Use in FilterTable function.
	enum
	{
		CLASSSCHEMA      = 0x00000001,
		ATTRIBUTESCHEMA  = 0x00000002,
		SUBSCHEMA        = 0x00000004,
		DISPLAYSPECIFIER = 0x00000008,
		OTHERS           = 0x00000010,
	};

    DECLARE_WND_SUPERCLASS(NULL, CListViewCtrl::GetWndClassName())
    BOOL PreTranslateMessage(MSG* pMsg){ return FALSE; }

    BEGIN_MSG_MAP(CTableListView)
        MSG_WM_CREATE(OnCreate)
        REFLECTED_NOTIFY_CODE_HANDLER_EX(NM_DBLCLK, OnListDblClick)
        DEFAULT_REFLECTION_HANDLER()
	END_MSG_MAP()

	CTableListView(ITableController* tableController, ITableModel* tableModel);
	~CTableListView();

    int OnCreate(LPCREATESTRUCT lpCreateStruct);
	// If a list item is double clicked, open detail dialog.
    LRESULT OnListDblClick(LPNMHDR pnmh);

	void LoadTable();
	void LoadDatatable();
	void FilterTable(int filterFlag);
	const wstring GetAdNameFromColumnName(wstring columnName);
	int GetColumnIdFromColumnName(wstring columnName);
	// ITableObserver Interface
	void UpdateTable();
	// ITableListObserver Interface
	void UpdateDb();

private:
	CDetailDialog* detailDialog_;
	std::map<std::wstring, int> columnMap_;
	std::map<std::wstring, std::wstring> adNameMap_;
	std::map<int, int> listItemIdToEseRowIndex_;
	ITableController* tableController_;
	ITableModel* tableModel_;

	// Delete all rows and columns
	void CleanupTable();
	// Close and free the detail dialog.
	void CleanupDetailDialog();
	void InsertColumnHelper(int nCol, wchar_t* ATT, int nWidth = 200);
	void AddItemHelper(int nItem, int nSubItem, wchar_t* ATT);
	bool MapColumnNameToColumnIndex(map<wstring, int>* columnMap );
	void MapColumnNameToAdName(map<wstring, wstring>* pAdNameMap );
};
