#include "core/internal.h"

void core_register(SymTable *st) {
    xf_module_t *math_m     = build_math();
    xf_module_t *str_m      = build_str();
    xf_module_t *os_m       = build_os();
    xf_module_t *generics_m = build_generics();
    xf_module_t *ds_m       = build_ds();
    xf_module_t *edit_m     = build_edit();
    xf_module_t *format_m   = build_format();
    xf_module_t *regex_m    = build_regex();
    xf_module_t *process_m  = build_process();

    xf_module_t *core_m = xf_module_new("core");
    xf_module_set(core_m, "math",     xf_val_ok_module(math_m));
    xf_module_set(core_m, "str",      xf_val_ok_module(str_m));
    xf_module_set(core_m, "os",       xf_val_ok_module(os_m));
    xf_module_set(core_m, "generics", xf_val_ok_module(generics_m));
    xf_module_set(core_m, "ds",       xf_val_ok_module(ds_m));
    xf_module_set(core_m, "regex",    xf_val_ok_module(regex_m));
    xf_module_set(core_m, "edit",     xf_val_ok_module(edit_m));
    xf_module_set(core_m, "format",   xf_val_ok_module(format_m));
    xf_module_set(core_m, "process",  xf_val_ok_module(process_m));

    xf_module_release(math_m);   xf_module_release(str_m);
    xf_module_release(os_m);     xf_module_release(generics_m);
    xf_module_release(ds_m);     xf_module_release(regex_m);
    xf_module_release(edit_m);   xf_module_release(format_m);
    xf_module_release(process_m);

    xf_Value core_val = xf_val_ok_module(core_m);
    xf_module_release(core_m);

    xf_Str *name = xf_str_from_cstr("core");
    Symbol *sym  = sym_declare(st, name, SYM_BUILTIN, XF_TYPE_MODULE,
                               (Loc){.source="<core>", .line=0, .col=0});
    if (sym) {
        sym->value      = core_val;
        sym->state      = XF_STATE_OK;
        sym->is_const   = true;
        sym->is_defined = true;
    } else {
        xf_value_release(core_val);
    }
    xf_str_release(name);
}