#pragma once
#include "MVCInterfaces.h"

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
	DECLARE_WND_SUPERCLASS(NULL, CTreeViewCtrl::GetWndClassName())
	BOOL PreTranslateMessage(MSG* pMsg){ return FALSE;}
    BEGIN_MSG_MAP(CDbTreeView)
        //MSG_WM_CREATE(OnCreate)
        REFLECTED_NOTIFY_CODE_HANDLER_EX(NM_DBLCLK, OnTreeDblClick)
        DEFAULT_REFLECTION_HANDLER()
    END_MSG_MAP()

	CDbTreeView(ITableController* tableController, ITableModel* tableModel);
	~CDbTreeView(void);

	LRESULT OnTreeDblClick(LPNMHDR pnmh);

	//IDbObserver Interface
	void UpdateDb();

private:
	ITableController* tableController_;
	ITableModel* tableModel_;
};
