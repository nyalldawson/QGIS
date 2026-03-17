/***************************************************************************
                       vbscriptparser.yy
                       -----------------
    Reentrant bison LALR(1) parser for VBScript.
    Modelled on qgssqlstatementparser.yy (QGIS).

    Conflict-free design:
      - seps right-recursive (eliminates "sep . NEWLINE/COLON" s/r)
      - block_body handles leading seps (eliminates seps_opt r/r)
      - trailing seps in block not block_body (eliminates GLR ambiguity)
      -  on single-line If (eliminates 5 nested-if r/r)
      - %expect 1 for the one unavoidable dangling-else s/r
 ***************************************************************************/

%code requires
{
#include <QList>
#include <QVariant>
#include "qgsvbscriptscript.h"
using VS = QgsVBScriptScript;

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void *yyscan_t;
#endif

struct vbs_parser_context
{
    yyscan_t flex_scanner;
    char     errorBuf[2048];
    QgsVBScriptScript::NodeStatementList *rootNode;
    bool     at_stmt_start;   // true -> next IDENTIFIER is IDENTIFIER_STMT
    vbs_parser_context() : flex_scanner(nullptr), rootNode(nullptr),
                           at_stmt_start(true)
    { errorBuf[0] = '\0'; }
};
}

%{
#include <cstdlib>
#include <cstring>
#include "qgsvbscriptscript.h"
#ifdef _MSC_VER
#  pragma warning(disable: 4065)
#  pragma warning(disable: 4702)
#endif
#define YYINCLUDED_STDLIB_H 1
#include "qgsvbscriptparser.hpp"

typedef void *yyscan_t;
typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern int  vbs_lex_init(yyscan_t *scanner);
extern int  vbs_lex_destroy(yyscan_t scanner);
extern int  vbs_lex(YYSTYPE *yylval_param, yyscan_t yyscanner);
extern YY_BUFFER_STATE vbs__scan_string(const char *buf, yyscan_t scanner);
void vbs_error(vbs_parser_context *ctx, const char *msg);
#define scanner ctx->flex_scanner
#define YYERROR_VERBOSE 1
using VS = QgsVBScriptScript;
%}

%define api.pure
%name-prefix "vbs_"
%lex-param   {void *scanner}
%parse-param {vbs_parser_context *ctx}

/* Grammar conflict summary (all resolved correctly by bison defaults):
   7 s/r: LPAREN access-chain extension (bison shifts -- correct).
          THEN_INLINE dangling-else (bison shifts -- correct).
   1 r/r: "If A THEN_INLINE If B THEN_INLINE simple_stmt . KW_ELSE"
          First-rule wins: inner If takes the Else. Correct VBScript semantics.
   No %expect directive: without it bison emits warnings but still generates
   the parser, which is preferable to a fatal error on the 1 r/r conflict. */

%union
{
    VS::Node          *node;
    VS::NodeStatementList  *stmtlist;
    VS::NodeExpressionList  *exprlist;
    VS::NodeParameterList *paramlist;
    VS::NodeDim       *nodedim;
    QList<VS::NodeElseIf*>  *elseiflist;
    QList<VS::NodeCase*>    *caselist;
    VS::NodeElseIf    *elseif;
    VS::NodeCase      *nodecase;
    qlonglong          numberInt;
    double             numberFloat;
    bool               boolVal;
    QString           *text;
}

%start program

%token NEWLINE COLON
/* KW_THEN is split by the lexer into THEN_BLOCK (followed by newline)
   and THEN_INLINE (followed by a statement on the same line).
   This gives LALR(1) the one extra token needed to resolve
   block-form vs inline-form If without any r/r conflicts. */
%token KW_THEN_BLOCK KW_THEN_INLINE
%token KW_AND KW_BYREF KW_BYVAL KW_CALL KW_CASE KW_CLASS KW_CONST
%token KW_DEFAULT KW_DIM KW_DO KW_EACH KW_ELSE KW_ELSEIF KW_EMPTY
%token KW_END KW_EQV KW_ERASE KW_ERROR KW_EXIT KW_EXPLICIT
%token <boolVal> KW_FALSE KW_TRUE
%token KW_FOR KW_FUNCTION KW_GET KW_GOTO KW_IF KW_IMP KW_IN KW_IS
%token KW_LET KW_LIKE KW_LOOP KW_ME KW_MOD KW_NEW KW_NEXT KW_NOT
%token KW_NOTHING KW_NULL KW_ON KW_OPTION KW_OR KW_PRESERVE KW_PRIVATE
%token KW_PROPERTY KW_PUBLIC KW_RANDOMIZE KW_REDIM KW_RESUME
%token KW_SELECT KW_SET KW_SHARED KW_STEP KW_STOP KW_SUB
%token KW_TO KW_UNTIL KW_WEND KW_WHILE KW_WITH KW_XOR
%token OP_LE OP_GE OP_NE OP_LT OP_GT OP_EQ
%token OP_PLUS OP_MINUS OP_MUL OP_DIV OP_IDIV OP_POW OP_CONCAT
%token LPAREN RPAREN COMMA DOT BANG
%token <numberInt>   NUMBER_INT
%token <numberFloat> NUMBER_FLOAT
%token <text>        STRING_LIT DATE_LIT IDENTIFIER IDENTIFIER_STMT
%token Unknown_CHARACTER

%type <stmtlist>  program block block_body
%type <node>      matched_stmt simple_stmt block_construct
%type <node>      option_stmt dim_stmt redim_stmt const_stmt
%type <node>      assign_stmt set_stmt call_stmt
%type <node>      if_block_stmt if_simple_stmt bare_call noident_expr ident_call_arg select_stmt
%type <node>      for_stmt foreach_stmt while_stmt do_stmt with_stmt
%type <node>      exit_stmt on_error_stmt erase_stmt stop_stmt randomize_stmt
%type <node>      sub_def func_def prop_def class_def
%type <node>      expr access_expr primary_expr
%type <exprlist>  expr_list expr_list_opt arg_list arg_list_opt
%type <paramlist> param_list param_list_opt param_item
%type <elseiflist> elseif_list
%type <elseif>    elseif_clause
%type <stmtlist>  else_clause
%type <caselist>  case_list
%type <nodecase>  case_clause
%type <exprlist>  case_expr_list
%type <nodedim>   dim_var_list
%type <text>      visibility_opt name
%type <boolVal>   preserve_opt

