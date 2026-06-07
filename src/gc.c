#include "../include/gc.h"
#include "../include/value.h"
#include "../include/vm.h"

#include <stdio.h>
#include <string.h>

static bool g_gc_trace = false;

void xf_gc_set_trace(bool enabled) {
    g_gc_trace = enabled;
}

bool xf_gc_get_trace(void) {
    return g_gc_trace;
}

static const char *gc_state_name(uint8_t state) {
    return state < XF_STATE_COUNT ? XF_STATE_NAMES[state] : "?";
}

static const char *gc_type_name(uint8_t type) {
    return type < XF_TYPE_COUNT ? XF_TYPE_NAMES[type] : "?";
}

XfGCStats xf_gc_collect_stats(VM *vm) {
    XfGCStats stats;
    memset(&stats, 0, sizeof(stats));

    if (!vm) return stats;

    if (vm->globals) {
        stats.globals_seen = vm->global_count;
    }

    if (g_gc_trace) {
        fprintf(stderr,
                "[gc] begin collect stack_top=%zu globals=%zu\n",
                vm->stack_top,
                stats.globals_seen);
    }

    while (vm->stack_top > 0) {
        size_t idx = vm->stack_top - 1;
        xf_Value v = vm_pop(vm);

        if (g_gc_trace) {
            fprintf(stderr,
                    "[gc] release stack[%zu] type=%s state=%s\n",
                    idx,
                    gc_type_name(v.type),
                    gc_state_name(v.state));
        }

        xf_value_release(v);
        stats.stack_values_released++;
    }

    if (g_gc_trace) {
        fprintf(stderr,
                "[gc] end collect stack_released=%zu globals_seen=%zu\n",
                stats.stack_values_released,
                stats.globals_seen);
    }

    return stats;
}

void xf_gc_collect(VM *vm) {
    (void)xf_gc_collect_stats(vm);
}

void xf_gc_release_vm(VM *vm) {
    (void)vm;
}