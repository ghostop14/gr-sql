/*
 * grsql.cc
 *
 *  Created on: Sep 17, 2017
 *      Author: Michael Piscopo
 */

#include <iostream>
#include <string.h>

#include "sqlsource_impl.h"
using namespace gr::sql;

void displayHelp() {
	std::cout << std::endl;
	std::cout << "Usage: <grsql string>" << std::endl;
	std::cout << "grsql string syntax:" << std::endl;
	std::cout << "SELECT [* | I | Q | TIMELENGTH] FROM '<source file>' ASDATATYPE [COMPLEX | REAL | FLOAT | INT | BYTE] SAMPLERATE <sps [ex: 10000000]> " <<
			     "[STARTATSAMPLE <sample #> ENDATSAMPLE <sample #>] | [STARTATTIMEOFFSET <hh:mm:ss.ms> | <time as float_sec> ENDATTIMEOFFSET <hh:mm:ss.ms> | <time as float_sec>] [SAVEAS <filename> saveas is not required for TIMELENGTH]" << std::endl;
	std::cout << std::endl;
}

int
main (int argc, char **argv)
{
	if (argc > 1) {
		// 1 is the file name
		if (strcmp(argv[1],"--help")==0) {
			displayHelp();
			exit(0);
		}

		std::string sqlstring = argv[1];
		std::cout << "Running with SQL string:" << std::endl << sqlstring << std::endl;

		sqlsource_impl sqlsrc(sqlstring.c_str());

		sqlsrc.runsql();

	}
	else {
		displayHelp();
		exit(1);
	}

	return 0;
}



