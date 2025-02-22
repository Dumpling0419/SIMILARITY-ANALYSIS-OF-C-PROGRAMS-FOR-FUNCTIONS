D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{D}+
FS			(f|F|l|L)
IS			(u|U|l|L)*

%{
#include "ast.h"
#include <string.h>
#include <stdio.h>
//#include "y.tab.h"
#include "my_parser.h"


char* type_names[] = {
    "void", "char", "short", "int", "long", "float", "double", 
    "signed", "unsigned", "const", "volatile", 
    "struct", "union", "enum",
    NULL
};

void count();
int comment();
int check_type();
%}

%%
"/*"			{ comment(); }

"//".*              { /* DO NOTHING */ }
^#.*                { /* DO NOTHING */ }

"auto"			{ yylval.sval = strdup(yytext); count(); return(AUTO); }
"break"			{ count(); return(BREAK); }
"case"			{ count(); return(CASE); }
"char"			{ yylval.sval = strdup(yytext); count(); return(CHAR); }
"const"			{ yylval.sval = strdup(yytext); count(); return(CONST); }
"continue"		{ count(); return(CONTINUE); }
"default"		{ count(); return(DEFAULT); }
"do"			{ count(); return(DO); }
"double"		{ yylval.sval = strdup(yytext); count(); return(DOUBLE); }
"else"			{ count(); return(ELSE); }
"enum"			{ yylval.sval = strdup(yytext); count(); return(ENUM); }
"extern"		{ yylval.sval = strdup(yytext); count(); return(EXTERN); }
"float"			{ yylval.sval = strdup(yytext); count(); return(FLOAT); }
"for"			{ count(); return(FOR); }
"goto"			{ count(); return(GOTO); }
"if"			{ count(); return(IF); }
"int"			{ yylval.sval = strdup(yytext); count(); return(INT); }
"long"			{ yylval.sval = strdup(yytext); count(); return(LONG); }
"register"		{ yylval.sval = strdup(yytext); count(); return(REGISTER); }
"return"		{ count(); return(RETURN); }
"short"			{ yylval.sval = strdup(yytext); count(); return(SHORT); }
"signed"		{ yylval.sval = strdup(yytext); count(); return(SIGNED); }
"sizeof"		{ count(); return(SIZEOF); }
"static"		{ yylval.sval = strdup(yytext); count(); return(STATIC); }
"struct"		{ yylval.sval = strdup(yytext); count(); return(STRUCT); }
"switch"		{ count(); return(SWITCH); }
"typedef"		{ yylval.sval = strdup(yytext); count(); return(TYPEDEF); }
"union"			{ yylval.sval = strdup(yytext); count(); return(UNION); }
"unsigned"		{ yylval.sval = strdup(yytext); count(); return(UNSIGNED); }
"void"			{ yylval.sval = strdup(yytext); count(); return(VOID); }
"volatile"		{ yylval.sval = strdup(yytext); count(); return(VOLATILE); }
"while"			{ count(); return(WHILE); }

{L}({L}|{D})* {
    yylval.sval = strdup(yytext);
    count();
    return check_type();
}

0[xX]{H}+{IS}? {
    yylval.sval = strdup(yytext);
    count();
    return CONSTANT;
}

0{D}+{IS}? {
    yylval.sval = strdup(yytext);
    count();
    return CONSTANT;
}

{D}+{IS}? {
    yylval.sval = strdup(yytext);
    count();
    return CONSTANT;
}

L?'(\\.|[^\\'])+' {
    yylval.sval = strdup(yytext);
    count();
    return CONSTANT;
}

{D}+{E}{FS}? {
    yylval.sval = strdup(yytext);
    count();
    return CONSTANT;
}

{D}*"."{D}+({E})?{FS}? {
    yylval.sval = strdup(yytext);
    count();
    return CONSTANT;
}

{D}+"."{D}*({E})?{FS}? {
    yylval.sval = strdup(yytext);
    count();
    return CONSTANT;
}

L?\"(\\.|[^\\"])*\" {
    yylval.sval = strdup(yytext);
    count();
    return STRING_LITERAL;
}

"..."			{ count(); return(ELLIPSIS); }
">>="			{ count(); return(RIGHT_ASSIGN); }
"<<="			{ count(); return(LEFT_ASSIGN); }
"+="			{ count(); return(ADD_ASSIGN); }
"-="			{ count(); return(SUB_ASSIGN); }
"*="			{ count(); return(MUL_ASSIGN); }
"/="			{ count(); return(DIV_ASSIGN); }
"%="			{ count(); return(MOD_ASSIGN); }
"&="			{ count(); return(AND_ASSIGN); }
"^="			{ count(); return(XOR_ASSIGN); }
"|="			{ count(); return(OR_ASSIGN); }
">>"			{ count(); return(RIGHT_OP); }
"<<"			{ count(); return(LEFT_OP); }
"++"			{ count(); return(INC_OP); }
"--"			{ count(); return(DEC_OP); }
"->"			{ count(); return(PTR_OP); }
"&&"			{ count(); return(AND_OP); }
"||"			{ count(); return(OR_OP); }
"<="			{ count(); return(LE_OP); }
">="			{ count(); return(GE_OP); }
"=="			{ count(); return(EQ_OP); }
"!="			{ count(); return(NE_OP); }
";"			{ count(); return(';'); }
("{"|"<%")		{ count(); return('{'); }
("}"|"%>")		{ count(); return('}'); }
","			{ count(); return(','); }
":"			{ count(); return(':'); }
"="			{ count(); return('='); }
"("			{ count(); return('('); }
")"			{ count(); return(')'); }
("["|"<:")		{ count(); return('['); }
("]"|":>")		{ count(); return(']'); }
"."			{ count(); return('.'); }
"&"			{ count(); return('&'); }
"!"			{ count(); return('!'); }
"~"			{ count(); return('~'); }
"-"			{ count(); return('-'); }
"+"			{ count(); return('+'); }
"*"			{ count(); return('*'); }
"/"			{ count(); return('/'); }
"%"			{ count(); return('%'); }
"<"			{ count(); return('<'); }
">"			{ count(); return('>'); }
"^"			{ count(); return('^'); }
"|"			{ count(); return('|'); }
"?"			{ count(); return('?'); }

[ \t\v\n\f]		{ count(); }
.			{ /* ignore bad characters */ }

%%

int yywrap() {
    return 1;
}


int comment() {
    int c;
    while ((c = input()) != 0) {
        if (c == '*' && (c = input()) == '/') {
            break;
        }
    }
    return 0;
}

int column = 0;
void count()
{
    int i;
    for (i = 0; yytext[i] != '\0'; i++)
        if (yytext[i] == '\n')
            column = 0;
        else if (yytext[i] == '\t')
            column += 8 - (column % 8);
        else
            column++;
    // ECHO;
}

int check_type() {
    int i = 0;
    while (type_names[i] != NULL) {
        if (strcmp(yytext, type_names[i]) == 0) {
            yylval.sval = strdup(yytext);
            return TYPE_NAME;
        }
        i++;
    }
    yylval.sval = strdup(yytext);
    return IDENTIFIER;
}

