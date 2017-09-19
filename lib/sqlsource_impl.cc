/* -*- c++ -*- */
/* 
 * Copyright 2017 ghostop14.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "sqlsource_impl.h"
#include <regex>
#include <boost/algorithm/string.hpp>
#include <sys/stat.h>
#include <iostream>
#include <fstream>

using namespace std; // for regex ease

namespace gr {
  namespace sql {

#define SELECT_UNKNOWN 0
#define SELECT_STAR 1
#define SELECT_I 2
#define SELECT_Q 3
#define SELECT_TIMELENGTH 4


    sqlsource::sptr
    sqlsource::make(const char *sqlstring, int igrcdatatype)
    {
      int dsize=0;

      switch(igrcdatatype) {
      case DATATYPE_COMPLEX:
    	  dsize = 8;
      break;

      case DATATYPE_FLOAT:
    	  dsize = 4;
      break;

      case DATATYPE_INT:
    	  dsize = 4;
      break;

      case DATATYPE_SHORT:
    	  dsize = 2;
      break;

      case DATATYPE_BYTE:
    	  dsize = 1;
      break;
      }
      return gnuradio::get_initial_sptr
        (new sqlsource_impl(sqlstring,igrcdatatype,dsize));
    }

    /*
     * The private constructor
     */
    sqlsource_impl::sqlsource_impl(const char *csqlstring,int igrcdatatype,int dsize)
      : gr::sync_block("sqlsource",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(1, 1, dsize))
    {
    	grcdatatype = igrcdatatype;

    	sqlstring = csqlstring;
    	sqlAction = GRSQL_UNKNOWN;
    	selectAction = SELECT_UNKNOWN;
    	filename = "";
    	outputfile = "";
    	dataType = DATATYPE_UNKNOWN;
    	samplerate = 0;
    	starttime = 0.0;
    	endtime = -1.0;

    	pInputFile = NULL;
    	curfileposition = 0;
    	endfileposition = 0;

    	parsesql(true);

    	// if we're in a flowgraph check that we match.
    	bool bDataError = false;

    	if (grcdatatype > 0) {
    		// only check if we instantiated from flowgraph (grcdatatype > 0)
    		if (dataType != grcdatatype) {
    			// We have a type mismatch between SQL and flowgraph.
    			if (dataType != DATATYPE_COMPLEX) {
    				// this is just an error
    				bDataError = true;
    			}
    			else {
    				// We're in a complex data type.
    				// let's make sure we didn't select I or Q and we asked for Float out.  That's correct.
    				if (grcdatatype == DATATYPE_FLOAT) {
        				if ((selectAction != SELECT_I) && (selectAction != SELECT_Q)) {
        					bDataError = true;
        				}
    				}
    				else {
    					// we have the SQL set to COMPLEX and the flowgraph is not COMPLEX or FLOAT
    					bDataError = true;
    				}
    			}
    		}
    	}

    	if (bDataError) {
    		std::cout << "ERROR: Your SQL and your flowgraph have mismatched data types.  Please check and try again." << std::endl;
    		exit(1);
    	}

    	if ((outputfile.length()) > 0) {
    		hasOutputFile = true;
    	}

		filesize = GetFileSize(filename);
		datatypesize = GetDataTypeSize();

		numdatapoints = filesize / (long)datatypesize;

		numsec = (float)numdatapoints / (float)samplerate;
    }

    int sqlsource_impl::runsql() {
    	if (selectAction == SELECT_TIMELENGTH) {

    		if (outputfile.length() == 0) {
        		std::cout << "Time length of recording: " << std::fixed << std::setw(11) <<
        				std::setprecision(6) << numsec << " seconds (" << numsec / 60.0 << ") min"<< std::endl;
    		}
    		else {
    			  ofstream outfile;
    			  try {
        			  outfile.open(outputfile);
    			  }
    			  catch (...) {
    				  std::cout << "ERROR: Unable to write to " << outputfile << std::endl;
    				  exit(1);
    			  }

    			  outfile << "Time length of recording: " << std::fixed << std::setw(11) <<
          				std::setprecision(6) << numsec << " seconds (" << numsec / 60.0 << ") min"<< std::endl;
    			  outfile.close();
    		}

    	}
    	else {
    		// need to seek to the specified time position
    		// Know the time difference in bytes (data size * sample rate * time difference)
    		// write those bytes to output file.
    		// If length goes beyond the end of the input file or if endtime = -1.0 just write the remaining file.
    		try {
    			pInputFile = fopen ( filename.c_str() , "rb" );
    		}
    		catch(...) {
    			std::cout << "ERROR: Unable to open input file " << filename << std::endl;
    		}
    		FILE * pOutputFile;
    		try {
    			pOutputFile = fopen ( outputfile.c_str() , "wb" );
    		}
    		catch(...) {
    			std::cout << "ERROR: Unable to open output file " << outputfile << std::endl;
    		}

    		long startpos = (long)((float)datatypesize * starttime * samplerate);

    		if (startpos > (filesize - datatypesize)) {
    			std::cout << "ERROR: start time is at or past the end of the file." << std::endl;
    			exit(1);
    		}

			fseek ( pInputFile , startpos , SEEK_SET );

    		long endpos;

    		if (endtime == -1.0) {
    			endpos = filesize;
    		}
    		else {
    			endpos = (long)((float)datatypesize * endtime * samplerate);

    			if (endpos > filesize) {
    				endpos = filesize;
    			}
    		}

    		// Let's use a 16K buffer to move through * blocks faster.
    		// Each file read will take some time so it's more efficient to do them in blocks.

    		size_t bytes_read = 0;

    		long i=startpos;
    		long bytesremaining;

    		// We're using a while here so we can dyamically adjust our step
    		// depending on if we're in bulk * mode or I/Q mode where we have
    		// to go 1 data set at a time to separate out the I and Q channels
    		while (i<endpos) {

    			if (selectAction == SELECT_STAR) {
    				// read in bigger blocks
    				bytesremaining = endpos - i;

    				if (bytesremaining >= FILEREADBLOCKSIZE)
    					bytes_read = fread(&buffer, 1, FILEREADBLOCKSIZE, pInputFile);
    				else
    					bytes_read = fread(&buffer, 1, bytesremaining, pInputFile);

        			fwrite(buffer,1,bytes_read,pOutputFile);

    				i = i + bytes_read;
    			}
    			else {
    				// read in a complex data set at a time so we can split them out.
        			bytes_read = fread(&buffer, 1, datatypesize, pInputFile);

    				if (selectAction == SELECT_I) {
    					// write first float
            			fwrite(buffer,1,4,pOutputFile);
    				} else if (selectAction == SELECT_Q) {
    					// write second float
            			fwrite(&buffer[4],1,4,pOutputFile);
    				}

        			i = i + datatypesize;
    			}
    		}

			fclose ( pInputFile );
			pInputFile = NULL;
			fclose ( pOutputFile );
    	}

    	return 0;
    }

    bool sqlsource_impl::stop() {
    	if (pInputFile) {
            gr::thread::scoped_lock lock(fp_mutex); // hold for the rest of this function
    		fclose(pInputFile);
    		pInputFile = NULL;
    	}

        return true;
    }

    /*
     * Our virtual destructor.
     */
    sqlsource_impl::~sqlsource_impl()
    {
    	stop();
    }

    void sqlsource_impl::parsesql(bool ignore_nosaveas) {
    	// This function breaks down the SQL statement into its representative components and does any initialization such as making sure the input file
    	// exists and opening the file pointer.

    	if (sqlstring.length() == 0) {
    		std::cout << "ERROR: Please provide a grsql SQL string." << std::endl;
    		exit(1);
    	}

		std::regex rgxselect("SELECT ?(\\*|I|Q|WATERFALL|FREQUENCY|CONSTELLATION|TIMELENGTH)",std::regex_constants::icase);
		std::regex rgxfile(" FROM '?(.*?)'",std::regex_constants::icase);
		std::regex rgxdatatype(" ASDATATYPE ?(COMPLEX|FLOAT|INT|SHORT|BYTE)",std::regex_constants::icase);
		std::regex rgxsamplerate(" SAMPLERATE ?([0-9]{1,}\\.?[0-9]{0,}M?)",std::regex_constants::icase);
		std::regex rgxstarttime(" STARTTIME ?([0-9]{1,}\\.?[0-9]{0,})",std::regex_constants::icase);
		std::regex rgxendtime(" ENDTIME ?([0-9]{1,}\\.?[0-9]{0,})",std::regex_constants::icase);
		std::regex rgxsaveas(" SAVEAS '?(.*?)'",std::regex_constants::icase);
		std::smatch match;

		// Find if we have a SELECT or INSERT
    	if ( std::regex_search(sqlstring, match, rgxselect) ) {
    		// SELECT CLAUSE
    	        //std::cout << "Found SELECT clause\n";
    	        std::string strselectaction=match[1];
    	        boost::to_upper(strselectaction);
    	        // std::cout << "matched string = " << selectaction << '\n';
    	        if (strselectaction == "TIMELENGTH") {
    	        	selectAction = SELECT_TIMELENGTH;
    	        } else if (strselectaction == "*") {
    	        	selectAction = SELECT_STAR;
				} else if (strselectaction == "I") {
					selectAction = SELECT_I;
				} else if (strselectaction == "Q") {
					selectAction = SELECT_Q;
				}
				else {
					std::cout << "ERROR: Unknown select action: " << strselectaction << std::endl;
					exit(1);
				}
    	        // Find the filename:
    	    	if ( std::regex_search(sqlstring, match, rgxfile) ) {
    	    	        filename=match[1];
    	    	        // std::cout << "filename: " << filename << std::endl;

    	    	        // now see if file exists:
    	    			std::ifstream infile;

    	    			try {
    	    				infile.open (filename);
    	    			}
    	    			catch (...) {
    	    				std::cout << "ERROR opening file: " << filename << std::endl;
    	    				exit(1);
    	    			}

    	    			infile.close();
    			}
    			else {
    				std::cout << "No source file found.  Please include FROM '<filename>' in statement (or did you forget the quotes?)." << std::endl;
    				exit(1);
    			}

    	    	// Data Type
    	    	if ( std::regex_search(sqlstring, match, rgxdatatype) ) {
    	    	        std::string dtype=match[1];
    	    	        boost::to_upper(dtype);
    	    	        // std::cout << "data type = " << dtype << '\n';
    	    	        if (dtype == "COMPLEX") {
    	    	        	dataType = DATATYPE_COMPLEX;
    	    	        } else if (dtype == "FLOAT") {
    	    	        	dataType = DATATYPE_FLOAT;
    					} else if (dtype == "INT") {
    						dataType = DATATYPE_INT;
    					} else if (dtype == "SHORT") {
    						dataType = DATATYPE_SHORT;
						} else if (dtype == "BYTE") {
							dataType = DATATYPE_BYTE;
						}
						else {
							std::cout << "ERROR: Unknown data type: " << dtype << std::endl;
							exit(1);
						}

    	    	        if ( ((selectAction == SELECT_I) || (selectAction == SELECT_Q)) && (dataType != DATATYPE_COMPLEX) ) {
    	    	        	std::cout << "ERROR: SELECT I/Q only available for complex data types." << std::endl;
    	    	        	exit(1);
    	    	        }
    			}
    			else {
    				std::cout << "No data type specified.  Please include ASDATATYPE [COMPLEX | FLOAT | INT | SHORT | BYTE] in statement." << std::endl;
    				exit(1);
    			}

    	    	// Sample Rate
    	    	if ( std::regex_search(sqlstring, match, rgxsamplerate) ) {
    	    	        std::string srate=match[1];

    	    	        if (srate.find("M") != std::string::npos) {
    	    	        	boost::replace_all(srate,"M","");
    	    	        	samplerate = atof(srate.c_str()) * 1000000.0;
    	    	        }
    	    	        else {
    	    	        	samplerate = atof(srate.c_str());
    	    	        }
    	    	        // std::cout << "sample rate = " << samplerate << " SPS" << std::endl;
    			}
    			else {
    				std::cout << "No sample rate specified.  Please include SAMPLERATE <sample rate> in statement. Sample rate may be specified as 10000000 or 10.2M" << std::endl;
    				exit(1);
    			}
    	    	// Start time
    	    	if ( std::regex_search(sqlstring, match, rgxstarttime) ) {
    	    	        std::string srate=match[1];

	    	        	starttime = atof(srate.c_str());
    	    	        // std::cout << "sample rate = " << samplerate << " SPS" << std::endl;
    			}
    			else {
    				if (selectAction != SELECT_TIMELENGTH) {
        				std::cout << "No start time specified.  Please include starttime <time as float sec> in statement." << std::endl;
        				exit(1);
    				}
    			}

    	    	// end time
    	    	if ( std::regex_search(sqlstring, match, rgxendtime) ) {
    	    	        std::string srate=match[1];

	    	        	endtime = atof(srate.c_str());
    	    	        // std::cout << "sample rate = " << samplerate << " SPS" << std::endl;
    			}
    			else {
    				if (selectAction != SELECT_TIMELENGTH) {
        				std::cout << "WARNING: No end time specified.  Assuming end of file." << std::endl;
    				}
    			}

    	    	// save as
    	    	if ( std::regex_search(sqlstring, match, rgxsaveas) ) {
    	    	        outputfile=match[1];
    	    	        // std::cout << "output file: " << outputfile << std::endl;
    			}
    			else {
    				if ((selectAction != SELECT_TIMELENGTH) && (!ignore_nosaveas)) {
        				std::cout << "No output file found.  Please include SAVEAS '<filename>' in statement (or did you forget the quotes?)." << std::endl;
        				exit(1);
    				}
    			}

		}
		else {
			std::cout << "No select clause found." << std::endl;
			exit(1);
		}

    	// Fields are populated.
    }

    long sqlsource_impl::GetFileSize(std::string filename)
    {
        struct stat stat_buf;
        int rc = stat(filename.c_str(), &stat_buf);
        return rc == 0 ? stat_buf.st_size : -1;
    }

    int sqlsource_impl::GetDataTypeSize() {
    	int retVal = 1;

    	switch (dataType) {
    	case DATATYPE_COMPLEX:
    		retVal = 8;
    	break;

    	case DATATYPE_FLOAT:
    		retVal = 4;
    	break;

    	case DATATYPE_INT:
    		retVal = 4;
    	break;

    	case DATATYPE_SHORT:
    		retVal = 2;
    	break;

    	case DATATYPE_BYTE:
    		retVal = 1;
    	break;
    	}

    	return retVal;
    }

    int
    sqlsource_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
        gr::thread::scoped_lock lock(fp_mutex); // hold for the rest of this function

    	int returnedItems=0;

       	if ((hasOutputFile) > 0) {
    		std::cout << "ERROR: output file not allowed in GR-SQL when running from a block.  Use the command-line grsql tool instead." << std::endl;
    		exit(1);
    	}
    	// If the file isn't already open, let's open it and set our start position
    	if (!pInputFile) {
    		try {
    			pInputFile = fopen ( filename.c_str() , "rb" );
    		}
    		catch(...) {
    			std::cout << "ERROR: Unable to open input file " << filename << std::endl;
    		}

    		long startpos = (long)((float)datatypesize * starttime * samplerate);

    		if (startpos > (filesize - datatypesize)) {
    			std::cout << "ERROR: start time is at or past the end of the file." << std::endl;
    			exit(1);
    		}

    		curfileposition = startpos;

    		fseek ( pInputFile , startpos , SEEK_SET );

    		if (endtime == -1.0) {
    			endfileposition = filesize;
    		}
    		else {
    			endfileposition = (long)((float)datatypesize * endtime * samplerate);

    			if (endfileposition > filesize) {
    				endfileposition = filesize;
    			}
    		}
    	}

		// Let's use a 16K buffer to move through * blocks faster.
		// Each file read will take some time so it's more efficient to do them in blocks.

		size_t bytes_read = 0;
		long bytesremaining;
		long bytesrequested = noutput_items * datatypesize;

		// We're using a while here so we can dyamically adjust our step
		// depending on if we're in bulk * mode or I/Q mode where we have
		// to go 1 data set at a time to separate out the I and Q channels
		if (curfileposition < endfileposition) {
			bytesremaining = endfileposition - curfileposition;

			if (selectAction == SELECT_STAR) {
				// read in bigger blocks
		    	gr_complex *out = (gr_complex *) output_items[0];

				if (bytesremaining >= bytesrequested)
					bytes_read = fread(&buffer, 1, bytesrequested, pInputFile);
				else
					bytes_read = fread(&buffer, 1, bytesremaining, pInputFile);

				curfileposition = curfileposition + bytes_read;

				returnedItems = (int)(bytes_read / datatypesize);

				memcpy((void *)out,(const void *)buffer,bytes_read);
			}
			else {
				// For I or Q, we're reading complex but writing out float.
				// So bytesrequested is 1/2 the bytes we need to read.
				long bytestoread = 2 * bytesrequested;
				long bytesprocessed = 0;
				int i=0;
		    	float *out = (float *) output_items[0];

				while ((curfileposition < endfileposition) && (bytesprocessed < bytestoread)) {
					// read in a complex data set at a time so we can split them out.
					bytes_read = fread(&buffer, 1, datatypesize, pInputFile);

					if (bytes_read > 0) {
						if (selectAction == SELECT_I) {
							// write first float
							memcpy(&out[i],buffer,4);
						} else if (selectAction == SELECT_Q) {
							// write second float
							memcpy(&out[i],&buffer[4],4);
						}
					}
					else {
						// just a safety check
						out[i] = 0.0;
					}

					bytesprocessed = bytesprocessed + datatypesize;
					i = i + 1;

					curfileposition = curfileposition + datatypesize;
				}

				returnedItems = (bytesprocessed / datatypesize);
			}
		}

		// Tell runtime system how many output items we produced.
		return returnedItems;
    }

  } /* namespace sql */
} /* namespace gr */

