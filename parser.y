%{
#include <stdio.h>
#include <stdint.h>    
#include <math.h>
#include <voba/include/value.h>
#include "parser.syn.h"
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
      {                                                                 \
          (Current).start_pos   = YYRHSLOC (Rhs, 1).start_pos;          \
          (Current).end_pos    = YYRHSLOC (Rhs, N).end_pos;             \
      }                                                                 \
      else                                                              \
      {                                                                 \
          (Current).start_pos   = (Current).end_pos   =                 \
              YYRHSLOC (Rhs, 0).end_pos;                                \
      }                                                                 \
    while (0)
%}

/* handle locations */
%define api.value.type {voba_value_t}
%define api.pure full
%output "parser.c"
%defines "parser.h"
%locations
%lex-param   {void* scanner}
%parse-param {void* scanner} {voba_value_t * yy_program }
%param       {voba_value_t module}
%defines
%token   T_INT T_FLOAT T_SYMBOL T_STRING
%debug
%code {
    int z1lex (YYSTYPE *lvalp, YYLTYPE *llocp, void *, voba_value_t module);
typedef void * yyscan_t;
void z1error (YYLTYPE * locp, yyscan_t scanner, voba_value_t * p, voba_value_t module, char const *s);
void z1error (YYLTYPE * locp, yyscan_t scanner, voba_value_t * p, voba_value_t module, char const *s) {
   fprintf (stderr, "%s\n", s);
}
};
%% 

program: list_of_sexp {
*yy_program = $$ = $1; syntax_loc($$,&@$);}
;

list_of_sexp:
sexp { $$ = make_syntax(voba_make_array_1($1),@$.start_pos, @$.end_pos); } 
| list_of_sexp sexp { $$ = $1; voba_array_push(SYNTAX($1)->v,$2); syntax_loc($$,&(@$)); } 
;

sexp:
T_INT     
|T_FLOAT  
|T_SYMBOL 
|T_STRING  
| '(' list_of_sexp ')' {$$ = $2; syntax_loc($$,&@$);}
| '(' ')' { $$ = make_syntax(voba_make_array_0(),@$.start_pos,@$.end_pos);}
| error { fprintf(stderr,"TODO ERRORR RECOVERY %d - %d\n",@$.start_pos, @$.end_pos); }
;

