/* bbInfoHandle.h
 *
 * Handle basic block related directives
 */

#ifndef BBINFOHANDLE_H
#define BBINFOHANDLE_H

#include "as.h"
#include <stdint.h>

extern const pseudo_typeS bbInfo_pseudo_table[];

extern void bbinfo_init(void);
// extern char bbinfo_in_text;
#endif
