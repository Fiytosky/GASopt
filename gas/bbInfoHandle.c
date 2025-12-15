/*
 * bbInfoHandle.c
 */

#include "bbInfoHandle.h"
#include "as.h"
#include "symbols.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void jmptable_bbInfo_handler (int);
static void funcb_bbInfo_handler (int);
static void funce_bbInfo_handler (int);
static void bb_bbInfo_handler (int);
static void be_bbInfo_handler (int);
static void inlineb_bbInfo_handler (int);
static void inlinee_bbInfo_handler (int);

char bbinfo_is_collect_sec(asection*);
bbinfo_mbb* init_basic_block(void);
void handwritten_funcb_bbinfo_handler();
void handwritten_funce_bbinfo_handler();

bbinfo_fixup* bbinfo_init_insert_fixup(asection*, int);

const pseudo_typeS bbInfo_pseudo_table[] = {
    {"bbinfo_jmptbl", jmptable_bbInfo_handler, 0},
    {"bbinfo_funb", funcb_bbInfo_handler, 0},
    {"bbinfo_fune", funce_bbInfo_handler, 0},
    {"bbinfo_bb", bb_bbInfo_handler, 0},
    {"bbinfo_be", be_bbInfo_handler, 0},
    {"bbinfo_inlineb", inlineb_bbInfo_handler, 0},
    {"bbinfo_inlinee", inlinee_bbInfo_handler, 0},
    {NULL, NULL, 0}
};

// debug related symbol
const char* symbol_blacklist[] = {
  ".Ldebug",
  ".LASF"
};

///////// global variable
uint32_t cur_function_id; // current function id
uint32_t cur_function_end_id; // current function end id
uint32_t prev_function_id; // prev function id
uint32_t cur_block_id; // global current basic block id
char bbinfo_in_text = 0;
symbolS *last_symbol;

bbinfo_fixup* fixups_list_head; // fixup list
bbinfo_mbb* mbbs_list_head; // basic blocks list
bbinfo_mbb* mbbs_list_tail; // the last element of basic blocks list

int bbinfo_app;

int bbinfo_handwritten_file;
const char* handwritten_bbinfo_func_name;

unsigned int bbinfo_last_inst_size;
int bbinfo_last_inst_offset;
fragS* bbinfo_last_frag;

/////////// local varialbe
unsigned char function_head; // represent that the current basic block is current function's first entry
char bbinfo_handled_ee = 0;


// init the global variables
void bbinfo_init() {
  if (fiy_dsok) {
    as_warn (_("[bbinfo initialize. fiy_dsok = true]"));
  } else {
    as_warn (_("[bbinfo initialize]. fiy_dsok = false"));
  }
  
	mbbs_list_head = NULL;
  mbbs_list_tail = NULL;
	cur_function_id = 0;
  cur_function_end_id = 0;
	prev_function_id = 0;
	cur_block_id = 0;
  last_symbol = NULL;
  fixups_list_head = NULL;

  function_head = 0;

	bbinfo_app = 0;

	bbinfo_last_inst_size = 0;
  bbinfo_last_inst_offset = 0;
  fragS* bbinfo_last_frag = NULL;

	handwritten_bbinfo_func_name = NULL;

	bbinfo_handwritten_file = 1;
	

  return;
}

// if this section is the collected section
// .text, .data.xxx, .rodata.xxxx, .init.xxx, .data.rel
char bbinfo_is_collect_sec(asection *sec){
  if (!sec){
    as_warn(_("[bbinfo]: in function bbinfo_is_new_sec. The section is NULL"));
    return -1;
  }
  const char* sec_name = sec->name;
  char* tmp_pointer = NULL;
  if ((tmp_pointer = strstr(sec_name, ".text")) &&
      tmp_pointer == sec_name){
      return 1;
  }

  if ((tmp_pointer = strstr(sec_name, ".rodata")) && 
	tmp_pointer == sec_name){
	return 1;
      }

  if ((tmp_pointer = strstr(sec_name, ".init_array")) &&
      tmp_pointer == sec_name){
      return 1;
  }

  if ((tmp_pointer = strstr(sec_name, ".data")) &&
      tmp_pointer == sec_name){

    if ((tmp_pointer = strstr(sec_name, ".data.rel.ro")) &&
	tmp_pointer == sec_name){
	return 1;
    }
      return 1;
  }
  return 0;
}