/* Operator precedence (low -> high) */
%left  KW_IMP
%left  KW_EQV
%left  KW_XOR
%left  KW_OR
%left  KW_AND
%right KW_NOT
%left  KW_IS KW_LIKE OP_EQ OP_NE OP_LT OP_GT OP_LE OP_GE
%left  OP_CONCAT
%left  OP_PLUS OP_MINUS
%left  OP_MUL OP_DIV
%left  OP_IDIV
%left  KW_MOD
%right UMINUS UPLUS
%right OP_POW
%left  DOT BANG

/* LOWER_THAN_EQ: lowest-precedence sentinel.
   Applied to "simple_stmt: access_expr" so that when bison sees OP_EQ
   after reducing a(i) to access_expr, it shifts OP_EQ (higher precedence)
   rather than reducing to simple_stmt. Fixes "a(i) = 0" assignments. */
%nonassoc LOWER_THAN_EQ

/* THEN_STMT: gives "If expr Then simple_stmt" lower precedence than KW_ELSE.
   When bison sees KW_ELSE after "If expr Then simple_stmt .", it shifts
   (attaches Else to this If) rather than reducing. Correct VBScript semantics. */
%nonassoc THEN_STMT
%nonassoc KW_ELSE

/* Destructors for leak-free error recovery.
   <stmtlist> is intentionally omitted: ownership always flows forward. */
%destructor { delete $$; } <node>
%destructor { delete $$; } <exprlist>
%destructor { delete $$; } <paramlist>
%destructor { delete $$; } <text>
%destructor { delete $$; } <elseif>
%destructor { delete $$; } <nodecase>
%destructor { qDeleteAll(*$$); delete $$; } <elseiflist>
%destructor { qDeleteAll(*$$); delete $$; } <caselist>

%define parse.error verbose

%%

program
    : block { ctx->rootNode = $1; }
    ;

/* --- Block / separator ---
   seps is right-recursive: always shifts next NEWLINE/COLON.
   block_body handles leading seps; block handles trailing seps.
   This design produces zero s/r and zero r/r from separators. */
sep
    : NEWLINE
    | COLON
    ;

seps
    : sep
    | sep seps
    ;

block_body
    : matched_stmt                         { $$ = new VS::NodeStatementList(); $$->append($1); }
    | if_simple_stmt                       { $$ = new VS::NodeStatementList(); $$->append($1); }
    | bare_call                            { $$ = new VS::NodeStatementList(); $$->append($1); }
    | seps matched_stmt                    { $$ = new VS::NodeStatementList(); $$->append($2); }
    | seps if_simple_stmt                  { $$ = new VS::NodeStatementList(); $$->append($2); }
    | seps bare_call                       { $$ = new VS::NodeStatementList(); $$->append($2); }
    | block_body seps matched_stmt         { $1->append($3); $$ = $1; }
    | block_body seps if_simple_stmt       { $1->append($3); $$ = $1; }
    | block_body seps bare_call            { $1->append($3); $$ = $1; }
    ;

/* block: sequence of matched_stmts only.
   All statements inside a block must be fully matched (every If has End If).
   This is the invariant that eliminates all nested-If r/r conflicts.
   Inline Ifs (unmatched) are valid as then/else branches of other inline
   Ifs, but not as standalone statements inside block constructs. */
block
    : /* empty */
        { $$ = new VS::NodeStatementList(); }
    | seps
        { $$ = new VS::NodeStatementList(); }
    | block_body
        { $$ = $1; }
    | block_body seps
        { $$ = $1; }
    ;

/* --- Statements ---
   Classical matched/unmatched split to eliminate ALL dangling-if
   ambiguities (both shift/reduce and reduce/reduce) in LALR(1).

   matched_stmt: every If branch is fully closed. A matched stmt
     inside a then-branch means the following Else/ElseIf/End unambiguously
     belongs to the OUTER If.

   unmatched_stmt: the last open branch has no closing keyword.
     An unmatched stmt can only appear where no Else follows
     (i.e., as the final statement in a block, or as the then-branch
     of another unmatched If).

   block_body contains only matched_stmts, ensuring that every
   closing keyword encountered belongs to its own enclosing construct.
   The block as a whole may end with an unmatched_stmt (optional).    */

/* Non-If statements that are always unambiguously terminated */
simple_stmt
    : option_stmt       { $$ = $1; }
    | dim_stmt          { $$ = $1; }
    | redim_stmt        { $$ = $1; }
    | const_stmt        { $$ = $1; }
    | assign_stmt       { $$ = $1; }
    | set_stmt          { $$ = $1; }
    | call_stmt         { $$ = $1; }
    | exit_stmt         { $$ = $1; }
    | on_error_stmt     { $$ = $1; }
    | erase_stmt        { $$ = $1; }
    | stop_stmt         { $$ = $1; }
    | randomize_stmt    { $$ = $1; }
    | access_expr                     { $$ = $1; }
    ;

/* Block constructs (always closed by an End/Next/Loop/Wend keyword) */
block_construct
    : select_stmt       { $$ = $1; }
    | for_stmt          { $$ = $1; }
    | foreach_stmt      { $$ = $1; }
    | while_stmt        { $$ = $1; }
    | do_stmt           { $$ = $1; }
    | with_stmt         { $$ = $1; }
    | sub_def           { $$ = $1; }
    | func_def          { $$ = $1; }
    | prop_def          { $$ = $1; }
    | class_def         { $$ = $1; }
    ;

