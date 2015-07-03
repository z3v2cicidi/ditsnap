#include "stdafx.h"
#include "EseDataAccess.h"
#include "atlbase.h"

namespace EseDataAccess
{
	string GetJetErrorMessage(JET_ERR err)
	{
		char jetErrorMessage[512];
		jetErrorMessage[0] = 0;
		auto r = JetGetSystemParameter(NULL, JET_sesidNil,
			JET_paramErrorToString, reinterpret_cast<ULONG_PTR *>(&err), jetErrorMessage, sizeof(jetErrorMessage));
		if (r == JET_errSuccess)
		{
			return string(jetErrorMessage);
		}
		else
		{
			return string("Unknown Error.");
		}
	}

	EseInstance::EseInstance(void) : jetInstance_(NULL), sessionId_(NULL),
	                                 pageSize_(DEFAULT_ESE_PAGE_SIZE)
	{
	}

	EseInstance::EseInstance(uint pageSize) : jetInstance_(NULL),
	                                          sessionId_(NULL), pageSize_(pageSize)
	{
	}

	EseInstance::~EseInstance(void)
	{
		if (sessionId_ != NULL)
		{
			JetEndSession(sessionId_, 0);
		}

		if (jetInstance_ != NULL)
		{
			JetTerm(jetInstance_);
		}
	}

	EseInstance* EseInstance::CreateInstance(uint pageSize)
	{
		EseInstance* eseInstance = new EseInstance(pageSize);
		eseInstance->Init();
		return eseInstance;
	}

	void EseInstance::Init()
	{
		const char* instanceName = "ditsnap";

		ThrowOnError( ::JetSetSystemParameter(&jetInstance_, 0,
			JET_paramDatabasePageSize, pageSize_, NULL) );
		ThrowOnError( ::JetCreateInstance(&jetInstance_, instanceName) );
		ThrowOnError( ::JetInit(&jetInstance_) );
		ThrowOnError( ::JetBeginSession(jetInstance_, &sessionId_, NULL, NULL));
	}


	EseDatabase* EseInstance::OpenDatabase(const wstring dbPath)
	{
		EseDatabase* db = new EseDatabase(this, string(CW2A(dbPath.c_str())));
		db->Init();
		return db;
	}


	EseDatabase::EseDatabase(const EseInstance* const parent, const string& dbPath)
		: parent_(parent), dbId_(0), dbPath_(dbPath), tableCount_(-1)
	{
	}

	EseDatabase::~EseDatabase(void)
	{
		if (dbId_ != 0)
		{
			JetCloseDatabase(parent_->GetSessionId(), dbId_, 0);
		}
		JetDetachDatabase(parent_->GetSessionId(), dbPath_.c_str());
	}

	void EseDatabase::Init()
	{
		ThrowOnError( ::JetAttachDatabase(parent_->GetSessionId(),
			dbPath_.c_str(), JET_bitDbReadOnly) );
		ThrowOnError( ::JetOpenDatabase(parent_->GetSessionId(), dbPath_.c_str(), NULL,
			&dbId_, JET_bitDbReadOnly) );
		return;
	}

	EseTable* EseDatabase::OpenTable(const wstring tableName)
	{
		EseTable* table = new EseTable(this, string(CW2A(tableName.c_str())));
		table->Init();
		return table;
	}

	vector<wstring> EseDatabase::GetTableNames()
	{
		//Get a temporary table which contains all table names.
		JET_OBJECTLIST objectList = {0};
		ThrowOnError( ::JetGetObjectInfo(parent_->GetSessionId(), dbId_, JET_objtypTable,
			NULL, NULL, &objectList, sizeof(JET_OBJECTLIST), JET_ObjInfoList) );

		tableCount_ = objectList.cRecord;
		vector<wstring> tableNames(objectList.cRecord);

		for (uint i = 0; i < objectList.cRecord; ++i)
		{
			unsigned long actualSize = 0;
			JET_RETINFO retInfo = {0};
			retInfo.cbStruct = sizeof(JET_RETINFO);
			retInfo.itagSequence = 1;
			char tableName[JET_cbNameMost + 1];

			try
			{
				ThrowOnError( ::JetRetrieveColumn(parent_->GetSessionId(), objectList.tableid,
					objectList.columnidobjectname, tableName, JET_cbNameMost, &actualSize, 0, &retInfo) );
				tableName[actualSize] = NULL;

				wchar_t tableNameAsWideChar[JET_cbNameMost + 1];
				if (0 == MultiByteToWideChar(CP_ACP, 0, tableName, -1, tableNameAsWideChar, JET_cbNameMost + 1))
				{
					throw std::runtime_error(to_string(GetLastError()));
				}
				tableNames.at(i) = wstring(tableNameAsWideChar);

				if (objectList.cRecord - 1 != i)
				{
					ThrowOnError( ::JetMove(parent_->GetSessionId(), objectList.tableid, JET_MoveNext, 0) );
				}
			}
			catch (runtime_error&)
			{
				JetCloseTable(parent_->GetSessionId(), objectList.tableid);
				throw;
			}
		}
		JetCloseTable(parent_->GetSessionId(), objectList.tableid);

		return tableNames;
	}

