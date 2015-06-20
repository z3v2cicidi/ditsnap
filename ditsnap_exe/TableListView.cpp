#include "stdafx.h"
#include "TableListView.h"
#include "DetailDialog.h"
#include "../EseDataAccess/EseDataAccess.h"

using namespace EseDataAccess;

CTableListView::CTableListView(EseDbManager* eseDbManager)
	: eseDbManager_(eseDbManager),
	  detailDialog_(nullptr)
{
	eseDbManager_->RegisterTableObserver(this);
	eseDbManager_->RegisterDbObserver(this);
}

CTableListView::~CTableListView()
{
	CleanupDetailDialog();
	eseDbManager_->RemoveTableObserver(this);
	eseDbManager_->RemoveDbObserver(this);
}

LRESULT CTableListView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	LRESULT lRet = DefWindowProc();
	SetExtendedListViewStyle(LVS_EX_INFOTIP | LVS_EX_FULLROWSELECT);
	return lRet;
}

LRESULT CTableListView::OnListDoubleClick(LPNMHDR pnmh)
{
	LPNMITEMACTIVATE pnmia = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);
	if (pnmia->iItem < 0)
	{
		return 0;
	}

	if (L"datatable" == eseDbManager_->GetCurrentTableName())
	{
		CleanupDetailDialog();
		detailDialog_ = new CDetailDialog(eseDbManager_, this,
		                                  listItemIdToEseRowIndex_[pnmia->iItem]);
		detailDialog_->Create(0);
		detailDialog_->ShowWindow(SW_SHOW);
	}
	return 0;
}

void CTableListView::LoadTable()
{
	const int nWidth = 70;
	try
	{
		for (uint columnIndex = 0; columnIndex < eseDbManager_->GetColumnCount(); ++columnIndex)
		{
			wstring columnName = eseDbManager_->GetColumnName(columnIndex);
			InsertColumn(columnIndex, columnName.c_str(), LVCFMT_LEFT, nWidth);
		}

		int rowIndex = 0;
		eseDbManager_->MoveFirstRecord();
		do
		{
			for (uint columnIndex = 0; columnIndex < eseDbManager_->GetColumnCount(); ++columnIndex)
			{
				// Support to display multi-valued column data.
				wstring columnValues;
				int numberOfColumnValue = eseDbManager_->CountColumnValue(columnIndex);
				for (int itagSequence = 1; itagSequence <= numberOfColumnValue; ++itagSequence)
				{
					wstring columnValue = eseDbManager_->RetrieveColumnDataAsString(columnIndex, itagSequence);
					columnValues += columnValue;
					if (numberOfColumnValue != itagSequence)
					{
						columnValues += L"; ";
					}
					else if (numberOfColumnValue > 1)
					{
						columnValues += L" (Multi-valued column)";
					}
				}
				if (columnValues.empty())
				{
					columnValues = L"<Not Set>";
				}
				AddItem(rowIndex, columnIndex, columnValues.c_str());
			}
			++rowIndex;
		}
		while (eseDbManager_->MoveNextRecord());
	}
	catch (EseException& e)
	{
		if (-1603 != e.GetErrorCode())
		{
			// Handle errors other than -1603 (no record warning)
			CString errorMessage;
			errorMessage.Format(L"Error Code : %d\n%s", e.GetErrorCode(), e.GetErrorMessage().c_str());
			MessageBox(errorMessage, L"Ditsnap", MB_ICONWARNING | MB_OK);
		}
	}
	return;
}

void CTableListView::LoadDatatable()
{
	columnMap_.clear();
	adNameMap_.clear();
	listItemIdToEseRowIndex_.clear();
	MapColumnNameToColumnIndex(&columnMap_);
	MapColumnNameToAdName(&adNameMap_);
	wchar_t* listHeaderColumnNames[] = {L"ATTm589825", L"DNT_col", L"PDNT_col", L"cnt_col",
		L"OBJ_col", L"RDNtyp_col", L"NCDNT_col", L"ATTb590606"};
	try
	{
		for (int i = 0; i < _countof(listHeaderColumnNames); ++i)
		{
			if (0 == i)
			{
				InsertColumnHelper(i, listHeaderColumnNames[i]);
			}
			else if (7 == i)
			{
				InsertColumnHelper(i, listHeaderColumnNames[i], 170);
			}
			else
			{
				InsertColumnHelper(i, listHeaderColumnNames[i], 70);
			}
		}

		// rowIndex is the index in list view. eseRowIndex is the index in EseTable.
		// these index is corresponded by listItemIdToEseRowIndex_,
		// and this map is used in filterTable function.
		int rowIndex = 0;
		int eseRowIndex = 0;
		eseDbManager_->MoveFirstRecord();
		do
		{
			for (int i = 0; i < _countof(listHeaderColumnNames); ++i)
			{
				AddItemHelper(rowIndex, i, listHeaderColumnNames[i]);
			}
			listItemIdToEseRowIndex_.insert(pair<int, int>(rowIndex, eseRowIndex));
			++eseRowIndex;
			++rowIndex;
		}
		while (eseDbManager_->MoveNextRecord());
	}
	catch (EseException& e)
	{
		CString errorMessage;
		errorMessage.Format(L"Error Code : %d\n%s", e.GetErrorCode(), e.GetErrorMessage().c_str());
		MessageBox(errorMessage, L"Ditsnap", MB_ICONWARNING | MB_OK);
	}
	return;
}

void CTableListView::FilterTable(int filterFlag)
{
	CleanupDetailDialog();
	if (L"datatable" != eseDbManager_->GetCurrentTableName())
	{
		return;
	}
	DeleteAllItems();
	listItemIdToEseRowIndex_.clear();
	wchar_t* listHeaderColumnNames[] = {L"ATTm589825", L"DNT_col", L"PDNT_col", L"cnt_col",
		L"OBJ_col", L"RDNtyp_col", L"NCDNT_col", L"ATTb590606"};
	wstring classSchemaDnt;
	wstring attributeSchemaDnt;
	wstring subSchemaDnt;
	wstring displaySpecifierDnt;
	try
	{
		eseDbManager_->MoveFirstRecord();
		do
		{
			wstring objectName = eseDbManager_->RetrieveColumnDataAsString(columnMap_[L"ATTm589825"]);
			if (classSchemaDnt.empty() && L"Class-Schema" == objectName)
			{
				classSchemaDnt = eseDbManager_->RetrieveColumnDataAsString(columnMap_[L"DNT_col"]);
			}
			if (attributeSchemaDnt.empty() && L"Attribute-Schema" == objectName)
			{
				attributeSchemaDnt = eseDbManager_->RetrieveColumnDataAsString(columnMap_[L"DNT_col"]);
			}
			if (subSchemaDnt.empty() && L"SubSchema" == objectName)
			{
				subSchemaDnt = eseDbManager_->RetrieveColumnDataAsString(columnMap_[L"DNT_col"]);
			}
			if (displaySpecifierDnt.empty() && L"Display-Specifier" == objectName)
			{
				displaySpecifierDnt = eseDbManager_->RetrieveColumnDataAsString(columnMap_[L"DNT_col"]);
			}
		}
		while (eseDbManager_->MoveNextRecord());
	}
	catch (EseException& e)
	{
		CString errorMessage;
		errorMessage.Format(L"Error Code : %d\n%s", e.GetErrorCode(), e.GetErrorMessage().c_str());
		MessageBox(errorMessage, L"Ditsnap", MB_ICONWARNING | MB_OK);
		return;
	}

	try
	{
		int eseRowIndex = -1;
		int rowIndex = 0;
		eseDbManager_->MoveFirstRecord();
		do
		{
			++eseRowIndex;
			wstring objectCategory = eseDbManager_->RetrieveColumnDataAsString(columnMap_[L"ATTb590606"]);
			if (!(filterFlag & CLASSSCHEMA) && classSchemaDnt == objectCategory) continue;
			if (!(filterFlag & ATTRIBUTESCHEMA) && attributeSchemaDnt == objectCategory) continue;
			if (!(filterFlag & SUBSCHEMA) && subSchemaDnt == objectCategory) continue;
			if (!(filterFlag & DISPLAYSPECIFIER) && displaySpecifierDnt == objectCategory) continue;
			if (!(filterFlag & OTHERS)
				&& classSchemaDnt != objectCategory
				&& attributeSchemaDnt != objectCategory
				&& subSchemaDnt != objectCategory
				&& displaySpecifierDnt != objectCategory
			)
				continue;

			for (int i = 0; i < _countof(listHeaderColumnNames); ++i)
			{
				wstring s(eseDbManager_->RetrieveColumnDataAsString(columnMap_[listHeaderColumnNames[i]]));
				AddItem(rowIndex, i, s.c_str());
			}
			listItemIdToEseRowIndex_.insert(pair<int, int>(rowIndex, eseRowIndex));
			++rowIndex;
		}
		while (eseDbManager_->MoveNextRecord());
	}
	catch (EseException& e)
	{
		CString errorMessage;
		errorMessage.Format(L"Error Code : %d\n%s", e.GetErrorCode(), e.GetErrorMessage().c_str());
		MessageBox(errorMessage, L"Ditsnap", MB_ICONWARNING | MB_OK);
		return;
	}
}

const wstring CTableListView::GetAdNameFromColumnName(wstring columnName)
{
	return adNameMap_[wstring(columnName)];
}

int CTableListView::GetColumnIdFromColumnName(wstring columnName)
{
	return columnMap_[wstring(columnName)];
}

void CTableListView::LoadEseTable()
{
	CleanupTable();
	CleanupDetailDialog();

	if (L"datatable" == eseDbManager_->GetCurrentTableName())
	{
		LoadDatatable();
	}
	else
	{
		LoadTable();
	}
}

void CTableListView::LoadEseDbManager()
{
	CleanupTable();
	CleanupDetailDialog();
}

