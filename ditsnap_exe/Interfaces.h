#pragma once

class CMainFrame;

class ITableObserver
{
public:
	virtual ~ITableObserver()
	{
	};

	virtual void LoadEseTable() = 0;
};

class IDbObserver
{
public:
	virtual ~IDbObserver()
	{
	};

	virtual void LoadEseDbManager() = 0;
};

class ITableObservable
{
public:
	virtual ~ITableObservable()
	{
	};

	virtual void RegisterTableObserver(ITableObserver* o) = 0;
	virtual void RemoveTableObserver(ITableObserver* o) = 0;
	virtual void NotifyTableObservers() = 0;
};

class IDbObservable
{
public:
	virtual ~IDbObservable()
	{
	};
	
	virtual void RegisterDbObserver(IDbObserver* o) = 0;
	virtual void RemoveDbObserver(IDbObserver* o) = 0;
	virtual void NotifyDbObservers() = 0;
};