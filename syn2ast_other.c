static int ok(voba_value_t any) {return 1;}
#define VOBA_DECLARE_KEYWORD(key) k_##key,
enum voba_keyword_e {
    VOBA_KEYWORDS(VOBA_DECLARE_KEYWORD)
    K_N_OF_KEYWORDS
};
#define K(toplevel_env,key) voba_array_at(TOPLEVEL_ENV(toplevel_env)->keywords,k_##key)
static inline int is_keyword(voba_value_t toplevel_env, voba_value_t x)
{
    voba_value_t keywords = TOPLEVEL_ENV(toplevel_env)->keywords;
    for(int i = 0; i < K_N_OF_KEYWORDS; ++i){
        if(voba_eq(x,voba_array_at(keywords,i))){
            return 1;
        }
    }
    return 0;
}
