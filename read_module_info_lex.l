%option nodebug
%option noyywrap
%{
#include <math.h>
#include <string.h>
#include <assert.h>
#define YY_DECL voba_value_t module_lex(voba_value_t module_info, voba_str_t * content, yyscan_t yyscanner, YYSTYPE * yylval_param, YYLTYPE * yylloc_param)

#define YY_INPUT(buf,result,max_size)                           \
             {                                                  \
             int c = getchar();                                 \
             result = (c == EOF) ? YY_NULL : (buf[0] = c, 1);   \
             }
int fileno(FILE *stream);
typedef int YYSTYPE ; // it is not used.
// #define YYLTYPE_IS_DECLARED
// struct YYLTYPE
// {
//   uint32_t start_pos;
//   uint32_t end_pos;
// };
typedef struct YYLTYPE YYLTYPE;
#define YY_USER_ACTION                                      \
    yylloc->start_pos = yylloc->end_pos;                    \
    yylloc->end_pos += yyleng;                              \
%}
%option yylineno
%option bison-bridge
%option bison-locations
%option outfile="read_module_info_lex.inc"
%option debug
%option reentrant
%x comment str
DIGIT    [0-9]
ID       [a-zA-Z_><+*/-][a-zA-Z0-9?_><+*/-]*
%%

^[[:space:]]*XX\({ID}\).*$  {
     int i = 0; while(yytext[i++] != '(');
     int j = i; while(yytext[++j] != ')');
     voba_str_t* symbol_name = (voba_str_from_data(yytext + i, j - i));
     voba_array_push(MODULE_INFO(module_info)->a_syn_symbols,
                     make_syntax(voba_make_string(voba_c_id_decode(voba_strdup(symbol_name))),yylloc->start_pos + i, yylloc->start_pos + j));
}
^#define[[:space:]]+VOBA_MODULE_NAME[[:space:]]*\".*\".*$  {
     int i = 0; while(yytext[i++] != '"');
     int j = i; while(yytext[++j] != '"');
     voba_str_t* name = voba_strdup(voba_str_from_data(yytext + i, j - i));
     MODULE_INFO(module_info)->syn_name = make_syntax(voba_make_string(name),yylloc->start_pos + i , yylloc->start_pos + j);
}
^#define[[:space:]]+VOBA_MODULE_ID[[:space:]]*\".*\".*$  {
     int i = 0; while(yytext[i++] != '"');
     int j = i; while(yytext[++j] != '"');
     voba_str_t* name = voba_strdup(voba_str_from_data(yytext + i, j - i));
     MODULE_INFO(module_info)->syn_id = make_syntax(voba_make_string(name), yylloc->start_pos + i , yylloc->start_pos + j);
}
\n   {
}
. {
   //fprintf(stderr,"unrecoganized token %d %s\n", yytext[0], yytext);
}
%%
void v_______foo()
{
   // avoid warning, unused function yyuninput
   if(0){
     yyunput(1,NULL,NULL);
     input(NULL);
   }
}
