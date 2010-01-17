#include "stdafx.h"
#include "WinUtil.h"

namespace WinUtil {

WinVersion GetWinVersion() 
{
	static bool checkedVersion = false;
	static WinVersion winVersion = WINVERSION_PRE_2000;
	if (!checkedVersion) 
	{
		OSVERSIONINFOEX versionInfo;
		versionInfo.dwOSVersionInfoSize = sizeof versionInfo;
		::GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&versionInfo));
		if (versionInfo.dwMajorVersion == 5) 
		{
			switch (versionInfo.dwMinorVersion) 
			{
			case 0:
				winVersion = WINVERSION_2000;
				break;
			case 1:
				winVersion = WINVERSION_XP;
				break;
			case 2:
			default:
				winVersion = WINVERSION_SERVER_2003;
				break;
			}
		} 
		else if (versionInfo.dwMajorVersion >= 6) 
		{
			if (versionInfo.wProductType == VER_NT_WORKSTATION)
			{
				winVersion = WINVERSION_VISTA; 
			}
			else
			{
				winVersion = WINVERSION_SERVER_2008;
			}
		}
		checkedVersion = true;
	}
	return winVersion;
}

}  // namespace WinUtil