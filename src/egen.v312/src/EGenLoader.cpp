/*
*	(c) Copyright 2002-2003, Microsoft Corporation
*	Provided to the TPC under license.
*	Written by Sergey Vasilevskiy.
*/
// 2006 Rilson Nascimento

//
// Defines the entry point for the console application.
//

#include "../inc/EGenLoader_stdafx.h"

using namespace TPCE;

TIdent				iStartFromCustomer = iDefaultStartFromCustomer;
TIdent				iCustomerCount = iDefaultLoadUnitSize;	// # of customers for this instance
TIdent				iTotalCustomerCount = iDefaultLoadUnitSize;	// total number of customers in the database
int					iLoadUnitSize = iDefaultLoadUnitSize;	// # of customers in one load unit
int					iScaleFactor = 500;	// # of customers for 1 tpsE
int					iDaysOfInitialTrades = 50;
int					iHoursOfInitialTrades = HoursPerWorkDay * iDaysOfInitialTrades;
#ifdef COMPILE_ODBC_LOAD
char				szServer[iMaxHostname];
char				szDB[iMaxDBName];
#endif
#ifdef COMPILE_PGSQL_LOAD
char				szHost[iMaxPGHost];
char				szDBName[iMaxPGDBName];
char 				szPostmasterPort[iMaxPGPort];
#endif
// These flags are used to control which tables get generated and loaded.
bool				bTableGenerationFlagNotSpecified = true;	// assume no flag is specified.
bool				bGenerateFixedTables = false;
bool				bGenerateGrowingTables = false;
bool				bGenerateScalingTables = false;


char				szInDir[iMaxPath];
#ifdef COMPILE_FLAT_FILE_LOAD
char				szOutDir[iMaxPath];
FlatFileOutputModes		FlatFileOutputMode;
#endif

enum eLoadImplementation {
	NULL_LOAD = 0,	// no load - generate data only
	FLAT_FILE_LOAD,
	ODBC_LOAD,
	PGSQL_LOAD
};
eLoadImplementation	LoadType = PGSQL_LOAD;

/* 
 * Prints program usage to std error.
 */
void Usage()
{
  fprintf( stderr, "Usage:\n" );
  fprintf( stderr, "EGenLoader [options] \n\n" );
  fprintf( stderr, " Where\n" );
  fprintf( stderr, "  Option                      Default     Description\n" );
  fprintf( stderr, "   -b number                  %" PRId64 "           Beginning customer ordinal position\n", iStartFromCustomer);
  fprintf( stderr, "   -c number                  %" PRId64 "        Number of customers (for this instance)\n", iCustomerCount );
  fprintf( stderr, "   -t number                  %" PRId64 "        Number of customers (total in the database)\n", iTotalCustomerCount );
  fprintf( stderr, "   -f number                  %d          Scale factor (customers per 1 tpsE)\n", iScaleFactor );
  fprintf( stderr, "   -w number                  %d          Number of Workdays (8-hour days) of \n", iDaysOfInitialTrades );
  fprintf( stderr, "                                          initial trades to populate\n" );
  fprintf( stderr, "   -i dir                     flat_in     Directory for input files\n" );
  fprintf( stderr, "   -l [PGSQL|FLAT|ODBC|NULL]  PGSQL       Type of load\n" );
  fprintf( stderr, "   -m [APPEND|OVERWRITE]      OVERWRITE   Flat File output mode\n" );
  fprintf( stderr, "   -o dir                     flat_out    Directory for output files\n" );
  fprintf( stderr, "   -s string                  localhost   Database server\n" );
#ifdef COMPILE_PGSQL_LOAD
  fprintf( stderr, "   -d string                  dbt5        Database name\n" );
  fprintf( stderr, "   -p string                  5432        Postmaster port\n" );
#else
  fprintf( stderr, "   -d string                  tpce        Database name\n" );
#endif
  fprintf( stderr, "\n" );
  fprintf( stderr, "   -x                         -x          Generate all tables\n");
  fprintf( stderr, "   -xf                                    Generate all fixed-size tables\n");
  fprintf( stderr, "   -xd                                    Generate all scaling and growing tables\n");
  fprintf( stderr, "                                          (equivalent to -xs -xg)\n");
  fprintf( stderr, "   -xs                                    Generate scaling tables\n");
  fprintf( stderr, "                                          (except BROKER)\n");
  fprintf( stderr, "   -xg                                    Generate growing tables and BROKER\n");
}

