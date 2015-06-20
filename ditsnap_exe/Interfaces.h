#pragma once

class CMainFrame;

class ITableObserver
{
public:
	virtual ~ITableObserver()
	{
	};

	virtual void UpdateTable() = 0;
};

class IDbObserver
{
public:
	virtual ~IDbObserver()
	{
	};

	virtual void UpdateDb() = 0;
};

class ITableModel
{
public:
	virtual ~ITableModel()
	{
	};

	virtual void RegisterTableObserver(ITableObserver* o) = 0;
	virtual void RemoveTableObserver(ITableObserver* o) = 0;
	virtual void NotifyTableObservers() = 0;

	virtual void RegisterDbObserver(IDbObserver* o) = 0;
	virtual void RemoveDbObserver(IDbObserver* o) = 0;
	virtual void NotifyDbObservers() = 0;

	virtual void OpenFile(wstring path) = 0;
	virtual wstring GetFilePath() = 0;
	virtual void SetTable(wstring name) = 0;
	virtual wstring GetCurrentTableName() = 0;
	virtual vector<wstring> GetTableNames() = 0;
	virtual void MoveFirstRecord() = 0;
	virtual BOOL MoveNextRecord() = 0;
	virtual void Move(uint rowIndex) = 0;
	virtual wstring RetrieveColumnDataAsString(uint columnIndex, uint itagSequence = 1) = 0;
	virtual uint GetColumnCount() const = 0;
	virtual wstring GetColumnName(uint columnIndex) const = 0;
	virtual int CountColumnValue(uint columnIndex) const = 0;
};

class ITableController
{
public:
	virtual ~ITableController()
	{
	};

	virtual void OpenTable(wstring name) = 0;
	virtual void SetTable(wstring name) = 0;
	virtual void ClearTable() = 0;
	virtual CMainFrame* GetView() = 0;
};
