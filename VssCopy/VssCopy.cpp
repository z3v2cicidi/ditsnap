#include "stdafx.h"
#include "VssCopy.h"
#include <winbase.h>

namespace Vss
{
#define CHECK_HRESULT(x) { HRESULT ckhr = ((x)); if (FAILED(ckhr)) AtlThrow(ckhr); }

	VssCopy::~VssCopy()
	{
		//Free resource allocated by GetSnapshotProperties()
		if (snapshotProperties_ != nullptr)
		{
			VssFreeSnapshotProperties(snapshotProperties_);
			delete snapshotProperties_;
		}

		//Free resource allocated by SetBackupState()
		if (backupState_)
		{
			try
			{
				CComPtr<IVssAsync> pBackupCompleteResults;
				CHECK_HRESULT( pBackupComponents_->BackupComplete(&pBackupCompleteResults) );
				WaitAndQueryStatus(pBackupCompleteResults);

				CHECK_HRESULT( pBackupComponents_->FreeWriterMetadata() );
			}
			catch (CAtlException&)
			{
			}
		}

		CoUninitialize();
	}

	void VssCopy::Init()
	{
		CHECK_HRESULT( ::CoInitialize(NULL) );

		CHECK_HRESULT( ::CreateVssBackupComponents(&pBackupComponents_) );
		CHECK_HRESULT( pBackupComponents_->InitializeForBackup() );

		CComPtr<IVssAsync> pWriterMetadataStatus;
		CHECK_HRESULT( pBackupComponents_->GatherWriterMetadata(&pWriterMetadataStatus) );
		WaitAndQueryStatus(pWriterMetadataStatus);

		GUID snapshotSetId = GUID_NULL;
		CHECK_HRESULT( pBackupComponents_->StartSnapshotSet(&snapshotSetId) );

		wchar_t volumePathName[MAX_PATH];
		if (! ::GetVolumePathName(sourcePath_, volumePathName, MAX_PATH))
		{
			AtlThrowLastWin32();
		}

		GUID snapshotId;
		CHECK_HRESULT( pBackupComponents_->AddToSnapshotSet(volumePathName, GUID_NULL, &snapshotId) );
		CHECK_HRESULT( pBackupComponents_->SetBackupState(TRUE, FALSE, VSS_BT_FULL) );

		CComPtr<IVssAsync> pPrepareForBackupResults;
		CHECK_HRESULT( pBackupComponents_->PrepareForBackup(&pPrepareForBackupResults) );
		WaitAndQueryStatus(pPrepareForBackupResults);
		backupState_ = TRUE;

		CComPtr<IVssAsync> pDoSnapshotSetResults;
		CHECK_HRESULT( pBackupComponents_->DoSnapshotSet(&pDoSnapshotSetResults) );
		WaitAndQueryStatus(pDoSnapshotSetResults);

		snapshotProperties_ = new VSS_SNAPSHOT_PROP;
		CHECK_HRESULT( pBackupComponents_->GetSnapshotProperties(snapshotId, snapshotProperties_) );

		snapshotDeviceObject_ = snapshotProperties_->m_pwszSnapshotDeviceObject;

		return;
	}


	void VssCopy::CopyFileFromSnapshot()
	{
		wchar_t volumePathName[MAX_PATH];
		if (! ::GetVolumePathName(sourcePath_, volumePathName, MAX_PATH))
		{
			AtlThrowLastWin32();
		}

		CString source(sourcePath_);
		CString volumeString(volumePathName);
		CString subPath = source.Mid(volumeString.GetLength());

		CString snapshotSourcePath(snapshotDeviceObject_);
		snapshotSourcePath.Append(L"\\");
		snapshotSourcePath.Append(subPath);

		if (! ::CopyFile(snapshotSourcePath, destinationPath_, FALSE))
		{
			AtlThrowLastWin32();
		}

		return;
	}

	void VssCopy::WaitAndQueryStatus(CComPtr<IVssAsync> pVssAsync)
	{
		CHECK_HRESULT( pVssAsync->Wait() );

		HRESULT hr;
		CHECK_HRESULT( pVssAsync->QueryStatus(&hr, nullptr) );

		if (hr == VSS_S_ASYNC_CANCELLED)
		{
			AtlThrow(hr);
		}

		return;
	}


	HRESULT CopyFileFromSnapshot(const wchar_t* sourcePath, const wchar_t* destinationPath)
	{
		wchar_t expandedSourcePath[MAX_PATH];
		wchar_t expandedDestinationPath[MAX_PATH];

		try
		{
			if (0 == ::ExpandEnvironmentStrings(sourcePath, expandedSourcePath, MAX_PATH))
			{
				throw CAtlException(GetLastError());
			}

			if (0 == ::ExpandEnvironmentStrings(destinationPath, expandedDestinationPath, MAX_PATH))
			{
				throw CAtlException(GetLastError());
			}

			VssCopy vss(expandedSourcePath, expandedDestinationPath);
			vss.Init();
			vss.CopyFileFromSnapshot();
		}
		catch (CAtlException& e)
		{
			return static_cast<HRESULT>(e);
		}

		return S_OK;
	}
} // namespace

