/***************************************************************************
                       vbscriptlexer.ll
                       ----------------
    Reentrant flex lexer for the VBScript language.

    Modelled on qgssqlstatementlexer.ll:
      %option reentrant   -- no global state, scanner handle passed around
      %option bison-bridge -- yylval passed as pointer by bison
      %option prefix      -- namespace all symbols to avoid clashes
      %option case-insensitive -- VBScript keywords are case-insensitive
 ***************************************************************************/

%option noyywrap
%option case-insensitive
%option nounput
%option noinput
%option prefix="vbs_"
%option reentrant
%option bison-bridge
%option 8bit

%{
#include "qgsvbscriptscript.h"
struct vbs_parser_context;
#include "qgsvbscriptparser.hpp"

#define YY_NEVER_INTERACTIVE 1

#ifndef YY_NO_UNPUT
#define YY_NO_UNPUT
#endif

#ifdef _MSC_VER
#define YY_NO_UNISTD_H
#endif

/* Convenience macros matching the QGIS style */
#define TEXT       yylval->text = new QString(QString::fromUtf8(yytext))
#define BOOL(b)    yylval->boolVal = (b)

/* Strip outer double-quotes from a string and unescape "" -> " */
static QString stripVBString(const char *raw)
{
    QString s = QString::fromUtf8(raw);
    // remove surrounding quotes
    s = s.mid(1, s.length() - 2);
    // VBScript escapes " as ""
    s.replace(QLatin1String("\"\""), QLatin1String("\""));
    return s;
}

/* Strip # delimiters from a date literal */
static QString stripDateLit(const char *raw)
{
    QString s = QString::fromUtf8(raw);
    return s.mid(1, s.length() - 2).trimmed();
}

%}

/* -----------------------------------------------------------------------
   Named patterns
   ----------------------------------------------------------------------- */

white           [ \t\r]+
newline         \n

non_ascii       [\x80-\xFF]
id_first        [A-Za-z_]|{non_ascii}
id_next         [A-Za-z0-9_]|{non_ascii}
identifier      {id_first}{id_next}*

dig             [0-9]
hexdig          [0-9A-Fa-f]

/* Integer: plain decimal, &H hex, &O octal */
int_dec         {dig}+
int_hex         "&H"{hexdig}+
int_oct         "&O"[0-7]+

/* Float: must have decimal point or exponent */
num_float       {dig}*"."{dig}+([Ee][+-]?{dig}+)?|{dig}+[Ee][+-]?{dig}+

