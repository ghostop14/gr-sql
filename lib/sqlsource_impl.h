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

#ifndef INCLUDED_SQL_SQLSOURCE_IMPL_H
#define INCLUDED_SQL_SQLSOURCE_IMPL_H

#include <sql/sqlsource.h>
#include <string>

#define GRSQL_UNKNOWN 0
#define GRSQL_SELECT 1
#define GRSQL_INSERT 2

#define DATATYPE_UNKNOWN 0
#define DATATYPE_COMPLEX 1
#define DATATYPE_FLOAT 2
#define DATATYPE_INT 3
#define DATATYPE_SHORT 4
#define DATATYPE_BYTE 5

#define FILEREADBLOCKSIZE 1024000

namespace gr {
  namespace sql {

    class sqlsource_impl : public sqlsource
    {
     protected:
    	std::string sqlstring;

    	int sqlAction;  // select or insert
    	int selectAction; // *, I, Q, TIMELENGTH, etc.
    	std::string filename;
		FILE * pInputFile;

        boost::mutex fp_mutex;

    	std::string outputfile;
    	bool hasOutputFile;

    	int grcdatatype; // set in flowgraph

    	int dataType; // defined in SQL
    	long samplerate;
    	float starttime;
    	float endtime;

		long filesize;
		int datatypesize;
		long numdatapoints;
		float numsec;

		unsigned char buffer[FILEREADBLOCKSIZE];
		long curfileposition;
		long endfileposition;

    	void parsesql(bool ignore_nosaveas=false);
    	long GetFileSize(std::string filename);
    	int GetDataTypeSize();

     public:
      sqlsource_impl(const char * csqlstring, int igrcdatatype=DATATYPE_UNKNOWN,int dsize=8 ); // used for command-line
      ~sqlsource_impl();

      bool stop();

      int runsql();

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
    };

  } // namespace sql
} // namespace gr

#endif /* INCLUDED_SQL_SQLSOURCE_IMPL_H */

