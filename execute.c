#include "myshell.h"

/*
   CITS2002 Project 2 2017
   Name(s):		student-name1 (, student-name2)
   Student number(s):	student-number-1 (, student-number-2)
   Date:		date-of-submission
 */

// -------------------------------------------------------------------

//  THIS FUNCTION SHOULD TRAVERSE THE COMMAND-TREE and EXECUTE THE COMMANDS
//  THAT IT HOLDS, RETURNING THE APPROPRIATE EXIT-STATUS.
//  READ print_shellcmd0() IN globals.c TO SEE HOW TO TRAVERSE THE COMMAND-TREE

int execute_shellcmd(SHELLCMD *t)
{
    int  exitstatus;

    if(t == NULL) {			// hmmmm, that's a problem
	exitstatus	= EXIT_FAILURE;
    }
    else {				// normal, exit commands
	exitstatus	= EXIT_SUCCESS;
    }

    return exitstatus;
}
