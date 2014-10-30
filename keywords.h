#pragma once
#define K(toplevel_env,key) voba_array_at(TOPLEVEL_ENV(toplevel_env)->keywords,k_##key)
#define VOBA_KEYWORDS(XX)                       \
    XX(def)                                     \
    XX(fun)                                     \
    XX(quote)                                   \
    XX(import)                                  \
    XX(if)                                      \
    XX(cond)                                    \
    XX(let)                                     \
    XX(match)                                   \
    XX(value)                                   \
    XX(for)                                     \
    XX(it)                                      \
    
#define VOBA_DECLARE_KEYWORD(key) k_##key,
enum voba_keyword_e {
    VOBA_KEYWORDS(VOBA_DECLARE_KEYWORD)
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
