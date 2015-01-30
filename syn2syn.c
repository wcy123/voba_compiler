#define EXEC_ONCE_TU_NAME "voba.compiler.syn2syn"
#include <exec_once.h>
#include <voba/value.h>
#include "syn2syn.h"
static voba_value_t syn_tmpt(voba_value_t x, voba_value_t args)
{
    // so do, think about it.....
}
voba_value_t m_def(voba_value_t syn_form, voba_value_t toplevel_env)
{
    voba_value_t form = SYNTAX(syn_form)->v;
    assert(voba_array_len(form) >= 2);
    uint32_t offset = 2; // skip def, args
    voba_value_t la_body = voba_la_from_array1(form,offset);
    voba_value_t syn_f_args = voba_array_at(form,1);
    voba_value_t a_f_args = SYNTAX(syn_f_args)->v;
    voba_value_t la_f_args = voba_la_from_array0(a_f_args);
    assert(voba_la_len(la_args) > 1);
    voba_value_t syn_s_name = voba_la_car(la_f_args);
    voba_value_t s_name = SYNTAX(syn_s_name)->v;
    assert(voba_is_a(s_name,voba_cls_symbol));
    voba_value_t la_args = voba_la_cdr(la_f_args); // skip f;
    la_f_args = VOBA_NIL; // it is changed, so discard the old value
    voba_value_t ret = voba_make_array0();
    voba_push(ret, K(toplevel_env,def));
    voba_push(ret, syn_s_name);
    voba_value_t new_args = voba_la_to_array(la_args);
    voba_value_t syn_new_args = syn_new1(new_args,syn_f_args);
    voba_push(ret, make_syn_const
}