/* matched_stmt: all If branches are closed.
   If present as then-branch, the following Else cannot be "stolen"
   by an outer If.

   Inline If without Else is matched when the then-branch is a simple_stmt
   (no If of any kind). Since simple_stmt contains no If constructs,
   "If cond Then simple_stmt" is self-terminating at the NEWLINE after
   simple_stmt -- no ambiguity about what closes it.
   This covers the very common: If flag Then x = 1 */
matched_stmt
    : simple_stmt       { $$ = $1; }
    | block_construct   { $$ = $1; }
    | if_block_stmt     { $$ = $1; }
    | KW_IF expr KW_THEN_INLINE matched_stmt KW_ELSE matched_stmt
        { auto *b=new VS::NodeStatementList(); b->append($4);
          auto *e=new VS::NodeStatementList(); e->append($6);
          $$ = new VS::NodeIf($2, b, {}, e); }
    ;

/* if_simple_stmt: "If cond Then simple_stmt" -- no Else, no End If. */
if_simple_stmt
    : KW_IF expr KW_THEN_INLINE simple_stmt
        { auto *b=new VS::NodeStatementList(); b->append($4);
          $$ = new VS::NodeIf($2, b, {}, nullptr); }
    | KW_IF expr KW_THEN_INLINE bare_call
        { auto *b=new VS::NodeStatementList(); b->append($4);
          $$ = new VS::NodeIf($2, b, {}, nullptr); }
    ;

/* bare_call: "Proc arg1, arg2" -- procedure call without Call keyword.
   Both the callee and first argument use ident_call_arg (IDENTIFIER-rooted
   access chains) rather than access_expr. This eliminates the LPAREN s/r
   conflicts: after "Sub Foo name", "name" is an ident_call_arg and LPAREN
   can only mean param_list_opt -- never a bare_call noident_expr -- because
   noident_expr has no LPAREN alternative.

   Supported forms:
     Callee.Method 42          literal first arg (noident_expr)
     Callee.Method "str"       string first arg
     Callee.Method bar         identifier first arg (ident_call_arg)
     Callee.Method bar.Baz     member-access first arg
     Callee.Method bar + 1     binary-op first arg (inline rules below)
     Callee.Method bar, x, y   multiple args via COMMA arg_list */
