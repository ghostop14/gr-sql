# gr-sql - SQL-Like Select Capabilities for GNURadio Data Files 

## Overview
The goal of the gr-sql project is to bring some SQL-like SELECT capabilities to GNURadio to facilitate working with smaller portions of large recorded files.  For instance if you recorded 10 minutes of data and only want to extract the first 2 minutes, or minutes 7-8.

gr-sql provides this capability as both a native GNURadio source block where the SQL syntax can be used to query the original file, as well as a command-line tool (grsql) that can be used to extract and save sub-portions to separate files.  The command-line tool also provides a query option to get the total time length of a recording given the sample rate and data type.

The syntax is very straightforward:
SELECT [* | I | Q | TIMELENGTH] FROM '<file source>' ASDATATYPE [COMPLEX | FLOAT | INT | SHORT | BYTE] SAMPLERATE <sps> 
[STARTTIME <time in seconds as float> ENDTIME <time in seconds as float>] [SAVEAS '<output file>']

Notes:
- The sample rate can be specified in either the 6200000 or 6.2M format
- Start and end time are relative to the beginning of the recording as t=0
- If no end time is specified, the end of the file is assumed
- SELECT I and Q only apply to COMPLEX data type
- If using the command-line tool and running TIMELENGTH, SAVEAS is not required (it'll just print it to the console)
- If using the flowgraph source block, SAVEAS and TIMELENGTH are not available (didn't make sense to save or just get time from a flowgraph)
- There is an sample flowgraph under examples.  You'll just need to update the SQL with an appropriate filename and sample rate.

Command-line Examples:
Get the total time length of a recording:

grsql "select TIMELENGTH FROM '/tmp/myrecording_593MHz_6.2MSPS.raw' ASDATATYPE complex SAMPLERATE 6.2M"


Select the recording sample from 45.2 to 80.0 seconds into the recording and save it in a new file:

grsql "SELECT * FROM '/tmp/myrecording_593MHz_6.2MSPS.raw' ASDATATYPE complex SAMPLERATE 6.2M STARTTIME 45.2 ENDTIME 80.0 SAVEAS '/tmp/extracted.raw'"


Select just the I channel from the entire complex stream:

grsql "SELECT I FROM '/tmp/myrecording_593MHz_6.2MSPS.raw' ASDATATYPE complex SAMPLERATE 6.2M STARTTIME 0.0 SAVEAS '/tmp/extracted.raw'"


## Building
Building is the standard approach:


cd <clone directory>

mkdir build

cd build

cmake ..

make

[sudo] make install

sudo ldconfig

If each step was successful (do not overlook the "sudo ldconfig" step).


