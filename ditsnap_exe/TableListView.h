#pragma once
#include "Interfaces.h"

class CDetailDialog;

class CTableListView : public CWindowImpl<CTableListView, CListViewCtrl>,
                       ITableObserver,
                       IDbObserver
{
public:
	enum
	{
		CLASSSCHEMA = 0x00000001,
		ATTRIBUTESCHEMA = 0x00000002,
		SUBSCHEMA = 0x00000004,
		DISPLAYSPECIFIER = 0x00000008,
		OTHERS = 0x00000010,
	};

	DECLARE_WND_SUPERCLASS(nullptr, CListViewCtrl::GetWndClassName())

	BEGIN_MSG_MAP_EX(CTableListView)
		MSG_WM_CREATE(OnCreate)
		REFLECTED_NOTIFY_CODE_HANDLER_EX(NM_DBLCLK, OnListDblClick)
		DEFAULT_REFLECTION_HANDLER()
	END_MSG_MAP()

	CTableListView(ITableController* tableController, ITableModel* tableModel);
	~CTableListView();

	int OnCreate(LPCREATESTRUCT lpCreateStruct);
	LRESULT OnListDblClick(LPNMHDR pnmh);
	void LoadTable();
	void LoadDatatable();
	void FilterTable(int filterFlag);
	const wstring GetAdNameFromColumnName(wstring columnName);
	int GetColumnIdFromColumnName(wstring columnName);
	virtual void UpdateTable() override;
	virtual void UpdateDb() override;

private:
	CDetailDialog* detailDialog_;
	map<wstring, int> columnMap_;
	map<wstring, wstring> adNameMap_;
	map<int, int> listItemIdToEseRowIndex_;
	ITableController* tableController_;
	ITableModel* tableModel_;

	void CleanupTable();
	void CleanupDetailDialog();
	void InsertColumnHelper(int nCol, wchar_t* ATT, int nWidth = 200);
	void AddItemHelper(int nItem, int nSubItem, wchar_t* ATT);
	bool MapColumnNameToColumnIndex(map<wstring, int>* columnMap);
	void MapColumnNameToAdName(map<wstring, wstring>* pAdNameMap);
};