	uint EseDatabase::GetTableCount()
	{
		if (-1 == tableCount_)
		{
			JET_OBJECTLIST objectList;
			ThrowOnError( ::JetGetObjectInfo(parent_->GetSessionId(), dbId_, JET_objtypTable,
				NULL, NULL, &objectList, sizeof(JET_OBJECTLIST), JET_ObjInfoList) );
			tableCount_ = objectList.cRecord;
			JetCloseTable(parent_->GetSessionId(), objectList.tableid);
		}

		return tableCount_;
	}

	EseTable::EseTable(const EseDatabase* const parent, const string& tableName)
		: parent_(parent), sessionId_(parent->GetParent()->GetSessionId()),
		  dbId_(parent->GetDbId()), tableId_(0), tableName_(tableName)
	{
	}

	EseTable::~EseTable(void)
	{
		for (size_t i = 0; i < columns_.size(); ++i)
		{
			delete columns_[i];
		}

		if (0 != tableId_)
		{
			JetCloseTable(sessionId_, tableId_);
		}
	}

	void EseTable::Init()
	{
		JET_COLUMNLIST columnList = {0};

		try
		{
			ThrowOnError( ::JetOpenTable(sessionId_, dbId_, tableName_.c_str(), NULL, 0,
				JET_bitTableReadOnly, &tableId_) );
			//This method opens a temporary table that contains column definitions
			ThrowOnError( ::JetGetTableColumnInfo(sessionId_, tableId_, NULL,
				&columnList, sizeof(JET_COLUMNLIST), JET_ColInfoList) );

			columns_.reserve(columnList.cRecord);

			ThrowOnError( ::JetMove(sessionId_, columnList.tableid, JET_MoveFirst, 0) );

			//Traverse the temporary table
			JET_ERR ret = 0;
			do
			{
				columns_.push_back(RetrieveColumnDefinition(columnList));
			}
			while (JET_errSuccess == (ret = JetMove(sessionId_,
			                                          columnList.tableid, JET_MoveNext, 0)));

			//if cursor don't reach to the end of records, throw exception
			if (ret != JET_errNoCurrentRecord)
			{
				throw runtime_error(GetJetErrorMessage(ret));
			}

			// close the temporary table 
			JetCloseTable(sessionId_, columnList.tableid);
		}
		catch (runtime_error&)
		{
			if (0 != columnList.tableid)
			{
				JetCloseTable(sessionId_, columnList.tableid);
			}
			throw;
		}
	}

	EseColumn* EseTable::RetrieveColumnDefinition(const JET_COLUMNLIST& columnList)
	{
		unsigned long actualSize = 0;
		JET_RETINFO retInfo = {0};
		retInfo.cbStruct = sizeof(JET_RETINFO);
		retInfo.itagSequence = 1;

		char columnName[JET_cbColumnMost + 1];
		JET_COLTYP colType = 0;
		JET_COLUMNID columnId = 0;
		unsigned short codePage = 0;
		bool isUnicode = true;

		//Get column name 
		ThrowOnError( ::JetRetrieveColumn(sessionId_, columnList.tableid,
			columnList.columnidBaseColumnName, &columnName,
			JET_cbColumnMost, &actualSize, 0, &retInfo) );

		columnName[actualSize] = NULL; //To be null-terminated string 

		//Get column type		
		ThrowOnError( ::JetRetrieveColumn(sessionId_, columnList.tableid,
			columnList.columnidcoltyp, &colType,
			JET_coltypLong, &actualSize, 0, &retInfo) );

		//Get column ID
		ThrowOnError( ::JetRetrieveColumn(sessionId_, columnList.tableid,
			columnList.columnidcolumnid, &columnId,
			JET_coltypLong, &actualSize, 0, &retInfo) );

		// Is Unicode?
		ThrowOnError( ::JetRetrieveColumn(sessionId_, columnList.tableid,
			columnList.columnidCp, &codePage,
			JET_coltypLong, &actualSize, 0, &retInfo) );
		if (1252 == codePage)
		{
			isUnicode = false;
		}

		return new EseColumn(columnId, columnName, colType, isUnicode);
	}

