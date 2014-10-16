#define LEVEL_ERROR 1
#define LEVEL_WARNING 2
inline static void report(int level, voba_str_t * msg,voba_value_t syn,voba_value_t toplevel_env)
{
    switch(level){
    case LEVEL_ERROR:
        TOPLEVEL_ENV(toplevel_env)->n_of_errors ++ ; break;
    case LEVEL_WARNING:
        TOPLEVEL_ENV(toplevel_env)->n_of_warnings ++ ; break;
    default:
        assert(0);
    }
    uint32_t start_line;
    uint32_t end_line;
    uint32_t start_col;
    uint32_t end_col;
    syn_get_line_column(1,syn,&start_line,&start_col);
    syn_get_line_column(0,syn,&end_line,&end_col);
    voba_str_t* s = voba_str_empty();
    s = voba_vstrcat
        (s,
         voba_value_to_str(syntax_source_filename(syn)),
         VOBA_CONST_CHAR(":"),
         voba_str_fmt_uint32_t(start_line,10),
         VOBA_CONST_CHAR(":"),
         voba_str_fmt_uint32_t(start_col,10),
         VOBA_CONST_CHAR(" - "),
         voba_str_fmt_uint32_t(end_line,10),
         VOBA_CONST_CHAR(":"),
         voba_str_fmt_uint32_t(end_col,10),
         VOBA_CONST_CHAR(" error: "),
         msg,
         //VOBA_CONST_CHAR("\n"),
         NULL);
    fwrite(s->data,s->len,1,stderr);
    fprintf(stderr,"    ");
    voba_str_t* c = voba_value_to_str(syntax_source_content(syn));
    uint32_t pos1 = SYNTAX(syn)->start_pos;
    uint32_t pos2 = SYNTAX(syn)->end_pos;
    uint32_t i = pos1;
    for(i = pos1; i != 0; i--){
        if(c->data[i] == '\n') break;
    }
    if(i==0) {fprintf(stderr,"\n");}
    for(;pos2 < c->len; pos2++){
        if(c->data[pos2] == '\n') break;
    }
    fwrite(c->data + i, pos2 - i, 1, stderr);
    if(pos2 == c->len) fprintf(stderr,"\n");
    fprintf(stderr,"\n");
    for(uint32_t n = i==0?0:1; n + i < pos1; ++n){
        fputs(" ",stderr);
    }
    fprintf(stderr,"^\n");
    return;
}
inline static void report_error(voba_str_t * msg,voba_value_t syn,voba_value_t toplevel_env)
{
    report(LEVEL_ERROR,msg,syn,toplevel_env);
}
inline static void report_warn(voba_str_t * msg,voba_value_t syn,voba_value_t toplevel_env)
{
    report(LEVEL_WARNING,msg,syn,toplevel_env);
}
/* Local Variables: */
/* mode:c */
/* coding: undecided-unix */
/* End: */
