M2MXML/M2MBSM/M2MMEM v1.0

These components are written in ANSI-C and provide support for 1) parsing and
creating M2MXML messages (defined in "m2mxml.h"), 2) a "Behavior State Machine"
(M2MBSM) that provides a framework for receiving/sending M2MXML messages as
well as managing on-device behaviors (defined in "m2mbsm.h"), and 3) providing
a limited memory management API for #1 and #2 that avoids heap memory
management (defined in "m2mmem.h").

These components are provided through the M2MXML Open Source initiative and can
be found at http://m2mxml.sourceforge.net/.

TESTING

The file "testmain.c" is provided as a test harness for the components. It
accepts "unittestin.txt" as a single command-line argument and should generate
the contents of "unittestout.txt" as output to STDOUT.

DEPENDENCIES:

1)	M2MMEM exists to minimize dependence on the standard "malloc.h" API that
	may have limited support on some embedded devices.

2)	The M2MXML parsing function depends on the Open Source XML parser, EXPAT
  	(currently version 1.95.8), that is included in the "expat" directory. This
	library provides a mechanism to replace its memory management function if
	this is required. Because some embedded systems are not compatible with
	EXPAT, a "light" version that supports only the minimum features required
	by the current M2MXML library is provided.

3)	Otherwise, these components rely on the following direct references to
	standard ANSI-C library functions (EXPAT dependencies not included):

	#include <time.h>
		time()		- getting current system time in seconds
		localtime()	- decomposing time into logical components

	#include <stdio.h>
		sprintf()	- converting 'double' values to strings

	#include <stdlib.h>
		 atof()		- converting strings into 'double' values
		 atoi()		- converting strings into 'int' values
		 itoa()		- converting 'int' values into strings

	#include <string.h>
		memset()	- intitializing memory
		strcmp()	- comparing strings
		strcpy()	- copying strings
		
SUPPORTED COMPILERS

Currently, these libraries have been tested with the GCC and Turbo-C compilers.
Several implementation choices have been made to be compatible with non-ANSI-C
compilers such as Turbo-C. In addition, choices have been made to minimize
the differences from very-non-ANSI-C compilers, such as Dynamic-C. However,
these compilers are so different in syntax and structure requirements as not
result in separate source files.
