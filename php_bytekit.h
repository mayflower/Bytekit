/*
  +----------------------------------------------------------------------+
  | Bytekit                                                              |
  +----------------------------------------------------------------------+
  | Copyright (c) 2008-2009 SektionEins GmbH                             |
  +----------------------------------------------------------------------+
  | This source file is subject to version 1.0 of the Bytekit license,   |
  | that is bundled with this package in the file LICENSE, and is        |
  | available at through the world-wide-web at                           |
  | http://www.bytekit.org/license.txt                                   |
  | If you did not receive a copy of the Bytekit license and are unable  |
  | to obtain it through the world-wide-web, please send a note to       |
  | license@bytekit.org so we can mail you a copy immediately.           |
  +----------------------------------------------------------------------+
  | Author: Stefan Esser <stefan.esser@sektioneins.de>                   |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_BYTEKIT_H
#define PHP_BYTEKIT_H

#define BYTEKIT_VERSION "0.1.1"

#ifdef ZTS
#include "TSRM.h"
#endif

extern zend_module_entry bytekit_module_entry;
#define phpext_bytekit_ptr &bytekit_module_entry

PHP_MINIT_FUNCTION(bytekit);
PHP_MSHUTDOWN_FUNCTION(bytekit);
PHP_MINFO_FUNCTION(bytekit);

PHP_FUNCTION(bytekit_disassemble_file);

ZEND_BEGIN_MODULE_GLOBALS(bytekit)
        zval *compile_errors;
        zend_bool in_error_cb;
                
        zend_op *op_array_opcodes;
        char *opcodes_base;
ZEND_END_MODULE_GLOBALS(bytekit)

#ifdef ZTS
#define BYTEKIT_G(v) TSRMG(bytekit_globals_id, zend_bytekit_globals *, v)
#else
#define BYTEKIT_G(v) (bytekit_globals.v)
#endif

/* Compatibility */
#ifndef ZEND_ISSET_ISEMPTY_MASK
#define ZEND_ISSET_ISEMPTY_MASK	(ZEND_ISSET | ZEND_ISEMPTY)
#endif

typedef struct _bytekit_define_list {
        long val;
        char *str;
        long flags;   /* flags for first opline */
        long flags2;  /* flags for second opline */
} bytekit_define_list;


#define BYTEKIT_UNKNOWN                 "UNKNOWN"

/* Operand Flags */
#define BYTEKIT_SRC_MASK                  15
#define BYTEKIT_SRC_RES1                  1
#define BYTEKIT_SRC_RES2                  2
#define BYTEKIT_SRC_OP1                   3
#define BYTEKIT_SRC_OP2                   4
#define BYTEKIT_SRC_OP3                   5
#define BYTEKIT_SRC_OP4                   6
#define BYTEKIT_SRC_EXT1                  7
#define BYTEKIT_SRC_EXT2                  8

#define BYTEKIT_OPERAND_WRITTEN           (1 << 4)

/* Operand Types */
#define BYTEKIT_TYPE_VALUE                1
#define BYTEKIT_TYPE_CV                   2
#define BYTEKIT_TYPE_VAR                  3
#define BYTEKIT_TYPE_TMP_VAR              4
#define BYTEKIT_TYPE_SYMBOL               5

/* Edge Types */
#define BYTEKIT_EDGE_TRUE                 0
#define BYTEKIT_EDGE_FALSE                1
#define BYTEKIT_EDGE_NORMAL               2
#define BYTEKIT_EDGE_EXCEPTION            3

/* Fake Opcodes */

#if PHP_VERSION_ID < 50300
#define ZEND_GOTO                               222
#define ZEND_INIT_NS_FCALL_BY_NAME              222
#define ZEND_DECLARE_CONST                      222
#define ZEND_DECLARE_INHERITED_CLASS_DELAYED    222
#define ZEND_JMP_SET                            222
#define ZEND_DECLARE_LAMBDA_FUNCTION            222
#endif

/* Disassemble File Flags */

#define BYTEKIT_SHOW_SPECIALS   1

/* Opcode Flags */

