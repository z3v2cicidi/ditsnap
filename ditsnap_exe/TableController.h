#pragma once
#include "MVCInterfaces.h"
#include "TableListView.h"

class CTableController :public ITableController
{
public:
	CTableController(ITableModel* tableModel);
	~CTableController(void);

	// ITableController Interface
	void OpenTable(wstring name);
	void SetTable(wstring name);
	void ClearTable();
	CMainFrame* GetView();
	
private:
	CMainFrame* view_;
	ITableModel* tableModel_;
	CTableListView* tableListView_;

	DISALLOW_COPY_AND_ASSIGN(CTableController);
};
