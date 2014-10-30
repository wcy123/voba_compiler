#pragma once
typedef struct c_backend_s {
    voba_str_t * decl;
    voba_str_t * start;
    voba_str_t * impl;
    voba_str_t * it;
} c_backend_t;

#define C_BACKEND(r) VOBA_USER_DATA_AS(c_backend_t*, r)
voba_value_t make_c_backend();
