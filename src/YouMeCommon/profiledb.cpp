#include "profiledb.h"
#include <Sqlite3/sqlite3.h>
#include <XFile.h>
CProfileDB::CProfileDB(const XString& strDBPath)
{
	//读取数据库
	m_sqliteDb.Open(strDBPath.c_str());

	for (int i = 0; i < sizeof(szProfileCreateTableSQL) / sizeof(szProfileCreateTableSQL[0]); i++)
	{
		if (!m_sqliteDb.IsTableExist(s_szProfileTableName[i].c_str()))
		{
			youmecommon::CSqliteOperator	sqliteOperator(m_sqliteDb);
			sqliteOperator.PrepareSQL(szProfileCreateTableSQL[i].c_str());
			sqliteOperator.Execute();
		}
	}
}


bool CProfileDB::setSetting(const std::string& strKey, const XString& strValue)
{
	XString strNowValue;
	bool bExist = getSetting(strKey, strNowValue);
	std::lock_guard<std::mutex> lock(m_utex);
	youmecommon::CSqliteOperator	sqliteOperator(m_sqliteDb);
	if (!bExist)
	{
		//插入
		XString strSql = __XT("insert into settings values(?1,?2)");
		sqliteOperator.PrepareSQL(strSql);
		sqliteOperator << UTF8TOXString(strKey);
		sqliteOperator << strValue;
	}
	else
	{
		XString strSql = __XT("update settings set value=?1 where key=?2");
		sqliteOperator.PrepareSQL(strSql);		
		sqliteOperator << strValue;
		sqliteOperator << UTF8TOXString(strKey);
	}
	return sqliteOperator.Execute();
}

bool CProfileDB::deleteByKey(const std::string& strKey)
{
	std::lock_guard<std::mutex> lock(m_utex);
	youmecommon::CSqliteOperator sqliteOperator(m_sqliteDb);
	XString strSql = __XT("delete from settings where key=?1");
	sqliteOperator.PrepareSQL(strSql);
	sqliteOperator << UTF8TOXString(strKey);
	sqliteOperator.Execute();
	return sqliteOperator.Execute();
}

bool CProfileDB::getSetting(const std::string& strKey, XString& strValue)
{
	std::lock_guard<std::mutex> lock(m_utex);
	youmecommon::CSqliteOperator sqliteOperator(m_sqliteDb);
	XString strSql = __XT("select value from settings where key=?1");
	sqliteOperator.PrepareSQL(strSql);
	sqliteOperator << UTF8TOXString(strKey);
	sqliteOperator.Execute();
	while (sqliteOperator.Next())
	{
		sqliteOperator >> strValue;
		return true;
	}

	return false;
}