void ParseCommandLine( int argc, char *argv[] )
{
int   arg;
char  *sp;
char  *vp;  

  /*
   *  Scan the command line arguments
   */
  for ( arg = 1; arg < argc; ++arg ) {

    /*
     *  Look for a switch 
     */
    sp = argv[arg];
    if ( *sp == '-' ) {
      ++sp;
    }
    *sp = tolower( *sp );

    /*
     *  Find the switch's argument.  It is either immediately after the
     *  switch or in the next argv
     */
    vp = sp + 1;
	// Allow for switched that don't have any parameters.
	// Need to check that the next argument is in fact a parameter
	// and not the next switch that starts with '-'.
	//
    if ( (*vp == 0) && ((arg + 1) < argc) && (argv[arg + 1][0] != '-') )
	{        
        vp = argv[++arg];
    }

    /*
     *  Parse the switch
     */
    switch ( *sp ) {
	  case 'b':
        sscanf(vp, "%"PRId64, &iStartFromCustomer);
		if (iStartFromCustomer <= 0)
		{	// set back to default
			// either number parsing was unsuccessful
			// or a bad value was specified
			iStartFromCustomer = iDefaultStartFromCustomer;
		}
        break;
      case 'c':
		sscanf(vp, "%"PRId64, &iCustomerCount);
        break;
	  case 't':
		sscanf(vp, "%"PRId64, &iTotalCustomerCount);
        break;
	  case 'f':
	    iScaleFactor = atoi( vp );
		break;
	  case 'w':
        iHoursOfInitialTrades = HoursPerWorkDay * atoi( vp );
		break;

	  case 'i':	// Location of input files.
		  strncpy(szInDir, vp, sizeof(szInDir)-1);
		  if(( '/' != szInDir[ strlen(szInDir) - 1 ] ) && ( '\\' != szInDir[ strlen(szInDir) - 1 ] ))
		  {
			  strcat( szInDir, "/" );
		  }
		  break;

	  case 'l':	// Load type.
		  if( 0 == strcmp( vp, "PGSQL" ))
		  {
			  LoadType = PGSQL_LOAD;
		  }
		  else
		  {
			if( 0 == strcmp( vp, "FLAT" ))
			{
				LoadType = FLAT_FILE_LOAD;
			}
			else 
			{
				if( 0 == strcmp( vp, "ODBC" ))
				{
					LoadType = ODBC_LOAD;
				}
				else
				{
					if ( 0 == strcmp( vp, "NULL" ))
					{
						LoadType = NULL_LOAD;
					}
					else
					{
						Usage();
						exit( ERROR_BAD_OPTION );
					}
				}
			}
		  }
		  break;

#ifdef COMPILE_FLAT_FILE_LOAD
	  case 'm':	// Mode for output of flat files.
		  if( 0 == strcmp( vp, "APPEND" ))
		  {
			  FlatFileOutputMode = FLAT_FILE_OUTPUT_APPEND;
		  }
		  else if( 0 == strcmp( vp, "OVERWRITE" ))
		  {
			  FlatFileOutputMode = FLAT_FILE_OUTPUT_OVERWRITE;
		  }
		  else
		  {
			Usage();
			exit( ERROR_BAD_OPTION );
		  }
		  break;

	  case 'o':	// Location for output files.
		  strncpy(szOutDir, vp, sizeof(szOutDir)-1);
		  if(( '/' != szOutDir[ strlen(szOutDir) - 1 ] ) && ( '\\' != szOutDir[ strlen(szOutDir) - 1 ] ))
		  {
			  strcat( szOutDir, "/" );
		  }
		  break;
#endif
#ifdef COMPILE_ODBC_LOAD
	  case 's':	// Database server name.
		  strncpy(szServer, vp, sizeof(szServer));
		  break;

	  case 'd':	// Database name.
		  strncpy(szDB, vp, sizeof(szDB));
		  break;
#endif
#ifdef COMPILE_PGSQL_LOAD
	  case 's':	// Database host name.
		  strncpy(szHost, vp, sizeof(szHost));
		  break;

	  case 'd':	// Database name.
		  strncpy(szDBName, vp, sizeof(szDBName));
		  break;

	  case 'p':     // Postmaster port
		  strncpy(szPostmasterPort, vp, sizeof(szPostmasterPort));
		  break;
#endif
	  case 'x':	// Table Generation
		  bTableGenerationFlagNotSpecified = false;	//A -x flag has been used

		  if( NULL == *vp )
		  {
			  bGenerateFixedTables = true;
			  bGenerateGrowingTables = true;
			  bGenerateScalingTables = true;
		  }
		  else if( 0 == strcmp( vp, "f" ))
		  {
			  bGenerateFixedTables = true;
		  }
		  else if( 0 == strcmp( vp, "g" ))
		  {
			  bGenerateGrowingTables = true;
		  }
		  else if( 0 == strcmp( vp, "s" ))
		  {
			  bGenerateScalingTables = true;
		  }
		  else if( 0 == strcmp( vp, "d" ))
		  {
			  bGenerateGrowingTables = true;
			  bGenerateScalingTables = true;
		  }
		  else
		  {
			  Usage();
			  exit( ERROR_BAD_OPTION );
		  }
		  break;

      default:
        Usage();
		fprintf( stderr, "Error: Unrecognized option: %s\n",sp);
        exit( ERROR_BAD_OPTION );
    }
  }

}

