#pragma once
#include "MVCInterfaces.h"

// Defined in EseDataAccess.h 
namespace EseDataAccess
{
	class EseTable;
	class EseDatabase;
	class EseInstance;
}

class CTableModel :public ITableModel
{
public:
	CTableModel();
	~CTableModel();

	// ITableModel Interface
	void RegisterTableObserver(ITableObserver* o);
	void RemoveTableObserver(ITableObserver* o);
	void NotifyTableObservers();
	void RegisterDbObserver(IDbObserver* o);
	void RemoveDbObserver(IDbObserver* o);
	void NotifyDbObservers();
	void OpenFile(wstring path);
	wstring GetFilePath();
	void SetTable(wstring name);
	wstring GetCurrentTableName();
	vector<wstring> GetTableNames();
	void MoveFirstRecord();
	BOOL MoveNextRecord();
	void Move(uint rowIndex);
	wstring RetrieveColumnDataAsString(uint columnIndex, uint itagSequence = 1);
	uint GetColumnCount() const;
	wstring GetColumnName(uint columnIndex) const;
	int CountColumnValue(uint columnIndex) const;

private:
	list<ITableObserver*> tableObservers_;
	list<IDbObserver*> tableNameObservers_;
	EseDataAccess::EseInstance* eseInstance_;
	EseDataAccess::EseDatabase* eseDatabase_;
	EseDataAccess::EseTable* eseTable_;
	wstring filePath_;
	vector<wstring> tableNames_;
	wstring currentTableName_;

	void CleanupEse();

	DISALLOW_COPY_AND_ASSIGN(CTableModel);
};
