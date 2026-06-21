#ifndef LS_ERR_H
#define LS_ERR_H

void err_clear(void);
void err_set(const char *fmt, ...);
int err_has(void);
const char *err_get(void);

#endif