#define BYTEKIT_OP_TYPE_MASK    7
#define BYTEKIT_OP_WRITE        8
#define BYTEKIT_OP_HIDE         16
#define BYTEKIT_OP_USED         (BYTEKIT_OP_TYPE_MASK | BYTEKIT_OP_WRITE)

#define BYTEKIT_OP_CVAR         1
#define BYTEKIT_OP_TVAR         2
#define BYTEKIT_OP_NUM          3
#define BYTEKIT_OP_OPNUM        4
#define BYTEKIT_OP_JMPADDR      5

#define BYTEKIT_RES_SHIFT       0
#define BYTEKIT_OP1_SHIFT       5
#define BYTEKIT_OP2_SHIFT       10
#define BYTEKIT_EXT_SHIFT       15
#define BYTEKIT_TYPE_SHIFT      20

#define BYTEKIT_OP_STOP         ( 1 << BYTEKIT_TYPE_SHIFT )

#define BYTEKIT_RES_TYPE_MASK   ( BYTEKIT_OP_TYPE_MASK << BYTEKIT_RES_SHIFT )
#define BYTEKIT_RES_WRITE       ( BYTEKIT_OP_WRITE     << BYTEKIT_RES_SHIFT )
#define BYTEKIT_RES_HIDE        ( BYTEKIT_OP_HIDE      << BYTEKIT_RES_SHIFT )
#define BYTEKIT_RES_USED        ( BYTEKIT_OP_USED      << BYTEKIT_RES_SHIFT )
#define BYTEKIT_RES_CVAR        ( BYTEKIT_OP_CVAR      << BYTEKIT_RES_SHIFT )
#define BYTEKIT_RES_TVAR        ( BYTEKIT_OP_TVAR      << BYTEKIT_RES_SHIFT )
#define BYTEKIT_RES_NUM         ( BYTEKIT_OP_NUM       << BYTEKIT_RES_SHIFT )
#define BYTEKIT_RES_OPNUM       ( BYTEKIT_OP_OPNUM     << BYTEKIT_RES_SHIFT )
#define BYTEKIT_RES_JMPADDR     ( BYTEKIT_OP_JMPADDR   << BYTEKIT_RES_SHIFT )

#define BYTEKIT_RES_FORCE       0x80000000

#define BYTEKIT_OP1_TYPE_MASK   ( BYTEKIT_OP_TYPE_MASK << BYTEKIT_OP1_SHIFT )
#define BYTEKIT_OP1_WRITE       ( BYTEKIT_OP_WRITE     << BYTEKIT_OP1_SHIFT )
#define BYTEKIT_OP1_HIDE        ( BYTEKIT_OP_HIDE      << BYTEKIT_OP1_SHIFT )
#define BYTEKIT_OP1_USED        ( BYTEKIT_OP_USED      << BYTEKIT_OP1_SHIFT )
#define BYTEKIT_OP1_CVAR        ( BYTEKIT_OP_CVAR      << BYTEKIT_OP1_SHIFT )
#define BYTEKIT_OP1_TVAR        ( BYTEKIT_OP_TVAR      << BYTEKIT_OP1_SHIFT )
#define BYTEKIT_OP1_NUM         ( BYTEKIT_OP_NUM       << BYTEKIT_OP1_SHIFT )
#define BYTEKIT_OP1_OPNUM       ( BYTEKIT_OP_OPNUM     << BYTEKIT_OP1_SHIFT )
#define BYTEKIT_OP1_JMPADDR     ( BYTEKIT_OP_JMPADDR   << BYTEKIT_OP1_SHIFT )

