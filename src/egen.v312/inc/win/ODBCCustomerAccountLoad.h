/*
*	(c) Copyright 2002-2003, Microsoft Corporation
*	Provided to the TPC under license.
*	Written by Sergey Vasilevskiy.
*/


/*
*	Database loader class for CUSTOMER_ACCOUNT table.
*/
#ifndef ODBC_CUSTOMER_ACCOUNT_LOAD_H
#define ODBC_CUSTOMER_ACCOUNT_LOAD_H

namespace TPCE
{

class CODBCCustomerAccountLoad : public CDBLoader <CUSTOMER_ACCOUNT_ROW>
{	
public:
	CODBCCustomerAccountLoad(char *szServer, char *szDatabase, char *szTable = "CUSTOMER_ACCOUNT")
		: CDBLoader<CUSTOMER_ACCOUNT_ROW>(szServer, szDatabase, szTable)
	{
	};

	virtual void BindColumns()
	{
		//Binding function we have to implement.
		int i = 0;
		if (   bcp_bind(m_hdbc, (BYTE *) &m_row.CA_ID, 0, SQL_VARLEN_DATA, NULL, 0, IDENT_BIND, ++i) != SUCCEED
			|| bcp_bind(m_hdbc, (BYTE *) &m_row.CA_B_ID, 0, SQL_VARLEN_DATA, NULL, 0, IDENT_BIND, ++i) != SUCCEED
			|| bcp_bind(m_hdbc, (BYTE *) &m_row.CA_C_ID, 0, SQL_VARLEN_DATA, NULL, 0, IDENT_BIND, ++i) != SUCCEED
			|| bcp_bind(m_hdbc, (BYTE *) &m_row.CA_NAME, 0, SQL_VARLEN_DATA, (BYTE *)"", 1, SQLCHARACTER, ++i) != SUCCEED
			|| bcp_bind(m_hdbc, (BYTE *) &m_row.CA_TAX_ST, 0, SQL_VARLEN_DATA, NULL, 0, SQLINT1, ++i) != SUCCEED
			|| bcp_bind(m_hdbc, (BYTE *) &m_row.CA_BAL, 0, SQL_VARLEN_DATA, NULL, 0, SQLFLT8, ++i) != SUCCEED
			)
			ThrowError(CODBCERR::eBcpBind);

		if ( bcp_control(m_hdbc, BCPHINTS, "ORDER (CA_ID)" ) != SUCCEED )	
			ThrowError(CODBCERR::eBcpControl);
	};

};

}	// namespace TPCE

#endif //ODBC_CUSTOMER_ACCOUNT_LOAD_H