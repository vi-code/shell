
/*
 *
 * CS-252 Fall 2013
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <string.h>
#include "y.tab.h"
#include <unistd.h>



static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
	unput(c);
}

%}

%%


[^<>&|` \t\n\\][^<>&|` \t\n\\]*  {
		/* Assume that file names have only alpha chars */
		yylval.string_val = strdup(yytext);
		return WORD;
	}
\n 	{
	return NEWLINE;
}

[ \t] 	{
	/* Discard spaces and tabs */
}	

">" 	{
	return GREAT;
}

">>" {
	return GREATGREAT;
}

"|" {
	return PIPE;
}

"&" {
	return AMPERSAND;
}

"<" {
	return LESS;
}

">&" {
	return GRAMPS;
}

">>&" {
	return GREATGRAMPS;
}

[^ \n\t]*\\[^ \t\n]* {
	char * ytext = strdup(yytext);
	char argument[strlen(yytext)];
	int i, j = 0;
	for ( i = 0; i < strlen(ytext); i++) {
		if ( ytext[i] != '\\')
			argument[j++] = ytext[i];
		else {
			argument[j++] = ytext[i+1];
			i++;
		}
	}
	argument[j] = '\0';
	yylval.string_val = strdup(argument);
	return WORD;

}


\"[^\n]*\" {
	char argument[strlen(yytext)];
	memcpy(argument, &yytext[1], strlen(yytext) - 2);
	yylval.string_val = strdup(argument);
	return WORD;

}

.  {
	return NOTOKEN;
}