#define BYTEKIT_OP2_TYPE_MASK   ( BYTEKIT_OP_TYPE_MASK << BYTEKIT_OP2_SHIFT )
#define BYTEKIT_OP2_WRITE       ( BYTEKIT_OP_WRITE     << BYTEKIT_OP2_SHIFT )
#define BYTEKIT_OP2_HIDE        ( BYTEKIT_OP_HIDE      << BYTEKIT_OP2_SHIFT )
#define BYTEKIT_OP2_USED        ( BYTEKIT_OP_USED      << BYTEKIT_OP2_SHIFT )
#define BYTEKIT_OP2_CVAR        ( BYTEKIT_OP_CVAR      << BYTEKIT_OP2_SHIFT )
#define BYTEKIT_OP2_TVAR        ( BYTEKIT_OP_TVAR      << BYTEKIT_OP2_SHIFT )
#define BYTEKIT_OP2_NUM         ( BYTEKIT_OP_NUM       << BYTEKIT_OP2_SHIFT )
#define BYTEKIT_OP2_OPNUM       ( BYTEKIT_OP_OPNUM     << BYTEKIT_OP2_SHIFT )
#define BYTEKIT_OP2_JMPADDR     ( BYTEKIT_OP_JMPADDR   << BYTEKIT_OP2_SHIFT )

#define BYTEKIT_EXT_SPECIAL     ( 1 << BYTEKIT_EXT_SHIFT )
#define BYTEKIT_EXT_NUM         ( 2 << BYTEKIT_EXT_SHIFT )
#define BYTEKIT_EXT_OPNUM       ( 3 << BYTEKIT_EXT_SHIFT )
#define BYTEKIT_EXT_FETCH       ( 4 << BYTEKIT_EXT_SHIFT )
#define BYTEKIT_EXT_TVAR        ( 5 << BYTEKIT_EXT_SHIFT )

#define BYTEKIT_EXT_USED        ( 15 << BYTEKIT_EXT_SHIFT )
#define BYTEKIT_EXT_TYPE_MASK   ( 15 << BYTEKIT_EXT_SHIFT )
#define BYTEKIT_EXT_HIDE        ( 16 << BYTEKIT_EXT_SHIFT )