	void EseTable::MoveFirstRecord()
	{
		ThrowOnError( ::JetMove(sessionId_, tableId_, JET_MoveFirst, 0) );
		return;
	}

	BOOL EseTable::MoveNextRecord()
	{
		JET_ERR error = JetMove(sessionId_, tableId_, JET_MoveNext, 0);
		if (JET_errSuccess != error)
		{ //TODO: Handle other errors
			return FALSE;
		}
		return TRUE;
	}

	void EseTable::Move(uint rowIndex)
	{
		ThrowOnError( ::JetMove(sessionId_, tableId_, JET_MoveFirst, 0) );
		ThrowOnError( ::JetMove(sessionId_, tableId_, rowIndex, 0) );
		return;
	}

	void* EseTable::RetrieveColumnData(uint columnIndex, uint itagSequence, uint* pDataSizeInByte)
	{
		unsigned long actualSize = 0;
		JET_RETINFO retInfo = { 0 };
		retInfo.cbStruct = sizeof(JET_RETINFO);
		retInfo.itagSequence = itagSequence;

		JET_ERR error = ::JetRetrieveColumn(sessionId_, tableId_, columns_[columnIndex]->GetId(),
			NULL, 0, &actualSize, 0, &retInfo);
		if (NULL == actualSize)
		{
			*pDataSizeInByte = actualSize;
			return NULL;
		}

		//adding sizeof(wchar_t) for not-null-terminated string
		void* pvData = new BYTE[actualSize + sizeof(wchar_t)];
		error = ::JetRetrieveColumn(sessionId_, tableId_, columns_[columnIndex]->GetId(),
			pvData, actualSize + sizeof(wchar_t), 0, 0, &retInfo);
		if (JET_errSuccess != error)
		{
			delete[] pvData;
			throw runtime_error(GetJetErrorMessage(error));
		}

		*pDataSizeInByte = actualSize;
		return pvData;
	}

	int EseTable::CountColumnValue(uint columnIndex) const
	{
		JET_RETRIEVECOLUMN retrieveColumn = {0};
		retrieveColumn.columnid = columns_[columnIndex]->GetId();

		auto jeterr = JetRetrieveColumns(sessionId_, tableId_, &retrieveColumn, 1);

		if (JET_errSuccess != jeterr && JET_wrnBufferTruncated != jeterr)
		{
			throw runtime_error(GetJetErrorMessage(jeterr));
		}

		return retrieveColumn.itagSequence;
	}

