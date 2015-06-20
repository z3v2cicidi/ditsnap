#pragma once
#include "Interfaces.h"
#include "TableListView.h"

class CTableController :public ITableController
{
public:
	CTableController(ITableModel* tableModel);
	~CTableController(void);

	virtual void OpenTable(wstring name) override;
	virtual void SetTable(wstring name) override;
	virtual void ClearTable() override;
	virtual CMainFrame* GetView() override;

private:
	CMainFrame* view_;
	ITableModel* tableModel_;
	CTableListView* tableListView_;

	DISALLOW_COPY_AND_ASSIGN(CTableController);
};