bare_call
    : access_expr noident_expr
        { auto *l = new VS::NodeExpressionList(); l->append($2);
          $$ = new VS::NodeFunctionCall($1, l); }
    | access_expr noident_expr COMMA arg_list
        { $4->prepend($2); $$ = new VS::NodeFunctionCall($1, $4); }
    | access_expr ident_call_arg
        { auto *l = new VS::NodeExpressionList(); l->append($2);
          $$ = new VS::NodeFunctionCall($1, l); }
    | access_expr ident_call_arg COMMA arg_list
        { $4->prepend($2); $$ = new VS::NodeFunctionCall($1, $4); }
    | access_expr ident_call_arg OP_PLUS expr
        { auto *l = new VS::NodeExpressionList(); l->append(new VS::NodeBinaryOperator(VS::BinaryOperator::boAdd, $2, $4));
          $$ = new VS::NodeFunctionCall($1, l); }
    | access_expr ident_call_arg OP_PLUS expr COMMA arg_list
        { $6->prepend(new VS::NodeBinaryOperator(VS::BinaryOperator::boAdd, $2, $4)); $$ = new VS::NodeFunctionCall($1, $6); }
    | access_expr ident_call_arg OP_MINUS expr
        { auto *l = new VS::NodeExpressionList(); l->append(new VS::NodeBinaryOperator(VS::BinaryOperator::boSubtract, $2, $4));
          $$ = new VS::NodeFunctionCall($1, l); }
    | access_expr ident_call_arg OP_MINUS expr COMMA arg_list
        { $6->prepend(new VS::NodeBinaryOperator(VS::BinaryOperator::boSubtract, $2, $4)); $$ = new VS::NodeFunctionCall($1, $6); }
    | access_expr ident_call_arg OP_MUL expr
        { auto *l = new VS::NodeExpressionList(); l->append(new VS::NodeBinaryOperator(VS::BinaryOperator::boMultiply, $2, $4));
          $$ = new VS::NodeFunctionCall($1, l); }
    | access_expr ident_call_arg OP_MUL expr COMMA arg_list
        { $6->prepend(new VS::NodeBinaryOperator(VS::BinaryOperator::boMultiply, $2, $4)); $$ = new VS::NodeFunctionCall($1, $6); }
    | access_expr ident_call_arg OP_DIV expr
        { auto *l = new VS::NodeExpressionList(); l->append(new VS::NodeBinaryOperator(VS::BinaryOperator::boDivide, $2, $4));
          $$ = new VS::NodeFunctionCall($1, l); }
    | access_expr ident_call_arg OP_DIV expr COMMA arg_list
        { $6->prepend(new VS::NodeBinaryOperator(VS::BinaryOperator::boDivide, $2, $4)); $$ = new VS::NodeFunctionCall($1, $6); }
    | access_expr ident_call_arg OP_IDIV expr
        { auto *l = new VS::NodeExpressionList(); l->append(new VS::NodeBinaryOperator(VS::BinaryOperator::boIntegerDivide, $2, $4));
          $$ = new VS::NodeFunctionCall($1, l); }
    | access_expr ident_call_arg OP_IDIV expr COMMA arg_list
        { $6->prepend(new VS::NodeBinaryOperator(VS::BinaryOperator::boIntegerDivide, $2, $4)); $$ = new VS::NodeFunctionCall($1, $6); }
    | access_expr ident_call_arg OP_POW expr
        { auto *l = new VS::NodeExpressionList(); l->append(new VS::NodeBinaryOperator(VS::BinaryOperator::boPower, $2, $4));
          $$ = new VS::NodeFunctionCall($1, l); }
    | access_expr ident_call_arg OP_POW expr COMMA arg_list
        { $6->prepend(new VS::NodeBinaryOperator(VS::BinaryOperator::boPower, $2, $4)); $$ = new VS::NodeFunctionCall($1, $6); }
    | access_expr ident_call_arg KW_MOD expr
        { auto *l = new VS::NodeExpressionList(); l->append(new VS::NodeBinaryOperator(VS::BinaryOperator::boMod, $2, $4));
          $$ = new VS::NodeFunctionCall($1, l); }
    | access_expr ident_call_arg KW_MOD expr COMMA arg_list
        { $6->prepend(new VS::NodeBinaryOperator(VS::BinaryOperator::boMod, $2, $4)); $$ = new VS::NodeFunctionCall($1, $6); }
    | access_expr ident_call_arg OP_CONCAT expr
        { auto *l = new VS::NodeExpressionList(); l->append(new VS::NodeBinaryOperator(VS::BinaryOperator::boConcat, $2, $4));
          $$ = new VS::NodeFunctionCall($1, l); }
    | access_expr ident_call_arg OP_CONCAT expr COMMA arg_list
        { $6->prepend(new VS::NodeBinaryOperator(VS::BinaryOperator::boConcat, $2, $4)); $$ = new VS::NodeFunctionCall($1, $6); }
    | access_expr ident_call_arg OP_EQ expr
        { auto *l = new VS::NodeExpressionList(); l->append(new VS::NodeBinaryOperator(VS::BinaryOperator::boEQ, $2, $4));
          $$ = new VS::NodeFunctionCall($1, l); }
    | access_expr ident_call_arg OP_EQ expr COMMA arg_list
        { $6->prepend(new VS::NodeBinaryOperator(VS::BinaryOperator::boEQ, $2, $4)); $$ = new VS::NodeFunctionCall($1, $6); }
    | access_expr ident_call_arg OP_NE expr
        { auto *l = new VS::NodeExpressionList(); l->append(new VS::NodeBinaryOperator(VS::BinaryOperator::boNE, $2, $4));
          $$ = new VS::NodeFunctionCall($1, l); }
    | access_expr ident_call_arg OP_NE expr COMMA arg_list
        { $6->prepend(new VS::NodeBinaryOperator(VS::BinaryOperator::boNE, $2, $4)); $$ = new VS::NodeFunctionCall($1, $6); }
    | access_expr ident_call_arg OP_LT expr
        { auto *l = new VS::NodeExpressionList(); l->append(new VS::NodeBinaryOperator(VS::BinaryOperator::boLT, $2, $4));
          $$ = new VS::NodeFunctionCall($1, l); }
    | access_expr ident_call_arg OP_LT expr COMMA arg_list
        { $6->prepend(new VS::NodeBinaryOperator(VS::BinaryOperator::boLT, $2, $4)); $$ = new VS::NodeFunctionCall($1, $6); }
    | access_expr ident_call_arg OP_GT expr
        { auto *l = new VS::NodeExpressionList(); l->append(new VS::NodeBinaryOperator(VS::BinaryOperator::boGT, $2, $4));
          $$ = new VS::NodeFunctionCall($1, l); }
    | access_expr ident_call_arg OP_GT expr COMMA arg_list
        { $6->prepend(new VS::NodeBinaryOperator(VS::BinaryOperator::boGT, $2, $4)); $$ = new VS::NodeFunctionCall($1, $6); }
    | access_expr ident_call_arg OP_LE expr
        { auto *l = new VS::NodeExpressionList(); l->append(new VS::NodeBinaryOperator(VS::BinaryOperator::boLE, $2, $4));
          $$ = new VS::NodeFunctionCall($1, l); }
    | access_expr ident_call_arg OP_LE expr COMMA arg_list
        { $6->prepend(new VS::NodeBinaryOperator(VS::BinaryOperator::boLE, $2, $4)); $$ = new VS::NodeFunctionCall($1, $6); }
    | access_expr ident_call_arg OP_GE expr
        { auto *l = new VS::NodeExpressionList(); l->append(new VS::NodeBinaryOperator(VS::BinaryOperator::boGE, $2, $4));
          $$ = new VS::NodeFunctionCall($1, l); }
    | access_expr ident_call_arg OP_GE expr COMMA arg_list
        { $6->prepend(new VS::NodeBinaryOperator(VS::BinaryOperator::boGE, $2, $4)); $$ = new VS::NodeFunctionCall($1, $6); }
    | access_expr ident_call_arg KW_AND expr
        { auto *l = new VS::NodeExpressionList(); l->append(new VS::NodeBinaryOperator(VS::BinaryOperator::boAnd, $2, $4));
          $$ = new VS::NodeFunctionCall($1, l); }
    | access_expr ident_call_arg KW_AND expr COMMA arg_list
        { $6->prepend(new VS::NodeBinaryOperator(VS::BinaryOperator::boAnd, $2, $4)); $$ = new VS::NodeFunctionCall($1, $6); }
    | access_expr ident_call_arg KW_OR expr
        { auto *l = new VS::NodeExpressionList(); l->append(new VS::NodeBinaryOperator(VS::BinaryOperator::boOr, $2, $4));
          $$ = new VS::NodeFunctionCall($1, l); }
    | access_expr ident_call_arg KW_OR expr COMMA arg_list
        { $6->prepend(new VS::NodeBinaryOperator(VS::BinaryOperator::boOr, $2, $4)); $$ = new VS::NodeFunctionCall($1, $6); }
    | access_expr ident_call_arg KW_XOR expr
        { auto *l = new VS::NodeExpressionList(); l->append(new VS::NodeBinaryOperator(VS::BinaryOperator::boXor, $2, $4));
          $$ = new VS::NodeFunctionCall($1, l); }
    | access_expr ident_call_arg KW_XOR expr COMMA arg_list
        { $6->prepend(new VS::NodeBinaryOperator(VS::BinaryOperator::boXor, $2, $4)); $$ = new VS::NodeFunctionCall($1, $6); }
    | access_expr ident_call_arg KW_EQV expr
        { auto *l = new VS::NodeExpressionList(); l->append(new VS::NodeBinaryOperator(VS::BinaryOperator::boEqv, $2, $4));
          $$ = new VS::NodeFunctionCall($1, l); }
    | access_expr ident_call_arg KW_EQV expr COMMA arg_list
        { $6->prepend(new VS::NodeBinaryOperator(VS::BinaryOperator::boEqv, $2, $4)); $$ = new VS::NodeFunctionCall($1, $6); }
    | access_expr ident_call_arg KW_IMP expr
        { auto *l = new VS::NodeExpressionList(); l->append(new VS::NodeBinaryOperator(VS::BinaryOperator::boImp, $2, $4));
          $$ = new VS::NodeFunctionCall($1, l); }
    | access_expr ident_call_arg KW_IMP expr COMMA arg_list
        { $6->prepend(new VS::NodeBinaryOperator(VS::BinaryOperator::boImp, $2, $4)); $$ = new VS::NodeFunctionCall($1, $6); }
    | access_expr ident_call_arg KW_IS expr
        { auto *l = new VS::NodeExpressionList(); l->append(new VS::NodeBinaryOperator(VS::BinaryOperator::boIs, $2, $4));
          $$ = new VS::NodeFunctionCall($1, l); }
    | access_expr ident_call_arg KW_IS expr COMMA arg_list
        { $6->prepend(new VS::NodeBinaryOperator(VS::BinaryOperator::boIs, $2, $4)); $$ = new VS::NodeFunctionCall($1, $6); }
    | access_expr ident_call_arg KW_LIKE expr
        { auto *l = new VS::NodeExpressionList(); l->append(new VS::NodeBinaryOperator(VS::BinaryOperator::boLike, $2, $4));
          $$ = new VS::NodeFunctionCall($1, l); }
    | access_expr ident_call_arg KW_LIKE expr COMMA arg_list
        { $6->prepend(new VS::NodeBinaryOperator(VS::BinaryOperator::boLike, $2, $4)); $$ = new VS::NodeFunctionCall($1, $6); }
    ;

