#pragma once
#include "keywords.h"

// (def (f a1 a2 ...) body ...)
// --> (def f (fun (a1 a2  ...) body ...)
voba_value_t m_def(voba_value_t syn_form, voba_value_t toplevel_env)
