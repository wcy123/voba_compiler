#pragma once
/** @file */

/** @brief convert a string to a module info object.
    
    @param v the content of module info file
    @return a module info object.
 */
voba_value_t read_module_info(voba_value_t v);