/* ident_call_arg: the access-chain part of an IDENTIFIER-first call arg.
   Only handles DOT/BANG/LPAREN suffixes on the initial IDENTIFIER.
   Binary operators are handled separately by ident_bin_suffix, which
   starts with an operator token -- guaranteeing no reduce/reduce with expr. */
ident_call_arg
    : IDENTIFIER
        { $$ = new VS::NodeIdent(*$1); delete $1; }
    | ident_call_arg DOT name
        { $$ = new VS::NodeMember($1, *$3); delete $3; }
    | ident_call_arg BANG name
        { $$ = new VS::NodeMember($1, *$3); delete $3; }
    | ident_call_arg LPAREN arg_list_opt RPAREN
        { $$ = new VS::NodeFunctionCall($1, $3); }
    ;

/* ident_bin_suffix removed -- expanded inline into bare_call rules below */

/* noident_expr: an expression whose first token is NOT an IDENTIFIER.
   This guarantees no conflict with assign_stmt after access_expr. */
noident_expr
    : NUMBER_INT        { $$ = new VS::NodeLiteral(QVariant((qlonglong)$1)); }
    | NUMBER_FLOAT      { $$ = new VS::NodeLiteral(QVariant($1)); }
    | STRING_LIT        { $$ = new VS::NodeLiteral(QVariant(*$1)); delete $1; }
    | DATE_LIT          { $$ = new VS::NodeLiteral(QVariant(*$1)); delete $1; }
    | KW_TRUE           { $$ = new VS::NodeLiteral(QVariant(true)); }
    | KW_FALSE          { $$ = new VS::NodeLiteral(QVariant(false)); }
    | KW_EMPTY          { $$ = new VS::NodeLiteral(QVariant()); }
    | KW_NOTHING        { $$ = new VS::NodeLiteral(QVariant()); }
    | KW_NULL           { $$ = new VS::NodeLiteral(QVariant()); }
    | KW_ME             { $$ = new VS::NodeIdent(QStringLiteral("Me")); }
    | KW_NEW name       { $$ = new VS::NodeFunctionCall(new VS::NodeIdent(*$2),
                                               new VS::NodeExpressionList()); delete $2; }
    | OP_MINUS expr %prec UMINUS { $$ = new VS::NodeUnaryOperator(VS::UnaryOperator::uoMinus, $2); }
    | OP_PLUS  expr %prec UPLUS  { $$ = new VS::NodeUnaryOperator(VS::UnaryOperator::uoPos, $2); }
    | KW_NOT expr                { $$ = new VS::NodeUnaryOperator(VS::UnaryOperator::uoNot, $2); }
    ;




option_stmt
    : KW_OPTION KW_EXPLICIT
        { $$ = new VS::NodeIdent(QStringLiteral("OptionExplicit")); }
    ;

dim_stmt
    : visibility_opt KW_DIM dim_var_list
        { if ($1) { $3->setVisibility(*$1); delete $1; } $$ = $3; }
    | KW_PUBLIC dim_var_list
        { $2->setVisibility(QStringLiteral("Public")); $$ = $2; }
    | KW_PRIVATE dim_var_list
        { $2->setVisibility(QStringLiteral("Private")); $$ = $2; }
    ;

dim_var_list
    : name
        { $$ = new VS::NodeDim();
          VS::DimVar v; v.name=*$1; delete $1; $$->append(v); }
    | name LPAREN expr_list_opt RPAREN
        { $$ = new VS::NodeDim();
          VS::DimVar v; v.name=*$1; delete $1; v.dims=$3; $$->append(v); }
    | dim_var_list COMMA name
        { VS::DimVar v; v.name=*$3; delete $3; $1->append(v); $$=$1; }
    | dim_var_list COMMA name LPAREN expr_list_opt RPAREN
        { VS::DimVar v; v.name=*$3; delete $3; v.dims=$5; $1->append(v); $$=$1; }
    ;