/* String: double-quoted, "" = escaped quote */
str_char        "\"\""|[^\"\n]
string_lit      "\""{str_char}*"\""

/* Date literal */
date_char       [^#]
date_lit        "#"{date_char}+"#"

%%

    /* -------------------------------------------------------------------
       Whitespace and line continuation
       ------------------------------------------------------------------- */
{white}                 { /* skip */ }
"_"{white}*{newline}    { /* line continuation: ignore newline */ }
"_"{white}*":"          { /* continuation before colon: ignore */ }

    /* -------------------------------------------------------------------
       Comments
       ------------------------------------------------------------------- */
"'"[^\n]*               { /* single-quote comment */ }
(?i:rem){white}[^\n]*   { /* REM comment */ }

    /* -------------------------------------------------------------------
       Significant newline — statement terminator
       ------------------------------------------------------------------- */
{newline}               { return NEWLINE; }

    /* -------------------------------------------------------------------
       Statement separator
       ------------------------------------------------------------------- */
":"                     { return COLON; }

    /* -------------------------------------------------------------------
       Keywords — longest match wins; listed before IDENTIFIER rule
       ------------------------------------------------------------------- */
"AND"           { return KW_AND; }
"BYREF"         { return KW_BYREF; }
"BYVAL"         { return KW_BYVAL; }
"CALL"          { return KW_CALL; }
"CASE"          { return KW_CASE; }
"CLASS"         { return KW_CLASS; }
"CONST"         { return KW_CONST; }
"DEFAULT"       { return KW_DEFAULT; }
"DIM"           { return KW_DIM; }
"DO"            { return KW_DO; }
"EACH"          { return KW_EACH; }
"ELSE"          { return KW_ELSE; }
"ELSEIF"        { return KW_ELSEIF; }
"EMPTY"         { return KW_EMPTY; }
"END"           { return KW_END; }
"EQV"           { return KW_EQV; }
"ERASE"         { return KW_ERASE; }
"ERROR"         { return KW_ERROR; }
"EXIT"          { return KW_EXIT; }
"EXPLICIT"      { return KW_EXPLICIT; }
"FALSE"         { BOOL(false); return KW_FALSE; }
"FOR"           { return KW_FOR; }
"FUNCTION"      { return KW_FUNCTION; }
"GET"           { return KW_GET; }
"GOTO"          { return KW_GOTO; }
"IF"            { return KW_IF; }
"IMP"           { return KW_IMP; }
"IN"            { return KW_IN; }
"IS"            { return KW_IS; }
"LET"           { return KW_LET; }
"LIKE"          { return KW_LIKE; }
"LOOP"          { return KW_LOOP; }
"ME"            { return KW_ME; }
"MOD"           { return KW_MOD; }
"NEW"           { return KW_NEW; }
"NEXT"          { return KW_NEXT; }
"NOT"           { return KW_NOT; }
"NOTHING"       { return KW_NOTHING; }
"NULL"          { return KW_NULL; }
"ON"            { return KW_ON; }
"OPTION"        { return KW_OPTION; }
"OR"            { return KW_OR; }
"PRESERVE"      { return KW_PRESERVE; }
"PRIVATE"       { return KW_PRIVATE; }
"PROPERTY"      { return KW_PROPERTY; }
"PUBLIC"        { return KW_PUBLIC; }
"RANDOMIZE"     { return KW_RANDOMIZE; }
"REDIM"         { return KW_REDIM; }
"RESUME"        { return KW_RESUME; }
"SELECT"        { return KW_SELECT; }
"SET"           { return KW_SET; }
"SHARED"        { return KW_SHARED; }
"STEP"          { return KW_STEP; }
"STOP"          { return KW_STOP; }
"SUB"           { return KW_SUB; }
"THEN"/{white}*\n    { return KW_THEN_BLOCK; }
"THEN"                { return KW_THEN_INLINE; }
"TO"            { return KW_TO; }
"TRUE"          { BOOL(true);  return KW_TRUE; }
"UNTIL"         { return KW_UNTIL; }
"WEND"          { return KW_WEND; }
"WHILE"         { return KW_WHILE; }
"WITH"          { return KW_WITH; }
"XOR"           { return KW_XOR; }

    /* -------------------------------------------------------------------
       Literals
       ------------------------------------------------------------------- */
{num_float}     {
    bool ok;
    yylval->numberFloat = QString::fromLatin1(yytext).toDouble(&ok);
    return NUMBER_FLOAT;
}

{int_hex}       {
    bool ok;
    yylval->numberInt = QString::fromLatin1(yytext + 2).toLongLong(&ok, 16);
    return NUMBER_INT;
}

{int_oct}       {
    bool ok;
    yylval->numberInt = QString::fromLatin1(yytext + 2).toLongLong(&ok, 8);
    return NUMBER_INT;
}

{int_dec}       {
    bool ok;
    yylval->numberInt = QString::fromLatin1(yytext).toLongLong(&ok, 10);
    return NUMBER_INT;
}

{string_lit}    {
    yylval->text = new QString(stripVBString(yytext));
    return STRING_LIT;
}

{date_lit}      {
    yylval->text = new QString(stripDateLit(yytext));
    return DATE_LIT;
}

    /* -------------------------------------------------------------------
       Identifier (after keywords so keywords match first)
       ------------------------------------------------------------------- */
{identifier}    { TEXT; return IDENTIFIER; }

    /* -------------------------------------------------------------------
       Operators
       ------------------------------------------------------------------- */
"<="            { return OP_LE; }
">="            { return OP_GE; }
"<>"            { return OP_NE; }
"<"             { return OP_LT; }
">"             { return OP_GT; }
"="             { return OP_EQ; }
"+"             { return OP_PLUS; }
"-"             { return OP_MINUS; }
"*"             { return OP_MUL; }
"/"             { return OP_DIV; }
"\\"            { return OP_IDIV; }
"^"             { return OP_POW; }
"&"             { return OP_CONCAT; }

    /* -------------------------------------------------------------------
       Punctuation
       ------------------------------------------------------------------- */
"("             { return LPAREN; }
")"             { return RPAREN; }
","             { return COMMA; }
"."             { return DOT; }
"!"             { return BANG; }

    /* -------------------------------------------------------------------
       Catch-all
       ------------------------------------------------------------------- */
.               { return Unknown_CHARACTER; }

%%