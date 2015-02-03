#include "StdAfx.h"
#include "TableController.h"
#include "MainFrm.h"

CTableController::CTableController(ITableModel* tableModel)
	: tableModel_(tableModel)
{
	view_ = new CMainFrame(this, tableModel_);
}

CTableController::~CTableController(void)
{
	delete view_;
}

// TODO: Is the method name OpenFile better?
void CTableController::OpenTable(wstring name)
{
	tableModel_->OpenFile(name);
}

void CTableController::SetTable(wstring name)
{
	tableModel_->SetTable(name);
}

void CTableController::ClearTable()
{
}

CMainFrame* CTableController::GetView()
{
	return view_;
}
