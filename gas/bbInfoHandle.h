/* bbInfoHandle.h
 *
 * Handle basic block related directives
 */

#ifndef BBINFOHANDLE_H
#define BBINFOHANDLE_H

#include "as.h"
#include <stdint.h>

// basic block related information
struct basic_block{
  uint32_t ID; // basic block id, every basic block has unique id in an object
  uint8_t type; // basic block type: basic block or function boundary.
    // 0 represent basic block with normal mode ie. arm
    // 1 represents function start with normal mode ie. arm
    // 2 represents object end with normal mode ie. arm
    // 4 represent basic block with special mode ie. thumb
    // 5 represents function start with special mode ie. thumb
    // 6 represents object end with special mode ie. thumb
  uint32_t offset; // offset from the section
  int size; // basic block size, include alignment size
  uint32_t alignment; // basic block alignment size
  uint32_t num_fixs; // number fixups
  unsigned char fall_through; // whether the basic block is fall through
  asection *sec; // which section the basic block belongs to
  struct basic_block *next; // link next basic blosk
  uint32_t parent_id; // function id
  uint8_t is_begin; // if current instruction is the first instruction of this basic block
  uint8_t is_inline; // if current basic block contains inline assemble code or current basic block
  fragS *parent_frag; // this basic block belongs to which frag.
  		      // FIXME. I'm not sure if there exists a basic block cross two fragS.
};

typedef struct basic_block bbinfo_mbb;


extern const pseudo_typeS bbInfo_pseudo_table[];

//////////////////////// global variable
extern uint32_t cur_function_id; // current function id
extern uint32_t cur_function_end_id; // current function end id
extern uint32_t prev_function_id; // prev function id
extern uint32_t cur_block_id; // global current basic block id

extern bbinfo_mbb* mbbs_list_head; // basic blocks list
extern bbinfo_mbb* mbbs_list_tail; // the last element of basic blocks list

extern int bbinfo_app; // If the current basic block contains inline assemble code

// hand-written file information
extern int bbinfo_handwritten_file;
extern const char* handwritten_bbinfo_func_name;

// it is used to begin a new `fake` basic block
extern int bbinfo_last_inst_offset;
extern unsigned int bbinfo_last_inst_size;
extern fragS* bbinfo_last_frag;


///////////////////////// function
extern void handwritten_funcb_bbinfo_handler();
extern void handwritten_funce_bbinfo_handler();
extern void bbinfo_initbb_handwritten(void);

extern void bbinfo_init(void);
// extern char bbinfo_in_text;
#endif
