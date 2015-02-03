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

	virtual void RegisterTableObserver(ITableObserver* o) override;
	virtual void RemoveTableObserver(ITableObserver* o) override;
	virtual void NotifyTableObservers() override;
	virtual void RegisterDbObserver(IDbObserver* o) override;
	virtual void RemoveDbObserver(IDbObserver* o) override;
	virtual void NotifyDbObservers() override;
	virtual void OpenFile(wstring path) override;
	virtual wstring GetFilePath() override;
	virtual void SetTable(wstring name) override;
	virtual wstring GetCurrentTableName() override;
	virtual vector<wstring> GetTableNames() override;
	virtual void MoveFirstRecord() override;
	virtual BOOL MoveNextRecord() override;
	virtual void Move(uint rowIndex) override;
	virtual wstring RetrieveColumnDataAsString(uint columnIndex, uint itagSequence = 1) override;
	virtual uint GetColumnCount() const override;
	virtual wstring GetColumnName(uint columnIndex) const override;
	virtual int CountColumnValue(uint columnIndex) const override;

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
