%option nodebug
%option noyywrap
%{
#include <math.h>
#include <string.h>
#include <assert.h>
#define YY_DECL voba_value_t module_lex(voba_value_t module_info, voba_str_t * content, yyscan_t yyscanner)
#define YY_INPUT(buf,result,max_size)                           \
             {                                                  \
             int c = getchar();                                 \
             result = (c == EOF) ? YY_NULL : (buf[0] = c, 1);   \
             }
int fileno(FILE *stream);
%}
%option outfile="read_module_header_lex.inc"
%option reentrant
%x comment str
DIGIT    [0-9]
ID       [a-zA-Z_><+*/-][a-zA-Z0-9?_><+*/-]*
%%

^[[:space:]]*XX\({ID}\).*$  {
     int i = 0; while(yytext[i++] != '(');
     int j = i; while(yytext[++j] != ')');
     voba_str_t* symbol_name = (voba_str_from_data(yytext + i, j - i));
     voba_array_push(MODULE_INFO(module_info)->symbols, voba_make_string(voba_strdup(symbol_name)));
}
^#define[[:space:]]+VOBA_MODULE_NAME[[:space:]]*\".*\".*$  {
     int i = 0; while(yytext[i++] != '"');
     int j = i; while(yytext[++j] != '"');
     voba_str_t* name = voba_strdup(voba_str_from_data(yytext + i, j - i));
     MODULE_INFO(module_info)->name = voba_make_string(name);
}
^#define[[:space:]]+VOBA_MODULE_ID[[:space:]]*\".*\".*$  {
     int i = 0; while(yytext[i++] != '"');
     int j = i; while(yytext[++j] != '"');
     voba_str_t* name = voba_strdup(voba_str_from_data(yytext + i, j - i));
     MODULE_INFO(module_info)->id = voba_make_string(name);
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
