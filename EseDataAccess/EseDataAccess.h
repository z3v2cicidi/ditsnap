#pragma once
#include "stdafx.h"

namespace EseDataAccess 
{

#define CHECK_JET_ERR(x) {         \
	JET_ERR ckerr = ((x));         \
	if (JET_errSuccess != ckerr)   \
		throw EseException(ckerr); \
}

// Defined in this file.
class EseInstance;
class EseDatabase;
class EseTable;
class EseColumn;
class EseException;

//All member functions throw EseException. 
class EseInstance
{
public:
	~EseInstance(void);
	//This static method returns initialized EseInstance 
	//instance. It is the client's responsibility 
	//to delete the instance.
	static EseInstance* CreateInstance(uint pageSize = DEFAULT_ESE_PAGE_SIZE);

	//Returns initialized EseDatabase instance. It is the client's
	//responsibility to delete the instance. First parameter 
	//dbPath indicates ese database file, such as %systemroot%\NTDS\ntds.dit.
	EseDatabase* OpenDatabase(const wstring dbPath);

	JET_SESID GetSessionId() const { return sessionId_; }
	JET_INSTANCE GetJetInstance() const { return jetInstance_; }

private:
	enum { DEFAULT_ESE_PAGE_SIZE = 8 * 1024 };

	JET_INSTANCE jetInstance_;
	JET_SESID sessionId_;
	uint pageSize_;

	// private constructors
	EseInstance(void);
	explicit EseInstance(uint pageSize);
	//Must be initialized with a call to Init(). CreateInstance()
	//calls this method.
	void Init();

	DISALLOW_COPY_AND_ASSIGN(EseInstance);
};

//All member functions throw EseException. 
class EseDatabase
{
public:
	EseDatabase(const EseInstance* const parent, const string& dbPath);	
	~EseDatabase(void);

	//Must be initialized with a call to Init()
	void Init();

	//Returns an initialized EseTable instance. It is the client's
	//responsiblity to delete the instance.
	EseTable* OpenTable(const wstring tableName);

	// Returns names of all tables.
	vector<wstring> GetTableNames();

	//Get the number of tables in the database.
	uint GetTableCount();
 
	const EseInstance * const GetParent() const { return parent_; }; 
	JET_DBID GetDbId() const { return dbId_; };

private:
	const EseInstance * const parent_;
	JET_DBID dbId_;
	const string dbPath_;
	int tableCount_;

	DISALLOW_COPY_AND_ASSIGN(EseDatabase);
};

//All member functions throw EseException. 
class EseTable
{
public:
	EseTable(const EseDatabase* const parent, const string& tableName);
	~EseTable();

	// Must be initialized with a call to Init(). This function gathers
	// columns definitions of the table and save an member variable, columns_.
	void Init();

	// These functions operates the database cursor.
	void MoveFirstRecord();
	BOOL MoveNextRecord();
	void Move(uint rowIndex);

	// Get the number of instances of a multi-valued column.
	int CountColumnValue(uint columnIndex) const;

	// Returns data of an column wstring, or "" if unsuccessful or no data. 
	// The second parameter is the sequence number of the values
	// that are contained in a multi-valued column.
	wstring RetrieveColumnDataAsString(uint columnIndex, uint itagSequence = 1);

	// Returns the number of columns in the table.
	uint GetColumnCount() const;

	wstring GetColumnName(uint columnIndex) const;



private:
	const EseDatabase* const parent_; 
	const JET_SESID sessionId_;	
	const JET_DBID dbId_;
	JET_TABLEID tableId_;
	const string tableName_;
	// Contains column definitions of the table.
	vector<EseColumn*> columns_;

	EseColumn* RetrieveColumnDefinition(const JET_COLUMNLIST& columnList);
	// Returns data of an column as void pointer. It is the client's 
	// respoinsibility to delete the data. the data may be NULL if 
	// there are no data in the column. pDataSizeInByte is an output 
	// parameter.
	void* RetrieveColumnData(uint columnIndex, uint itagSequence,
						uint* pDataSizeInByte);
	//this function appends NULL charactor to end of void data, 
	//which contains a string. Jet API (JetRetrieveColumn) returns 
	//data as void pointer, so this function is needed.
	void AppendNullToEndOfData(void* pvData, uint dataSizeInByte);
	
	DISALLOW_COPY_AND_ASSIGN(EseTable);
};

class EseColumn
{
public:
	EseColumn(uint id, const string& name, uint type, bool isUnicode);
	~EseColumn(){};

	uint GetId() const { return id_; };
	const string GetName() const { return name_; };
	uint GetType() const { return type_; };
	bool IsUnicode() const { return isUnicode_; };

private:
	uint id_;
	const string name_;
	uint type_;
	bool isUnicode_;

	DISALLOW_COPY_AND_ASSIGN(EseColumn);
};

class EseException
{
public:
	EseException() : err_( E_FAIL ) {}
	EseException( int err ) : err_( err ) {}
	int GetErrorCode() const { return err_; }
	wstring GetErrorMessage();

private:
	int err_;
};

} // name space EseDataAccess