void CTableListView::CleanupTable()
{
	DeleteAllItems();
	int columnCount = GetHeader().GetItemCount();
	for (int i = 0; i < columnCount; ++i)
	{
		DeleteColumn(0);
	}
}

void CTableListView::CleanupDetailDialog()
{
	if (nullptr != detailDialog_ && detailDialog_->IsWindow())
	{
		detailDialog_->DestroyWindow();
	}
	delete detailDialog_;
	detailDialog_ = nullptr;
	return;
}

void CTableListView::InsertColumnHelper(int nCol, wchar_t* columnNameLikeATTxxxx, int nWidth)
{
	CString columnName;
	if (adNameMap_[columnNameLikeATTxxxx].empty())
	{
		columnName.Format(L"%s", columnNameLikeATTxxxx);
	}
	else
	{
		columnName.Format(L"%s <%s>", columnNameLikeATTxxxx, adNameMap_[columnNameLikeATTxxxx].c_str());
	}
	InsertColumn(nCol, columnName, LVCFMT_LEFT, nWidth);
	return;
}

void CTableListView::AddItemHelper(int nItem, int nSubItem, wchar_t* columnNameLikeATTxxxx)
{
	wstring s(eseDbManager_->RetrieveColumnDataAsString(columnMap_[columnNameLikeATTxxxx]));
	AddItem(nItem, nSubItem, s.c_str());
	return;
}

bool CTableListView::MapColumnNameToColumnIndex(map<wstring, int>* pColumnMap)
{
	try
	{
		for (uint columnIndex = 0; columnIndex < eseDbManager_->GetColumnCount(); ++columnIndex)
		{
			wstring columnName(eseDbManager_->GetColumnName(columnIndex));
			pColumnMap->insert(pair<wstring, int>(wstring(columnName), columnIndex));
		}
	}
	catch (EseException& e)
	{
		CString errorMessage;
		errorMessage.Format(L"Error Code : %d\n%s", e.GetErrorCode(), e.GetErrorMessage().c_str());
		MessageBox(errorMessage, L"Ditsnap", MB_ICONWARNING | MB_OK);
		return FALSE;
	}
	return TRUE;
}

