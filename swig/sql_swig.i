/* -*- c++ -*- */

#define SQL_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "sql_swig_doc.i"

%{
#include "sql/sqlsource.h"
%}


%include "sql/sqlsource.h"
GR_SWIG_BLOCK_MAGIC2(sql, sqlsource);
