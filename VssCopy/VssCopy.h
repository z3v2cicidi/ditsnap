#include "VSSSDK72/inc/win2003/vss.h"
#include "VSSSDK72/inc/win2003/vswriter.h"
#include "VSSSDK72/inc/win2003/vsbackup.h"

namespace Vss
{

class VssCopy
{
public:
	VssCopy(const wchar_t* sourcePath, const wchar_t* destinationPath) : 
	  sourcePath_(sourcePath), destinationPath_(destinationPath), snapshotDeviceObject_(NULL), 
	  snapshotProperties_(NULL), backupState_(FALSE) {};
	~VssCopy();
	
	//Call this order
	void Init();
	void CopyFileFromSnapshot();

	//Accessors
	const wchar_t* GetSourcePath(){ return sourcePath_; };
	const wchar_t* GetDestinationPath(){ return destinationPath_; };

private:
	const wchar_t* sourcePath_;
	const wchar_t* destinationPath_;
	ATL::CComPtr<IVssBackupComponents> pBackupComponents_;
	wchar_t* snapshotDeviceObject_;
	VSS_SNAPSHOT_PROP* snapshotProperties_;
	bool backupState_;

	void WaitAndQueryStatus(ATL::CComPtr<IVssAsync> pVssAsync);
};

HRESULT CopyFileFromSnapshot(const wchar_t* sourcePath, const wchar_t* destinationPath);

}// namespace