/*
* This function validates EGenLoader parameters that may have been
* specified on the command line. It's purpose is to prevent passing of 
* parameter values that would cause the loader to produce incorrect data.
*
* Condition: needs to be called after ParseCommandLine.
*
* PARAMETERS:
*		NONE
*
* RETURN: 
*		true					- if all parameters are valid
*		false					- if at least one parameter is invalid
*/
bool ValidateParameters()
{
	bool bRet = true;

	// Starting customer must be a non-zero integral multiple of load unit size + 1.
	//
	if ((iStartFromCustomer % iLoadUnitSize) != 1)
	{
		cout << "The specified starting customer (-b " << iStartFromCustomer 
			<< ") must be a non-zero integral multiple of the load unit size (" 
			<< iLoadUnitSize << ") + 1." << endl;

		bRet = false;
	}

	// Total number of customers in the database cannot be less
	// than the number of customers for this loader instance
	//
	if (iTotalCustomerCount < iStartFromCustomer + iCustomerCount - 1)
	{
		iTotalCustomerCount = iStartFromCustomer + iCustomerCount - 1;
	}

	// Customer count must be a non-zero integral multiple of load unit size.
	//
	if ((iLoadUnitSize > iCustomerCount) || (0 != iCustomerCount % iLoadUnitSize))
	{
		cout << "The specified customer count (-c " << iCustomerCount 
			<< ") must be a non-zero integral multiple of the load unit size (" 
			<< iLoadUnitSize << ")." << endl;

		bRet = false;
	}

	// Total customer count must be a non-zero integral multiple of load unit size.
	//
	if ((iLoadUnitSize > iTotalCustomerCount) || (0 != iTotalCustomerCount % iLoadUnitSize))
	{
		cout << "The total customer count (-t " << iTotalCustomerCount 
			<< ") must be a non-zero integral multiple of the load unit size (" 
			<< iLoadUnitSize << ")." << endl;

		bRet = false;
	}	

	// Completed trades in 8 hours must be a non-zero integral multiple of 100
	// so that exactly 1% extra trade ids can be assigned to simulate aborts.
	//
	if ((INT64)(HoursPerWorkDay * SecondsPerHour * iLoadUnitSize / iScaleFactor) % 100 != 0)
	{
		cout << "Incompatible value for Scale Factor (-f) specified." << endl;
		cout << HoursPerWorkDay << " * " << SecondsPerHour << " * Load Unit Size (" << iLoadUnitSize 
			<< ") / Scale Factor (" << iScaleFactor 
			<< ") must be integral multiple of 100." << endl;

		bRet = false;
	}

	if (iHoursOfInitialTrades <= 0) 
	{
		cout << "The specified number of 8-Hour Workdays (-w " 
			<< (iHoursOfInitialTrades/HoursPerWorkDay) << ") must be non-zero." << endl;

		bRet = false;
	}

	return bRet;
}