redim_stmt
    : KW_REDIM preserve_opt dim_var_list
        {
            auto *r = new VS::NodeReDim($2);
            for (const auto &v : $3->variables()) r->append(v);
            const_cast<QList<VS::DimVar>&>($3->variables()).clear();
            delete $3; $$ = r;
        }
    ;

preserve_opt
    : /* empty */ { $$ = false; }
    | KW_PRESERVE { $$ = true; }
    ;

const_stmt
    : visibility_opt KW_CONST name OP_EQ expr
        { QString v=$1?*$1:QString(); delete $1;
          $$ = new VS::NodeConst(v, *$3, $5); delete $3; }
    ;

assign_stmt
    : KW_LET access_expr OP_EQ expr  { $$ = new VS::NodeAssign($2, $4, false); }
    | access_expr OP_EQ expr          { $$ = new VS::NodeAssign($1, $3, false); }
    ;

set_stmt
    : KW_SET access_expr OP_EQ expr  { $$ = new VS::NodeAssign($2, $4, true); }
    ;

call_stmt
    : KW_CALL access_expr { $$ = $2; }
    ;

/* --- If statements ---
   Split into two non-terminals, which is what eliminates the r/r:

   if_block_stmt: the block form, always closed by End If.
     Used in closed_stmt -- unambiguous in block_body because End If
     is always consumed as part of this rule, never left dangling.

   if_inline_stmt: the single-line form, no End If.
     Used in open_stmt -- only appears as the last item in a block,
     or as the then/else branch of another inline If.

   The nested-If situation that previously caused r/r:
     block_body: ...closed_stmt* open_stmt?
     Inside if_block_stmt's block, the last item may be an open_stmt
     (inline If). When bison sees KW_END after that inline If, it
     has no ambiguity: KW_END cannot be part of open_stmt (which has
     no closing keyword), so it must belong to the enclosing
     if_block_stmt. Zero r/r conflicts. */

if_block_stmt
    : KW_IF expr KW_THEN_BLOCK block elseif_list else_clause KW_END KW_IF
        { $$ = new VS::NodeIf($2, $4, *$5, $6); delete $5; }
    | KW_IF expr KW_THEN_INLINE NEWLINE block elseif_list else_clause KW_END KW_IF
        { $$ = new VS::NodeIf($2, $5, *$6, $7); delete $6; }
    ;



elseif_list
    : /* empty */                  { $$ = new QList<VS::NodeElseIf*>(); }
    | elseif_list elseif_clause    { $1->append($2); $$ = $1; }
    ;

elseif_clause
    : KW_ELSEIF expr KW_THEN_BLOCK block
        { $$ = new VS::NodeElseIf($2, $4); }
    | KW_ELSEIF expr KW_THEN_INLINE NEWLINE block
        { $$ = new VS::NodeElseIf($2, $5); }
    ;

else_clause
    : /* empty */    { $$ = nullptr; }
    | KW_ELSE block  { $$ = $2; }
    ;

select_stmt
    : KW_SELECT KW_CASE expr seps case_list KW_END KW_SELECT
        { $$ = new VS::NodeSelect($3, *$5); delete $5; }
    ;

case_list
    : /* empty */                  { $$ = new QList<VS::NodeCase*>(); }
    | case_list case_clause        { $1->append($2); $$ = $1; }
    ;

case_clause
    : KW_CASE case_expr_list block { $$ = new VS::NodeCase($2, $3); }
    | KW_CASE KW_ELSE block        { $$ = new VS::NodeCase(nullptr, $3); }
    ;

case_expr_list
    : expr                        { $$ = new VS::NodeExpressionList(); $$->append($1); }
    | case_expr_list COMMA expr   { $1->append($3); $$ = $1; }
    ;

for_stmt
    : KW_FOR name OP_EQ expr KW_TO expr block KW_NEXT
        { $$ = new VS::NodeFor(*$2, $4, $6, nullptr, $7); delete $2; }
    | KW_FOR name OP_EQ expr KW_TO expr KW_STEP expr block KW_NEXT
        { $$ = new VS::NodeFor(*$2, $4, $6, $8, $9); delete $2; }
    ;

foreach_stmt
    : KW_FOR KW_EACH name KW_IN expr block KW_NEXT
        { $$ = new VS::NodeForEach(*$3, $5, $6); delete $3; }
    ;

while_stmt
    : KW_WHILE expr block KW_WEND
        { $$ = new VS::NodeWhile($2, $3); }
    ;

do_stmt
    : KW_DO block KW_LOOP
        { $$ = new VS::NodeDo(VS::DoMode::doInfinite, nullptr, $2); }
    | KW_DO KW_WHILE expr block KW_LOOP
        { $$ = new VS::NodeDo(VS::DoMode::doWhilePre,  $3, $4); }
    | KW_DO KW_UNTIL expr block KW_LOOP
        { $$ = new VS::NodeDo(VS::DoMode::doUntilPre,  $3, $4); }
    | KW_DO block KW_LOOP KW_WHILE expr
        { $$ = new VS::NodeDo(VS::DoMode::doWhilePost, $5, $2); }
    | KW_DO block KW_LOOP KW_UNTIL expr
        { $$ = new VS::NodeDo(VS::DoMode::doUntilPost, $5, $2); }
    ;

with_stmt
    : KW_WITH expr block KW_END KW_WITH
        { $$ = new VS::NodeWith($2, $3); }
    ;

exit_stmt
    : KW_EXIT KW_SUB      { $$ = new VS::NodeExit(QStringLiteral("Sub")); }
    | KW_EXIT KW_FUNCTION { $$ = new VS::NodeExit(QStringLiteral("Function")); }
    | KW_EXIT KW_FOR      { $$ = new VS::NodeExit(QStringLiteral("For")); }
    | KW_EXIT KW_DO       { $$ = new VS::NodeExit(QStringLiteral("Do")); }
    | KW_EXIT KW_PROPERTY { $$ = new VS::NodeExit(QStringLiteral("Property")); }
    ;

