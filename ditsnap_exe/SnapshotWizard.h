#pragma once
#include "stdafx.h"
#include "resource.h"

class CSnapshotWizardPage1 : public CPropertyPageImpl<CSnapshotWizardPage1>
{
public:
	enum
	{
		IDD = IDD_SCWIZARD1
	};

	explicit CSnapshotWizardPage1(_U_STRINGorID title = static_cast<LPCTSTR>(nullptr))
		: CPropertyPageImpl<CSnapshotWizardPage1>(title)
	{
	}

	BEGIN_MSG_MAP(CSnapshotWizardPage1)
		MSG_WM_INITDIALOG(OnInitDialog)
		CHAIN_MSG_MAP(CPropertyPageImpl<CSnapshotWizardPage1>)
	END_MSG_MAP()

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	LRESULT OnWizardNext();

	void SetSharedString(wchar_t* sharedStringPage1)
	{
		sharedStringPage1_ = sharedStringPage1;
	}

private:
	CEdit sourceEdit_;
	CEdit destinationEdit_;
	wchar_t* sharedStringPage1_;

	BOOL InvokeEsentutilP(const wchar_t* targetDbPath);
};

class CSnapshotWizardPage2 : public CPropertyPageImpl<CSnapshotWizardPage2>
{
public:
	enum
	{
		IDD = IDD_SCWIZARD2
	};

	explicit CSnapshotWizardPage2(_U_STRINGorID title = static_cast<LPCTSTR>(nullptr))
		: CPropertyPageImpl<CSnapshotWizardPage2>(title)
	{
	}

	BEGIN_MSG_MAP(CSnapshotWizardPage2)
		CHAIN_MSG_MAP(CPropertyPageImpl<CSnapshotWizardPage2>)
	END_MSG_MAP()

	BOOL OnSetActive();

	void SetSharedString(wchar_t* sharedStringPage2)
	{
		sharedStringPage2_ = sharedStringPage2;
	}

private:
	wchar_t* sharedStringPage2_;
};

class CSnapshotWizard : public CPropertySheetImpl<CSnapshotWizard>
{
public:
	CSnapshotWizard(_U_STRINGorID title = static_cast<LPCTSTR>(nullptr),
	                UINT uStartPage = 0, HWND hWndParent = nullptr);

	~CSnapshotWizard()
	{
		delete sharedString_;
	}

	BEGIN_MSG_MAP(CSnapshotWizard)
		CHAIN_MSG_MAP(CPropertySheetImpl<CSnapshotWizard>)
	END_MSG_MAP()

	//It is the client's responsibility that the return string should be deleted.
	wchar_t* GetSnapshotFilePath()
	{
		return sharedString_;
	}

private:
	CSnapshotWizardPage1 page1_;
	CSnapshotWizardPage2 page2_;
	wchar_t* sharedString_;
};
