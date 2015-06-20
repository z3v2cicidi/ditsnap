#include "StdAfx.h"
#include "TableController.h"
#include "MainFrame.h"

CTableController::CTableController(ITableModel* tableModel)
	: tableModel_(tableModel)
{
}

CTableController::~CTableController(void)
{	
}

