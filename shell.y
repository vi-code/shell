
/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%token	<string_val> WORD

%token 	NOTOKEN GREAT NEWLINE GRAMPS GREATGRAMPS PIPE GREATGREAT AMPERSAND LESS

%union	{
		char   *string_val;
	}

%{
//#define yylex yylex
#include <stdio.h>
#include <string.h>
#include "command.h"
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>

char ** array;
void yyerror(const char * s);
int yylex();
void expandWildcardsIfNecessary(char *);
void WildcardRecurse(char *, char *);
%}

%%
 
goal:	
	commands
	;

commands: 
	command
	| 
	commands command 
	;

command: simple_command
        ;

simple_command:	
	//command_and_args iomodifier_opt NEWLINE {
	pipe_list iomodifier_list background_optional NEWLINE{
		//printf("   Yacc: Execute command\n");
		Command::_currentCommand.execute();
	}
	| NEWLINE { Command::_currentCommand.prompt(); }
	| error NEWLINE { yyerrok; }
	;

pipe_list:
	pipe_list PIPE command_and_args
	|
	command_and_args
	;

command_and_args:
	command_word argument_list {
		Command::_currentCommand.
			insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;




argument_list:
	argument_list argument
	| // can be empty 
	;

argument:
	WORD {
               //printf("   Yacc: insert argument \"%s\"\n", $1);
			if (!(strchr($1, '*') == NULL && strchr($1, '?') == NULL)) {
				expandWildcardsIfNecessary($1);
			}
			else
	       		Command::_currentSimpleCommand->insertArgument( $1 );
		//expandWildcardsIfNecessary($1);
		

	}
	;

command_word:
	WORD {
               //printf("   Yacc: insert command \"%s\"\n", $1);
	       
	       Command::_currentSimpleCommand = new SimpleCommand();
	       Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

iomodifier_list:
	iomodifier_list iomodifier_opt
	|
	;

iomodifier_opt:
	GREAT WORD {
		//printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._numFiles++;
	}
| /*can be empty */

	LESS WORD  {
		Command::_currentCommand._inFile = $2;
	}
|
	GRAMPS WORD {
		Command::_currentCommand._errFile = strdup($2);
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._numFiles++;
	}
|
	GREATGRAMPS WORD {
		Command::_currentCommand._errFile = strdup($2);
		Command::_currentCommand._append = 1;
		Command::_currentCommand._numFiles++;
		Command::_currentCommand._outFile = $2;
	}
|
	GREATGREAT WORD {
		Command::_currentCommand._append = 1;
		Command::_currentCommand._numFiles++;
		Command::_currentCommand._outFile = $2;
	}
	;

background_optional:
	AMPERSAND {
		Command::_currentCommand._background = 1;
	}
	| /*empty*/
	;


;

%%



void WildcardRecurse(char* arg1, char * arg)
{
	char byteComponent[1024];
	char * firstOccur = strchr(arg, '/');
	if(arg[0] == 0)
	{
		return;
	}

	if(firstOccur != NULL && ((firstOccur - arg) < 1))
	{
		byteComponent[0] = '\0';
		arg = firstOccur + 1;
	}
	else if (firstOccur != NULL && !((firstOccur - arg) < 1))
	{
		strncpy(byteComponent, arg, firstOccur - arg);
		arg = firstOccur + 1;
	}
	else
	{
		strcpy(byteComponent, arg);
		arg += strlen(arg);
	}

	char preByteComponent[1024];
	if((strchr(byteComponent, '*') == NULL) &&
		(strchr(byteComponent, '?') == NULL) &&
		(strlen(arg1) == 1) && 
		(*arg1 == '/'))
	{
		sprintf(preByteComponent, "/%s", byteComponent);
		WildcardRecurse(strdup(preByteComponent), strdup(arg));
		return;
	}
	else if ((strchr(byteComponent, '*') == NULL) && (strchr(byteComponent, '?') == NULL))
	{
		sprintf(preByteComponent, "%s/%s", arg1, byteComponent);
		WildcardRecurse(strdup(preByteComponent), strdup(arg));
		return;
	}
	//char ** array;
	char * reg = (char *) malloc(2 * strlen(byteComponent) + 10);
	char * a = byteComponent;
	char * r = reg;
	*r = '^'; r++;
	while (*a) {
		if (*a == '*') { *r='.'; r++; *r='*'; r++; }
		else if (*a == '?') { *r='.'; r++;}
		else if (*a == '.') { *r='\\'; r++; *r='.'; r++;}
		else { *r=*a; r++;}
		a++;
	}
	*r='$'; r++; *r='\0';

	regmatch_t match;
	regex_t re;	 // from regular.cc
	int result = regcomp( &re, reg,  REG_EXTENDED|REG_NOSUB);
	if( result != 0 ) {
		//fprintf( stderr, "%s: Bad regular expresion \"%s\"\n", argv[ 0 ], reg );
		perror("compile");
		return;
    }

    char * dot;
  	if (arg1[0] == 0) 
    {	
    	dot = "."; 
    }
	else 
		{ 
			dot = arg1;
		}

    DIR * dir = opendir(dot);
    if(dir == NULL) {
    	return;
    }

    struct dirent * ent;
  	int numberOfEntries = 0;
	int maxEntries = 20;
	array = (char**) malloc(maxEntries*sizeof(char*));

	
    while ((ent = readdir(dir)) != NULL) 
    {
    	//check if name matches
    	if(regexec(&re, ent->d_name, 1, &match, 0) == 0)
    	{
    		if(strlen(arg1) == 1 && *arg1 == '/')
    		{
    			sprintf(preByteComponent, "/%s", strdup(ent->d_name));
    			WildcardRecurse(strdup(preByteComponent), strdup(arg));
    		}
    		else if(strlen(arg1) == 0)
    		{
    			sprintf(preByteComponent, "%s", strdup(ent->d_name));
    			WildcardRecurse(strdup(preByteComponent), strdup(arg));
    		}
    		else
    		{
    			sprintf(preByteComponent, "%s/%s", arg1, strdup(ent->d_name));
    			WildcardRecurse(strdup(preByteComponent), strdup(arg));
    		}
    		//

    		if (numberOfEntries >= maxEntries) 
    		{

    			maxEntries += maxEntries;
    			array = (char **) realloc(array, maxEntries * sizeof(char *));
    		}

    		if(*ent->d_name == '.' && *byteComponent == '.') {
    			array[numberOfEntries++] = strdup(arg1);
    		}

    	}
    }
    for(int i= 0; i < numberOfEntries; i++)
    {
    	for(int j = 0; j < numberOfEntries - 1; j++) {
    		if(strcmp(array[j], array[j+1])> 0)
    		{
    			char * a = array[j+1];
    			array[j+1] = array[j];
    			array[j] = a;
    		}
    	}
    }

    for(int i = 0; i < numberOfEntries; i++)
    {
    	Command::_currentSimpleCommand->insertArgument(strdup(array[i]));
    }
    closedir(dir);


}


void expandWildcardsIfNecessary(char * arg)
{
	// Return if arg does not contain * or ?
	char arg1[1];
	WildcardRecurse(arg1, arg);
	free(array);

}
void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}

#if 0
main()
{
	yyparse();
}
#endif
