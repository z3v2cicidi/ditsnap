#pragma once
#include "Interfaces.h"

// Defined in EseDataAccess.h 
namespace EseDataAccess
{
	class EseTable;
	class EseDatabase;
	class EseInstance;
}

class CDbTreeView : public CWindowImpl<CDbTreeView, CTreeViewCtrl>, IDbObserver
{
public:
	DECLARE_WND_SUPERCLASS(nullptr, CTreeViewCtrl::GetWndClassName())

	BEGIN_MSG_MAP_EX(CDbTreeView)
		REFLECTED_NOTIFY_CODE_HANDLER_EX(NM_DBLCLK, OnTreeDblClick)
		DEFAULT_REFLECTION_HANDLER()
	END_MSG_MAP()

	CDbTreeView(ITableController* tableController, ITableModel* tableModel);
	~CDbTreeView(void);

	LRESULT OnTreeDblClick(LPNMHDR pnmh);
	virtual void UpdateDb() override;

private:
	ITableController* tableController_;
	ITableModel* tableModel_;
};