static bytekit_define_list bytekit_constant_names[] = {
    { BYTEKIT_OP_TYPE_MASK,             "OP_TYPE_MASK" },
    { BYTEKIT_OP_WRITE,                 "OP_WRITE" },
    { BYTEKIT_OP_HIDE,                  "OP_HIDE" },
    { BYTEKIT_OP_USED,                  "OP_USED" },

    { BYTEKIT_OP_CVAR,                  "OP_CVAR" },
    { BYTEKIT_OP_TVAR,                  "OP_TVAR" },
    { BYTEKIT_OP_NUM,                   "OP_NUM" },
    { BYTEKIT_OP_OPNUM,                 "OP_OPNUM" },
    { BYTEKIT_OP_JMPADDR,               "OP_JMPADDR" },

    { BYTEKIT_RES_SHIFT,                "RES_SHIFT" },
    { BYTEKIT_OP1_SHIFT,                "OP1_SHIFT" },
    { BYTEKIT_OP2_SHIFT,                "OP2_SHIFT" },
    { BYTEKIT_EXT_SHIFT,                "EXT_SHIFT" },
    { BYTEKIT_TYPE_SHIFT,               "TYPE_SHIFT" },

    { BYTEKIT_OP_STOP,                  "OP_STOP" },

    { BYTEKIT_RES_TYPE_MASK,            "RES_TYPE_MASK" },
    { BYTEKIT_RES_WRITE,                "RES_WRITE" },
    { BYTEKIT_RES_HIDE,                 "RES_HIDE" },
    { BYTEKIT_RES_USED,                 "RES_USED" },
    { BYTEKIT_RES_CVAR,                 "RES_CVAR" },
    { BYTEKIT_RES_TVAR,                 "RES_TVAR" },
    { BYTEKIT_RES_NUM,                  "RES_NUM" },
    { BYTEKIT_RES_OPNUM,                "RES_OPNUM" },
    { BYTEKIT_RES_JMPADDR,              "RES_JMPADDR" },

    { BYTEKIT_RES_FORCE,                "RES_FORCE" },

    { BYTEKIT_OP1_TYPE_MASK,            "OP1_TYPE_MASK" },
    { BYTEKIT_OP1_WRITE,                "OP1_WRITE" },
    { BYTEKIT_OP1_HIDE,                 "OP1_HIDE" },
    { BYTEKIT_OP1_USED,                 "OP1_USED" },
    { BYTEKIT_OP1_CVAR,                 "OP1_CVAR" },
    { BYTEKIT_OP1_TVAR,                 "OP1_TVAR" },
    { BYTEKIT_OP1_NUM,                  "OP1_NUM" },
    { BYTEKIT_OP1_OPNUM,                "OP1_OPNUM" },
    { BYTEKIT_OP1_JMPADDR,              "OP1_JMPADDR" },

    { BYTEKIT_OP2_TYPE_MASK,            "OP2_TYPE_MASK" },
    { BYTEKIT_OP2_WRITE,                "OP2_WRITE" },
    { BYTEKIT_OP2_HIDE,                 "OP2_HIDE" },
    { BYTEKIT_OP2_USED,                 "OP2_USED" },
    { BYTEKIT_OP2_CVAR,                 "OP2_CVAR" },
    { BYTEKIT_OP2_TVAR,                 "OP2_TVAR" },
    { BYTEKIT_OP2_NUM,                  "OP2_NUM" },
    { BYTEKIT_OP2_OPNUM,                "OP2_OPNUM" },
    { BYTEKIT_OP2_JMPADDR,              "OP2_JMPADDR" },

    { BYTEKIT_EXT_SPECIAL,              "EXT_SPECIAL" },
    { BYTEKIT_EXT_NUM,                  "EXT_NUM" },
    { BYTEKIT_EXT_OPNUM,                "EXT_OPNUM" },
    { BYTEKIT_EXT_FETCH,                "EXT_FETCH" },
    { BYTEKIT_EXT_TVAR,                 "EXT_TVAR" },

    { BYTEKIT_EXT_USED,                 "EXT_USED" },
    { BYTEKIT_EXT_HIDE,                 "EXT_HIDE" },

    { BYTEKIT_SHOW_SPECIALS,            "SHOW_SPECIALS" },

    { BYTEKIT_SRC_MASK,                 "SRC_MASK" },
    { BYTEKIT_SRC_RES1,                 "SRC_RES1" },
    { BYTEKIT_SRC_RES2,                 "SRC_RES2" },
    { BYTEKIT_SRC_OP1,                  "SRC_OP1" },
    { BYTEKIT_SRC_OP2,                  "SRC_OP2" },
    { BYTEKIT_SRC_OP3,                  "SRC_OP3" },
    { BYTEKIT_SRC_OP4,                  "SRC_OP4" },
    { BYTEKIT_SRC_EXT1,                 "SRC_EXT1" },
    { BYTEKIT_SRC_EXT2,                 "SRC_EXT2" },

    { BYTEKIT_OPERAND_WRITTEN,          "OPERAND_WRITTEN" },

    { BYTEKIT_TYPE_VALUE,               "TYPE_VALUE" },
    { BYTEKIT_TYPE_CV,                  "TYPE_CV" },
    { BYTEKIT_TYPE_VAR,                 "TYPE_VAR" },
    { BYTEKIT_TYPE_TMP_VAR,             "TYPE_TMP_VAR" },
    { BYTEKIT_TYPE_SYMBOL,              "TYPE_SYMBOL" },

    { BYTEKIT_EDGE_TRUE,                "EDGE_TRUE" },
    { BYTEKIT_EDGE_FALSE,               "EDGE_FALSE" },
    { BYTEKIT_EDGE_NORMAL,              "EDGE_NORMAL" },
    { BYTEKIT_EDGE_EXCEPTION,           "EDGE_EXCEPTION" },

    { 0, NULL }    
};

