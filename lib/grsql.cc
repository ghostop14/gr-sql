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
	std::cout << "SELECT [* | I | Q | TIMELENGTH] FROM '<source file>' ASDATATYPE [COMPLEX | REAL | FLOAT | INT | BYTE | HACKRF (alias for SIGNED8) | RTLSDR (alias for UNSIGNED8) | SIGNED8 | UNSIGNED8] SAMPLERATE <sps [ex: 10000000]> " <<
			     "[STARTATSAMPLE <sample #> ENDATSAMPLE <sample #>] | [STARTATTIMEOFFSET <hh:mm:ss.ms> | <time as float_sec> ENDATTIMEOFFSET <hh:mm:ss.ms> | <time as float_sec>] [SAVEAS <filename> saveas is not required for TIMELENGTH]" << std::endl;
	std::cout << std::endl;
	std::cout << "Examples: " << std::endl;
	std::cout << "Get total time length of a file given its type and sample rate:" << std::endl;
	std::cout << "grsql \"select TIMELENGTH FROM '/tmp/myrecording_593MHz_6.2MSPS.raw' ASDATATYPE complex SAMPLERATE 6.2M\"" << std::endl;
	std::cout << std::endl;
	std::cout << "Select the recording sample from 45.2 to 80.0 seconds into the recording and save it in a new file:" << std::endl;
	std::cout << "grsql \"SELECT * FROM '/tmp/myrecording_593MHz_6.2MSPS.raw' ASDATATYPE complex SAMPLERATE 6.2M STARTTIME 45.2 ENDTIME 80.0 SAVEAS '/tmp/extracted.raw'\"" << std::endl;
	std::cout << std::endl;
	std::cout << "Select just the I channel from the entire complex stream:" << std::endl;
	std::cout << "grsql \"SELECT I FROM '/tmp/myrecording_593MHz_6.2MSPS.raw' ASDATATYPE complex SAMPLERATE 6.2M STARTTIME 0.0 SAVEAS '/tmp/extracted.raw'\"" << std::endl;
	std::cout << std::endl;
	std::cout << "Convert entire hackrf_transfer (signed 8-bit format) to gnuradio float IQ stream:" << std::endl;
	std::cout << "grsql \"SELECT * FROM '/tmp/myrecording.raw' ASDATATYPE HACKRF SAMPLERATE 6.2M STARTTIME 0.0 SAVEAS '/tmp/extracted.raw'\"" << std::endl;
	std::cout << std::endl;
	std::cout << "Convert entire rtl_sdr (unsigned 8-bit format) to gnuradio float IQ stream:" << std::endl;
	std::cout << "grsql \"SELECT * FROM '/tmp/myrecording.raw' ASDATATYPE RTLSDR SAMPLERATE 6.2M STARTTIME 0.0 SAVEAS '/tmp/extracted.raw'\"" << std::endl;
	std::cout << std::endl;
	std::cout << "Note: for hackrf/rtlsdr in the gnuradio flowgraph block you can go straight from the signed/unsigned file to output complex to save the conversion step.  ";
	std::cout << "Also, because on hackrf/rtlsdr processing each sample needs to be processed, expect this to take some time to run through."<< std::endl;
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