on_error_stmt
    : KW_ON KW_ERROR KW_RESUME KW_NEXT
        { $$ = new VS::NodeOnError(VS::NodeOnError::Mode::ResumeNext); }
    | KW_ON KW_ERROR KW_GOTO NUMBER_INT
        { $$ = new VS::NodeOnError(VS::NodeOnError::Mode::GoTo0); }
    ;

erase_stmt
    : KW_ERASE name
        { $$ = new VS::NodeIdent(QStringLiteral("Erase:")+*$2); delete $2; }
    ;

stop_stmt
    : KW_STOP { $$ = new VS::NodeIdent(QStringLiteral("Stop")); }
    ;

randomize_stmt
    : KW_RANDOMIZE
        { $$ = new VS::NodeIdent(QStringLiteral("Randomize")); }
    | KW_RANDOMIZE expr
        { auto *l=new VS::NodeExpressionList(); l->append($2);
          $$ = new VS::NodeFunctionCall(new VS::NodeIdent(QStringLiteral("Randomize")), l); }
    ;

sub_def
    : visibility_opt KW_SUB name param_list_opt block KW_END KW_SUB
        { QString v=$1?*$1:QString(); delete $1;
          $$ = new VS::NodeSubDef(v, *$3, $4, $5); delete $3; }
    ;

func_def
    : visibility_opt KW_FUNCTION name param_list_opt block KW_END KW_FUNCTION
        { QString v=$1?*$1:QString(); delete $1;
          $$ = new VS::NodeFuncDef(v, *$3, $4, $5); delete $3; }
    ;

prop_def
    : visibility_opt KW_PROPERTY KW_GET name param_list_opt block KW_END KW_PROPERTY
        { QString v=$1?*$1:QString(); delete $1;
          $$ = new VS::NodePropDef(v, VS::PropKind::pkGet, *$4, $5, $6); delete $4; }
    | visibility_opt KW_PROPERTY KW_LET name param_list_opt block KW_END KW_PROPERTY
        { QString v=$1?*$1:QString(); delete $1;
          $$ = new VS::NodePropDef(v, VS::PropKind::pkLet, *$4, $5, $6); delete $4; }
    | visibility_opt KW_PROPERTY KW_SET name param_list_opt block KW_END KW_PROPERTY
        { QString v=$1?*$1:QString(); delete $1;
          $$ = new VS::NodePropDef(v, VS::PropKind::pkSet, *$4, $5, $6); delete $4; }
    ;

class_def
    : KW_CLASS name block KW_END KW_CLASS
        { $$ = new VS::NodeClassDef(*$2, $3); delete $2; }
    ;

visibility_opt
    : /* empty */ { $$ = nullptr; }
    | KW_PUBLIC   { $$ = new QString(QStringLiteral("Public")); }
    | KW_PRIVATE  { $$ = new QString(QStringLiteral("Private")); }
    ;

param_list_opt
    : /* empty */              { $$ = new VS::NodeParameterList(); }
    | LPAREN RPAREN            { $$ = new VS::NodeParameterList(); }
    | LPAREN param_list RPAREN { $$ = $2; }
    ;

param_list
    : param_item                        { $$ = $1; }
    | param_list COMMA param_item
        { for (const auto &p : $3->parameters()) $1->append(p);
          delete $3; $$ = $1; }
    ;

param_item
    : name
        { $$ = new VS::NodeParameterList();
          VS::Parameter p; p.name=*$1; delete $1; p.byRef=true;  p.isArray=false; $$->append(p); }
    | KW_BYREF name
        { $$ = new VS::NodeParameterList();
          VS::Parameter p; p.name=*$2; delete $2; p.byRef=true;  p.isArray=false; $$->append(p); }
    | KW_BYVAL name
        { $$ = new VS::NodeParameterList();
          VS::Parameter p; p.name=*$2; delete $2; p.byRef=false; p.isArray=false; $$->append(p); }
    | name LPAREN RPAREN
        { $$ = new VS::NodeParameterList();
          VS::Parameter p; p.name=*$1; delete $1; p.byRef=true;  p.isArray=true;  $$->append(p); }
    | KW_BYREF name LPAREN RPAREN
        { $$ = new VS::NodeParameterList();
          VS::Parameter p; p.name=*$2; delete $2; p.byRef=true;  p.isArray=true;  $$->append(p); }
    | KW_BYVAL name LPAREN RPAREN
        { $$ = new VS::NodeParameterList();
          VS::Parameter p; p.name=*$2; delete $2; p.byRef=false; p.isArray=true;  $$->append(p); }
    ;

