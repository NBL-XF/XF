#ifndef LS_INTERP_H
#define LS_INTERP_H

#include <stddef.h>

#include "../include/value.h"

Value *interp_step_normal(const Value *term);
Value *interp_reduce_normal(const Value *term, size_t max_steps, size_t *steps_taken, int *reached_limit);

#endif