	wstring EseTable::RetrieveColumnDataAsString(uint columnIndex, uint itagSequence)
	{
		const uint bufferSize = 1024 * 4;
		const uint maxBufferSize = 1024 * 16;
		wchar_t* returnString = new wchar_t[bufferSize / sizeof(wchar_t)];
		uint dataSize = 0;
		void* pvData = nullptr;
		int error = E_FAIL;

		try
		{
			pvData = RetrieveColumnData(columnIndex, itagSequence, &dataSize);

			if (NULL == pvData)
			{
				if (-1 == swprintf_s(returnString, bufferSize / sizeof(wchar_t), L""))
				{
					throw runtime_error(to_string(GetLastError()));
				}
				return returnString;
			}

			switch (columns_[columnIndex]->GetType())
			{
			case JET_coltypNil:
				error = swprintf_s(returnString, bufferSize / sizeof(wchar_t), L"");
				break;

			case JET_coltypBit: /* True or False, Never NULL */
				error = swprintf_s(returnString, bufferSize / sizeof(wchar_t), L"%d",
				                   *reinterpret_cast<int*>(pvData));
				break;

			case JET_coltypUnsignedByte: /* 1-byte integer, unsigned */
				error = swprintf_s(returnString, bufferSize / sizeof(wchar_t), L"%u",
				                   *reinterpret_cast<uint*>(pvData));
				break;

			case JET_coltypShort: /* 2-byte integer, signed */
				error = swprintf_s(returnString, bufferSize / sizeof(wchar_t), L"%d",
				                   *reinterpret_cast<int*>(pvData));
				break;

			case JET_coltypLong: /* 4-byte integer, signed */
				error = swprintf_s(returnString, bufferSize / sizeof(wchar_t), L"%ld",
				                   *reinterpret_cast<long*>(pvData));
				break;

			case JET_coltypCurrency: /* 8 byte integer, signed */
				error = swprintf_s(returnString, bufferSize / sizeof(wchar_t), L"%lld",
				                   *reinterpret_cast<long long int*>(pvData));
				break;

			case JET_coltypIEEESingle: /* 4-byte IEEE single precision */
				error = swprintf_s(returnString, bufferSize / sizeof(wchar_t), L"%f",
				                   *reinterpret_cast<float*>(pvData));
				break;

			case JET_coltypIEEEDouble: /* 8-byte IEEE double precision */
				error = swprintf_s(returnString, bufferSize / sizeof(wchar_t), L"%f",
				                   *reinterpret_cast<double*>(pvData));
				break;

			case JET_coltypDateTime:
				{/* This column type is identical to the variant date type.*/
					SYSTEMTIME systemTime = {0};
					VariantTimeToSystemTime(*reinterpret_cast<double*>(pvData), &systemTime);
					error = swprintf_s(returnString, bufferSize / sizeof(wchar_t), L"%u/%u/%u %u:%u %ums",
					                   systemTime.wMonth, systemTime.wDay, systemTime.wYear,
					                   systemTime.wHour, systemTime.wMinute, systemTime.wMilliseconds);
					break;
				}
			case JET_coltypBinary: /* Binary data, < 255 bytes */
				//Like 00 03 1C FF...
				for (size_t i = 0; i < dataSize; ++i)
				{
					error = swprintf_s(returnString + i * 3, 4, L"%02x ",
					                   *(reinterpret_cast<BYTE*>(pvData) + i));
					if (-1 == error)
					{
						throw runtime_error(to_string(GetLastError()));
					}
				}
				break;

			case JET_coltypText: /* ANSI text, case insensitive, < 255 bytes */
				AppendNullToEndOfData(pvData, dataSize);
				if (columns_[columnIndex]->IsUnicode())
				{
					error = swprintf_s(returnString, bufferSize / sizeof(wchar_t), L"%s",
					                   reinterpret_cast<wchar_t*>(pvData));
				}
				else
				{
					char tempString[bufferSize];
					error = sprintf_s(reinterpret_cast<char*>(tempString),
					                  bufferSize / sizeof(char), "%s", reinterpret_cast<char*>(pvData));
					if (-1 != error)
					{
						wcscpy_s(returnString, bufferSize / sizeof(wchar_t), CA2WEX<bufferSize>(tempString));
					}
				}
				break;

			case JET_coltypLongBinary: /* Binary data, long value */
				if (dataSize * sizeof(wchar_t) * 3 + 2 <= bufferSize)
				{
					for (size_t i = 0; i < dataSize; ++i)
					{
						error = swprintf_s(returnString + i * 3, 4, L"%02x ",
						                   *(reinterpret_cast<BYTE*>(pvData) + i));
						if (-1 == error)
						{
							throw runtime_error(to_string(GetLastError()));
						}
					}
				}
				else if (dataSize * sizeof(wchar_t) * 3 + 2 < maxBufferSize)
				{
					//realloc returnString to expand
					delete[] returnString;
					returnString = new wchar_t[dataSize * sizeof(wchar_t) * 3 + 2];
					for (size_t i = 0; i < dataSize; ++i)
					{
						error = swprintf_s(returnString + i * 3, 4, L"%02x ",
						                   *(reinterpret_cast<BYTE*>(pvData) + i));
						if (-1 == error)
						{
							throw runtime_error(to_string(GetLastError()));
						}
					}
				}
				else
				{
					error = swprintf_s(returnString, bufferSize / sizeof(wchar_t),
					                   L"Too big binary data to retrieve.");
				}
				break;

			case JET_coltypLongText: /* ANSI text, long value  */
				// TODO: Add if (isUnicode) sentence
				if (dataSize <= bufferSize - 2)
				{
					AppendNullToEndOfData(pvData, dataSize);
					error = swprintf_s(returnString, bufferSize / sizeof(wchar_t), L"%s",
					                   reinterpret_cast<wchar_t*>(pvData));
				}
				else if (dataSize < maxBufferSize - 2)
				{
					AppendNullToEndOfData(pvData, dataSize);
					//reallocate returnString to expand
					delete[] returnString;
					returnString = new wchar_t[dataSize / sizeof(wchar_t) + 2];
					error = swprintf_s(returnString, dataSize / sizeof(wchar_t) + 2, L"%s",
					                   reinterpret_cast<wchar_t*>(pvData));
				}
				else
				{
					error = swprintf_s(returnString, bufferSize / sizeof(wchar_t), L"%s",
					                   L"Too big binary data to retrieve.");
				}
				break;
			case JET_coltypUnsignedLong:
				error = swprintf_s(returnString, bufferSize / sizeof(wchar_t), L"%llu",
					*reinterpret_cast<unsigned long long int*>(pvData));
				break;
			case JET_coltypLongLong:
				error = swprintf_s(returnString, bufferSize / sizeof(wchar_t), L"%lld",
					*reinterpret_cast<long long int*>(pvData));
				break;
			case JET_coltypGUID:
				error = swprintf_s(returnString, bufferSize / sizeof(wchar_t), L"(GUID)");
				break;
			case JET_coltypUnsignedShort:
				error = swprintf_s(returnString, bufferSize / sizeof(wchar_t), L"%hu",
					*reinterpret_cast<long long int*>(pvData));
				break;
			default:
				error = swprintf_s(returnString, bufferSize / sizeof(wchar_t), L"unknown type");
				break;
			}

			if (-1 == error)
			{
				throw runtime_error(to_string(GetLastError()));
			}
		}
		catch (runtime_error&)
		{
			delete[] returnString;
			delete[] pvData;
			throw;
		}

		wstring returnStdString(returnString);
		delete[] returnString;
		delete[] pvData;
		return returnStdString;
	}