static bytekit_define_list bytekit_zend_constant_names[] = {
    { IS_CONST,                         "IS_CONST" },
    { IS_TMP_VAR,                       "IS_TMP_VAR" },
    { IS_VAR,                           "IS_VAR" },
    { IS_UNUSED,                        "IS_UNUSED" },
    { IS_CV,                            "IS_CV" },
    { EXT_TYPE_UNUSED,                  "EXT_TYPE_UNUSED" },
    { ZEND_FETCH_CLASS_SELF,            "FETCH_CLASS_SELF" },
    { ZEND_FETCH_CLASS_PARENT,          "FETCH_CLASS_PARENT" },
    { IS_NULL,                          "IS_NULL" },
    { IS_LONG,                          "IS_LONG" },
    { IS_DOUBLE,                        "IS_DOUBLE" },
    { IS_BOOL,                          "IS_BOOL" },
    { IS_ARRAY,                         "IS_ARRAY" },
    { IS_OBJECT,                        "IS_OBJECT" },
    { IS_STRING,                        "IS_STRING" },
    { IS_RESOURCE,                      "IS_RESOURCE" },
    { IS_CONSTANT,                      "IS_CONSTANT" },
    { IS_CONSTANT_ARRAY,                "IS_CONSTANT_ARRAY" },
    { ZEND_ISSET,                       "ISSET" },
    { ZEND_ISEMPTY,                     "ISEMPTY" },
    { ZEND_FETCH_STATIC_MEMBER,         "FETCH_STATIC_MEMBER"},
    { ZEND_FETCH_LOCAL,                 "FETCH_LOCAL"},
    { ZEND_FETCH_GLOBAL,                "FETCH_GLOBAL"},
    { ZEND_FETCH_GLOBAL_LOCK,           "FETCH_GLOBAL_LOCK"},
    { ZEND_FE_FETCH_WITH_KEY,           "FE_FETCH_WITH_KEY"},
    { ZEND_ACC_PUBLIC,                  "ACC_PUBLIC" },
    { ZEND_ACC_PRIVATE,                 "ACC_PRIVATE" },
    { ZEND_ACC_PROTECTED,               "ACC_PROTECTED" },
    { ZEND_ACC_PPP_MASK,                "ACC_PPP_MASK" },
    { ZEND_ACC_INTERFACE,               "ACC_INTERFACE" },
    { ZEND_ACC_FINAL,                   "ACC_FINAL" },
    { ZEND_ACC_ABSTRACT,                "ACC_ABSTRACT" },
    { ZEND_ACC_IMPLEMENTED_ABSTRACT,    "ACC_IMPLEMENTED_ABSTRACT" },
    { ZEND_ACC_STATIC,                  "ACC_STATIC" },
    { ZEND_ACC_SHADOW,                  "ACC_SHADOW" },
    { ZEND_ACC_FINAL_CLASS,             "ACC_FINAL_CLASS" },
    { ZEND_ACC_IMPLICIT_ABSTRACT_CLASS, "ACC_IMPLICIT_ABSTRACT_CLASS" },
    { ZEND_ACC_EXPLICIT_ABSTRACT_CLASS, "ACC_EXPLICIT_ABSTRACT_CLASS" },
    { 0, NULL }
};

