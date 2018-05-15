
/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <regex.h>
#include <signal.h>
#include <pwd.h>
#include "command.h"

SimpleCommand::SimpleCommand()
{
	// Create available space for 5 arguments
	_numOfAvailableArguments = 5;
	_numOfArguments = 0;
	_arguments = (char **) malloc( _numOfAvailableArguments * sizeof( char * ) );
}

	void
SimpleCommand::insertArgument( char * argument )
{
	if ( _numOfAvailableArguments == _numOfArguments  + 1 ) {
		// Double the available space
		_numOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				_numOfAvailableArguments * sizeof( char * ) );
	}
	if ( argument[0] == '~') {
		if ( strlen(argument) == 1)
		argument = strdup(getenv("HOME"));
	} if ( argument[0] == '~') {
		argument = strdup(getpwnam(argument+1)->pw_dir);
	}
	
	//HERE ${...}...${..}......
	char * buffer = "^.*${[^}][^}]*}.*$";
	regex_t re;
	regmatch_t match;	
	
	if ((regcomp( &re, buffer, 0)) != 0) 
	  {
	    perror("Bad regular expression: compile");
	    return;
	}

	while(regexec (&re, argument, 1, &match, 0) == 0)
	
	{
		int first, last;
		for(int i =0; i < strlen(argument); i++)
	  	{
	    	if(argument[i] == '$' && argument[i+1] == '{')
	      	first = i;
	    	if(argument[i] == '}')
	      	{
			last = i;
			break;
	      	}
	  	}

	  	int ext = last - first - 1;
	  	char var[ext];
	  	var[ext-1] = '\0';
	  	memcpy(var, &argument[first+2], ext-1);
	  	int lenght = strlen(argument) + strlen ((char *) getenv(var)) + 1;
	  	char a[lenght];
	  	char * envirnment = getenv(var);
	  	for(int j = 0; j < lenght; j++)
	  	{
	  		a[j] = 0;
	  	}
		memcpy(a, &argument[0], first);
		strcat(a, envirnment);
		strcat(a, &argument[last+1]);
		argument = strdup(a);
	}
	
	
	
	
	_arguments[ _numOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numOfArguments + 1] = NULL;

	_numOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
	_numFiles = 0;
}

	void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numOfAvailableSimpleCommands == _numOfSimpleCommands ) {
		_numOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
				_numOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}

	_simpleCommands[ _numOfSimpleCommands ] = simpleCommand;
	_numOfSimpleCommands++;
}

	void
Command::clear()
{
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}

		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inFile ) {
		free( _inFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;
	_numFiles = 0;
}

	void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");

	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
			_inFile?_inFile:"default", _errFile?_errFile:"default",
			_background?"YES":"NO");
	printf( "\n\n" );

}

	void
Command::execute()
{
	// Don't do anything if there are no simple commands
	if ( _numOfSimpleCommands == 0 ) {
		prompt();
		return;
	}
	if ( _numFiles > 1) {
		printf("Ambiguous output redirect\n");
		clear();
		prompt();
		return;
	}
	if (strcmp(_simpleCommands[0]->_arguments[0], "exit") == 0) {
		printf("Good bye!\n");
		clear();
		exit(0);
	}
	if (strcmp(_simpleCommands[0]->_arguments[0], "setenv") == 0) {
		setenv(_simpleCommands[0]->_arguments[1], _simpleCommands[0]->_arguments[2], 1);
	}
	else if(strcmp(_simpleCommands[0]->_arguments[0], "unsetenv") == 0) {
		unsetenv(_simpleCommands[0]->_arguments[1]);
	}
	if (strcmp(_simpleCommands[0]->_arguments[0], "cd") == 0) {
		if (_simpleCommands[0]->_arguments[1] == NULL){
			chdir(getenv("HOME"));
		}
		else {
			int flag = chdir(_simpleCommands[0]->_arguments[1]);
			if (flag< 0) { 

				perror("chdir");
			}
		}
		clear();
		prompt();
		return;



	}
	// Print contents of Command data structure
	//print();

	// Add execution here
	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec

	int tempin = dup(0);
	int tempout = dup(1);
	int temperr = dup(2);
	int fdout;
	int fdin;
	int fderr;
	//int fdin, fdout;
	if(_inFile){
		fdin = open(_inFile, O_RDONLY, 0777);
	}
	else {
		fdin = dup(tempin);
	}

	if ( _errFile && _append) {
		fdout = open(_outFile, O_WRONLY|O_CREAT|O_APPEND, 0777);
	}
	else if ( _errFile) {
		fdout = open(_outFile, O_WRONLY|O_CREAT|O_TRUNC, 0777);
	}
	else {
		fdout = dup(temperr);
	}
	dup2(fdout, 2);
	close(fdout);
	int ret;

	int fdpipe[2];
	for(int i = 0; i < _numOfSimpleCommands; i++) 
	{
		dup2(fdin, 0);
		close(fdin);

		if( i == _numOfSimpleCommands -1 ) {
			if(_outFile && !_append){
				fdout = open(_outFile, O_WRONLY|O_CREAT|O_TRUNC, 0777);
			}
			else if(_outFile && _append) {
				fdout = open(_outFile, O_WRONLY|O_CREAT|O_APPEND, 0777);
			}
			else {
				fdout = dup(tempout);
			}
		}
		else {
			pipe(fdpipe);
			fdout = fdpipe[1];
			fdin = fdpipe[0];
		}
		dup2(fdout, 1);
		close(fdout);



		//else {
		ret = fork();
		if (ret == 0)
		{
			if ( strcmp(_simpleCommands[i]->_arguments[0], "printenv")==0) {
				char ** env = environ;
				while (*env != NULL) {
					printf("%s\n", *env++);
				}
				exit(0);
			}
			execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
			perror("execvp");
			_exit(1);
		}

		else if (ret < 0) {
			perror("fork");
			return;
		}
		//}
	}

	dup2(tempin,0);
	dup2(tempout, 1);
	dup2(temperr, 2);
	close(tempin);
	close(tempout);
	close(temperr);

	if(!_background) {
		waitpid(ret, NULL, 0);
	}
	// Clear to prepare for next command
	clear();

	// Print new prompt
	prompt();
}

// Shell implementation

	void
Command::prompt()
{
	if ( isatty(0)) {
		printf("myshell>");
		fflush(stdout);
	}
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

extern "C" void handler(int sig) {
	if ( sig == SIGCHLD) {
		while(waitpid(-1, 0,WNOHANG) > 0);
	}
	else if ( sig == SIGINT) {
		printf("\n");
		Command::_currentCommand.clear();
	}
}


main()
{
	struct sigaction sa;
	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if ( sigaction(SIGINT, &sa, NULL) ) {
		perror("sigaction");
		exit(1);
	} else if (sigaction (SIGCHLD, &sa, NULL)) {
		perror("sigaction");
		exit(1);
	}
	Command::_currentCommand.prompt();
	yyparse();
}

