#ifndef DATASCOPE_H
#define DATASCOPE_H

#include "as.h"
#include <stdint.h>

// 全局变量
extern symbolS *text_cur_symbol;
extern char *log_file;

// 全局函数
extern void datascope_init (void);
extern void datascope_clean (void);
extern void update_cur_symbol (symbolS *sym);
extern void update_frag_symbol (void);

#endif // DATASCOPE_H