	uint EseTable::GetColumnCount() const
	{
		return static_cast<uint>(columns_.size());
	}

	wstring EseTable::GetColumnName(uint columnIndex) const
	{
		return wstring(CA2W(columns_.at(columnIndex)->GetName().c_str()));
	}

	void EseTable::AppendNullToEndOfData(void* pvData, uint dataSizeInByte)
	{
		if (*(static_cast<wchar_t*>(reinterpret_cast<wchar_t*>(pvData) + dataSizeInByte / sizeof(wchar_t))) != L'\0')
		{
			*(static_cast<wchar_t*>(reinterpret_cast<wchar_t*>(pvData) + dataSizeInByte / sizeof(wchar_t))) = L'\0';
		}
		return;
	}

	EseColumn::EseColumn(uint id, const string& name, uint type, bool isUnicode) :
		id_(id), name_(name), type_(type), isUnicode_(isUnicode)
	{
	}

	//wstring EseException::GetErrorMessage()
	//{
	//	wstring errorMessage;
	//	wchar_t* pTemp = NULL;

	//	int nLen = ::FormatMessage(
	//		FORMAT_MESSAGE_ALLOCATE_BUFFER |
	//		                              FORMAT_MESSAGE_IGNORE_INSERTS |
	//		                              FORMAT_MESSAGE_FROM_SYSTEM,
	//		                              NULL,
	//		                              err_,
	//		                              MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
	//		                              reinterpret_cast<LPWSTR>(&pTemp),
	//		                              1,
	//		                              NULL);

	//	if (0 != nLen && NULL != pTemp)
	//	{
	//		errorMessage = pTemp;
	//		LocalFree(pTemp);
	//	}
	//	else
	//	{
	//		char jetErrorMessage[512];
	//		jetErrorMessage[0] = 0;

	//		if (JET_errSuccess == JetGetSystemParameter(NULL, JET_sesidNil,
	//		                                                  JET_paramErrorToString, reinterpret_cast<ULONG_PTR *>(&err_), jetErrorMessage, sizeof( jetErrorMessage )))
	//		{
	//			errorMessage = CA2W(jetErrorMessage);
	//		}
	//		else
	//		{
	//			errorMessage = L"Unknown Error.";
	//		}
	//	}

	//	return errorMessage;
	//}



} // name space EseDataAccess