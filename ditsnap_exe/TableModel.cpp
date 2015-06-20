#include "StdAfx.h"
#include "TableModel.h"
#include "../EseDataAccess/EseDataAccess.h"

using namespace EseDataAccess;

CTableModel::CTableModel()
	: eseInstance_(nullptr),
	  eseDatabase_(nullptr),
	  eseTable_(nullptr)
{
}

CTableModel::~CTableModel()
{
	CleanupEse();
}

void CTableModel::RegisterTableObserver(ITableObserver* o)
{
	tableObservers_.push_back(o);
}

void CTableModel::RemoveTableObserver(ITableObserver* o)
{
	tableObservers_.remove(o);
}

void CTableModel::NotifyTableObservers()
{
	for (auto& o : tableObservers_)
	{
		o->LoadEseTable();
	}
}

void CTableModel::RegisterDbObserver(IDbObserver* o)
{
	tableNameObservers_.push_back(o);
}

void CTableModel::RemoveDbObserver(IDbObserver* o)
{
	tableNameObservers_.remove(o);
}

void CTableModel::NotifyDbObservers()
{
	for (auto& o : tableNameObservers_)
	{
		o->LoadEseDb();
	}
}

void CTableModel::OpenFile(wstring path)
{
	filePath_ = path;
	CleanupEse();
	eseInstance_ = EseInstance::CreateInstance();
	eseDatabase_ = eseInstance_->OpenDatabase(path);
	tableNames_ = eseDatabase_->GetTableNames();
	NotifyDbObservers();
}

wstring CTableModel::GetFilePath()
{
	return filePath_;
}

void CTableModel::SetTable(wstring name)
{
	eseTable_ = eseDatabase_->OpenTable(name);
	currentTableName_ = name;
	NotifyTableObservers();
}

wstring CTableModel::GetCurrentTableName()
{
	return currentTableName_;
}

vector<wstring> CTableModel::GetTableNames()
{
	return tableNames_;
}

void CTableModel::MoveFirstRecord()
{
	eseTable_->MoveFirstRecord();
	return;
}

BOOL CTableModel::MoveNextRecord()
{
	return eseTable_->MoveNextRecord();
}

void CTableModel::Move(uint rowIndex)
{
	return eseTable_->Move(rowIndex);
}

wstring CTableModel::RetrieveColumnDataAsString(uint columnIndex, uint itagSequence)
{
	return eseTable_->RetrieveColumnDataAsString(columnIndex, itagSequence);
}

uint CTableModel::GetColumnCount() const
{
	return eseTable_->GetColumnCount();
}

wstring CTableModel::GetColumnName(uint columnIndex) const
{
	return eseTable_->GetColumnName(columnIndex);
}

int CTableModel::CountColumnValue(uint columnIndex) const
{
	return eseTable_->CountColumnValue(columnIndex);
}

void CTableModel::CleanupEse()
{
	delete eseInstance_;
	eseInstance_ = nullptr;
	delete eseDatabase_;
	eseDatabase_ = nullptr;
	delete eseTable_;
	eseTable_ = nullptr;
}