// init the fixup struct and insert it into fixups_list serially
bbinfo_fixup* bbinfo_init_insert_fixup(asection* sec, int offset){

  bbinfo_fixup* result_fixup = malloc(sizeof(bbinfo_fixup));
  // init
  memset (result_fixup, 0, sizeof(bbinfo_fixup));


  if (fixups_list_head == NULL){
    fixups_list_head = result_fixup;
    return result_fixup;
  }

  bbinfo_fixup* prev = NULL;
  bbinfo_fixup* cur = fixups_list_head;
  // find the section that is equal to sec
  while(cur && cur->sec != sec){
    prev = cur;
    cur = cur->next;
  }

  // The list does not have section sec
  if (!cur){
    prev->next = result_fixup;
    return result_fixup;
  }

  // find the proper place accourding to its offset
  while(cur && offset > cur->offset && cur->sec == sec){
    prev = cur;
    cur = cur->next;
  }

  // insert into the head
  if (!prev){
    result_fixup->next = fixups_list_head;
    fixups_list_head = result_fixup;
    return result_fixup;
  }
  prev->next = result_fixup;
  result_fixup->next = cur;
  return result_fixup;
}

// init the bbinfo struct
bbinfo_mbb* init_basic_block(){
  // malloc space
  bbinfo_mbb *result_mbb = malloc(sizeof(bbinfo_mbb));
  memset(result_mbb, 0, sizeof(bbinfo_mbb));
  result_mbb->next = NULL;

  // put it into the global basic blocks list
  if (mbbs_list_head == NULL){
    mbbs_list_head = result_mbb;
  }else{
    mbbs_list_tail->next = result_mbb;
  }
  mbbs_list_tail = result_mbb;
  return result_mbb;
}

// update the last_symbol global variable
// exclude dedug defined label
int update_last_symbol(symbolS *sym){
  unsigned int size = sizeof(symbol_blacklist) / sizeof(char*);
  const char* symbol_name = S_GET_NAME(sym);

  if (!strcmp(symbol_name, "")){
    as_warn (_("[bbInfo]: the symbol name is null"));
    return -1;
  }
// check if the symbol_name is in black list
  for (unsigned int i = 0; i < size; i++){
    if(strstr(symbol_name, symbol_blacklist[i]))
      return -1;
  } 
  last_symbol = sym;
  return 0;
}

void handwritten_funcb_bbinfo_handler(){
  // make sure that current file is handwritten file
  if (!bbinfo_handwritten_file)
    return;
  // if the last basic block is not used, we don't need initialize another basic block
  if (mbbs_list_tail && mbbs_list_tail->is_begin)
    return;
 
  // we type the last basic block type as the end of the function
  if (mbbs_list_tail)
  {
    mbbs_list_tail->type &= (1 << 6);
    mbbs_list_tail->type |= 1;
  }
  
    bbinfo_mbb *cur_mbb = init_basic_block();
    cur_mbb->ID = cur_block_id++;
    cur_mbb->type = 0;
    cur_mbb->offset = -1;
    cur_mbb->size = 0;
    cur_mbb->alignment = 0;
    cur_mbb->num_fixs = 0;
    cur_mbb->fall_through = 0;
    cur_mbb->sec = NULL;
    cur_mbb->parent_id = cur_function_id;
    cur_mbb->is_begin = 1;
}

void handwritten_funce_bbinfo_handler(){
  if (!bbinfo_handwritten_file)
    return;
  if (!mbbs_list_tail)
     as_fatal("[bbinfo]: funce_bbinfo_handler. the mbbs_list_tail is null");
  mbbs_list_tail->type &= (1 << 6);
  mbbs_list_tail->type |= 1;
}

// For handwritten file, add `fake` basic block.
void bbinfo_initbb_handwritten(void){
    bbinfo_mbb *cur_mbb = init_basic_block();

    // init the basic_block element
    cur_mbb->ID = cur_block_id++;
    cur_mbb->type = 0;
    cur_mbb->offset = -1;
    cur_mbb->size = 0;
    cur_mbb->alignment = 0;
    cur_mbb->num_fixs = 0;
    cur_mbb->fall_through = 0;
    cur_mbb->sec = NULL;
    cur_mbb->parent_id = 0;
    cur_mbb->is_begin = 1;
}

