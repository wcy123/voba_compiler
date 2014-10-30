static inline voba_str_t * var_c_id(var_t* var)
{
    voba_value_t syn_s_name = var->syn_s_name;
    return voba_strcat(voba_str_from_char('x',1), voba_str_fmt_int64_t(syn_s_name,16));
}
static inline voba_str_t * var_c_symbol_name(var_t* var)
{
    voba_value_t syn_s_name = var->syn_s_name;
    return voba_value_to_str(voba_symbol_name(SYNTAX(syn_s_name)->v));
}
static inline voba_str_t * quote_string(voba_str_t * s)
{
    voba_str_t * ret = voba_str_empty();
    ret = voba_strcat_char(ret,'"');
    ret = voba_strcat(ret,s);
    ret = voba_strcat_char(ret,'"');
    return ret;
}
static inline voba_str_t* new_uniq_id()
{
    static int c = 0;
    return voba_strcat(VOBA_CONST_CHAR("s"),voba_str_fmt_int32_t(c++,10));
}
static inline voba_str_t* indent(voba_str_t * s)
{
    uint32_t i = 0;
    voba_str_t * ret = voba_str_empty();
    for(i = 0; i < s->len; ++i){
        if(s->data[i] == '\n' && i != s->len-1){
            ret = voba_strcat(ret, VOBA_CONST_CHAR("\n    "));
        }else{
            ret = voba_strcat_char(ret, s->data[i]);
        }
    }
    return ret;
}
static inline void ast2c_template(const char * file, int line,const char * fn, voba_str_t **s, voba_str_t* template, size_t len, voba_str_t* array[])
{
    uint32_t i = 0;
    if(0){
        *s = voba_strcat(*s,voba_str_from_cstr("\n/*"));
        *s = voba_strcat(*s,voba_str_from_cstr(file));
        *s = voba_strcat(*s,voba_str_from_cstr(":"));
        *s = voba_strcat(*s,voba_str_fmt_int32_t(line,10));
        *s = voba_strcat(*s,voba_str_from_cstr(":["));
        *s = voba_strcat(*s,voba_str_from_cstr(fn));
        *s = voba_strcat(*s,voba_str_from_cstr("]"));
        *s = voba_strcat(*s,voba_str_from_cstr("    { */\n"));
    }
    for(i = 0; i < template->len; ){
        if(template->data[i] == '#' && 
           i+1 < template->len &&
           template->data[i+1] >='0' &&
           template->data[i+1] <='9'){
            uint32_t index = template->data[i+1] - '0';
            if(index < len){
                *s = voba_strcat(*s, array[index]);
                i+=2;
            }else{
                fprintf(stderr,__FILE__ ":%d:[%s] out of range; index = %d, len = %ld\n", line,file,
                        index,len);
                assert(0&&"out of range");
            }
        }else if(template->data[i] == '#' && 
                 i+1 < template->len &&
                 template->data[i+1] =='#'){
            *s = voba_strcat_char(*s, '#');
            i+=2;
        }else if(template->data[i] == '\n'){
            *s = voba_strcat(*s,voba_str_from_cstr("\n"));
            i+=1;
        }else{
            *s = voba_strcat_char(*s, template->data[i]);
            i++;
        }
    }
    if(0){
        *s = voba_strcat(*s,voba_str_from_cstr("/*"));
        *s = voba_strcat(*s,voba_str_from_cstr(file));
        *s = voba_strcat(*s,voba_str_from_cstr(":"));
        *s = voba_strcat(*s,voba_str_fmt_int32_t(line,10));
        *s = voba_strcat(*s,voba_str_from_cstr(":["));
        *s = voba_strcat(*s,voba_str_from_cstr(fn));
        *s = voba_strcat(*s,voba_str_from_cstr("]"));
        *s = voba_strcat(*s,voba_str_from_cstr("    } */\n"));
    }
    return;
}
