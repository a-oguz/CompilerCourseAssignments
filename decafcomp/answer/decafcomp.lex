%{
#include "default-defs.h"
#include "decafcomp.tab.h"
#include <cstring>
#include <string>
#include <sstream>
#include <iostream>

using namespace std;

int lineno = 1;
int tokenpos = 1;
string errstr = "";

%}

  /*
	Execptional cases
 */
stringVal	[^"\\\n\r\v\f\a\b\t]
charVal		[^'\\\n\r\v\f\a\b\t]
charErr		['"nrvfabt\\]


%%

  /*
    Pattern definitions for all tokens 
  */
bool    					{ errstr += yytext;return T_BOOLTYPE; }	//From 1 to 18 is Keywords
break   					{ errstr += yytext;return T_BREAK; }
continue	  				{ errstr += yytext;return T_CONTINUE; }
else   						{ errstr += yytext;return T_ELSE; }
extern  					{ errstr += yytext;return T_EXTERN; }
false   					{ errstr += yytext;return T_FALSE; }
for     					{ errstr += yytext;return T_FOR; }
func    					{ errstr += yytext;return T_FUNC; }
if        					{ errstr += yytext;return T_IF; }
int    						{ errstr += yytext;return T_INTTYPE; }
null    					{ errstr += yytext;return T_NULL; }
package 					{ errstr += yytext;return T_PACKAGE; }
return  					{ errstr += yytext;return T_RETURN; }
string  					{ errstr += yytext;return T_STRINGTYPE; }
true      					{ errstr += yytext;return T_TRUE; }
var    						{ errstr += yytext;return T_VAR; }
while  						{ errstr += yytext;return T_WHILE; }
void    					{ errstr += yytext;return T_VOID; }
\{  						{ errstr += yytext;return T_LCB; } // from 19 to 44 are operators and delimiters
\}							{ errstr += yytext;return T_RCB; }
\[   						{ errstr += yytext;return T_LSB; }
\]   						{ errstr += yytext;return T_RSB; }
\,   						{ errstr += yytext;return T_COMMA; }
\;   						{ errstr += yytext;return T_SEMICOLON; }
\(   						{ errstr += yytext;return T_LPAREN; }
\)  						{ errstr += yytext;return T_RPAREN; }
\=  						{ errstr += yytext;return T_ASSIGN; }
\-  						{ errstr += yytext;return T_MINUS; }
\!   						{ errstr += yytext;return T_NOT; }
\+   						{ errstr += yytext;return T_PLUS; }
\*   						{ errstr += yytext;return T_MULT; }
\/   						{ errstr += yytext;return T_DIV; }
\<\<  						{ errstr += yytext;return T_LBW; }
\>\>  						{ errstr += yytext;return T_RBW; }
\<  						{ errstr += yytext;return T_LT; }
\>  						{ errstr += yytext;return T_GT; }
\%  						{ errstr += yytext;return T_MOD; }
\<\=  						{ errstr += yytext;return T_LEQ; }
\>\=  						{ errstr += yytext;return T_GEQ; }
\=\=  						{ errstr += yytext;return T_EQ; }
\!\=						{ errstr += yytext;return T_NEQ; }
\&\&  						{ errstr += yytext;return T_AND; }
\|\|  						{ errstr += yytext;return T_OR; }
\.							{ errstr += yytext;return T_DOT; }

[a-zA-Z\_][a-zA-Z\_0-9]*   { yylval.sval = new string(yytext); errstr += yytext;return T_ID; } /* note that identifier pattern must be after all keywords */
[\n\t\r\a\v\b ]+           	{ tokenpos++; } //Whitespace

[0-9]+						{ yylval.sval = new string(yytext); errstr += yytext;return T_INTCONSTANT; } //47 to 49 are consts 
\'({charVal}|\\{charErr})\'		{ yylval.sval = new string(yytext); errstr += yytext;return T_CHARCONSTANT; }
\"({stringVal}|\\{charErr}+)*\"	{ yylval.sval = new string(yytext); errstr += yytext;return T_STRINGCONSTANT; }
\/\/.*					{  } //Single Line Comment Identifier	
\'{charErr}\'				{ cerr << "Error: Syntax Error" << endl; return -1; }
\'{charVal}{charVal}+\'		{ cerr << "Error: Syntax Error" << endl; return -1; }
\"({charErr}|\v*)\"			{ cerr << "Error: Syntax Error" << endl; return -1; }
.                          	{ cerr << "Error: unexpected character in input" << endl; return -1; }

%%

int yyerror(const char *s) {
  cerr << lineno << ": " << s << " at char " << tokenpos <<" "<< errstr << endl;
  return 1;
}