static bytekit_define_list bytekit_opcode_names[] = {
    { ZEND_NOP,                             "NOP",                              0, 0 },
    { ZEND_ADD,                             "ADD",                              BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_SUB,                             "SUB",                              BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_MUL,                             "MUL",                              BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_DIV,                             "DIV",                              BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_MOD,                             "MOD",                              BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_SL,                              "SL",                               BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_SR,                              "SR",                               BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_CONCAT,                          "CONCAT",                           BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_BW_OR,                           "BW_OR",                            BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_BW_AND,                          "BW_AND",                           BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_BW_XOR,                          "BW_XOR",                           BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_BW_NOT,                          "BW_NOT",                           BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR, 0 },
    { ZEND_BOOL_NOT,                        "BOOL_NOT",                         BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR, 0 },
    { ZEND_BOOL_XOR,                        "BOOL_XOR",                         BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_IS_IDENTICAL,                    "IS_IDENTICAL",                     BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_IS_NOT_IDENTICAL,                "IS_NOT_IDENTICAL",                 BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_IS_EQUAL,                        "IS_EQUAL",                         BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_IS_NOT_EQUAL,                    "IS_NOT_EQUAL",                     BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_IS_SMALLER,                      "IS_SMALLER",                       BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_IS_SMALLER_OR_EQUAL,             "IS_SMALLER_OR_EQUAL",              BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_CAST,                            "CAST",                             BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_QM_ASSIGN,                       "QM_ASSIGN",                        BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR, 0 },
    { ZEND_ASSIGN_ADD,                      "ASSIGN_ADD",                       BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_ASSIGN_SUB,                      "ASSIGN_SUB",                       BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_ASSIGN_MUL,                      "ASSIGN_MUL",                       BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_ASSIGN_DIV,                      "ASSIGN_DIV",                       BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_ASSIGN_MOD,                      "ASSIGN_MOD",                       BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_ASSIGN_SL,                       "ASSIGN_SL",                        BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_ASSIGN_SR,                       "ASSIGN_SR",                        BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_ASSIGN_CONCAT,                   "ASSIGN_CONCAT",                    BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_ASSIGN_BW_OR,                    "ASSIGN_BW_OR",                     BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_ASSIGN_BW_AND,                   "ASSIGN_BW_AND",                    BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_ASSIGN_BW_XOR,                   "ASSIGN_BW_XOR",                    BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_PRE_INC,                         "PRE_INC",                          BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR, 0 },
    { ZEND_PRE_DEC,                         "PRE_DEC",                          BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR, 0 },
    { ZEND_POST_INC,                        "POST_INC",                         BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR, 0 },
    { ZEND_POST_DEC,                        "POST_DEC",                         BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR, 0 },
    { ZEND_ASSIGN,                          "ASSIGN",                           BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_ASSIGN_REF,                      "ASSIGN_REF",                       BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_NUM, 0 },
    { ZEND_ECHO,                            "ECHO",                             BYTEKIT_OP1_CVAR, 0 },
    { ZEND_PRINT,                           "PRINT",                            BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR, 0 },
    { ZEND_JMP,                             "JMP",                              BYTEKIT_OP1_JMPADDR, 0 },
    { ZEND_JMPZ,                            "JMPZ",                             BYTEKIT_OP1_CVAR | BYTEKIT_OP2_JMPADDR, 0 },
    { ZEND_JMPNZ,                           "JMPNZ",                            BYTEKIT_OP1_CVAR | BYTEKIT_OP2_JMPADDR, 0 },
    { ZEND_JMPZNZ,                          "JMPZNZ",                           BYTEKIT_OP1_CVAR | BYTEKIT_OP2_OPNUM | BYTEKIT_EXT_OPNUM, 0 },
    { ZEND_JMPZ_EX,                         "JMPZ_EX",                          BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_JMPADDR, 0 },
    { ZEND_JMPNZ_EX,                        "JMPNZ_EX",                         BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_JMPADDR, 0 },
    { ZEND_CASE,                            "CASE",                             BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_SWITCH_FREE,                     "SWITCH_FREE",                      BYTEKIT_OP1_WRITE | BYTEKIT_OP1_CVAR | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_BRK,                             "BRK",                              BYTEKIT_OP1_OPNUM | BYTEKIT_OP1_HIDE | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_CONT,                            "CONT",                             BYTEKIT_OP1_OPNUM | BYTEKIT_OP1_HIDE |BYTEKIT_OP2_CVAR, 0 },
    { ZEND_BOOL,                            "BOOL",                             BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR, 0 },
    { ZEND_INIT_STRING,                     "INIT_STRING",                      BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR, 0 },
    { ZEND_ADD_CHAR,                        "ADD_CHAR",                         BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_ADD_STRING,                      "ADD_STRING",                       BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_ADD_VAR,                         "ADD_VAR",                          BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_BEGIN_SILENCE,                   "BEGIN_SILENCE",                    BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR, 0 },
    { ZEND_END_SILENCE,                     "END_SILENCE",                      BYTEKIT_OP1_CVAR, 0 },
    { ZEND_INIT_FCALL_BY_NAME,              "INIT_FCALL_BY_NAME",               BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_DO_FCALL,                        "DO_FCALL",                         BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_DO_FCALL_BY_NAME,                "DO_FCALL_BY_NAME",                 BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_EXT_SPECIAL, 0 },    
    { ZEND_RETURN,                          "RETURN",                           BYTEKIT_OP1_CVAR | BYTEKIT_EXT_SPECIAL | BYTEKIT_OP_STOP, 0 },
    { ZEND_RECV,                            "RECV",                             BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_RES_FORCE | BYTEKIT_OP1_CVAR | BYTEKIT_EXT_FETCH, 0 },
    { ZEND_RECV_INIT,                       "RECV_INIT",                        BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_RES_FORCE | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_FETCH, 0 },
    { ZEND_SEND_VAL,                        "SEND_VAL",                         BYTEKIT_OP1_CVAR | BYTEKIT_OP2_NUM | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_SEND_VAR,                        "SEND_VAR",                         BYTEKIT_OP1_CVAR | BYTEKIT_OP2_NUM | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_SEND_REF,                        "SEND_REF",                         BYTEKIT_OP1_CVAR, 0 },
    { ZEND_NEW,                             "NEW",                              BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_TVAR | BYTEKIT_OP2_OPNUM, 0 },
    { ZEND_INIT_NS_FCALL_BY_NAME,           "INIT_NS_FCALL_BY_NAME",            BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_SPECIAL, BYTEKIT_OP1_CVAR },
    { ZEND_FREE,                            "FREE",                             BYTEKIT_OP1_WRITE | BYTEKIT_OP1_CVAR, 0 },
    { ZEND_INIT_ARRAY,                      "INIT_ARRAY",                       BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_NUM, 0 },
    { ZEND_ADD_ARRAY_ELEMENT,               "ADD_ARRAY_ELEMENT",                BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_NUM, 0 },
    { ZEND_INCLUDE_OR_EVAL,                 "INCLUDE_OR_EVAL",                  BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_UNSET_VAR,                       "UNSET_VAR",                        BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_UNSET_DIM,                       "UNSET_DIM",                        BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_UNSET_OBJ,                       "UNSET_OBJ",                        BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_FE_RESET,                        "FE_RESET",                         BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_OPNUM, 0 },
    { ZEND_FE_FETCH,                        "FE_FETCH",                         BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_OPNUM | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_EXIT,                            "EXIT",                             BYTEKIT_OP1_CVAR | BYTEKIT_OP_STOP, 0 },
    { ZEND_FETCH_R,                         "FETCH_R",                          BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_FETCH_DIM_R,                     "FETCH_DIM_R",                      BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_FETCH_OBJ_R,                     "FETCH_OBJ_R",                      BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_FETCH_W,                         "FETCH_W",                          BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_FETCH_DIM_W,                     "FETCH_DIM_W",                      BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_FETCH_OBJ_W,                     "FETCH_OBJ_W",                      BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_FETCH_RW,                        "FETCH_RW",                         BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_FETCH_DIM_RW,                    "FETCH_DIM_RW",                     BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR},
    { ZEND_FETCH_OBJ_RW,                    "FETCH_OBJ_RW",                     BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_FETCH_IS,                        "FETCH_IS",                         BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_FETCH_DIM_IS,                    "FETCH_DIM_IS",                     BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_FETCH_OBJ_IS,                    "FETCH_OBJ_IS",                     BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_FETCH_FUNC_ARG,                  "FETCH_FUNC_ARG",                   BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_FETCH_DIM_FUNC_ARG,              "FETCH_DIM_FUNC_ARG",               BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_FETCH_OBJ_FUNC_ARG,              "FETCH_OBJ_FUNC_ARG",               BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_FETCH_UNSET,                     "FETCH_UNSET",                      BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_FETCH_DIM_UNSET,                 "FETCH_DIM_UNSET",                  BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_FETCH_OBJ_UNSET,                 "FETCH_OBJ_UNSET",                  BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_FETCH_DIM_TMP_VAR,               "FETCH_DIM_TMP_VAR",                BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_FETCH_CONSTANT,                  "FETCH_CONSTANT",                   BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_FETCH, 0 },
    { ZEND_GOTO,                            "GOTO",                             BYTEKIT_OP1_JMPADDR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_OPNUM, 0 },
    { ZEND_EXT_STMT,                        "EXT_STMT",                         0, 0 },
    { ZEND_EXT_FCALL_BEGIN,                 "EXT_FCALL_BEGIN",                  0, 0 },
    { ZEND_EXT_FCALL_END,                   "EXT_FCALL_END",                    0, 0 },
    { ZEND_EXT_NOP,                         "EXT_NOP",                          0, 0 },
    { ZEND_TICKS,                           "TICKS",                            BYTEKIT_OP1_CVAR, 0 },
    { ZEND_SEND_VAR_NO_REF,                 "SEND_VAR_NO_REF",                  BYTEKIT_OP1_CVAR | BYTEKIT_OP2_NUM | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_CATCH,                           "CATCH",                            BYTEKIT_OP1_TVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_OPNUM, 0 },    
    { ZEND_THROW,                           "THROW",                            BYTEKIT_OP1_CVAR, 0 },
    { ZEND_FETCH_CLASS,                     "FETCH_CLASS",                      BYTEKIT_RES_WRITE | BYTEKIT_RES_TVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_FETCH, 0 },
    { ZEND_CLONE,                           "CLONE",                            BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR, 0 },
    { ZEND_INIT_METHOD_CALL,                "INIT_METHOD_CALL",                 BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_INIT_STATIC_METHOD_CALL,         "INIT_STATIC_METHOD_CALL",          BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_ISSET_ISEMPTY_VAR,               "ISSET_ISEMPTY_VAR",                BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_ISSET_ISEMPTY_DIM_OBJ,           "ISSET_ISEMPTY_DIM_OBJ",            BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_PRE_INC_OBJ,                     "PRE_INC_OBJ",                      BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_PRE_DEC_OBJ,                     "PRE_DEC_OBJ",                      BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_POST_INC_OBJ,                    "POST_INC_OBJ",                     BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_POST_DEC_OBJ,                    "POST_DEC_OBJ",                     BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_ASSIGN_OBJ,                      "ASSIGN_OBJ",                       BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, BYTEKIT_OP1_CVAR },
    { ZEND_OP_DATA,                         "OP_DATA",                          0, 0 },
    { ZEND_INSTANCEOF,                      "INSTANCEOF",                       BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_DECLARE_CLASS,                   "DECLARE_CLASS",                    BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_DECLARE_INHERITED_CLASS,         "DECLARE_INHERITED_CLASS",          BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_TVAR, 0 },
    { ZEND_DECLARE_FUNCTION,                "DECLARE_FUNCTION",                 BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_RAISE_ABSTRACT_ERROR,            "RAISE_ABSTRACT_ERROR",             BYTEKIT_OP_STOP, 0 },
    { ZEND_DECLARE_CONST,                   "DECLARE_CONST",                    BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { ZEND_ADD_INTERFACE,                   "ADD_INTERFACE",                    BYTEKIT_OP1_TVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_FETCH, 0 },
    { ZEND_DECLARE_INHERITED_CLASS_DELAYED, "DECLARE_INHERITED_CLASS_DELAYED",  BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_TVAR, 0 },
    { ZEND_VERIFY_ABSTRACT_CLASS,           "VERIFY_ABSTRACT_CLASS",            BYTEKIT_OP1_TVAR, 0 },    
    { ZEND_ASSIGN_DIM,                      "ASSIGN_DIM",                       BYTEKIT_RES_WRITE | BYTEKIT_OP1_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR },
    { ZEND_ISSET_ISEMPTY_PROP_OBJ,          "ISSET_ISEMPTY_PROP_OBJ",           BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR | BYTEKIT_EXT_SPECIAL, 0 },
    { ZEND_HANDLE_EXCEPTION,                "HANDLE_EXCEPTION",                 0, 0 },
    { ZEND_USER_OPCODE,                     "USER_OPCODE",                      0, 0 },
    { ZEND_JMP_SET,                         "JMP_SET",                          BYTEKIT_OP1_CVAR | BYTEKIT_OP2_JMPADDR, 0 },
    { ZEND_DECLARE_LAMBDA_FUNCTION,         "DECLARE_LAMBDA_FUNCTION",          BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR, 0 },
    { 0, NULL }
};

#endif  /* PHP_BYTEKIT_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

