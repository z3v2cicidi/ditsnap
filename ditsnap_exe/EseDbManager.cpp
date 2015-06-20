#include "StdAfx.h"
#include "EseDbManager.h"
#include "../EseDataAccess/EseDataAccess.h"

using namespace EseDataAccess;

EseDbManager::EseDbManager()
	: eseInstance_(nullptr),
	  eseDatabase_(nullptr),
	  eseTable_(nullptr)
{
}

EseDbManager::~EseDbManager()
{
	CleanupEse();
}

void EseDbManager::RegisterTableObserver(ITableObserver* o)
{
	tableObservers_.push_back(o);
}

void EseDbManager::RemoveTableObserver(ITableObserver* o)
{
	tableObservers_.remove(o);
}

void EseDbManager::NotifyTableObservers()
{
	for (auto& o : tableObservers_)
	{
		o->LoadEseTable();
	}
}

void EseDbManager::RegisterDbObserver(IDbObserver* o)
{
	tableNameObservers_.push_back(o);
}

void EseDbManager::RemoveDbObserver(IDbObserver* o)
{
	tableNameObservers_.remove(o);
}

void EseDbManager::NotifyDbObservers()
{
	for (auto& o : tableNameObservers_)
	{
		o->LoadEseDbManager();
	}
}

void EseDbManager::OpenFile(wstring path)
{
	filePath_ = path;
	CleanupEse();
	eseInstance_ = EseInstance::CreateInstance();
	eseDatabase_ = eseInstance_->OpenDatabase(path);
	tableNames_ = eseDatabase_->GetTableNames();
	NotifyDbObservers();
}

wstring EseDbManager::GetFilePath()
{
	return filePath_;
}

void EseDbManager::SetTable(wstring name)
{
	eseTable_ = eseDatabase_->OpenTable(name);
	currentTableName_ = name;
	NotifyTableObservers();
}

wstring EseDbManager::GetCurrentTableName()
{
	return currentTableName_;
}

vector<wstring> EseDbManager::GetTableNames()
{
	return tableNames_;
}

void EseDbManager::MoveFirstRecord()
{
	eseTable_->MoveFirstRecord();
	return;
}

BOOL EseDbManager::MoveNextRecord()
{
	return eseTable_->MoveNextRecord();
}

void EseDbManager::Move(uint rowIndex)
{
	return eseTable_->Move(rowIndex);
}

wstring EseDbManager::RetrieveColumnDataAsString(uint columnIndex, uint itagSequence)
{
	return eseTable_->RetrieveColumnDataAsString(columnIndex, itagSequence);
}

uint EseDbManager::GetColumnCount() const
{
	return eseTable_->GetColumnCount();
}

wstring EseDbManager::GetColumnName(uint columnIndex) const
{
	return eseTable_->GetColumnName(columnIndex);
}

int EseDbManager::CountColumnValue(uint columnIndex) const
{
	return eseTable_->CountColumnValue(columnIndex);
}

void EseDbManager::CleanupEse()
{
	delete eseInstance_;
	eseInstance_ = nullptr;
	delete eseDatabase_;
	eseDatabase_ = nullptr;
	delete eseTable_;
	eseTable_ = nullptr;
}