void CTableListView::MapColumnNameToAdName(map<wstring, wstring>* pAdNameMap)
{
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589983", L"ATT_ACCOUNT_EXPIRES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591131", L"ATT_ACCOUNT_NAME_HISTORY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590584", L"ATT_ACS_AGGREGATE_TOKEN_RATE_PER_USER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590590", L"ATT_ACS_ALLOCABLE_RSVP_BANDWIDTH"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590603", L"ATT_ACS_CACHE_TIMEOUT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590581", L"ATT_ACS_DIRECTION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590602", L"ATT_ACS_DSBM_DEADTIME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590600", L"ATT_ACS_DSBM_PRIORITY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590601", L"ATT_ACS_DSBM_REFRESH"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590594", L"ATT_ACS_ENABLE_ACS_SERVICE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590723", L"ATT_ACS_ENABLE_RSVP_ACCOUNTING"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590592", L"ATT_ACS_ENABLE_RSVP_MESSAGE_LOGGING"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590593", L"ATT_ACS_EVENT_LOG_LEVEL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590608", L"ATT_ACS_IDENTITY_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590721", L"ATT_ACS_MAX_AGGREGATE_PEAK_RATE_PER_USER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590585", L"ATT_ACS_MAX_DURATION_PER_FLOW"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590725", L"ATT_ACS_MAX_NO_OF_ACCOUNT_FILES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590598", L"ATT_ACS_MAX_NO_OF_LOG_FILES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590591", L"ATT_ACS_MAX_PEAK_BANDWIDTH"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590583", L"ATT_ACS_MAX_PEAK_BANDWIDTH_PER_FLOW"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590726", L"ATT_ACS_MAX_SIZE_OF_RSVP_ACCOUNT_FILE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590599", L"ATT_ACS_MAX_SIZE_OF_RSVP_LOG_FILE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq591137", L"ATT_ACS_MAX_TOKEN_BUCKET_PER_FLOW"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590582", L"ATT_ACS_MAX_TOKEN_RATE_PER_FLOW"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq591138", L"ATT_ACS_MAXIMUM_SDU_SIZE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq591141", L"ATT_ACS_MINIMUM_DELAY_VARIATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq591140", L"ATT_ACS_MINIMUM_LATENCY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq591139", L"ATT_ACS_MINIMUM_POLICED_SIZE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq591144", L"ATT_ACS_NON_RESERVED_MAX_SDU_SIZE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq591145", L"ATT_ACS_NON_RESERVED_MIN_POLICED_SIZE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq591142", L"ATT_ACS_NON_RESERVED_PEAK_RATE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq591143", L"ATT_ACS_NON_RESERVED_TOKEN_SIZE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590604", L"ATT_ACS_NON_RESERVED_TX_LIMIT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590722", L"ATT_ACS_NON_RESERVED_TX_SIZE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590589", L"ATT_ACS_PERMISSION_BITS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590596", L"ATT_ACS_POLICY_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590588", L"ATT_ACS_PRIORITY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590724", L"ATT_ACS_RSVP_ACCOUNT_FILES_LOCATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590597", L"ATT_ACS_RSVP_LOG_FILES_LOCATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590586", L"ATT_ACS_SERVICE_TYPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590580", L"ATT_ACS_TIME_OF_DAY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590587", L"ATT_ACS_TOTAL_NO_OF_FLOWS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591136", L"ATT_ACS_SERVER_LIST"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590089", L"ATT_ADDITIONAL_INFORMATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590713", L"ATT_ADDITIONAL_TRUSTED_SERVICE_NAMES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131328", L"ATT_ADDRESS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591068", L"ATT_ADDRESS_BOOK_ROOTS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk131396", L"ATT_ADDRESS_ENTRY_DISPLAY_TABLE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk131472", L"ATT_ADDRESS_ENTRY_DISPLAY_TABLE_MSDOS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131689", L"ATT_ADDRESS_HOME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk131327", L"ATT_ADDRESS_SYNTAX"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTe131422", L"ATT_ADDRESS_TYPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590438", L"ATT_ADMIN_CONTEXT_MENU"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589974", L"ATT_ADMIN_COUNT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131298", L"ATT_ADMIN_DESCRIPTION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131266", L"ATT_ADMIN_DISPLAY_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591514", L"ATT_ADMIN_MULTISELECT_PROPERTY_PAGES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590386", L"ATT_ADMIN_PROPERTY_PAGES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTc590737", L"ATT_ALLOWED_ATTRIBUTES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTc590738", L"ATT_ALLOWED_ATTRIBUTES_EFFECTIVE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTc590735", L"ATT_ALLOWED_CHILD_CLASSES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTc590736", L"ATT_ALLOWED_CHILD_CLASSES_EFFECTIVE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590691", L"ATT_ALT_SECURITY_IDENTITIES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591032", L"ATT_ANR"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590672", L"ATT_APP_SCHEMA_VERSION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590042", L"ATT_APPLICATION_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590165", L"ATT_APPLIES_TO"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590107", L"ATT_ASSET_NUMBER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590476", L"ATT_ASSISTANT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk591037", L"ATT_ASSOC_NT_ACCOUNT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTf1376293", L"ATT_ASSOCIATEDDOMAIN"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb1376294", L"ATT_ASSOCIATEDNAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk58", L"ATT_ATTRIBUTECERTIFICATEATTRIBUTE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590572", L"ATT_ATTRIBUTE_DISPLAY_NAMES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTc131102", L"ATT_ATTRIBUTE_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589973", L"ATT_ATTRIBUTE_SECURITY_GUID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTc131104", L"ATT_ATTRIBUTE_SYNTAX"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1572869", L"ATT_ATTRIBUTE_TYPES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk1376311", L"ATT_AUDIO"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590026", L"ATT_AUDITING_POLICY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589835", L"ATT_AUTHENTICATION_OPTIONS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk38", L"ATT_AUTHORITY_REVOCATION_LIST"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTc131423", L"ATT_AUXILIARY_CLASS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589873", L"ATT_BAD_PASSWORD_TIME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589836", L"ATT_BAD_PWD_COUNT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590156", L"ATT_BIRTH_LOCATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590644", L"ATT_BRIDGEHEAD_SERVER_LIST_BL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590643", L"ATT_BRIDGEHEAD_TRANSPORT_LIST"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1376304", L"ATT_BUILDINGNAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589837", L"ATT_BUILTIN_CREATION_TIME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589838", L"ATT_BUILTIN_MODIFIED_COUNT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm15", L"ATT_BUSINESS_CATEGORY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590108", L"ATT_BYTES_PER_MINUTE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk37", L"ATT_CA_CERTIFICATE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590521", L"ATT_CA_CERTIFICATE_DN"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590511", L"ATT_CA_CONNECT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590514", L"ATT_CA_USAGES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590512", L"ATT_CA_WEB_URL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590639", L"ATT_CAN_UPGRADE_SCRIPT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590740", L"ATT_CANONICAL_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1441793", L"ATT_CARLICENSE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590499", L"ATT_CATALOGS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590496", L"ATT_CATEGORIES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590146", L"ATT_CATEGORY_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590508", L"ATT_CERTIFICATE_AUTHORITY_OBJECT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk39", L"ATT_CERTIFICATE_REVOCATION_LIST"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590647", L"ATT_CERTIFICATE_TEMPLATES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590434", L"ATT_CLASS_DISPLAY_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589840", L"ATT_CODE_PAGE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589843", L"ATT_COM_CLASSID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590073", L"ATT_COM_CLSID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589844", L"ATT_COM_INTERFACEID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590077", L"ATT_COM_OTHER_PROG_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589845", L"ATT_COM_PROGID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590075", L"ATT_COM_TREAT_AS_CLASS_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590078", L"ATT_COM_TYPELIB_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590074", L"ATT_COM_UNIQUE_LIBID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131153", L"ATT_COMMENT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm3", L"ATT_COMMON_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131218", L"ATT_COMPANY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi589848", L"ATT_CONTENT_INDEXING_ALLOWED"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590323", L"ATT_CONTEXT_MENU"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590024", L"ATT_CONTROL_ACCESS_RIGHTS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj131207", L"ATT_COST"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589849", L"ATT_COUNTRY_CODE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm6", L"ATT_COUNTRY_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590634", L"ATT_CREATE_DIALOG"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTl1638401", L"ATT_CREATE_TIME_STAMP"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590636", L"ATT_CREATE_WIZARD_EXT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589850", L"ATT_CREATION_TIME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590322", L"ATT_CREATION_WIZARD"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590503", L"ATT_CREATOR"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590513", L"ATT_CRL_OBJECT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590507", L"ATT_CRL_PARTITIONED_REVOCATION_LIST"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk40", L"ATT_CROSS_CERTIFICATE_PAIR"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590161", L"ATT_CURR_MACHINE_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590159", L"ATT_CURRENT_LOCATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590520", L"ATT_CURRENT_PARENT_CA"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589851", L"ATT_CURRENT_VALUE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589879", L"ATT_DBCS_PWD"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590037", L"ATT_DEFAULT_CLASS_STORE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590304", L"ATT_DEFAULT_GROUP"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590342", L"ATT_DEFAULT_HIDING_VALUE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb589881", L"ATT_DEFAULT_LOCAL_POLICY_OBJECT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590607", L"ATT_DEFAULT_OBJECT_CATEGORY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590056", L"ATT_DEFAULT_PRIORITY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590048", L"ATT_DEFAULT_SECURITY_DESCRIPTOR"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk53", L"ATT_DELTA_REVOCATION_LIST"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131213", L"ATT_DEPARTMENT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1441794", L"ATT_DEPARTMENTNUMBER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm13", L"ATT_DESCRIPTION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590170", L"ATT_DESKTOP_PROFILE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTf27", L"ATT_DESTINATION_INDICATOR"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590539", L"ATT_DHCP_CLASSES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590524", L"ATT_DHCP_FLAGS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590525", L"ATT_DHCP_IDENTIFICATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTf590530", L"ATT_DHCP_MASK"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590543", L"ATT_DHCP_MAXKEY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590527", L"ATT_DHCP_OBJ_DESCRIPTION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590526", L"ATT_DHCP_OBJ_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590538", L"ATT_DHCP_OPTIONS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590542", L"ATT_DHCP_PROPERTIES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTf590531", L"ATT_DHCP_RANGES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTf590533", L"ATT_DHCP_RESERVATIONS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTf590528", L"ATT_DHCP_SERVERS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTf590532", L"ATT_DHCP_SITES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTf590541", L"ATT_DHCP_STATE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTf590529", L"ATT_DHCP_SUBNETS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590523", L"ATT_DHCP_TYPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590522", L"ATT_DHCP_UNIQUE_KEY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590544", L"ATT_DHCP_UPDATE_TIME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131085", L"ATT_DISPLAY_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTf131425", L"ATT_DISPLAY_NAME_PRINTABLE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1572866", L"ATT_DIT_CONTENT_RULES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590085", L"ATT_DIVISION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb131108", L"ATT_DMD_LOCATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131670", L"ATT_DMD_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591066", L"ATT_DN_REFERENCE_UPDATE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590202", L"ATT_DNS_ALLOW_DYNAMIC"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590203", L"ATT_DNS_ALLOW_XFR"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590443", L"ATT_DNS_HOST_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590205", L"ATT_DNS_NOTIFY_SECONDARIES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk591130", L"ATT_DNS_PROPERTY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590206", L"ATT_DNS_RECORD"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589852", L"ATT_DNS_ROOT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590204", L"ATT_DNS_SECURE_SECONDARIES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi591238", L"ATT_DNS_TOMBSTONED"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb1376270", L"ATT_DOCUMENTAUTHOR"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1376267", L"ATT_DOCUMENTIDENTIFIER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1376271", L"ATT_DOCUMENTLOCATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1376312", L"ATT_DOCUMENTPUBLISHER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1376268", L"ATT_DOCUMENTTITLE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1376269", L"ATT_DOCUMENTVERSION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590492", L"ATT_DOMAIN_CERTIFICATE_AUTHORITIES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1376281", L"ATT_DOMAIN_COMPONENT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590296", L"ATT_DOMAIN_CROSS_REF"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590510", L"ATT_DOMAIN_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590579", L"ATT_DOMAIN_IDENTIFIER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb589856", L"ATT_DOMAIN_POLICY_OBJECT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590246", L"ATT_DOMAIN_POLICY_REFERENCE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589982", L"ATT_DOMAIN_REPLICA"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590245", L"ATT_DOMAIN_WIDE_POLICY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1376261", L"ATT_DRINK"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590053", L"ATT_DRIVER_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590100", L"ATT_DRIVER_VERSION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTl591181", L"ATT_DS_CORE_PROPAGATION_DATA"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131284", L"ATT_DS_HEURISTICS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591168", L"ATT_DS_UI_ADMIN_MAXIMUM"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591167", L"ATT_DS_UI_ADMIN_NOTIFICATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591169", L"ATT_DS_UI_SHELL_MAXIMUM"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk131146", L"ATT_DSA_SIGNATURE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590361", L"ATT_DYNAMIC_LDAP_SERVER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1376259", L"ATT_E_MAIL_ADDRESSES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590092", L"ATT_EFSPOLICY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589859", L"ATT_EMPLOYEE_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131682", L"ATT_EMPLOYEE_NUMBER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131685", L"ATT_EMPLOYEE_TYPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi131629", L"ATT_ENABLED"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi589860", L"ATT_ENABLED_CONNECTION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590649", L"ATT_ENROLLMENT_PROVIDERS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590733", L"ATT_EXTENDED_ATTRIBUTE_INFO"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi131452", L"ATT_EXTENDED_CHARS_ALLOWED"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590732", L"ATT_EXTENDED_CLASS_INFO"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131299", L"ATT_EXTENSION_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591511", L"ATT_EXTRA_COLUMNS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm23", L"ATT_FACSIMILE_TELEPHONE_NUMBER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590640", L"ATT_FILE_EXT_PRIORITY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589862", L"ATT_FLAGS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590335", L"ATT_FLAT_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589863", L"ATT_FORCE_LOGOFF"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590180", L"ATT_FOREIGN_IDENTIFIER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590506", L"ATT_FRIENDLY_NAMES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590734", L"ATT_FROM_ENTRY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb589864", L"ATT_FROM_SERVER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590693", L"ATT_FRS_COMPUTER_REFERENCE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590694", L"ATT_FRS_COMPUTER_REFERENCE_BL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590695", L"ATT_FRS_CONTROL_DATA_CREATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590696", L"ATT_FRS_CONTROL_INBOUND_BACKLOG"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590697", L"ATT_FRS_CONTROL_OUTBOUND_BACKLOG"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590308", L"ATT_FRS_DIRECTORY_FILTER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590314", L"ATT_FRS_DS_POLL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590360", L"ATT_FRS_EXTENSIONS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590315", L"ATT_FRS_FAULT_CONDITION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590307", L"ATT_FRS_FILE_FILTER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590698", L"ATT_FRS_FLAGS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590358", L"ATT_FRS_LEVEL_LIMIT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590699", L"ATT_FRS_MEMBER_REFERENCE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590700", L"ATT_FRS_MEMBER_REFERENCE_BL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590701", L"ATT_FRS_PARTNER_AUTH_LEVEL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590702", L"ATT_FRS_PRIMARY_MEMBER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590357", L"ATT_FRS_REPLICA_SET_GUID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589855", L"ATT_FRS_REPLICA_SET_TYPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590311", L"ATT_FRS_ROOT_PATH"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTp590359", L"ATT_FRS_ROOT_SECURITY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590324", L"ATT_FRS_SERVICE_COMMAND"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590703", L"ATT_FRS_SERVICE_COMMAND_STATUS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590312", L"ATT_FRS_STAGING_PATH"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTl590704", L"ATT_FRS_TIME_LAST_COMMAND"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTl590705", L"ATT_FRS_TIME_LAST_CONFIG_CHANGE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590309", L"ATT_FRS_UPDATE_TIMEOUT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590706", L"ATT_FRS_VERSION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589867", L"ATT_FRS_VERSION_GUID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590310", L"ATT_FRS_WORKING_PATH"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590193", L"ATT_FSMO_ROLE_OWNER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj131373", L"ATT_GARBAGE_COLL_PERIOD"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi589865", L"ATT_GENERATED_CONNECTION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm44", L"ATT_GENERATION_QUALIFIER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm42", L"ATT_GIVEN_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591069", L"ATT_GLOBAL_ADDRESS_LIST"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTc131094", L"ATT_GOVERNS_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590715", L"ATT_GP_LINK"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590716", L"ATT_GP_OPTIONS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590718", L"ATT_GPC_FILE_SYS_PATH"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590717", L"ATT_GPC_FUNCTIONALITY_VERSION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591172", L"ATT_GPC_MACHINE_EXTENSION_NAMES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591173", L"ATT_GPC_USER_EXTENSION_NAMES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591518", L"ATT_GPC_WQL_FILTER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589976", L"ATT_GROUP_ATTRIBUTES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589990", L"ATT_GROUP_MEMBERSHIP_SAM"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590169", L"ATT_GROUP_PRIORITY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590574", L"ATT_GROUP_TYPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590168", L"ATT_GROUPS_TO_IGNORE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb131086", L"ATT_HAS_MASTER_NCS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb131087", L"ATT_HAS_PARTIAL_REPLICA_NCS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk131474", L"ATT_HELP_DATA16"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk131081", L"ATT_HELP_DATA32"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131399", L"ATT_HELP_FILE_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi591604", L"ATT_HIDE_FROM_AB"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589868", L"ATT_HOME_DIRECTORY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589869", L"ATT_HOME_DRIVE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm51", L"ATT_HOUSEIDENTIFIER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1376265", L"ATT_HOST"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590043", L"ATT_ICON_PATH"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590144", L"ATT_IMPLEMENTED_CATEGORIES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590505", L"ATT_INDEXEDSCOPES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590363", L"ATT_INITIAL_AUTH_INCOMING"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590364", L"ATT_INITIAL_AUTH_OUTGOING"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm43", L"ATT_INITIALS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590671", L"ATT_INSTALL_UI_LEVEL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj131073", L"ATT_INSTANCE_TYPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591072", L"ATT_INTER_SITE_TOPOLOGY_FAILOVER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591070", L"ATT_INTER_SITE_TOPOLOGY_GENERATOR"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591071", L"ATT_INTER_SITE_TOPOLOGY_RENEW"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTg25", L"ATT_INTERNATIONAL_ISDN_NUMBER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk131187", L"ATT_INVOCATION_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590447", L"ATT_IPSEC_DATA"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590446", L"ATT_IPSEC_DATA_TYPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590453", L"ATT_IPSEC_FILTER_REFERENCE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590445", L"ATT_IPSEC_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590450", L"ATT_IPSEC_ISAKMP_REFERENCE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590444", L"ATT_IPSEC_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590712", L"ATT_IPSEC_NEGOTIATION_POLICY_ACTION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590452", L"ATT_IPSEC_NEGOTIATION_POLICY_REFERENCE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590711", L"ATT_IPSEC_NEGOTIATION_POLICY_TYPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590451", L"ATT_IPSEC_NFA_REFERENCE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590448", L"ATT_IPSEC_OWNERS_REFERENCE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590341", L"ATT_IPSEC_POLICY_REFERENCE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590692", L"ATT_IS_CRITICAL_SYSTEM_OBJECT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590485", L"ATT_IS_DEFUNCT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi131120", L"ATT_IS_DELETED"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi591036", L"ATT_IS_EPHEMERAL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb131174", L"ATT_IS_MEMBER_OF_DL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590463", L"ATT_IS_MEMBER_OF_PARTIAL_ATTRIBUTE_SET"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590462", L"ATT_IS_PRIVILEGE_HOLDER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi131105", L"ATT_IS_SINGLE_VALUED"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk1376316", L"ATT_JPEGPHOTO"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589872", L"ATT_KEYWORDS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTe2", L"ATT_KNOWLEDGE_INFORMATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590343", L"ATT_LAST_BACKUP_RESTORATION_TIME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589874", L"ATT_LAST_CONTENT_INDEXED"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590605", L"ATT_LAST_KNOWN_PARENT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589875", L"ATT_LAST_LOGOFF"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589876", L"ATT_LAST_LOGON"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq591520", L"ATT_LAST_LOGON_TIMESTAMP"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589877", L"ATT_LAST_SET_TIME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590154", L"ATT_LAST_UPDATE_SEQUENCE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590667", L"ATT_LDAP_ADMIN_LIMITS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131532", L"ATT_LDAP_DISPLAY_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590668", L"ATT_LDAP_IPDENY_LIST"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTe590479", L"ATT_LEGACY_EXCHANGE_DN"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj131122", L"ATT_LINK_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590093", L"ATT_LINK_TRACK_SECRET"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589984", L"ATT_LM_PWD_HISTORY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589880", L"ATT_LOCAL_POLICY_FLAGS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590281", L"ATT_LOCAL_POLICY_REFERENCE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589882", L"ATT_LOCALE_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm7", L"ATT_LOCALITY_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590641", L"ATT_LOCALIZED_DESCRIPTION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591177", L"ATT_LOCALIZATION_DISPLAY_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590046", L"ATT_LOCATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589885", L"ATT_LOCK_OUT_OBSERVATION_WINDOW"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589884", L"ATT_LOCKOUT_DURATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589897", L"ATT_LOCKOUT_THRESHOLD"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590486", L"ATT_LOCKOUT_TIME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk1441828", L"ATT_LOGO"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589993", L"ATT_LOGON_COUNT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589888", L"ATT_LOGON_HOURS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589889", L"ATT_LOGON_WORKSTATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589890", L"ATT_LSA_CREATION_TIME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589891", L"ATT_LSA_MODIFIED_COUNT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589892", L"ATT_MACHINE_ARCHITECTURE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590344", L"ATT_MACHINE_PASSWORD_CHANGE_INTERVAL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589895", L"ATT_MACHINE_ROLE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590283", L"ATT_MACHINE_WIDE_POLICY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590477", L"ATT_MANAGED_BY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590478", L"ATT_MANAGED_OBJECTS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb1376266", L"ATT_MANAGER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj131121", L"ATT_MAPI_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589896", L"ATT_MARSHALLED_INTERFACE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591233", L"ATT_MASTERED_BY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589898", L"ATT_MAX_PWD_AGE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589899", L"ATT_MAX_RENEW_AGE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589900", L"ATT_MAX_STORAGE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589901", L"ATT_MAX_TICKET_AGE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTc131097", L"ATT_MAY_CONTAIN"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590406", L"ATT_MEETINGADVERTISESCOPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590397", L"ATT_MEETINGAPPLICATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590413", L"ATT_MEETINGBANDWIDTH"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590414", L"ATT_MEETINGBLOB"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590402", L"ATT_MEETINGCONTACTINFO"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590391", L"ATT_MEETINGDESCRIPTION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTl590412", L"ATT_MEETINGENDTIME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590389", L"ATT_MEETINGID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590404", L"ATT_MEETINGIP"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590409", L"ATT_MEETINGISENCRYPTED"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590392", L"ATT_MEETINGKEYWORD"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590398", L"ATT_MEETINGLANGUAGE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590393", L"ATT_MEETINGLOCATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590400", L"ATT_MEETINGMAXPARTICIPANTS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590390", L"ATT_MEETINGNAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590401", L"ATT_MEETINGORIGINATOR"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590403", L"ATT_MEETINGOWNER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590394", L"ATT_MEETINGPROTOCOL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590408", L"ATT_MEETINGRATING"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590410", L"ATT_MEETINGRECURRENCE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590405", L"ATT_MEETINGSCOPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTl590411", L"ATT_MEETINGSTARTTIME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590395", L"ATT_MEETINGTYPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590407", L"ATT_MEETINGURL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb31", L"ATT_MEMBER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590474", L"ATT_MHS_OR_ADDRESS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589902", L"ATT_MIN_PWD_AGE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589903", L"ATT_MIN_PWD_LENGTH"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589904", L"ATT_MIN_TICKET_AGE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589992", L"ATT_MODIFIED_COUNT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589905", L"ATT_MODIFIED_COUNT_AT_LAST_PROM"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTl1638402", L"ATT_MODIFY_TIME_STAMP"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589906", L"ATT_MONIKER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589907", L"ATT_MONIKER_DISPLAY_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk591129", L"ATT_MOVE_TREE_STATE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591251", L"ATT_MS_COM_DEFAULTPARTITIONLINK"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk591252", L"ATT_MS_COM_OBJECTID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591247", L"ATT_MS_COM_PARTITIONLINK"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591248", L"ATT_MS_COM_PARTITIONSETLINK"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591249", L"ATT_MS_COM_USERLINK"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591250", L"ATT_MS_COM_USERPARTITIONSETLINK"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk591667", L"ATT_MS_DRM_IDENTITY_CERTIFICATE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591541", L"ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591542", L"ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591613", L"ATT_MS_DS_ALL_USERS_TRUST_QUOTA"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591534", L"ATT_MS_DS_ALLOWED_DNS_SUFFIXES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591611", L"ATT_MS_DS_ALLOWED_TO_DELEGATE_TO"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTc591282", L"ATT_MS_DS_AUXILIARY_CLASSES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591493", L"ATT_MS_DS_APPROX_IMMED_SUBORDINATES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591643", L"ATT_MS_DS_AZ_APPLICATION_DATA"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591622", L"ATT_MS_DS_AZ_APPLICATION_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591641", L"ATT_MS_DS_AZ_APPLICATION_VERSION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591625", L"ATT_MS_DS_AZ_BIZ_RULE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591626", L"ATT_MS_DS_AZ_BIZ_RULE_LANGUAGE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591640", L"ATT_MS_DS_AZ_CLASS_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591619", L"ATT_MS_DS_AZ_DOMAIN_TIMEOUT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi591629", L"ATT_MS_DS_AZ_GENERATE_AUDITS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591627", L"ATT_MS_DS_AZ_LAST_IMPORTED_BIZ_RULE_PATH"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591616", L"ATT_MS_DS_AZ_LDAP_QUERY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591648", L"ATT_MS_DS_AZ_MAJOR_VERSION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591649", L"ATT_MS_DS_AZ_MINOR_VERSION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591624", L"ATT_MS_DS_AZ_OPERATION_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591623", L"ATT_MS_DS_AZ_SCOPE_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591620", L"ATT_MS_DS_AZ_SCRIPT_ENGINE_CACHE_MAX"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591621", L"ATT_MS_DS_AZ_SCRIPT_TIMEOUT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi591642", L"ATT_MS_DS_AZ_TASK_IS_ROLE_DEFINITION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591283", L"ATT_MS_DS_BEHAVIOR_VERSION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk591655", L"ATT_MS_DS_BYTE_ARRAY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk591265", L"ATT_MS_DS_CACHED_MEMBERSHIP"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq591266", L"ATT_MS_DS_CACHED_MEMBERSHIP_TIME_STAMP"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk591184", L"ATT_MS_DS_CONSISTENCY_GUID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591185", L"ATT_MS_DS_CONSISTENCY_CHILD_COUNT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTr591234", L"ATT_MS_DS_CREATOR_SID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTl591656", L"ATT_MS_DS_DATE_TIME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591670", L"ATT_MS_DS_DEFAULT_QUOTA"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591543", L"ATT_MS_DS_DNSROOTALIAS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTl591446", L"ATT_MS_DS_ENTRY_TIME_TO_DIE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk591607", L"ATT_MS_DS_EXECUTESCRIPTPASSWORD"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591657", L"ATT_MS_DS_EXTERNAL_KEY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591658", L"ATT_MS_DS_EXTERNAL_STORE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591527", L"ATT_MS_DS_FILTER_CONTAINERS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTh591533", L"ATT_MS_DS_HAS_INSTANTIATED_NCS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591644", L"ATT_MS_DS_HAS_DOMAIN_NCS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591660", L"ATT_MS_DS_HAS_MASTER_NCS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591659", L"ATT_MS_DS_INTEGER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591540", L"ATT_MS_DS_INTID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591606", L"ATT_MS_DS_KEYVERSIONNUMBER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591608", L"ATT_MS_DS_LOGON_TIME_SYNC_INTERVAL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591661", L"ATT_MS_DS_MASTERED_BY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591666", L"ATT_MS_DS_MAX_VALUES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591630", L"ATT_MS_DS_MEMBERS_FOR_AZ_ROLE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591631", L"ATT_MS_DS_MEMBERS_FOR_AZ_ROLE_BL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591617", L"ATT_MS_DS_NON_MEMBERS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591618", L"ATT_MS_DS_NON_MEMBERS_BL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk591526", L"ATT_MS_DS_TRUST_FOREST_TRUST_INFO"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591671", L"ATT_MS_DS_TOMBSTONE_QUOTA_FACTOR"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591674", L"ATT_MS_DS_TOP_QUOTA_USAGE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591235", L"ATT_MS_DS_MACHINE_ACCOUNT_QUOTA"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591664", L"ATT_MS_DS_OBJECT_REFERENCE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591665", L"ATT_MS_DS_OBJECT_REFERENCE_BL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591636", L"ATT_MS_DS_OPERATIONS_FOR_AZ_ROLE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591637", L"ATT_MS_DS_OPERATIONS_FOR_AZ_ROLE_BL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591632", L"ATT_MS_DS_OPERATIONS_FOR_AZ_TASK"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591633", L"ATT_MS_DS_OPERATIONS_FOR_AZ_TASK_BL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591445", L"ATT_MS_DS_OTHER_SETTINGS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591669", L"ATT_MS_DS_QUOTA_AMOUNT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591672", L"ATT_MS_DS_QUOTA_EFFECTIVE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTr591668", L"ATT_MS_DS_QUOTA_TRUSTEE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591673", L"ATT_MS_DS_QUOTA_USED"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591528", L"ATT_MS_DS_NC_REPL_CURSORS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591529", L"ATT_MS_DS_NC_REPL_INBOUND_NEIGHBORS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591530", L"ATT_MS_DS_NC_REPL_OUTBOUND_NEIGHBORS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591485", L"ATT_MS_DS_NC_REPLICA_LOCATIONS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591513", L"ATT_MS_DS_NON_SECURITY_GROUP_EXTRA_CLASSES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591612", L"ATT_MS_DS_PER_USER_TRUST_QUOTA"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591614", L"ATT_MS_DS_PER_USER_TRUST_TOMBSTONES_QUOTA"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591268", L"ATT_MS_DS_PREFERRED_GC_SITE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591531", L"ATT_MS_DS_REPL_ATTRIBUTE_META_DATA"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591532", L"ATT_MS_DS_REPL_VALUE_META_DATA"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTh591232", L"ATT_MS_DS_REPLICATES_NC_REASON"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591487", L"ATT_MS_DS_REPLICATION_NOTIFY_FIRST_DSA_DELAY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591488", L"ATT_MS_DS_REPLICATION_NOTIFY_SUBSEQUENT_DSA_DELAY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591544", L"ATT_MS_DS_REPLICATIONEPOCH"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk591650", L"ATT_MS_DS_RETIRED_REPL_NC_SIGNATURES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk591264", L"ATT_MS_DS_SCHEMA_EXTENSIONS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591535", L"ATT_MS_DS_SD_REFERENCE_DOMAIN"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591512", L"ATT_MS_DS_SECURITY_GROUP_EXTRA_CLASSES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591521", L"ATT_MS_DS_SETTINGS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk591267", L"ATT_MS_DS_SITE_AFFINITY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591539", L"ATT_MS_DS_SPN_SUFFIXES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591638", L"ATT_MS_DS_TASKS_FOR_AZ_ROLE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591639", L"ATT_MS_DS_TASKS_FOR_AZ_ROLE_BL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591634", L"ATT_MS_DS_TASKS_FOR_AZ_TASK"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591635", L"ATT_MS_DS_TASKS_FOR_AZ_TASK_BL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591284", L"ATT_MS_DS_USER_ACCOUNT_CONTROL_COMPUTED"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591545", L"ATT_MS_DS_UPDATESCRIPT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131516", L"ATT_MS_EXCH_ASSISTANT_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131668", L"ATT_MS_EXCH_HOUSE_IDENTIFIER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131665", L"ATT_MS_EXCH_LABELEDURI"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb131176", L"ATT_MS_EXCH_OWNER_BL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591517", L"ATT_MS_FRS_HUB_MEMBER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591516", L"ATT_MS_FRS_TOPOLOGY_PREF"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk591645", L"ATT_MS_IEEE_80211_DATA"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591646", L"ATT_MS_IEEE_80211_DATA_TYPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591647", L"ATT_MS_IEEE_80211_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591610", L"ATT_MS_IIS_FTP_DIR"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591609", L"ATT_MS_IIS_FTP_ROOT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591260", L"ATT_MS_PKI_CERT_TEMPLATE_OID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591498", L"ATT_MS_PKI_CERTIFICATE_APPLICATION_POLICY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591256", L"ATT_MS_PKI_CERTIFICATE_NAME_FLAG"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591263", L"ATT_MS_PKI_CERTIFICATE_POLICY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591254", L"ATT_MS_PKI_ENROLLMENT_FLAG"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591257", L"ATT_MS_PKI_MINIMAL_KEY_SIZE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591495", L"ATT_MS_PKI_OID_ATTRIBUTE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591496", L"ATT_MS_PKI_OID_CPS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591536", L"ATT_MS_PKI_OID_LOCALIZEDNAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591497", L"ATT_MS_PKI_OID_USER_NOTICE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591255", L"ATT_MS_PKI_PRIVATE_KEY_FLAG"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591261", L"ATT_MS_PKI_SUPERSEDE_TEMPLATES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591259", L"ATT_MS_PKI_TEMPLATE_MINOR_REVISION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591258", L"ATT_MS_PKI_TEMPLATE_SCHEMA_VERSION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591499", L"ATT_MS_PKI_RA_APPLICATION_POLICIES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591262", L"ATT_MS_PKI_RA_POLICIES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591253", L"ATT_MS_PKI_RA_SIGNATURE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590708", L"ATT_MS_RRAS_ATTRIBUTE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590707", L"ATT_MS_RRAS_VENDOR_ATTRIBUTE_ENTRY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591187", L"ATT_MS_SQL_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591188", L"ATT_MS_SQL_REGISTEREDOWNER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591189", L"ATT_MS_SQL_CONTACT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591190", L"ATT_MS_SQL_LOCATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq591191", L"ATT_MS_SQL_MEMORY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591192", L"ATT_MS_SQL_BUILD"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591193", L"ATT_MS_SQL_SERVICEACCOUNT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591194", L"ATT_MS_SQL_CHARACTERSET"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591195", L"ATT_MS_SQL_SORTORDER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591196", L"ATT_MS_SQL_UNICODESORTORDER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi591197", L"ATT_MS_SQL_CLUSTERED"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591198", L"ATT_MS_SQL_NAMEDPIPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591199", L"ATT_MS_SQL_MULTIPROTOCOL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591200", L"ATT_MS_SQL_SPX"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591201", L"ATT_MS_SQL_TCPIP"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591202", L"ATT_MS_SQL_APPLETALK"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591203", L"ATT_MS_SQL_VINES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq591204", L"ATT_MS_SQL_STATUS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591205", L"ATT_MS_SQL_LASTUPDATEDDATE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591206", L"ATT_MS_SQL_INFORMATIONURL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591207", L"ATT_MS_SQL_CONNECTIONURL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591208", L"ATT_MS_SQL_PUBLICATIONURL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591209", L"ATT_MS_SQL_GPSLATITUDE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591210", L"ATT_MS_SQL_GPSLONGITUDE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591211", L"ATT_MS_SQL_GPSHEIGHT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591212", L"ATT_MS_SQL_VERSION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591213", L"ATT_MS_SQL_LANGUAGE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591214", L"ATT_MS_SQL_DESCRIPTION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591215", L"ATT_MS_SQL_TYPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi591216", L"ATT_MS_SQL_INFORMATIONDIRECTORY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591217", L"ATT_MS_SQL_DATABASE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi591218", L"ATT_MS_SQL_ALLOWANONYMOUSSUBSCRIPTION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591219", L"ATT_MS_SQL_ALIAS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq591220", L"ATT_MS_SQL_SIZE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591221", L"ATT_MS_SQL_CREATIONDATE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591222", L"ATT_MS_SQL_LASTBACKUPDATE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591223", L"ATT_MS_SQL_LASTDIAGNOSTICDATE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591224", L"ATT_MS_SQL_APPLICATIONS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591225", L"ATT_MS_SQL_KEYWORDS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591226", L"ATT_MS_SQL_PUBLISHER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi591227", L"ATT_MS_SQL_ALLOWKNOWNPULLSUBSCRIPTION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi591228", L"ATT_MS_SQL_ALLOWIMMEDIATEUPDATINGSUBSCRIPTION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi591229", L"ATT_MS_SQL_ALLOWQUEUEDUPDATINGSUBSCRIPTION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi591230", L"ATT_MS_SQL_ALLOWSNAPSHOTFILESFTPDOWNLOADING"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi591231", L"ATT_MS_SQL_THIRDPARTY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk591524", L"ATT_MS_TAPI_CONFERENCE_BLOB"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591525", L"ATT_MS_TAPI_IP_ADDRESS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591523", L"ATT_MS_TAPI_PROTOCOL_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591522", L"ATT_MS_TAPI_UNIQUE_IDENTIFIER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591447", L"ATT_MS_WMI_AUTHOR"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591448", L"ATT_MS_WMI_CHANGEDATE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591500", L"ATT_MS_WMI_CLASS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591449", L"ATT_MS_WMI_CLASSDEFINITION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591450", L"ATT_MS_WMI_CREATIONDATE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591501", L"ATT_MS_WMI_GENUS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591451", L"ATT_MS_WMI_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591452", L"ATT_MS_WMI_INTDEFAULT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591502", L"ATT_MS_WMI_INTFLAGS1"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591503", L"ATT_MS_WMI_INTFLAGS2"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591504", L"ATT_MS_WMI_INTFLAGS3"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591505", L"ATT_MS_WMI_INTFLAGS4"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591453", L"ATT_MS_WMI_INTMAX"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591454", L"ATT_MS_WMI_INTMIN"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591455", L"ATT_MS_WMI_INTVALIDVALUES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq591456", L"ATT_MS_WMI_INT8DEFAULT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq591457", L"ATT_MS_WMI_INT8MAX"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq591458", L"ATT_MS_WMI_INT8MIN"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq591459", L"ATT_MS_WMI_INT8VALIDVALUES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591462", L"ATT_MS_WMI_MOF"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591463", L"ATT_MS_WMI_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591464", L"ATT_MS_WMI_NORMALIZEDCLASS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591506", L"ATT_MS_WMI_PARM1"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591507", L"ATT_MS_WMI_PARM2"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591508", L"ATT_MS_WMI_PARM3"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591509", L"ATT_MS_WMI_PARM4"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591465", L"ATT_MS_WMI_PROPERTYNAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591466", L"ATT_MS_WMI_QUERY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591467", L"ATT_MS_WMI_QUERYLANGUAGE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591510", L"ATT_MS_WMI_SCOPEGUID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591468", L"ATT_MS_WMI_SOURCEORGANIZATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591460", L"ATT_MS_WMI_STRINGDEFAULT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591461", L"ATT_MS_WMI_STRINGVALIDVALUES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591469", L"ATT_MS_WMI_TARGETCLASS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591470", L"ATT_MS_WMI_TARGETNAMESPACE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk591471", L"ATT_MS_WMI_TARGETOBJECT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591472", L"ATT_MS_WMI_TARGETPATH"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591473", L"ATT_MS_WMI_TARGETTYPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTf590540", L"ATT_MSCOPE_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590495", L"ATT_MSI_FILE_LIST"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590638", L"ATT_MSI_SCRIPT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590669", L"ATT_MSI_SCRIPT_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589839", L"ATT_MSI_SCRIPT_PATH"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590670", L"ATT_MSI_SCRIPT_SIZE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590747", L"ATT_MSMQ_AUTHENTICATE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590744", L"ATT_MSMQ_BASE_PRIORITY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTe590757", L"ATT_MSMQ_COMPUTER_TYPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591241", L"ATT_MSMQ_COMPUTER_TYPE_EX"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590770", L"ATT_MSMQ_COST"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTe590764", L"ATT_MSMQ_CSP_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi591063", L"ATT_MSMQ_DEPENDENT_CLIENT_SERVICE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi591050", L"ATT_MSMQ_DEPENDENT_CLIENT_SERVICES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590772", L"ATT_MSMQ_DIGESTS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590790", L"ATT_MSMQ_DIGESTS_MIG"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi591062", L"ATT_MSMQ_DS_SERVICE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi591052", L"ATT_MSMQ_DS_SERVICES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590760", L"ATT_MSMQ_ENCRYPT_KEY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590758", L"ATT_MSMQ_FOREIGN"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590753", L"ATT_MSMQ_IN_ROUTING_SERVERS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591132", L"ATT_MSMQ_INTERVAL1"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591133", L"ATT_MSMQ_INTERVAL2"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590742", L"ATT_MSMQ_JOURNAL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590745", L"ATT_MSMQ_JOURNAL_QUOTA"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTe590746", L"ATT_MSMQ_LABEL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591239", L"ATT_MSMQ_LABEL_EX"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590765", L"ATT_MSMQ_LONG_LIVED"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590776", L"ATT_MSMQ_MIGRATED"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591538", L"ATT_MSMQ_MULTICAST_ADDRESS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590763", L"ATT_MSMQ_NAME_STYLE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590788", L"ATT_MSMQ_NT4_FLAGS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590784", L"ATT_MSMQ_NT4_STUB"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590759", L"ATT_MSMQ_OS_TYPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590752", L"ATT_MSMQ_OUT_ROUTING_SERVERS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590749", L"ATT_MSMQ_OWNER_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591049", L"ATT_MSMQ_PREV_SITE_GATES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590748", L"ATT_MSMQ_PRIVACY_LEVEL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590775", L"ATT_MSMQ_QM_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590787", L"ATT_MSMQ_QUEUE_JOURNAL_QUOTA"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591067", L"ATT_MSMQ_QUEUE_NAME_EXT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590786", L"ATT_MSMQ_QUEUE_QUOTA"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590741", L"ATT_MSMQ_QUEUE_TYPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590743", L"ATT_MSMQ_QUOTA"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591519", L"ATT_MSMQ_RECIPIENT_FORMATNAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi591061", L"ATT_MSMQ_ROUTING_SERVICE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi591051", L"ATT_MSMQ_ROUTING_SERVICES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi591537", L"ATT_MSMQ_SECURED_SOURCE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590754", L"ATT_MSMQ_SERVICE_TYPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590774", L"ATT_MSMQ_SERVICES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590771", L"ATT_MSMQ_SIGN_CERTIFICATES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590791", L"ATT_MSMQ_SIGN_CERTIFICATES_MIG"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590761", L"ATT_MSMQ_SIGN_KEY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590767", L"ATT_MSMQ_SITE_1"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590768", L"ATT_MSMQ_SITE_2"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590785", L"ATT_MSMQ_SITE_FOREIGN"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590769", L"ATT_MSMQ_SITE_GATES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591134", L"ATT_MSMQ_SITE_GATES_MIG"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590777", L"ATT_MSMQ_SITE_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTe590789", L"ATT_MSMQ_SITE_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591240", L"ATT_MSMQ_SITE_NAME_EX"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590751", L"ATT_MSMQ_SITES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590750", L"ATT_MSMQ_TRANSACTIONAL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk591161", L"ATT_MSMQ_USER_SID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590766", L"ATT_MSMQ_VERSION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590943", L"ATT_MSNPALLOWDIALIN"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTf590947", L"ATT_MSNPCALLEDSTATIONID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTf590948", L"ATT_MSNPCALLINGSTATIONID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTf590954", L"ATT_MSNPSAVEDCALLINGSTATIONID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTf590969", L"ATT_MSRADIUSCALLBACKNUMBER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590977", L"ATT_MSRADIUSFRAMEDIPADDRESS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTf590982", L"ATT_MSRADIUSFRAMEDROUTE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590995", L"ATT_MSRADIUSSERVICETYPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTf591013", L"ATT_MSRASSAVEDCALLBACKNUMBER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591014", L"ATT_MSRASSAVEDFRAMEDIPADDRESS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTf591015", L"ATT_MSRASSAVEDFRAMEDROUTE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTc131096", L"ATT_MUST_CONTAIN"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590577", L"ATT_NAME_SERVICE_FLAGS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb131088", L"ATT_NC_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589911", L"ATT_NETBIOS_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590673", L"ATT_NETBOOT_ALLOW_NEW_CLIENTS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590678", L"ATT_NETBOOT_ANSWER_ONLY_VALID_CLIENTS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590677", L"ATT_NETBOOT_ANSWER_REQUESTS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590676", L"ATT_NETBOOT_CURRENT_CLIENT_COUNT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590183", L"ATT_NETBOOT_GUID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590182", L"ATT_NETBOOT_INITIALIZATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590681", L"ATT_NETBOOT_INTELLIMIRROR_OSES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590674", L"ATT_NETBOOT_LIMIT_CLIENTS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590683", L"ATT_NETBOOT_LOCALLY_INSTALLED_OSES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590185", L"ATT_NETBOOT_MACHINE_FILE_PATH"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590675", L"ATT_NETBOOT_MAX_CLIENTS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591065", L"ATT_NETBOOT_MIRROR_DATA_FILE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590679", L"ATT_NETBOOT_NEW_MACHINE_NAMING_POLICY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590680", L"ATT_NETBOOT_NEW_MACHINE_OU"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590688", L"ATT_NETBOOT_SCP_BL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590684", L"ATT_NETBOOT_SERVER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591064", L"ATT_NETBOOT_SIF_FILE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590682", L"ATT_NETBOOT_TOOLS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTe131531", L"ATT_NETWORK_ADDRESS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590038", L"ATT_NEXT_LEVEL_STORE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589912", L"ATT_NEXT_RID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590354", L"ATT_NON_SECURITY_MEMBER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590355", L"ATT_NON_SECURITY_MEMBER_BL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590127", L"ATT_NOTIFICATION_LIST"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589913", L"ATT_NT_GROUP_MEMBERS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590181", L"ATT_NT_MIXED_DOMAIN"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589918", L"ATT_NT_PWD_HISTORY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTp131353", L"ATT_NT_SECURITY_DESCRIPTOR"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb49", L"ATT_OBJ_DIST_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590606", L"ATT_OBJECT_CATEGORY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTc0", L"ATT_OBJECT_CLASS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj131442", L"ATT_OBJECT_CLASS_CATEGORY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1572870", L"ATT_OBJECT_CLASSES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590330", L"ATT_OBJECT_COUNT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589826", L"ATT_OBJECT_GUID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTr589970", L"ATT_OBJECT_SID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj131148", L"ATT_OBJECT_VERSION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589975", L"ATT_OEM_INFORMATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk131290", L"ATT_OM_OBJECT_CLASS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj131303", L"ATT_OM_SYNTAX"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590329", L"ATT_OMT_GUID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590157", L"ATT_OMT_INDX_GUID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590187", L"ATT_OPERATING_SYSTEM"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590239", L"ATT_OPERATING_SYSTEM_HOTFIX"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590189", L"ATT_OPERATING_SYSTEM_SERVICE_PACK"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590188", L"ATT_OPERATING_SYSTEM_VERSION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589968", L"ATT_OPERATOR_COUNT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590536", L"ATT_OPTION_DESCRIPTION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590131", L"ATT_OPTIONS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTf590537", L"ATT_OPTIONS_LOCATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm10", L"ATT_ORGANIZATION_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm11", L"ATT_ORGANIZATIONAL_UNIT_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1376301", L"ATT_ORGANIZATIONALSTATUS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk131517", L"ATT_ORIGINAL_DISPLAY_TABLE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk131286", L"ATT_ORIGINAL_DISPLAY_TABLE_MSDOS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589915", L"ATT_OTHER_LOGIN_WORKSTATIONS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590475", L"ATT_OTHER_MAILBOX"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1441826", L"ATT_OTHER_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTh591183", L"ATT_OTHER_WELL_KNOWN_OBJECTS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb32", L"ATT_OWNER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590151", L"ATT_PACKAGE_FLAGS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590150", L"ATT_PACKAGE_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590148", L"ATT_PACKAGE_TYPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590381", L"ATT_PARENT_CA"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590509", L"ATT_PARENT_CA_CERTIFICATE_CHAIN"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk591048", L"ATT_PARENT_GUID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590487", L"ATT_PARTIAL_ATTRIBUTE_DELETION_LIST"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590464", L"ATT_PARTIAL_ATTRIBUTE_SET"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590690", L"ATT_PEK_KEY_CHANGE_INTERVAL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590689", L"ATT_PEK_LIST"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590517", L"ATT_PENDING_CA_CERTIFICATES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590519", L"ATT_PENDING_PARENT_CA"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk131397", L"ATT_PER_MSG_DIALOG_DISPLAY_TABLE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk131398", L"ATT_PER_RECIP_DIALOG_DISPLAY_TABLE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131687", L"ATT_PERSONAL_TITLE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590470", L"ATT_PHONE_FAX_OTHER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131349", L"ATT_PHONE_HOME_OTHER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1376276", L"ATT_PHONE_HOME_PRIMARY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590546", L"ATT_PHONE_IP_OTHER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590545", L"ATT_PHONE_IP_PRIMARY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590473", L"ATT_PHONE_ISDN_PRIMARY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590471", L"ATT_PHONE_MOBILE_OTHER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1376297", L"ATT_PHONE_MOBILE_PRIMARY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131090", L"ATT_PHONE_OFFICE_OTHER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131190", L"ATT_PHONE_PAGER_OTHER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1376298", L"ATT_PHONE_PAGER_PRIMARY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk1376263", L"ATT_PHOTO"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm19", L"ATT_PHYSICAL_DELIVERY_OFFICE_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590338", L"ATT_PHYSICAL_LOCATION_OBJECT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk1441827", L"ATT_PICTURE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591154", L"ATT_PKI_CRITICAL_EXTENSIONS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591158", L"ATT_PKI_DEFAULT_CSPS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591151", L"ATT_PKI_DEFAULT_KEY_SPEC"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTp591159", L"ATT_PKI_ENROLLMENT_ACCESS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk591155", L"ATT_PKI_EXPIRATION_PERIOD"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591157", L"ATT_PKI_EXTENDED_KEY_USAGE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk591152", L"ATT_PKI_KEY_USAGE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591153", L"ATT_PKI_MAX_ISSUING_DEPTH"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk591156", L"ATT_PKI_OVERLAP_PERIOD"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590030", L"ATT_PKT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590029", L"ATT_PKT_GUID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590457", L"ATT_POLICY_REPLICATION_FLAGS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590052", L"ATT_PORT_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTc131080", L"ATT_POSS_SUPERIORS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTc590739", L"ATT_POSSIBLE_INFERIORS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm18", L"ATT_POST_OFFICE_BOX"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm16", L"ATT_POSTAL_ADDRESS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm17", L"ATT_POSTAL_CODE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj28", L"ATT_PREFERRED_DELIVERY_METHOD"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1441831", L"ATT_PREFERREDLANGUAGE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb589921", L"ATT_PREFERRED_OU"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590362", L"ATT_PREFIX_MAP"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTn29", L"ATT_PRESENTATION_ADDRESS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590516", L"ATT_PREVIOUS_CA_CERTIFICATES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590518", L"ATT_PREVIOUS_PARENT_CA"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589922", L"ATT_PRIMARY_GROUP_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591236", L"ATT_PRIMARY_GROUP_TOKEN"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590071", L"ATT_PRINT_ATTRIBUTES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590061", L"ATT_PRINT_BIN_NAMES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590066", L"ATT_PRINT_COLLATE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590067", L"ATT_PRINT_COLOR"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi591135", L"ATT_PRINT_DUPLEX_SUPPORTED"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590058", L"ATT_PRINT_END_TIME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590059", L"ATT_PRINT_FORM_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590099", L"ATT_PRINT_KEEP_PRINTED_JOBS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590070", L"ATT_PRINT_LANGUAGE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590112", L"ATT_PRINT_MAC_ADDRESS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590065", L"ATT_PRINT_MAX_COPIES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590062", L"ATT_PRINT_MAX_RESOLUTION_SUPPORTED"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590101", L"ATT_PRINT_MAX_X_EXTENT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590102", L"ATT_PRINT_MAX_Y_EXTENT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590113", L"ATT_PRINT_MEDIA_READY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590123", L"ATT_PRINT_MEDIA_SUPPORTED"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590106", L"ATT_PRINT_MEMORY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590103", L"ATT_PRINT_MIN_X_EXTENT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590104", L"ATT_PRINT_MIN_Y_EXTENT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590111", L"ATT_PRINT_NETWORK_ADDRESS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590096", L"ATT_PRINT_NOTIFY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590114", L"ATT_PRINT_NUMBER_UP"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590064", L"ATT_PRINT_ORIENTATIONS_SUPPORTED"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590095", L"ATT_PRINT_OWNER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590455", L"ATT_PRINT_PAGES_PER_MINUTE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590109", L"ATT_PRINT_RATE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590110", L"ATT_PRINT_RATE_UNIT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590054", L"ATT_PRINT_SEPARATOR_FILE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590094", L"ATT_PRINT_SHARE_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590098", L"ATT_PRINT_SPOOLING"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590105", L"ATT_PRINT_STAPLING_SUPPORTED"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590057", L"ATT_PRINT_START_TIME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590097", L"ATT_PRINT_STATUS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590124", L"ATT_PRINTER_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589923", L"ATT_PRIOR_SET_TIME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589924", L"ATT_PRIOR_VALUE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590055", L"ATT_PRIORITY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589925", L"ATT_PRIVATE_KEY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590460", L"ATT_PRIVILEGE_ATTRIBUTES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590458", L"ATT_PRIVILEGE_DISPLAY_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590461", L"ATT_PRIVILEGE_HOLDER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590459", L"ATT_PRIVILEGE_VALUE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590642", L"ATT_PRODUCT_CODE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589963", L"ATT_PROFILE_PATH"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTh591073", L"ATT_PROXIED_OBJECT_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131282", L"ATT_PROXY_ADDRESSES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi131595", L"ATT_PROXY_GENERATION_ENABLED"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589927", L"ATT_PROXY_LIFETIME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590244", L"ATT_PUBLIC_KEY_POLICY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590710", L"ATT_PURPORTED_SEARCH"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589919", L"ATT_PWD_HISTORY_LENGTH"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq589920", L"ATT_PWD_LAST_SET"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589917", L"ATT_PWD_PROPERTIES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590282", L"ATT_QUALITY_OF_SERVICE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591179", L"ATT_QUERY_FILTER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590432", L"ATT_QUERY_POLICY_BL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590431", L"ATT_QUERY_POLICY_OBJECT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590504", L"ATT_QUERYPOINT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj131106", L"ATT_RANGE_LOWER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj131107", L"ATT_RANGE_UPPER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589825", L"ATT_RDN"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTc131098", L"ATT_RDN_ATT_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk26", L"ATT_REGISTERED_ADDRESS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589929", L"ATT_REMOTE_SERVER_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589931", L"ATT_REMOTE_SOURCE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589932", L"ATT_REMOTE_SOURCE_TYPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590633", L"ATT_REMOTE_STORAGE_GUID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589827", L"ATT_REPL_PROPERTY_META_DATA"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590501", L"ATT_REPL_TOPOLOGY_STAY_OF_EXECUTION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589828", L"ATT_REPL_UPTODATE_VECTOR"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589933", L"ATT_REPLICA_SOURCE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb131508", L"ATT_REPORTS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591160", L"ATT_REPL_INTERVAL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk131163", L"ATT_REPS_FROM"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk131155", L"ATT_REPS_TO"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590145", L"ATT_REQUIRED_CATEGORIES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590497", L"ATT_RETIRED_REPL_DSA_SIGNATURES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTr591125", L"ATT_TOKEN_GROUPS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTr591242", L"ATT_TOKEN_GROUPS_GLOBAL_AND_UNIVERSAL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTr591127", L"ATT_TOKEN_GROUPS_NO_GC_ACCEPTABLE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589969", L"ATT_REVISION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589977", L"ATT_RID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590195", L"ATT_RID_ALLOCATION_POOL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590194", L"ATT_RID_AVAILABLE_POOL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590192", L"ATT_RID_MANAGER_REFERENCE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590198", L"ATT_RID_NEXT_RID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590196", L"ATT_RID_PREVIOUS_ALLOCATION_POOL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590493", L"ATT_RID_SET_REFERENCES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590197", L"ATT_RID_USED_POOL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590164", L"ATT_RIGHTS_GUID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb33", L"ATT_ROLE_OCCUPANT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1376262", L"ATT_ROOMNUMBER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590498", L"ATT_ROOT_TRUST"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590190", L"ATT_RPC_NS_ANNOTATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589937", L"ATT_RPC_NS_BINDINGS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590191", L"ATT_RPC_NS_CODESET"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590578", L"ATT_RPC_NS_ENTRY_FLAGS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589938", L"ATT_RPC_NS_GROUP"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589939", L"ATT_RPC_NS_INTERFACE_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590136", L"ATT_RPC_NS_OBJECT_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589941", L"ATT_RPC_NS_PRIORITY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589942", L"ATT_RPC_NS_PROFILE_ENTRY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590138", L"ATT_RPC_NS_TRANSFER_SYNTAX"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590045", L"ATT_SAM_ACCOUNT_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590126", L"ATT_SAM_ACCOUNT_TYPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590035", L"ATT_SCHEDULE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589944", L"ATT_SCHEMA_FLAGS_EX"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589972", L"ATT_SCHEMA_ID_GUID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk591182", L"ATT_SCHEMA_INFO"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTl590305", L"ATT_SCHEMA_UPDATE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj131543", L"ATT_SCHEMA_VERSION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591178", L"ATT_SCOPE_FLAGS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589886", L"ATT_SCRIPT_PATH"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591128", L"ATT_SD_RIGHTS_EFFECTIVE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj131406", L"ATT_SEARCH_FLAGS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk14", L"ATT_SEARCH_GUIDE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb1376277", L"ATT_SECRETARY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTr589945", L"ATT_SECURITY_IDENTIFIER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb34", L"ATT_SEE_ALSO"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590328", L"ATT_SEQ_NOTIFICATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTf5", L"ATT_SERIAL_NUMBER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590047", L"ATT_SERVER_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590339", L"ATT_SERVER_REFERENCE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590340", L"ATT_SERVER_REFERENCE_BL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589981", L"ATT_SERVER_ROLE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589978", L"ATT_SERVER_STATE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590334", L"ATT_SERVICE_BINDING_INFORMATION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589946", L"ATT_SERVICE_CLASS_ID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589947", L"ATT_SERVICE_CLASS_INFO"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590333", L"ATT_SERVICE_CLASS_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590481", L"ATT_SERVICE_DNS_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590483", L"ATT_SERVICE_DNS_NAME_TYPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590023", L"ATT_SERVICE_INSTANCE_VERSION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590595", L"ATT_SERVICE_PRINCIPAL_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590149", L"ATT_SETUP_COMMAND"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590439", L"ATT_SHELL_CONTEXT_MENU"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590387", L"ATT_SHELL_PROPERTY_PAGES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591033", L"ATT_SHORT_SERVER_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590468", L"ATT_SHOW_IN_ADDRESS_BOOK"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi131241", L"ATT_SHOW_IN_ADVANCED_VIEW_ONLY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTr590433", L"ATT_SID_HISTORY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590648", L"ATT_SIGNATURE_ALGORITHMS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590186", L"ATT_SITE_GUID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590646", L"ATT_SITE_LINK_LIST"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590645", L"ATT_SITE_LIST"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590336", L"ATT_SITE_OBJECT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590337", L"ATT_SITE_OBJECT_BL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590318", L"ATT_SITE_SERVER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590610", L"ATT_SMTP_MAIL_ADDRESS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm591171", L"ATT_SPN_MAPPINGS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm8", L"ATT_STATE_OR_PROVINCE_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm9", L"ATT_STREET_ADDRESS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTc1572873", L"ATT_STRUCTURAL_OBJECT_CLASS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTc131093", L"ATT_SUB_CLASS_OF"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb131079", L"ATT_SUB_REFS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb1638410", L"ATT_SUBSCHEMASUBENTRY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590535", L"ATT_SUPER_SCOPE_DESCRIPTION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTf590534", L"ATT_SUPER_SCOPES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590356", L"ATT_SUPERIOR_DNS_ROOT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589949", L"ATT_SUPPLEMENTAL_CREDENTIALS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk30", L"ATT_SUPPORTED_APPLICATION_CONTEXT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm4", L"ATT_SURNAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590490", L"ATT_SYNC_ATTRIBUTES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590489", L"ATT_SYNC_MEMBERSHIP"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590488", L"ATT_SYNC_WITH_OBJECT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTr590491", L"ATT_SYNC_WITH_SID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTc590022", L"ATT_SYSTEM_AUXILIARY_CLASS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590199", L"ATT_SYSTEM_FLAGS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTc590020", L"ATT_SYSTEM_MAY_CONTAIN"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTc590021", L"ATT_SYSTEM_MUST_CONTAIN"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi589994", L"ATT_SYSTEM_ONLY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTc590019", L"ATT_SYSTEM_POSS_SUPERIORS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm20", L"ATT_TELEPHONE_NUMBER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk22", L"ATT_TELETEX_TERMINAL_IDENTIFIER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk21", L"ATT_TELEX_NUMBER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590472", L"ATT_TELEX_PRIMARY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb591170", L"ATT_TEMPLATE_ROOTS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590709", L"ATT_TERMINAL_SERVER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131203", L"ATT_TEXT_COUNTRY"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1376258", L"ATT_TEXT_ENCODED_OR_ADDRESS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590327", L"ATT_TIME_REFRESH"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590326", L"ATT_TIME_VOL_CHANGE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm12", L"ATT_TITLE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj131126", L"ATT_TOMBSTONE_LIFETIME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTc590719", L"ATT_TRANSPORT_ADDRESS_ATTRIBUTE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590613", L"ATT_TRANSPORT_DLL_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590615", L"ATT_TRANSPORT_TYPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTi590630", L"ATT_TREAT_AS_LEAF"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590484", L"ATT_TREE_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590294", L"ATT_TRUST_ATTRIBUTES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589953", L"ATT_TRUST_AUTH_INCOMING"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589959", L"ATT_TRUST_AUTH_OUTGOING"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589956", L"ATT_TRUST_DIRECTION"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb590295", L"ATT_TRUST_PARENT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589957", L"ATT_TRUST_PARTNER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589958", L"ATT_TRUST_POSIX_OFFSET"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589960", L"ATT_TRUST_TYPE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589979", L"ATT_UAS_COMPAT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1376257", L"ATT_UID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589961", L"ATT_UNC_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589914", L"ATT_UNICODE_PWD"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1376300", L"ATT_UNIQUEIDENTIFIER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTb50", L"ATT_UNIQUEMEMBER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590637", L"ATT_UPGRADE_PRODUCT_CODE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590714", L"ATT_UPN_SUFFIXES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589832", L"ATT_USER_ACCOUNT_CONTROL"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590469", L"ATT_USER_CERT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589980", L"ATT_USER_COMMENT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589962", L"ATT_USER_PARAMETERS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk35", L"ATT_USER_PASSWORD"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm1376264", L"ATT_USERCLASS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk1442008", L"ATT_USERPKCS12"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590480", L"ATT_USER_PRINCIPAL_NAME"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590575", L"ATT_USER_SHARED_FOLDER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590576", L"ATT_USER_SHARED_FOLDER_OTHER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk1310860", L"ATT_USER_SMIME_CERTIFICATE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm589910", L"ATT_USER_WORKSTATIONS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq131192", L"ATT_USN_CHANGED"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq131091", L"ATT_USN_CREATED"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq131339", L"ATT_USN_DSA_LAST_OBJ_REMOVED"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj131541", L"ATT_USN_INTERSITE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq131193", L"ATT_USN_LAST_OBJ_REM"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTq590720", L"ATT_USN_SOURCE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj591180", L"ATT_VALID_ACCESSES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590079", L"ATT_VENDOR"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj589965", L"ATT_VERSION_NUMBER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590152", L"ATT_VERSION_NUMBER_HI"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590153", L"ATT_VERSION_NUMBER_LO"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590160", L"ATT_VOL_TABLE_GUID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk590158", L"ATT_VOL_TABLE_IDX_GUID"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTj590331", L"ATT_VOLUME_COUNT"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590125", L"ATT_WBEM_PATH"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTh590442", L"ATT_WELL_KNOWN_OBJECTS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTl131075", L"ATT_WHEN_CHANGED"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTl131074", L"ATT_WHEN_CREATED"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk589966", L"ATT_WINSOCK_ADDRESSES"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm131536", L"ATT_WWW_HOME_PAGE"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTm590573", L"ATT_WWW_PAGE_OTHER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTg24", L"ATT_X121_ADDRESS"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk45", L"ATT_X500UNIQUEIDENTIFIER"));
	pAdNameMap->insert(pair<wstring, wstring>(L"ATTk36", L"ATT_X509_CERT"));
}
