#DIT Snapshot Viewer
DIT Snapshot Viewer is an inspection tool for Active Directory database, ntds.dit. This tool connects to ESE(Extensible Storage Engine) and read tables/records including hidden objects by [low level C API](https://msdn.microsoft.com/en-us/library/gg269259%28v=exchg.10%29.aspx).

Additionally the tool can extract ntds.dit without stopping lsass.exe. Snapshot wizard copies ntds.dit using VSS(Volume Shadow Copy Service). As copying ntds.dit may cause inconsistency state in ESE DB, the wizard automatically runs __esentutil /repair__ command to fix the inconsistency.

Filtering feature is available because datatable can have a huge number of records. You can filter out metadata like schema objects by Tools->Filter.

EseDataAccess static library can be used for other ESE inspection applications. It has C++ object-oriented representation of ESE C API. For example, ESE table is represented by EseTable class defined as below.
```C++
class EseTable
{
	public:
		EseTable(const EseDatabase* const eseDatabase, const string& tableName);
		~EseTable();
		void Init();
		void MoveFirstRecord();
		BOOL MoveNextRecord();
		void Move(uint rowIndex);
		int CountColumnValue(uint columnIndex) const;
		wstring RetrieveColumnDataAsString(uint columnIndex, uint itagSequence = 1);
		uint GetColumnCount() const;
		wstring GetColumnName(uint columnIndex) const;
}
```
Prebuild executable is available here.
[Download ditsnap.exe](https://github.com/yosqueoy/ditsnap/blob/master/x64/Release/ditsnap.exe?raw=true)

##Main Window
![screenshot1](https://raw.githubusercontent.com/yosqueoy/ditsnap/master/images/screenshot1.png)
##Detail Dialog
![screenshot2](https://raw.githubusercontent.com/yosqueoy/ditsnap/master/images/screenshot2.png)

##Build Environment
Visual C++ 2013
