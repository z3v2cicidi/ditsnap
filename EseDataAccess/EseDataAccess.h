#pragma once
#include "stdafx.h"

namespace EseDataAccess
{
#define CHECK_JET_ERR(x) {         \
	JET_ERR ckerr = ((x));         \
	if (JET_errSuccess != ckerr)   \
		throw EseException(ckerr); \
}

	class EseInstance;
	class EseDatabase;
	class EseTable;
	class EseColumn;
	class EseException;

	class EseInstance
	{
	public:
		~EseInstance(void);
		static EseInstance* CreateInstance(uint pageSize = DEFAULT_ESE_PAGE_SIZE);
		EseDatabase* OpenDatabase(const wstring dbPath);

		JET_SESID GetSessionId() const
		{
			return sessionId_;
		}

		JET_INSTANCE GetJetInstance() const
		{
			return jetInstance_;
		}

	private:
		enum
		{
			DEFAULT_ESE_PAGE_SIZE = 8 * 1024
		};

		JET_INSTANCE jetInstance_;
		JET_SESID sessionId_;
		uint pageSize_;

		EseInstance(void);
		explicit EseInstance(uint pageSize);
		void Init();

		DISALLOW_COPY_AND_ASSIGN(EseInstance);
	};

	class EseDatabase
	{
	public:
		EseDatabase(const EseInstance* const parent, const string& dbPath);
		~EseDatabase(void);
		void Init();
		EseTable* OpenTable(const wstring tableName);
		vector<wstring> GetTableNames();
		uint GetTableCount();

		const EseInstance* GetParent() const
		{
			return parent_;
		};

		JET_DBID GetDbId() const
		{
			return dbId_;
		};

	private:
		const EseInstance* const parent_;
		JET_DBID dbId_;
		const string dbPath_;
		int tableCount_;

		DISALLOW_COPY_AND_ASSIGN(EseDatabase);
	};

	class EseTable
	{
	public:
		EseTable(const EseDatabase* const parent, const string& tableName);
		~EseTable();
		void Init();
		void MoveFirstRecord();
		BOOL MoveNextRecord();
		void Move(uint rowIndex);
		int CountColumnValue(uint columnIndex) const;
		wstring RetrieveColumnDataAsString(uint columnIndex, uint itagSequence = 1);
		uint GetColumnCount() const;
		wstring GetColumnName(uint columnIndex) const;

	private:
		const EseDatabase* const parent_;
		const JET_SESID sessionId_;
		const JET_DBID dbId_;
		JET_TABLEID tableId_;
		const string tableName_;
		vector<EseColumn*> columns_;
		EseColumn* RetrieveColumnDefinition(const JET_COLUMNLIST& columnList);
		void* RetrieveColumnData(uint columnIndex, uint itagSequence,
		                         uint* pDataSizeInByte);
		void AppendNullToEndOfData(void* pvData, uint dataSizeInByte);
		DISALLOW_COPY_AND_ASSIGN(EseTable);
	};

	class EseColumn
	{
	public:
		EseColumn(uint id, const string& name, uint type, bool isUnicode);

		~EseColumn()
		{
		};

		uint GetId() const
		{
			return id_;
		};

		string GetName() const
		{
			return name_;
		};

		uint GetType() const
		{
			return type_;
		};

		bool IsUnicode() const
		{
			return isUnicode_;
		};

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
		EseException() : err_(E_FAIL)
		{
		}

		explicit EseException(int err) : err_(err)
		{
		}

		int GetErrorCode() const
		{
			return err_;
		}

		wstring GetErrorMessage();

	private:
		int err_;
	};
} // name space EseDataAccess

