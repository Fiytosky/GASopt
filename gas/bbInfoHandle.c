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

// init the global variables
void bbinfo_init(){
  as_warn (_("[bbinfo initialize]"));
  return;
}

// handle bbinfo_jmptbl directive
void jmptable_bbInfo_handler(int ignored ATTRIBUTE_UNUSED){
    return; 
}

// handle bbinfo_funcb directive, it represents function begin
void funcb_bbInfo_handler (int ignored ATTRIBUTE_UNUSED){
    return;
}

// handle bbinfo_funce directive, it represents function end
void funce_bbInfo_handler (int ignored ATTRIBUTE_UNUSED){
    return;
}

// handle bbinfo_bb directive, it represents basic block begin
void bb_bbInfo_handler (int ignored ATTRIBUTE_UNUSED){
    return;
}

// handle bbinfo_be directive, it represents basic block end
void be_bbInfo_handler (int ignored ATTRIBUTE_UNUSED){
    return;
}

void inlineb_bbInfo_handler (int ignored ATTRIBUTE_UNUSED){
    return;
}

void inlinee_bbInfo_handler (int ignored ATTRIBUTE_UNUSED){
    return;
}
