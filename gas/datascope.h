#ifndef DATASCOPE_H
#define DATASCOPE_H

#include "as.h"
#include <stdint.h>
#include "frags.h"

#ifndef XOM_SECTION_NAME
#define XOM_SECTION_NAME	".xom"
#endif

extern segT xom_section;
extern void xom_section_init (void);

// 全局变量
extern symbolS *text_cur_symbol;
extern const char* cur_func_name;
extern char *log_file;

extern bool is_in_func; /* 当前处理位置是否在function中*/
extern bool is_finish_subseg; /* gas是否已经遍历一遍汇编文件 */
extern unsigned long long text_frag_index;

///////////////// frag 使用的结构体 /////////////////
struct meteheader {
	/* magic "xom" */
	char magic[4];
	
	/* file name size */
	unsigned int name_size;

	/* file name, begin here */
	// char name[1];
} ATTRIBUTE_PACKED;
typedef struct meteheader meteheaderS;
extern meteheaderS mete_header;

struct metedata {
	/* frag在对应汇编文件中的索引号 */
	unsigned int frag_index;

	/* frag的大小 */
	unsigned int frag_size;

	/* frag在汇编结束后的段内偏移 */
	addressT frag_address;

	/* frag元数据属性 */
	struct frag_flags fr_flags;

	/* frag绑定符号的size */
	unsigned int symbol_size;

	/* symbol name, begin here */
	// char name[1];
} ATTRIBUTE_PACKED;
typedef struct metedata metedataS;

extern void write_meteheader (void);
extern void write_metedata (metedataS *data, const char* sym_name);

// 全局函数
extern void datascope_init (void);
extern void datascope_clean (void);
extern void update_cur_symbol (symbolS *sym);
extern void update_frag_symbol (void);

#endif // DATASCOPE_H