/* --- Expressions --- */
expr
    : expr KW_IMP  expr  { $$ = new VS::NodeBinaryOperator(VS::BinaryOperator::boImp,    $1, $3); }
    | expr KW_EQV  expr  { $$ = new VS::NodeBinaryOperator(VS::BinaryOperator::boEqv,    $1, $3); }
    | expr KW_XOR  expr  { $$ = new VS::NodeBinaryOperator(VS::BinaryOperator::boXor,    $1, $3); }
    | expr KW_OR   expr  { $$ = new VS::NodeBinaryOperator(VS::BinaryOperator::boOr,     $1, $3); }
    | expr KW_AND  expr  { $$ = new VS::NodeBinaryOperator(VS::BinaryOperator::boAnd,    $1, $3); }
    | KW_NOT expr        { $$ = new VS::NodeUnaryOperator(VS::UnaryOperator::uoNot,     $2);     }
    | expr KW_IS   expr  { $$ = new VS::NodeBinaryOperator(VS::BinaryOperator::boIs,     $1, $3); }
    | expr KW_LIKE expr  { $$ = new VS::NodeBinaryOperator(VS::BinaryOperator::boLike,   $1, $3); }
    | expr OP_LT   expr  { $$ = new VS::NodeBinaryOperator(VS::BinaryOperator::boLT,     $1, $3); }
    | expr OP_GT   expr  { $$ = new VS::NodeBinaryOperator(VS::BinaryOperator::boGT,     $1, $3); }
    | expr OP_LE   expr  { $$ = new VS::NodeBinaryOperator(VS::BinaryOperator::boLE,     $1, $3); }
    | expr OP_GE   expr  { $$ = new VS::NodeBinaryOperator(VS::BinaryOperator::boGE,     $1, $3); }
    | expr OP_EQ   expr  { $$ = new VS::NodeBinaryOperator(VS::BinaryOperator::boEQ,     $1, $3); }
    | expr OP_NE   expr  { $$ = new VS::NodeBinaryOperator(VS::BinaryOperator::boNE,     $1, $3); }
    | expr OP_CONCAT expr { $$ = new VS::NodeBinaryOperator(VS::BinaryOperator::boConcat, $1, $3); }
    | expr OP_PLUS  expr { $$ = new VS::NodeBinaryOperator(VS::BinaryOperator::boAdd,    $1, $3); }
    | expr OP_MINUS expr { $$ = new VS::NodeBinaryOperator(VS::BinaryOperator::boSubtract,    $1, $3); }
    | expr OP_MUL   expr { $$ = new VS::NodeBinaryOperator(VS::BinaryOperator::boMultiply,    $1, $3); }
    | expr OP_DIV   expr { $$ = new VS::NodeBinaryOperator(VS::BinaryOperator::boDivide,    $1, $3); }
    | expr OP_IDIV  expr { $$ = new VS::NodeBinaryOperator(VS::BinaryOperator::boIntegerDivide,   $1, $3); }
    | expr KW_MOD   expr { $$ = new VS::NodeBinaryOperator(VS::BinaryOperator::boMod,    $1, $3); }
    | OP_MINUS expr %prec UMINUS { $$ = new VS::NodeUnaryOperator(VS::UnaryOperator::uoMinus, $2); }
    | OP_PLUS  expr %prec UPLUS  { $$ = new VS::NodeUnaryOperator(VS::UnaryOperator::uoPos, $2); }
    | expr OP_POW   expr { $$ = new VS::NodeBinaryOperator(VS::BinaryOperator::boPower,    $1, $3); }
    | access_expr        { $$ = $1; }
    ;

access_expr
    : primary_expr
        { $$ = $1; }
    | access_expr DOT name
        { $$ = new VS::NodeMember($1, *$3); delete $3; }
    | access_expr BANG name
        { $$ = new VS::NodeMember($1, *$3); delete $3; }
    | access_expr LPAREN arg_list_opt RPAREN
        { $$ = new VS::NodeFunctionCall($1, $3); }
    ;

primary_expr
    : NUMBER_INT     { $$ = new VS::NodeLiteral(QVariant((qlonglong)$1)); }
    | NUMBER_FLOAT   { $$ = new VS::NodeLiteral(QVariant($1)); }
    | STRING_LIT     { $$ = new VS::NodeLiteral(QVariant(*$1)); delete $1; }
    | DATE_LIT       { $$ = new VS::NodeLiteral(QVariant(*$1)); delete $1; }
    | KW_TRUE        { $$ = new VS::NodeLiteral(QVariant(true)); }
    | KW_FALSE       { $$ = new VS::NodeLiteral(QVariant(false)); }
    | KW_EMPTY       { $$ = new VS::NodeLiteral(QVariant()); }
    | KW_NOTHING     { $$ = new VS::NodeLiteral(QVariant()); }
    | KW_NULL        { $$ = new VS::NodeLiteral(QVariant()); }
    | KW_ME          { $$ = new VS::NodeIdent(QStringLiteral("Me")); }
    | IDENTIFIER     { $$ = new VS::NodeIdent(*$1); delete $1; }
    | LPAREN expr RPAREN { $$ = $2; }
    | KW_NEW name    { $$ = new VS::NodeFunctionCall(new VS::NodeIdent(*$2),
                                             new VS::NodeExpressionList()); delete $2; }
    ;

arg_list_opt
    : /* empty */ { $$ = new VS::NodeExpressionList(); }
    | arg_list    { $$ = $1; }
    ;

arg_list
    : expr                { $$ = new VS::NodeExpressionList(); $$->append($1); }
    | arg_list COMMA expr { $1->append($3); $$ = $1; }
    | arg_list COMMA      { $1->append(new VS::NodeLiteral(QVariant())); $$ = $1; }
    ;


expr_list_opt
    : /* empty */ { $$ = new VS::NodeExpressionList(); }
    | expr_list   { $$ = $1; }
    ;

expr_list
    : expr                   { $$ = new VS::NodeExpressionList(); $$->append($1); }
    | expr_list COMMA expr   { $1->append($3); $$ = $1; }
    ;

name
    : IDENTIFIER { $$ = $1; }
    | KW_DEFAULT { $$ = new QString(QStringLiteral("Default")); }
    | KW_ERROR   { $$ = new QString(QStringLiteral("Error")); }
    | KW_STEP    { $$ = new QString(QStringLiteral("Step")); }
    ;

%%

QgsVBScriptScript::Node *vbs_parse_string(const QString &code, QString &errorMsg)
{
    vbs_parser_context ctx;
    vbs_lex_init(&ctx.flex_scanner);
    vbs__scan_string(code.toUtf8().constData(), ctx.flex_scanner);
    int res = vbs_parse(&ctx);
    vbs_lex_destroy(ctx.flex_scanner);
    if (res == 0 && ctx.rootNode)
        return ctx.rootNode;
    errorMsg = QString::fromUtf8(ctx.errorBuf);
    delete ctx.rootNode;
    return nullptr;
}

void vbs_error(vbs_parser_context *ctx, const char *msg)
{
    if (msg) {
        // Include line number from the reentrant lexer
        qstrncpy(ctx->errorBuf, msg, sizeof(ctx->errorBuf) - 1);
    }
}