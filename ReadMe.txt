Frame337 v2.0.0
===============

Introduction
------------

The purpose of this command line tool is to support the conversion of an elementary stream file into a format that can be placed directly onto an AES3 transport. This tool supports AC-3 (Dolby Digital), E-AC-3 (Dolby Digital Plus) and Dolby E.

AES3 is a professional audio streaming format which was orginally designed to carry linear PCM audio for professional applications. The SMPTE 337 suite of standards (337,338,339, & 340) extended that functionality to support the carriage of encoded audio. Various codecs are supported and a different data type number is associated with each one. These are defined in SMPTE 338. Audio codecs typically support a raw file format which is typically used by file based encoders and decoders. These are called elementary stream files. Elementary stream files containing AC-3 (Dolby Digital) use the '.ac3' extension whereas E-AC-3 (Dolby Digital Plus) uses 'ec3' and Dolby E uses '.dde'.
It is possible to take an elementary stream file containing raw encoded audio information and play out of a PC equipped with a digital professional sound card and software. To achieve this, the raw elementary stream file first needs to be transformed into a format that a regular audio player can deal with. This most common format is Microsoft RIFF 'wav' files although raw PCM is also sometime used. The frame337 tool provided here performs the transformation from elementary stream file to 'wav' file format as well as in the reverse direction. 

Building
--------

Two build options are provided. A GNU Makefile is provided for use on Unix systems. A Visual Studio 2003 project is provided for Windows systems which can be imported into newer versions of Visual Studio. Both the Makefile and the MSVS project file can be found in this directory.

Testing
-------

A test suite is provided to support verification of changes to the source code. This test suite uses most of the provided options to exercise the most important functionality of the tool. The test suite can be executed by running python run_test.py from the test sub-directory. Only a basic Python interpreter is required. When failures occur, it is a good idea to redirect standard output to a log file so it can easily determined which tests pass or fail. Normally the test script should be run without any arguments. A full set of sources can be generated from a minimal set, if required. This is achieved using the single parameter 'sources'. The test reference files can be generated using a reference executable and sources, using the single parameter options 'references'. Normally these two options are only used when the test system needs to be modified to cope with changes in functionality. As well as the executable, source and reference files, the test script uses a text file (run_test_cases.txt). If the functionality of the tool is extended then additional test cases should be added to this file. Each test case is defined by a single line whose is input file, reference file and then the command line options seperated by spaces. The process of adding more tests is as follows:

1. Place any additional source files in the 'sources' subdirectory. If the source files can be regenerated from existing sources then modify the Python script accordingly and execute it with the 'sources' option
2. Place the new reference output files in the 'reference_output' subdirectory or use the 'references' option to generate them
3. Add additional lines to the test case text file (run_test_cases.txt) to cover the new tests
4. Run the python test script (run_test.py)

Execution
---------
The command line options for the tool can be discovered by executing it at the command line using the '-help' option. When deformatting AC-3 and E-AC-3 files, the elementary streams produced are in 'big endian' format. However, when formatting, the tool can ingest either big or little endian format files.

Dolby Digital is a registered trademark of Dolby Licensing Corporation
SMPTE is a registered trademark of the Society of Motion Picture and Television Engineers