// handle bbinfo_jmptbl directive
void jmptable_bbInfo_handler(int ignored ATTRIBUTE_UNUSED){
	offsetT table_size, entry_size;
	table_size = get_absolute_expression();
	SKIP_WHITESPACE();

	entry_size = get_absolute_expression();
  if (last_symbol == NULL) {
    return;
  }

  ///TODO: 当前只关注x86平台的情况

  S_SET_JMPTBL_SIZE(last_symbol, table_size);
  S_SET_JMPTBL_ENTRY_SZ(last_symbol, entry_size);
	// as_warn (_("[handle .bbinfo_jmptbl]"));
  return; 
}

// handle bbinfo_funcb directive, it represents function begin
void funcb_bbInfo_handler (int ignored ATTRIBUTE_UNUSED){
  // as_warn (_("[handle .bbinfo_funb]"));
  prev_function_id = cur_function_id;
  cur_function_id++;
  function_head = 1;

  // current file is c/c++ file
  if (bbinfo_handwritten_file){
    bbinfo_handwritten_file = 0;
  }

  if (mbbs_list_tail && mbbs_list_tail->is_begin)
      return;

  // Here, we initialize the bbinfo_mbb
  // For some specifal case(such as c++ non-virtual thunk to function)
  // gcc can't output basic block information
  bbinfo_mbb *cur_mbb = init_basic_block();
  cur_mbb->ID = cur_block_id++;
  cur_mbb->type = 0;
  cur_mbb->offset = -1;
  cur_mbb->size = 0;
  cur_mbb->alignment = 0;
  cur_mbb->num_fixs = 0;
  cur_mbb->fall_through = 0;
  cur_mbb->sec = NULL;
  cur_mbb->parent_id = cur_function_id;
  cur_mbb->is_begin = 1;

  return; 
}

// handle bbinfo_funce directive, it represents function end
void funce_bbInfo_handler (int ignored ATTRIBUTE_UNUSED){
  // as_warn (_("[handle .bbinfo_fune]"));
  cur_function_end_id++;
  if (!mbbs_list_tail){
    as_fatal("[bbinfo]: funce_bbinfo_handler. the mbbs_list_tail is null");
    exit(-1);
  }
  mbbs_list_tail->type &= (1 << 6);
  mbbs_list_tail->type |= 1;
  if (cur_function_end_id != cur_function_id)
    as_warn(_("[bbInfo]: current function end id don not match current function id"));
  return; 
}

// handle bbinfo_bb directive, it represents basic block begin
void bb_bbInfo_handler (int ignored ATTRIBUTE_UNUSED){
  offsetT fall_through;
	fall_through = get_absolute_expression();

  if (mbbs_list_tail && mbbs_list_tail->is_begin == 1) {
    if (fall_through == 1) {
      mbbs_list_tail->fall_through = 1;
    } else {
      mbbs_list_tail->fall_through = 0;
    }
    return;
  }
  bbinfo_mbb *cur_mbb = init_basic_block();

  // current file is c/c++ file
  if (bbinfo_handwritten_file){
    bbinfo_handwritten_file = 0;
  }

  // init the basic_block element
  cur_mbb->ID = cur_block_id++;
  cur_mbb->type = 0;
  cur_mbb->offset = -1;
  cur_mbb->size = 0;
  cur_mbb->alignment = 0;
  cur_mbb->num_fixs = 0;
  if (fall_through == 1)
    cur_mbb->fall_through = 1;
  else
    cur_mbb->fall_through = 0;
  cur_mbb->sec = NULL;
  cur_mbb->parent_id = cur_function_id;
  cur_mbb->is_begin = 1;
	// as_warn (_("[handle .bbinfo_bb %d]"), fall_through);
  return; 
}

// handle bbinfo_be directive, it represents basic block end
void be_bbInfo_handler (int ignored ATTRIBUTE_UNUSED){
	offsetT fall_through;
	fall_through = get_absolute_expression();
  if (fall_through == 1)
    mbbs_list_tail->fall_through = 1;
  bbinfo_handled_ee = 1;
	// as_warn (_("[handle .bbinfo_be %d]"), fall_through);
  return; 
}

void inlineb_bbInfo_handler (int ignored ATTRIBUTE_UNUSED){
  // as_warn (_("[handle .bbinfo_inlineb]"));
  return; 
}

void inlinee_bbInfo_handler (int ignored ATTRIBUTE_UNUSED){
  // as_warn (_("[handle .bbinfo_inlinee]"));
  return;
}
