#pragma once
#define K(toplevel_env,key) voba_array_at(TOPLEVEL_ENV(toplevel_env)->keywords,k_##key)
#define COLON(toplevel_env,key) voba_array_at(TOPLEVEL_ENV(toplevel_env)->keywords,c_##key)
#define VOBA_KEYWORDS(XX)                              \
    XX(def)                                            \
    XX(fun)                                            \
    XX(quote)                                          \
    XX(import)                                         \
    XX(if)                                             \
    XX(cond)                                           \
    XX(break)                                          \
    XX(let)                                            \
    XX(match)                                          \
    XX(value) /* used by pattern matching*/            \
    XX(for)                                            \
    XX(it)                                             \
    XX(and)                                            \
    XX(or)                                             \
    XX(yield)                                          \
    
#define VOBA_COLON_KEYWORDS(XX)                 \
    XX(if)                                      \
    XX(each)                                    \
    XX(init)                                    \
    XX(accumulate)                              \
    XX(do)

#define VOBA_DECLARE_KEYWORD(key) k_##key,
#define VOBA_DECLARE_COLON_KEYWORD(key) c_##key,

enum voba_keyword_e {
    VOBA_KEYWORDS(VOBA_DECLARE_KEYWORD)
    VOBA_COLON_KEYWORDS(VOBA_DECLARE_COLON_KEYWORD)
    k_vbar,
    K_N_OF_KEYWORDS
};

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