/*
*	Create loader class factory corresponding to the
*	selected load type.
*/
CBaseLoaderFactory* CreateLoaderFactory(eLoadImplementation eLoadType)
{
	switch (eLoadType)
	{
	case NULL_LOAD:
		return new CNullLoaderFactory();

#ifdef COMPILE_ODBC_LOAD
	case ODBC_LOAD:

		return new CODBCLoaderFactory(szServer, szDB);

#endif	// #ifdef COMPILE_ODBC_LOAD

#ifdef COMPILE_FLAT_FILE_LOAD
	case FLAT_FILE_LOAD:
		
		return new CFlatLoaderFactory(szOutDir, FlatFileOutputMode);
		
#endif	//#ifdef COMPILE_FLAT_FILE_LOAD

#ifdef COMPILE_PGSQL_LOAD
	case PGSQL_LOAD:
		
		return new CPGSQLLoaderFactory(szHost, szDBName, szPostmasterPort);
		
#endif //#ifdef COMPILE_PGSQL_LOAD

	default:
		assert(false);	// this should never happen
	}

	return NULL;	//should never happen
}

int main(int argc, char* argv[])
{
	CInputFiles						inputFiles;	
	CBaseLoaderFactory*				pLoaderFactory;	// class factory that creates table loaders
	CGenerateAndLoadStandardOutput	Output;
	CGenerateAndLoad*				pGenerateAndLoad;
	CDateTime						StartTime, EndTime, LoadTime;	// to time the database load
	
	// Output EGen version
	cout<<"\t\t\t\t";
	PrintEGenVersion();	

	// Establish defaults for command line options.
#ifdef COMPILE_ODBC_LOAD
	strncpy(szServer, "localhost", sizeof(szServer)-1);
	strncpy(szDB, "tpce", sizeof(szDB)-1);
#endif
#ifdef COMPILE_PGSQL_LOAD
	strncpy(szHost, "localhost", sizeof(szHost)-1);
	strncpy(szDBName, "dbt5", sizeof(szDBName)-1);
#endif
	strncpy(szInDir, "flat_in/", sizeof(szInDir)-1);
#ifdef COMPILE_FLAT_FILE_LOAD
	strncpy(szOutDir, "flat_out/", sizeof(szOutDir)-1);
	FlatFileOutputMode = FLAT_FILE_OUTPUT_OVERWRITE;
#endif

	// Get command line options.
	//
	ParseCommandLine(argc, argv);

	// Validate global parameters that may have been modified on
	// the command line.
	//
	if (!ValidateParameters())
	{
		return ERROR_INVALID_OPTION_VALUE;	// exit from the loader returning a non-zero code
	}	

	// Let the user know what settings will be used.
	cout<<endl<<endl<<"Using the following settings."<<endl<<endl;
	switch ( LoadType ) {
#ifdef COMPILE_ODBC_LOAD
	case ODBC_LOAD:
		cout<<"\tLoad Type:\t\tODBC"<<endl;
		cout<<"\tServer Name:\t\t"<<szServer<<endl;
		cout<<"\tDatabase:\t\t"<<szDB<<endl;
		break;
#endif
#ifdef COMPILE_FLAT_FILE_LOAD
	case FLAT_FILE_LOAD:
		cout<<"\tLoad Type:\t\tFlat File"<<endl;
		switch ( FlatFileOutputMode ) {
		case FLAT_FILE_OUTPUT_APPEND:
			cout<<"\tOutput Mode:\t\tAPPEND"<<endl;
			break;
		case FLAT_FILE_OUTPUT_OVERWRITE:
			cout<<"\tOutput Mode:\t\tOVERWRITE"<<endl;
			break;
		}
		cout<<"\tOut Directory:\t\t"<<szOutDir<<endl;
		break;
#endif
#ifdef COMPILE_PGSQL_LOAD
	case PGSQL_LOAD:
		cout<<"\tLoad Type:\t\tPGSQL"<<endl;
		cout<<"\tHost:\t\t"<<szHost<<endl;
		cout<<"\tDatabase:\t\t"<<szDBName<<endl;
		cout<<"\tPort:\t\t"<<szPostmasterPort<<endl;
		break;
#endif
	case NULL_LOAD:
	default:
		cout<<"\tLoad Type:\t\tNull load (generation only)"<<endl;
		break;
	}
	cout<<"\tIn Directory:\t\t"<<szInDir<<endl;
	cout<<"\tStart From Customer:\t"<<iStartFromCustomer<<endl;
	cout<<"\tCustomer Count:\t\t"<<iCustomerCount<<endl;
	cout<<"\tTotal customers:\t"<<iTotalCustomerCount<<endl;
	cout<<"\tLoad Unit:\t\t"<<iLoadUnitSize<<endl;	
	cout<<"\tScale Factor:\t\t"<<iScaleFactor<<endl;
	cout<<"\tHours Of Trades:\t"<<iHoursOfInitialTrades<<endl;
	cout<<endl<<endl;
	
	// Know the load type => create the loader factory.
	pLoaderFactory = CreateLoaderFactory(LoadType);

	try {
		// Load all of the input files into memory.
		inputFiles.Initialize(eDriverEGenLoader, iTotalCustomerCount, iTotalCustomerCount, szInDir);

		// Create the main class instance
		pGenerateAndLoad = new CGenerateAndLoad(inputFiles, iCustomerCount, iStartFromCustomer, 
												iTotalCustomerCount, iLoadUnitSize,
												iScaleFactor, iHoursOfInitialTrades,
												pLoaderFactory,	&Output, szInDir);

		//	The generate and load phase starts here.
		//
		StartTime.SetToCurrent();

		// Generate static tables if appropriate.
		if(( bTableGenerationFlagNotSpecified && ( iStartFromCustomer == iDefaultStartFromCustomer )) || bGenerateFixedTables )
		{
			pGenerateAndLoad->GenerateAndLoadFixedTables();			
		}

 		// Generate dynamic scaling tables if appropriate.
 		if( bTableGenerationFlagNotSpecified || bGenerateScalingTables )
 		{
 			pGenerateAndLoad->GenerateAndLoadScalingTables();
 		}

		// Generate dynamic trade tables if appropriate.
		if( bTableGenerationFlagNotSpecified || bGenerateGrowingTables )
		{			
			pGenerateAndLoad->GenerateAndLoadGrowingTables();
		}

		//	The generate and load phase ends here.
		//
		EndTime.SetToCurrent();

		LoadTime.Set(0);	// clear time
		LoadTime.Add(0, (int)((EndTime - StartTime) * MsPerSecond));	// add ms

		cout<<endl<<"Generate and load time: "<<LoadTime.ToStr(01)<<endl<<endl;

		delete pGenerateAndLoad;	// don't really need to do that, but just for good style
		delete pLoaderFactory;		// don't really need to do that, but just for good style

	}
	catch (CBaseErr *pErr)
	{
		cout<<endl<<"Error "<<pErr->ErrorNum()<<": "<<pErr->ErrorText();
		if (pErr->ErrorLoc()) {
			cout<<" at "<<pErr->ErrorLoc()<<endl;
		} else {
			cout<<endl;
		}
		return 1;
	}
	// operator new will throw std::bad_alloc exception if there is no sufficient memory for the request.
	//
	catch (std::bad_alloc err)
	{
		cout<<endl<<endl<<"*** Out of memory ***"<<endl;
		return 2;
	}

	return 0;
}