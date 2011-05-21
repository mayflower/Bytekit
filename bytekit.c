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

/*************************************************************************/
/* This code combines features of vld, parsekit with additional features */
/* therefore some parts of thr code are based on these extensions        */
/*************************************************************************/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"
#include "php_bytekit.h"

/* This code does only work on PHP >= 5.2.0 - PHP 6 not yet supported */
#if (!defined(PHP_VERSION_ID) || (PHP_VERSION_ID < 50200) || (PHP_VERSION_ID >= 60000))
#  error "Bytekit can only be compiled for PHP >= 5.2.0 and not yet for PHP 6" 
#endif

ZEND_DECLARE_MODULE_GLOBALS(bytekit)

/* original error callback handler */
void (*bytekit_original_error_cb)(int type, const char *error_filename, const uint error_lineno, const char *format, va_list args);

/* {{{ bytekit_error_cb 
    ignore, catch and return non core errors */
static void bytekit_error_cb(int type, const char *error_filename, const uint error_lineno, const char *format, va_list args)
{
    char *buffer;
    int buffer_len;
    zval *tmpzval;
    TSRMLS_FETCH();

    /* Do not touch core errors and avoid loops */
    if (type == E_CORE_ERROR || BYTEKIT_G(in_error_cb)) {
        bytekit_original_error_cb(type, (char *)error_filename, error_lineno, format, args);
        return;
    }

    /* Ignore errors if selected */
    if (!BYTEKIT_G(compile_errors)) {
        return;
    }

    /* We are processing an error */
    BYTEKIT_G(in_error_cb) = 1;

    MAKE_STD_ZVAL(tmpzval);
    array_init(tmpzval);
    add_assoc_long(tmpzval, "errno", type);
    add_assoc_string(tmpzval, "filename", (char *)error_filename, 1);
    add_assoc_long(tmpzval, "lineno", error_lineno);
    buffer_len = vspprintf(&buffer, PG(log_errors_max_len), format, args);
    add_assoc_stringl(tmpzval, "errstr", buffer, buffer_len, 1);

    if (Z_TYPE_P(BYTEKIT_G(compile_errors)) == IS_NULL) {
        array_init(BYTEKIT_G(compile_errors));
    }
    add_next_index_zval(BYTEKIT_G(compile_errors), tmpzval);

    /* We are no longer processing the error */
    BYTEKIT_G(in_error_cb) = 0;
}
/* }}} */

/* {{{ bytekit_get_opcode_info */
static char* bytekit_get_opcode_info(long val, long *pflags, long *pflags2)
{
    bytekit_define_list *names;

    for (names = bytekit_opcode_names; names->str; names++) {
        if (names->val == val) {
            if (pflags) {
                *pflags = names->flags;
            }
            if (pflags2) {
                *pflags2 = names->flags2;
            }
            return names->str;
        }
    }

    return BYTEKIT_UNKNOWN;
}
/* }}} */

/* {{{ bytekit_get_opcode_mnemonic */
static char* bytekit_get_opcode_mnemonic(zend_op_array *zop, zend_op *op TSRMLS_DC)
{
    bytekit_define_list *names;
    char *postfix = "", *n, *r;
    int i;

    switch (op->opcode) {
        case ZEND_INCLUDE_OR_EVAL:
            switch (Z_LVAL(op->op2.u.constant)) {
                case ZEND_INCLUDE:
                    return estrndup("INCLUDE", sizeof("INCLUDE")-1);
                case ZEND_REQUIRE:
                    return estrndup("REQUIRE", sizeof("REQUIRE")-1);
                case ZEND_INCLUDE_ONCE:
                    return estrndup("INCLUDE_ONCE", sizeof("INCLUDE_ONCE")-1);
                case ZEND_REQUIRE_ONCE:
                    return estrndup("REQUIRE_ONCE", sizeof("REQUIRE_ONCE")-1);
                case ZEND_EVAL:
                    return estrndup("EVAL", sizeof("EVAL")-1);
                default:
                    /* UNKNOWN decode as INCLUDE_OR_EVAL */
                    break;
            }
            break;
        case ZEND_ISSET_ISEMPTY_VAR:
            {
                int ev = op->extended_value & ZEND_ISSET_ISEMPTY_MASK;
                if (ev == ZEND_ISSET) return estrndup("ISSET_VAR", sizeof("ISSET_VAR")-1);
                if (ev == ZEND_ISEMPTY) return estrndup("ISEMPTY_VAR", sizeof("ISEMPTY_VAR")-1);
            }
            break;
        case ZEND_ISSET_ISEMPTY_DIM_OBJ:
            {
                int ev = op->extended_value & ZEND_ISSET_ISEMPTY_MASK;
                if (ev == ZEND_ISSET) return estrndup("ISSET_DIM_OBJ", sizeof("ISSET_DIM_OBJ")-1);
                if (ev == ZEND_ISEMPTY) return estrndup("ISEMPTY_DIM_OBJ", sizeof("ISEMPTY_DIM_OBJ")-1);
            }
            break;
        case ZEND_ISSET_ISEMPTY_PROP_OBJ:
            {
                int ev = op->extended_value & ZEND_ISSET_ISEMPTY_MASK;
                if (ev == ZEND_ISSET) return estrndup("ISSET_PROP_OBJ", sizeof("ISSET_PROP_OBJ")-1);
                if (ev == ZEND_ISEMPTY) return estrndup("ISEMPTY_PROP_OBJ", sizeof("ISEMPTY_PROP_OBJ")-1);
            }
            break;
        case ZEND_ASSIGN_ADD:
        case ZEND_ASSIGN_SUB:
        case ZEND_ASSIGN_MUL:
        case ZEND_ASSIGN_DIV:
        case ZEND_ASSIGN_MOD:
        case ZEND_ASSIGN_SL:
        case ZEND_ASSIGN_SR:
        case ZEND_ASSIGN_CONCAT:
        case ZEND_ASSIGN_BW_OR:
        case ZEND_ASSIGN_BW_AND:
        case ZEND_ASSIGN_BW_XOR:
            if (op->extended_value == ZEND_ASSIGN_OBJ) {
                postfix = "_OBJ";
            } else if (op->extended_value == ZEND_ASSIGN_DIM) {
                postfix = "_DIM";
            }
            n = bytekit_get_opcode_info(op->opcode, NULL, NULL);
            if (n == BYTEKIT_UNKNOWN) {
                /* should not happen */
                n = "UNKNOWN";
            }
            i = strlen(n);
            r = emalloc(i + strlen(postfix) + 1);
            memcpy(r, n, i);
            memcpy(r + i, postfix, strlen(postfix));
            r[i+strlen(postfix)] = 0;
            return r;
        case ZEND_CAST:
            switch (op->extended_value) {
                case IS_NULL:
                    return estrndup("CAST_NULL", sizeof("CAST_NULL")-1);
                case IS_BOOL:
                    return estrndup("CAST_BOOL", sizeof("CAST_BOOL")-1);
                case IS_LONG:
                    return estrndup("CAST_LONG", sizeof("CAST_LONG")-1);
                case IS_DOUBLE:
                    return estrndup("CAST_DOUBLE", sizeof("CAST_DOUBLE")-1);
                case IS_STRING:
                    return estrndup("CAST_STRING", sizeof("CAST_STRING")-1);
                case IS_ARRAY:
                    return estrndup("CAST_ARRAY", sizeof("CAST_ARRAY")-1);
                case IS_OBJECT:
                    return estrndup("CAST_OBJECT", sizeof("CAST_OBJECT")-1);
                default:
                    /* UNKNOWN decode as CAST */
                    break;
            }
            break;
        default:
            break;
    }

    return estrdup(bytekit_get_opcode_info(op->opcode, NULL, NULL));
}
/* }}} */

/* {{{ bytekit_get_opcode_flags */
static int bytekit_get_opcode_flags(zend_op_array *zop, zend_op *op, long *flags, long *flags2 TSRMLS_DC)
{
    bytekit_define_list *names;

    switch (op->opcode) {
        case ZEND_INCLUDE_OR_EVAL:
            switch (Z_LVAL(op->op2.u.constant)) {
                case ZEND_INCLUDE:
                case ZEND_REQUIRE:
                case ZEND_INCLUDE_ONCE:
                case ZEND_REQUIRE_ONCE:
                case ZEND_EVAL:
                    break;
                default:
                    /* UNKNOWN decode as INCLUDE_OR_EVAL */
                    break;
            }
            break;
        case ZEND_CAST:
            switch (op->extended_value) {
                case IS_NULL:
                case IS_BOOL:
                case IS_LONG:
                case IS_DOUBLE:
                case IS_STRING:
                case IS_ARRAY:
                case IS_OBJECT:
                    break;
                default:
                    /* UNKNOWN decode as CAST */
                    break;
            }
            break;
        case ZEND_ASSIGN_ADD:
        case ZEND_ASSIGN_SUB:
        case ZEND_ASSIGN_MUL:
        case ZEND_ASSIGN_DIV:
        case ZEND_ASSIGN_MOD:
        case ZEND_ASSIGN_SL:
        case ZEND_ASSIGN_SR:
        case ZEND_ASSIGN_CONCAT:
        case ZEND_ASSIGN_BW_OR:
        case ZEND_ASSIGN_BW_AND:
        case ZEND_ASSIGN_BW_XOR:
            bytekit_get_opcode_info(op->opcode, flags, flags2);
            if (op->extended_value == ZEND_ASSIGN_OBJ || op->extended_value == ZEND_ASSIGN_DIM) {
                *flags2 = BYTEKIT_OP1_CVAR | BYTEKIT_OP2_CVAR;
            }
            return SUCCESS;
        case ZEND_FE_FETCH:
            if (op->extended_value & ZEND_FE_FETCH_WITH_KEY) {
                *flags  = BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_OPNUM | BYTEKIT_EXT_SPECIAL;
                *flags2 = BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR;
                return SUCCESS;
            }
            *flags  = BYTEKIT_RES_WRITE | BYTEKIT_RES_CVAR | BYTEKIT_OP1_CVAR | BYTEKIT_OP2_OPNUM | BYTEKIT_EXT_SPECIAL;
            *flags2 = 0;
            return SUCCESS;
        default:
            break;
    }

    *flags = *flags2 = 0;
    
    if (bytekit_get_opcode_info(op->opcode, flags, flags2) == BYTEKIT_UNKNOWN) {
        return SUCCESS;
    }
    return FAILURE;
}
/* }}} */

/* {{{ bytekit_get_opcode_oplines */
static int bytekit_get_opcode_oplines(zend_op_array *zop, zend_op *op TSRMLS_DC)
{
    int length = 1;
    
    switch (op->opcode) {
        case ZEND_ASSIGN_OBJ:
        case ZEND_ASSIGN_DIM:
            length = 2;
            break;
        case ZEND_INIT_NS_FCALL_BY_NAME:    
            length = 2;
            break;
        case ZEND_FE_FETCH:
            length = 2;
            break;
            
        case ZEND_ASSIGN_ADD:
        case ZEND_ASSIGN_SUB:
        case ZEND_ASSIGN_MUL:
        case ZEND_ASSIGN_DIV:
        case ZEND_ASSIGN_MOD:
        case ZEND_ASSIGN_SL:
        case ZEND_ASSIGN_SR:
        case ZEND_ASSIGN_CONCAT:
        case ZEND_ASSIGN_BW_OR:
        case ZEND_ASSIGN_BW_AND:
        case ZEND_ASSIGN_BW_XOR:
            if (op->extended_value == ZEND_ASSIGN_OBJ || op->extended_value == ZEND_ASSIGN_DIM) {
                length = 2;
            }        
    }
    
    if (length > 1) {
        if ((op+1)->opcode != ZEND_OP_DATA) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "bytekit_get_opcode_oplines: ZEND_OP_DATA expected but %u found.", (op+1)->opcode);
        }
    }
    
    return length;
}
/* }}} */

/* {{{ bytekit_destroy_function */
static int bytekit_destroy_function(HashTable *function_table, zend_function *function, char *function_name, int function_name_len TSRMLS_DC)
{
    /* TODO: properly destroy the function */
    if (zend_hash_del(function_table, function_name, function_name_len) == FAILURE) {
        return FAILURE;
    }   
    
    return SUCCESS;
}
/* }}} */


/* {{{ bytekit_cleanup_functions */
static int bytekit_cleanup_functions(HashTable *function_table, int clean_count TSRMLS_DC)
{
    HashPosition pos;

    zend_hash_internal_pointer_end_ex(function_table, &pos);
    while (clean_count < zend_hash_num_elements(function_table)) {
        long func_index;
        unsigned int func_name_len;
        char *func_name;
        zend_function *function;

        if (zend_hash_get_current_data_ex(function_table, (void **)&function, &pos) == FAILURE) {
            php_error_docref(NULL TSRMLS_CC, E_ERROR, "bytekit_cleanup_functions: error traversing function table.");
            return FAILURE;
        }
        
        if (function->type == ZEND_INTERNAL_FUNCTION) {
            /* Inherited internal method */
            zend_hash_move_backwards_ex(function_table, &pos);
            clean_count++;
            continue;
        } else if (function->type != ZEND_USER_FUNCTION) {
            php_error_docref(NULL TSRMLS_CC, E_ERROR, "bytekit_cleanup_functions: illegal function entry found.");
            return FAILURE;
        }
        
        if (zend_hash_get_current_key_ex(function_table, &func_name, &func_name_len, &func_index, 0, &pos) == HASH_KEY_IS_STRING) {
            zend_hash_move_backwards_ex(function_table, &pos);

            if (bytekit_destroy_function(function_table, function, func_name, func_name_len TSRMLS_CC) == FAILURE) {
                php_error_docref(NULL TSRMLS_CC, E_ERROR, "bytekit_cleanup_functions: unable to destroy function.");
                return FAILURE;
            }
        } else {
            php_error_docref(NULL TSRMLS_CC, E_ERROR, "bytekit_cleanup_functions: illegal function table key found.");
            return FAILURE;
        }
    }
    return SUCCESS;
}
/* }}} */


/* {{{ bytekit_destroy_class */
static int bytekit_destroy_class(HashTable *class_table, zend_class_entry *class_entry, char *class_name, int class_name_len TSRMLS_DC)
{
    int rval = SUCCESS;
    
    /* TODO: properly destroy the class */
    if (zend_hash_num_elements(&(class_entry->function_table)) > 0) {
        rval = bytekit_cleanup_functions(&(class_entry->function_table), 0 TSRMLS_CC);
    }
    
    if (zend_hash_del(class_table, class_name, class_name_len) == FAILURE) {
        rval = FAILURE;
    }
    
    return rval;
}
/* }}} */


/* {{{ bytekit_cleanup_classes */
static int bytekit_cleanup_classes(HashTable *class_table, int clean_count TSRMLS_DC)
{
    while (clean_count < zend_hash_num_elements(class_table)) {
        long class_index;
        unsigned int class_name_len;
        char *class_name;
        zend_class_entry *class_entry, **pce;

        zend_hash_internal_pointer_end(class_table);
        if (zend_hash_get_current_data(class_table, (void **)&pce) == FAILURE || !pce || !(*pce)) {
            php_error_docref(NULL TSRMLS_CC, E_ERROR, "bytekit_cleanup_classes: unable to traverse class table.");
            return FAILURE;
        }
        class_entry = *pce;

        if (class_entry->type != ZEND_USER_CLASS) {
            php_error_docref(NULL TSRMLS_CC, E_ERROR, "bytekit_cleanup_classes: illegal class found in class table.");
            return FAILURE;
        }

        if (zend_hash_get_current_key_ex(class_table, &class_name, &class_name_len, &class_index, 0, NULL) == HASH_KEY_IS_STRING) {
            
            if (bytekit_destroy_class(class_table, class_entry, class_name, class_name_len TSRMLS_CC) == FAILURE) {
                php_error_docref(NULL TSRMLS_CC, E_ERROR, "bytekit_cleanup_classes: unable to destroy class.");
                return FAILURE;
            }
        } else {
            php_error_docref(NULL TSRMLS_CC, E_ERROR, "bytekit_cleanup_classes: illegal class table key found.");
        }
    }
    return SUCCESS;
}
/* }}} */

/* {{{ bytekit_cleanup_functions_and_classes */
static void bytekit_cleanup_functions_and_classes(int original_num_functions, int original_num_classes TSRMLS_DC)
{
    
    if (original_num_functions < zend_hash_num_elements(EG(function_table))) {
        bytekit_cleanup_functions(EG(function_table), original_num_functions TSRMLS_CC);
    }
    
    if (original_num_classes < zend_hash_num_elements(EG(class_table))) {
        bytekit_cleanup_classes(EG(class_table), original_num_classes TSRMLS_CC);
    }
    
}
/* }}} */

inline int bytekit_disassemble_zval_null(zval *return_value, zvalue_value value TSRMLS_DC)
{
    char *tmpstr;
    add_assoc_string(return_value, "string", "null", 1);
    return SUCCESS;
}

inline int bytekit_disassemble_zval_long(zval *return_value, zvalue_value value TSRMLS_DC)
{
    char *tmpstr;
    spprintf(&tmpstr, 0, "%ld", value.lval);
    add_assoc_string(return_value, "string", tmpstr, 0);
    return SUCCESS;
}

inline int bytekit_disassemble_zval_double(zval *return_value, zvalue_value value TSRMLS_DC)
{
    char *tmpstr;
    spprintf(&tmpstr, 0, "%g", value.dval);
    add_assoc_string(return_value, "string", tmpstr, 0);
    return SUCCESS;
}

inline int bytekit_disassemble_zval_string(zval *return_value, zvalue_value value TSRMLS_DC)
{
    char *tmpstr;
    char *newstr;
    if (value.str.len == 0) {
        newstr = estrdup("");
    } else {
        newstr = php_addcslashes(value.str.val, value.str.len, NULL, 0, "\n\r\t\a\b\f\v\0", 9 TSRMLS_CC);
    }
    spprintf(&tmpstr, 0, "'%s'", newstr);
    efree(newstr);
    add_assoc_string(return_value, "string", tmpstr, 0);
    return SUCCESS;
}

inline int bytekit_disassemble_zval_array(zval *return_value, zvalue_value value TSRMLS_DC)
{
    char *tmpstr;
    spprintf(&tmpstr, 0, "<array>");
    add_assoc_string(return_value, "string", tmpstr, 0);
    return SUCCESS;
}

inline int bytekit_disassemble_zval_object(zval *return_value, zvalue_value value TSRMLS_DC)
{
    char *tmpstr;
    spprintf(&tmpstr, 0, "<object>");
    add_assoc_string(return_value, "string", tmpstr, 0);
    return SUCCESS;
}

inline int bytekit_disassemble_zval_bool(zval *return_value, zvalue_value value TSRMLS_DC)
{
    char *tmpstr;
    spprintf(&tmpstr, 0, value.lval ? "true" : "false");
    add_assoc_string(return_value, "string", tmpstr, 0);
    return SUCCESS;
}

inline int bytekit_disassemble_zval_resource(zval *return_value, zvalue_value value TSRMLS_DC)
{
    char *tmpstr;
    spprintf(&tmpstr, 0, "<resource>");
    add_assoc_string(return_value, "string", tmpstr, 0);
    return SUCCESS;
}

inline int bytekit_disassemble_zval_constant(zval *return_value, zvalue_value value TSRMLS_DC)
{
    char *tmpstr;
    spprintf(&tmpstr, 0, "<const>");
    add_assoc_string(return_value, "string", tmpstr, 0);
    return SUCCESS;
}

inline int bytekit_disassemble_zval_constant_array(zval *return_value, zvalue_value value TSRMLS_DC)
{
    char *tmpstr;
    spprintf(&tmpstr, 0, "<const_array>");
    add_assoc_string(return_value, "string", tmpstr, 0);
    return SUCCESS;
}


int bytekit_disassemble_zval (zval *return_value, zval *val TSRMLS_DC)
{
    char *tmpstr;
    zval *cval;
    
    add_assoc_long(return_value, "type", BYTEKIT_TYPE_VALUE);

    MAKE_STD_ZVAL(cval);
    *cval = *val;
    zval_copy_ctor(cval);
#if PHP_VERSION_ID >= 50300
    Z_SET_REFCOUNT_P(cval, 1);
    Z_UNSET_ISREF_P(cval);
#else
    cval->refcount = 1;
    cval->is_ref = 0;
#endif
    add_assoc_zval(return_value, "value", cval);

    switch (val->type) {
        case IS_NULL:           return bytekit_disassemble_zval_null (return_value, val->value TSRMLS_CC);
        case IS_LONG:           return bytekit_disassemble_zval_long (return_value, val->value TSRMLS_CC);
        case IS_DOUBLE:         return bytekit_disassemble_zval_double (return_value, val->value TSRMLS_CC);
        case IS_STRING:         return bytekit_disassemble_zval_string (return_value, val->value TSRMLS_CC);
        case IS_ARRAY:          return bytekit_disassemble_zval_array (return_value, val->value TSRMLS_CC);
        case IS_OBJECT:         return bytekit_disassemble_zval_object (return_value, val->value TSRMLS_CC);
        case IS_BOOL:           return bytekit_disassemble_zval_bool (return_value, val->value TSRMLS_CC);
        case IS_RESOURCE:       return bytekit_disassemble_zval_resource (return_value, val->value TSRMLS_CC);
        case IS_CONSTANT:       return bytekit_disassemble_zval_constant (return_value, val->value TSRMLS_CC);
        case IS_CONSTANT_ARRAY: return bytekit_disassemble_zval_constant_array (return_value, val->value TSRMLS_CC);
    }
    spprintf(&tmpstr, 0, "<unknown>");
    add_assoc_string(return_value, "string", tmpstr, 0);
    return SUCCESS;
}

/* {{{ bytekit_address2string */
static char *bytekit_address2string(long long address, int cnt TSRMLS_DC)
{
    char *res = (char *)emalloc(64);
    
    if (cnt == 0) {
        sprintf(res, "%lld", address);
    } else {
        sprintf(res, "%0*lld", cnt, address);
    }
    return res;
}
/* }}} */

/* {{{ bytekit_address2hexstring */
static char *bytekit_address2hexstring(long long address, int cnt TSRMLS_DC)
{
    char *res = (char *)emalloc(64);
    
    if (cnt == 0) {
        sprintf(res, "%llx", address);
    } else {
        sprintf(res, "%0*llx", cnt, address);
    }
    return res;
}
/* }}} */

/* {{{ bytekit_disassemble_node */
static int bytekit_disassemble_node(zval *return_value, zend_op_array *op_array, znode *node, int i, long long address, long opcode_flags, long options TSRMLS_DC)
{
    char *tmpstr;
    char *base_address = BYTEKIT_G(opcodes_base) ? BYTEKIT_G(opcodes_base) : op_array->opcodes;
    long long dst;
    char *dststr;
    
    array_init(return_value);
    
    switch (opcode_flags) {
        case BYTEKIT_OP_OPNUM:
            dst = address - i + node->u.opline_num;
            dststr = bytekit_address2hexstring(dst, 0 TSRMLS_CC);
            spprintf(&tmpstr, 0, "loc_%s", dststr);
            efree(dststr);
            add_assoc_string(return_value, "string", tmpstr, 0);
            add_assoc_long(return_value, "type", BYTEKIT_TYPE_SYMBOL);
            add_assoc_long(return_value, "value", dst);
            return SUCCESS;
        case BYTEKIT_OP_NUM:
            spprintf(&tmpstr, 0, "%d", node->u.opline_num);
            add_assoc_string(return_value, "string", tmpstr, 0);
            add_assoc_long(return_value, "type", BYTEKIT_TYPE_VALUE);
            add_assoc_long(return_value, "value", node->u.opline_num );
            return SUCCESS;
        case BYTEKIT_OP_JMPADDR:
            dst = address - i + ((char *)node->u.jmp_addr - base_address) / sizeof(zend_op);
            dststr = bytekit_address2hexstring(dst, 0 TSRMLS_CC);
            spprintf(&tmpstr, 0, "loc_%s", dststr);
            efree(dststr);
            add_assoc_string(return_value, "string", tmpstr, 0);
            add_assoc_long(return_value, "type", BYTEKIT_TYPE_SYMBOL);
            add_assoc_long(return_value, "value", dst);
            return SUCCESS;
        case BYTEKIT_OP_TVAR:
            spprintf(&tmpstr, 0, "~%d", node->u.var / sizeof(temp_variable));
            add_assoc_string(return_value, "string", tmpstr, 0);
            add_assoc_long(return_value, "type", BYTEKIT_TYPE_TMP_VAR);
            add_assoc_long(return_value, "value", node->u.var / sizeof(temp_variable));
            return SUCCESS;
    }
    
    switch (node->op_type) {
        case IS_CONST: /* 1 */
            bytekit_disassemble_zval (return_value, &node->u.constant TSRMLS_CC);
            return SUCCESS;
        case IS_CV:  /* 16 */
            spprintf(&tmpstr, 0, "!%d", node->u.var);
            add_assoc_string(return_value, "string", tmpstr, 0);
            add_assoc_long(return_value, "type", BYTEKIT_TYPE_CV);
            add_assoc_long(return_value, "value", node->u.var );
            return SUCCESS;
        case IS_TMP_VAR: /* 2 */
            spprintf(&tmpstr, 0, "~%d", node->u.var / sizeof(temp_variable));
            add_assoc_string(return_value, "string", tmpstr, 0);
            add_assoc_long(return_value, "type", BYTEKIT_TYPE_TMP_VAR);
            add_assoc_long(return_value, "value", node->u.var / sizeof(temp_variable));
            return SUCCESS;
        case IS_VAR: /* 4 */
            spprintf(&tmpstr, 0, "$%d", node->u.var / sizeof(temp_variable));
            add_assoc_string(return_value, "string", tmpstr, 0);
            add_assoc_long(return_value, "type", BYTEKIT_TYPE_VAR);
            add_assoc_long(return_value, "value", node->u.var / sizeof(temp_variable));
            return SUCCESS;
        case IS_UNUSED:
            return FAILURE;
        default:
            return SUCCESS;
    }
}
/* }}} */


/* {{{ bytekit_disassemble_ext */
static int bytekit_disassemble_ext(zval *return_value, zend_op_array *op_array, ulong ext, int i, long long address, long opcode_flags, long options TSRMLS_DC)
{
    char *tmpstr;
    char *base_address = BYTEKIT_G(opcodes_base) ? BYTEKIT_G(opcodes_base) : op_array->opcodes;
    long long dst;
    char *dststr;
    
    array_init(return_value);
    
    switch (opcode_flags) {
        case BYTEKIT_EXT_SPECIAL:
            if ((options & BYTEKIT_SHOW_SPECIALS) == 0) {
                return FAILURE;
            }
            spprintf(&tmpstr, 0, "s(%d)", ext);
            add_assoc_string(return_value, "string", tmpstr, 0);
            add_assoc_long(return_value, "type", BYTEKIT_TYPE_VALUE);
            add_assoc_long(return_value, "value", ext);
            return SUCCESS;
        case BYTEKIT_EXT_NUM:
            spprintf(&tmpstr, 0, "%d", ext);
            add_assoc_string(return_value, "string", tmpstr, 0);
            add_assoc_long(return_value, "type", BYTEKIT_TYPE_VALUE);
            add_assoc_long(return_value, "value", ext);
            return SUCCESS;
        case BYTEKIT_EXT_FETCH:
            spprintf(&tmpstr, 0, "f(%d)", ext);
            add_assoc_string(return_value, "string", tmpstr, 0);
            add_assoc_long(return_value, "type", BYTEKIT_TYPE_VALUE);
            add_assoc_long(return_value, "value", ext);
            return SUCCESS;
        case BYTEKIT_EXT_OPNUM:
            dst = address - i + ext;
            dststr = bytekit_address2hexstring(dst, 0 TSRMLS_CC);
            spprintf(&tmpstr, 0, "loc_%s", dststr);
            efree(dststr);
            add_assoc_string(return_value, "string", tmpstr, 0);
            add_assoc_long(return_value, "type", BYTEKIT_TYPE_SYMBOL);
            add_assoc_long(return_value, "value", dst);
            return SUCCESS;
        case BYTEKIT_EXT_TVAR:
            spprintf(&tmpstr, 0, "~%d", ext / sizeof(temp_variable));
            add_assoc_string(return_value, "string", tmpstr, 0);
            add_assoc_long(return_value, "type", BYTEKIT_TYPE_TMP_VAR);
            add_assoc_long(return_value, "value", ext / sizeof(temp_variable));
            return SUCCESS;
    }
    
    return FAILURE;
}
/* }}} */


/* {{{ bytekit_disassemble_op */
static void bytekit_disassemble_op(zval *return_value, zend_op_array *op_array, zend_op *op, int i, long long address, long options, int *oplines TSRMLS_DC)
{
    zval *result, *op1, *op2, *operands, *ext;
    long opcode_flags = 0, opcode_flags2 = 0;
    int opcode;
    char *mnemonic;

    array_init(return_value);

    opcode = op->opcode;

    add_assoc_string(return_value, "address", bytekit_address2string(address, 0 TSRMLS_CC), 0);
    add_assoc_long(return_value, "opline", i);
    
    /* determine mnemonic (including our aliases) and further opcode specific infos */
    mnemonic = bytekit_get_opcode_mnemonic(op_array, op TSRMLS_CC);
    add_assoc_string(return_value, "mnemonic", mnemonic, 0);
    bytekit_get_opcode_flags(op_array, op, &opcode_flags, &opcode_flags2 TSRMLS_CC);
    if (oplines) {
        *oplines = bytekit_get_opcode_oplines(op_array, op TSRMLS_CC);
    }
    
    /* initialize operands */
    MAKE_STD_ZVAL(operands);
    array_init(operands);

    /* args: result, op1, op2 */
    if ((opcode_flags & BYTEKIT_RES_USED) && ((opcode_flags & BYTEKIT_RES_HIDE)==0) && (!(op->result.u.EA.type & EXT_TYPE_UNUSED) || (opcode_flags & BYTEKIT_RES_FORCE))) {
        MAKE_STD_ZVAL(result);
        if (bytekit_disassemble_node(result, op_array, &(op->result), i, address, (opcode_flags & BYTEKIT_RES_TYPE_MASK) >> BYTEKIT_RES_SHIFT, options TSRMLS_CC) == SUCCESS) {
            add_assoc_long(result, "flags", BYTEKIT_SRC_RES1 | ((opcode_flags & BYTEKIT_RES_WRITE) ? BYTEKIT_OPERAND_WRITTEN : 0));
            add_next_index_zval(operands, result);
        } else {
            zval_dtor(result);
            FREE_ZVAL(result);
        }
    }

    if ((opcode_flags2 & BYTEKIT_RES_USED) && ((opcode_flags2 & BYTEKIT_RES_HIDE)==0) && !((op+1)->result.u.EA.type & EXT_TYPE_UNUSED)) {
        MAKE_STD_ZVAL(result);
        if (bytekit_disassemble_node(result, op_array, &((op+1)->result), i, address, (opcode_flags2 & BYTEKIT_RES_TYPE_MASK) >> BYTEKIT_RES_SHIFT, options TSRMLS_CC) == SUCCESS) {
            add_assoc_long(result, "flags", BYTEKIT_SRC_RES2 | ((opcode_flags2 & BYTEKIT_RES_WRITE) ? BYTEKIT_OPERAND_WRITTEN : 0));
            add_next_index_zval(operands, result);
        } else {
            zval_dtor(result);
            FREE_ZVAL(result);
        }
    }

    if ((opcode_flags & BYTEKIT_OP1_USED) && ((opcode_flags & BYTEKIT_OP1_HIDE)==0)) {
        MAKE_STD_ZVAL(op1);
        if (bytekit_disassemble_node(op1, op_array, &(op->op1), i, address, (opcode_flags & BYTEKIT_OP1_TYPE_MASK) >> BYTEKIT_OP1_SHIFT, options TSRMLS_CC) == SUCCESS) {
            add_assoc_long(op1, "flags", BYTEKIT_SRC_OP1 | ((opcode_flags & BYTEKIT_OP1_WRITE) ? BYTEKIT_OPERAND_WRITTEN : 0));
            add_next_index_zval(operands, op1);
        } else {
            zval_dtor(op1);
            FREE_ZVAL(op1);
        }
    }

    if ((opcode_flags & BYTEKIT_OP2_USED) && ((opcode_flags & BYTEKIT_OP2_HIDE)==0)) {
        MAKE_STD_ZVAL(op2);
        if (bytekit_disassemble_node(op2, op_array, &(op->op2), i, address, (opcode_flags & BYTEKIT_OP2_TYPE_MASK) >> BYTEKIT_OP2_SHIFT, options TSRMLS_CC) == SUCCESS) {
            add_assoc_long(op2, "flags", BYTEKIT_SRC_OP2 | ((opcode_flags & BYTEKIT_OP2_WRITE) ? BYTEKIT_OPERAND_WRITTEN : 0));
            add_next_index_zval(operands, op2);
        } else {
            zval_dtor(op2);
            FREE_ZVAL(op2);
        }
    }
    
    if ((opcode_flags & BYTEKIT_EXT_USED) && ((opcode_flags & BYTEKIT_EXT_HIDE)==0)) {
        MAKE_STD_ZVAL(ext);
        if (bytekit_disassemble_ext(ext, op_array, op->extended_value, i, address, opcode_flags & BYTEKIT_EXT_TYPE_MASK, options TSRMLS_CC) == SUCCESS) {
            add_assoc_long(ext, "flags", BYTEKIT_SRC_EXT1);
            add_next_index_zval(operands, ext);
        } else {
            zval_dtor(ext);
            FREE_ZVAL(ext);
        }
    }

    if ((opcode_flags2 & BYTEKIT_OP1_USED) && ((opcode_flags2 & BYTEKIT_OP1_HIDE)==0)) {
        MAKE_STD_ZVAL(op1);
        if (bytekit_disassemble_node(op1, op_array, &((op+1)->op1), i, address, (opcode_flags2 & BYTEKIT_OP1_TYPE_MASK) >> BYTEKIT_OP1_SHIFT, options TSRMLS_CC) == SUCCESS) {
            add_assoc_long(op1, "flags", BYTEKIT_SRC_OP3 | ((opcode_flags2 & BYTEKIT_OP1_WRITE) ? BYTEKIT_OPERAND_WRITTEN : 0));
            add_next_index_zval(operands, op1);
        } else {
            zval_dtor(op1);
            FREE_ZVAL(op1);
        }
    }

    if ((opcode_flags2 & BYTEKIT_OP2_USED) && ((opcode_flags2 & BYTEKIT_OP2_HIDE)==0)) {
        MAKE_STD_ZVAL(op2);
        if (bytekit_disassemble_node(op2, op_array, &((op+1)->op2), i, address, (opcode_flags2 & BYTEKIT_OP2_TYPE_MASK) >> BYTEKIT_OP2_SHIFT, options TSRMLS_CC) == SUCCESS) {
            add_assoc_long(op2, "flags", BYTEKIT_SRC_OP4 | ((opcode_flags & BYTEKIT_OP2_WRITE) ? BYTEKIT_OPERAND_WRITTEN : 0));
            add_next_index_zval(operands, op2);
        } else {
            zval_dtor(op2);
            FREE_ZVAL(op2);
        }
    }
    
    if ((opcode_flags2 & BYTEKIT_EXT_USED) && ((opcode_flags2 & BYTEKIT_EXT_HIDE)==0)) {
        MAKE_STD_ZVAL(ext);
        if (bytekit_disassemble_ext(ext, op_array, (op+1)->extended_value, i, address, opcode_flags2 & BYTEKIT_EXT_TYPE_MASK, options TSRMLS_CC) == SUCCESS) {
            add_assoc_long(ext, "flags", BYTEKIT_SRC_EXT2);
            add_next_index_zval(operands, ext);
        } else {
            zval_dtor(ext);
            FREE_ZVAL(ext);
        }
    }

    add_assoc_zval(return_value, "operands", operands);
}
/* }}} */




static void bytekit_get_next_oplines(zend_op_array *ops, zend_op *op, int i, int len, int *next_t, int *next_f, int *exception TSRMLS_DC)
{
    int tc;
    char *opcodes_base = BYTEKIT_G(opcodes_base) ? BYTEKIT_G(opcodes_base) : ops->opcodes;

    /* first assume we end */
    *next_t    = -1;
    *next_f    = -1;
    *exception = -1;

    /* next line(s) depend(s) on opcodes */
    switch (op->opcode) {
        default:
            *next_t = i + len;
            break;
        case ZEND_GOTO:
        case ZEND_JMP:
            *next_t = (op->op1.u.jmp_addr - (zend_op *)opcodes_base);
            break;

        /* case ZEND_FE_RESET: <- handling FE_RESET correctly makes CFG unnecessary complex */
        case ZEND_FE_FETCH:
            *next_t = i + len;
            *next_f = op->op2.u.opline_num;
            break;

        case ZEND_JMPZ:
        case ZEND_JMPNZ:
        case ZEND_JMPZ_EX:
        case ZEND_JMPNZ_EX:
            *next_f = i + len;
            *next_t = (op->op2.u.jmp_addr - (zend_op *)opcodes_base);
            break;

        case ZEND_JMPZNZ:
            *next_t = op->extended_value;
            *next_f = op->op2.u.opline_num;
            break;

        case ZEND_BRK:
        case ZEND_CONT:
            {
                zend_brk_cont_element *jmp_to;
                int array_offset = op->op1.u.opline_num;
                int nest_level = 1;
                do {
                    if (array_offset < 0 || array_offset >= ops->last_brk_cont) {
                        break;
                    }
                    jmp_to = &ops->brk_cont_array[array_offset];
                    array_offset = jmp_to->parent;
                } while (--nest_level > 0);
                if (nest_level == 1) {
                    break;
                }
                if (op->opcode == ZEND_BRK) {
                    *next_t = jmp_to->brk;
                } else {
                    *next_t = jmp_to->cont;
                }
            }
            break;

        case ZEND_EXIT:
        case ZEND_RETURN:
            break;

        case ZEND_HANDLE_EXCEPTION:
            break;

        case ZEND_THROW:
            /* search the try/catch array for the correct catch block */
            for (tc=0; tc<ops->last_try_catch; tc++) {
                if (ops->try_catch_array[tc].try_op > i) {
                    /* ignore all further blocks */
                    break;
                }
                if (i >= ops->try_catch_array[tc].try_op && i < ops->try_catch_array[tc].catch_op) {
                    *exception = ops->try_catch_array[tc].catch_op;
                } else {
                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "bytekit_get_next_oplines: found throw outside of try/catch");
                }
            }
            break;
    }
    
    /* special handling of last opcode before catch branch */
    for (tc=0; tc<ops->last_try_catch; tc++) {
        if (ops->try_catch_array[tc].try_op > i) {
            /* ignore all further blocks */
            break;
        }
        if (ops->try_catch_array[tc].catch_op == i+1) {
            *exception = ops->try_catch_array[tc].catch_op;
        }
    }
}

/* {{{ bytekit_build_basic_blocks */
static void bytekit_build_basic_blocks(zval *return_value, zend_op_array *ops, long flags TSRMLS_DC)
{
    zend_op *op;
    int i = 0;
    int *entries, *exits, *bb, *next_f, *next_t, *exception;
    zval *a_entries, *a_exits, *a_bb, *a_next_f, *a_next_t, *a_exception, *a_cfg, **a_cfg_elements;
    unsigned int numopcodes;
    int oplines, bbid;

    /* Allocate slots for all opcodes */
    numopcodes = ops->last;
    entries = (int *) ecalloc(numopcodes, sizeof(int));
    exits = (int *) ecalloc(numopcodes, sizeof(int));
    bb = (int *) ecalloc(numopcodes, sizeof(int));
    next_t = (int *) ecalloc(numopcodes, sizeof(int));
    next_f = (int *) ecalloc(numopcodes, sizeof(int));
    exception = (int *) ecalloc(numopcodes, sizeof(int));

    /* determine the number of entries and exits */
    /* opcodes with zero or more than 1 entries  */
    /* start a new basic block, opcodes with zero */
    /* or more than 1 exits end a basic block */
    for (op = ops->opcodes, i = 0; op && i < ops->last; op++, i++) {
        oplines = bytekit_get_opcode_oplines(ops, op TSRMLS_CC);

        bytekit_get_next_oplines(ops, op, i, oplines, &next_t[i], &next_f[i], &exception[i] TSRMLS_CC);

        if (next_t[i] != -1) {
            exits[i]++;
            entries[next_t[i]]++;
        }

        if (next_f[i] != -1) {
            exits[i]++;
            entries[next_f[i]]++;
        }

        if (exception[i] != -1) {
            exits[i]++;
            entries[exception[i]]++;
        }

        while (oplines > 1) {
            i++;
            entries[i] = -1;
            exits[i] = -1;
            op++;
            oplines--;
        }
    }
    
    /* fill in the basic block number */
    for (op = ops->opcodes, i = 0, bbid = 1; op && i < ops->last; op++, i++) {
        if (i > 0 && exits[i-1] == 0) {
            bbid++;
        } else if (i > 0 && exits[i-1] > 1) {
            bbid++;
        } else if (i > 0 && entries[i] == 1) {
            int z = i-1; 
            while (z > 0 && entries[z] == -1) z--;
            if (next_t[z] != i && next_f[z] != i) {
                bbid++;
            }
        } else if (i > 0 && entries[i] == 0) {
            bbid++;
        } else if (i > 0 && entries[i] > 1) {
            bbid++;
        }
        bb[i] = bbid;
    }
    
    /* create a simple CFG as nested array */
    MAKE_STD_ZVAL(a_cfg);
    array_init(a_cfg);
    a_cfg_elements = (zval **) ecalloc(bbid, sizeof(zval *));
    for (i=0; i<bbid; i++) {
        MAKE_STD_ZVAL(a_cfg_elements[i]);
        array_init(a_cfg_elements[i]);
    }
    for (op = ops->opcodes, i=0; op && i < ops->last; op++, i++) {
        int type = (exits[i]-(exception[i]!=-1?1:0)) == 2 ? BYTEKIT_EDGE_TRUE : BYTEKIT_EDGE_NORMAL;
        if (entries[i] == -1 || exits[i] < 1) {
            continue;
        }
        
        if (next_t[i] != -1) {
            /* jump to another basic block? */
            if ((bb[i] != bb[next_t[i]]) || (bb[i]==bb[next_t[i]] && (i > next_t[i]))) {
                add_index_long(a_cfg_elements[bb[i]-1],bb[next_t[i]], type);
            }
        }
        if (next_f[i] != -1) {
            /* jump to another basic block? */
            if ((bb[i] != bb[next_f[i]]) || (bb[i]==bb[next_f[i]] && (i > next_f[i]))) {
                add_index_long(a_cfg_elements[bb[i]-1],bb[next_f[i]], BYTEKIT_EDGE_FALSE);
            }
        }
        if (exception[i] != -1) {
            /* jump to another basic block? */
            if ((bb[i] != bb[exception[i]]) || (bb[i]==bb[exception[i]] && (i > exception[i]))) {
                add_index_long(a_cfg_elements[bb[i]-1],bb[exception[i]], BYTEKIT_EDGE_EXCEPTION);
            }
        }
    }
    for (i=0; i<bbid; i++) {
        add_index_zval(a_cfg, i+1, a_cfg_elements[i]);
    }
    efree(a_cfg_elements);
    /* MEMORY LEAK PROBLEMS ?!? */
    add_assoc_zval(return_value, "cfg", a_cfg);
    
    /* now return all the information as PHP arrays */
    MAKE_STD_ZVAL(a_entries);
    MAKE_STD_ZVAL(a_exits);
    MAKE_STD_ZVAL(a_bb);
    MAKE_STD_ZVAL(a_next_f);
    MAKE_STD_ZVAL(a_next_t);
    MAKE_STD_ZVAL(a_exception);
    array_init(a_entries);
    array_init(a_exits);
    array_init(a_bb);
    array_init(a_next_f);
    array_init(a_next_t);
    array_init(a_exception);
    for (op = ops->opcodes, i = 0, bbid = 0; op && i < ops->last; op++, i++) {
        if (entries[i] != -1) {
            add_next_index_long(a_entries, entries[i]);
            add_next_index_long(a_exits, exits[i]);
            add_next_index_long(a_bb, bb[i]);
            add_next_index_long(a_next_t, next_t[i]);
            add_next_index_long(a_next_f, next_f[i]);
            add_next_index_long(a_exception, exception[i]);
        }
    }
    add_assoc_zval(return_value, "entries", a_entries);
    add_assoc_zval(return_value, "exits", a_exits);
    add_assoc_zval(return_value, "bb", a_bb);
    add_assoc_zval(return_value, "next_t", a_next_t);
    add_assoc_zval(return_value, "next_f", a_next_f);
    add_assoc_zval(return_value, "exception", a_exception);
    
    /* free variables */
    efree(entries);
    efree(exits);
    efree(bb);
    efree(next_t);
    efree(next_f);
    efree(exception);
}
/* }}} */

long long base_address = 1024;

/* {{{ bytekit_raw_arginfo */
static void bytekit_raw_arginfo(zval *return_value, zend_uint num_args, zend_arg_info *arginfo, long flags TSRMLS_DC)
{
    zend_uint i;

    array_init(return_value);

    for (i = 0; i < num_args; i++) {
        zval *tmpzval;

        MAKE_STD_ZVAL(tmpzval);
        array_init(tmpzval);
        add_assoc_stringl(tmpzval, "name", arginfo[i].name, arginfo[i].name_len, 1);
        if (arginfo[i].class_name_len) {
            add_assoc_stringl(tmpzval, "class_name", arginfo[i].class_name, arginfo[i].class_name_len, 1);
        } else {
            add_assoc_null(tmpzval, "class_name");
        }
        add_assoc_bool(tmpzval, "allow_null", arginfo[i].allow_null);
        add_assoc_bool(tmpzval, "pass_by_reference", arginfo[i].pass_by_reference);
        add_assoc_bool(tmpzval, "array_type_hint", arginfo[i].array_type_hint);

        add_next_index_zval(return_value, tmpzval);
    }   
}
/* }}} */

/* {{{ bytekit_raw_node */
static void bytekit_raw_node(zval *return_value, zend_op_array *op_array, znode *node, long opcode_flags, long options TSRMLS_DC)
{
    char *opcodes_base = BYTEKIT_G(opcodes_base) ? BYTEKIT_G(opcodes_base) : op_array->opcodes;
    
    array_init(return_value);
    add_assoc_long(return_value, "type", node->op_type);

    add_assoc_long(return_value, "EA.var", node->u.EA.var);
    add_assoc_long(return_value, "EA.type", node->u.EA.type);

    if (node->op_type == IS_CONST) {
        zval *tmpzval;
        MAKE_STD_ZVAL(tmpzval);
        *tmpzval = node->u.constant;
        zval_copy_ctor(tmpzval);
#if PHP_VERSION_ID >= 50300
        tmpzval->refcount__gc = 1;
#else
        tmpzval->refcount = 1;
#endif
        if (tmpzval->type == IS_CONSTANT_ARRAY) tmpzval->type = IS_ARRAY;
        if (tmpzval->type == IS_CONSTANT) tmpzval->type = IS_STRING;         
        add_assoc_zval(return_value, "constant", tmpzval);
    } else if (node->op_type == IS_CV) {
        add_assoc_long(return_value, "var", node->u.var);
    } else {

        add_assoc_long(return_value, "var", node->u.var / sizeof(temp_variable));
        
        switch (opcode_flags) {
            case BYTEKIT_OP_OPNUM:
                add_assoc_long(return_value, "opline", node->u.opline_num);
                break;
            case BYTEKIT_OP_NUM:
                add_assoc_long(return_value, "number", node->u.opline_num);
                break;
            case BYTEKIT_OP_JMPADDR:
                add_assoc_long(return_value, "opline", ((char *)node->u.jmp_addr - opcodes_base) / sizeof(zend_op));
                break;
            case BYTEKIT_OP_TVAR:
                break;
        }
        add_assoc_long(return_value, "constant", Z_LVAL(node->u.constant));
    }
}
/* }}} */


/* {{{ bytekit_raw_op */
static void bytekit_raw_op(zval *return_value, zend_op_array *op_array, zend_op *op, long options TSRMLS_DC)
{
    zval *result, *op1, *op2;
    long opcode_flags = 0, opcode_flags2 = 0;
    int i, opcode;

    array_init(return_value);

    opcode = op->opcode;
    add_assoc_long(return_value, "opcode", opcode);

    bytekit_get_opcode_flags(op_array, op, &opcode_flags, &opcode_flags2 TSRMLS_CC);
    add_assoc_long(return_value, "flags", opcode_flags);

    if (opcode_flags & BYTEKIT_RES_USED) {
        MAKE_STD_ZVAL(result);
        bytekit_raw_node(result, op_array, &(op->result), (opcode_flags & BYTEKIT_RES_USED) >> BYTEKIT_RES_SHIFT, options TSRMLS_CC);
        add_assoc_zval(return_value, "result", result);
    } else {
        add_assoc_null(return_value, "result");
    }

    if (opcode_flags & BYTEKIT_OP1_USED) {
        MAKE_STD_ZVAL(op1);
        bytekit_raw_node(op1, op_array, &(op->op1), (opcode_flags & BYTEKIT_OP1_USED) >> BYTEKIT_OP1_SHIFT, options TSRMLS_CC);
        add_assoc_zval(return_value, "op1", op1);
    } else {
        add_assoc_null(return_value, "op1");
    }

    if (opcode_flags & BYTEKIT_OP2_USED) {
        MAKE_STD_ZVAL(op2);
        bytekit_raw_node(op2, op_array, &(op->op2), (opcode_flags & BYTEKIT_OP2_USED) >> BYTEKIT_OP2_SHIFT, options TSRMLS_CC);
        add_assoc_zval(return_value, "op2", op2);
    } else {
        add_assoc_null(return_value, "op2");
    }

    add_assoc_long(return_value, "extended_value", op->extended_value);
    add_assoc_long(return_value, "lineno", op->lineno);
}
/* }}} */

/* {{{ bytekit_disassemble_op_array */
static void bytekit_disassemble_op_array(zval *return_value, zend_op_array *ops, long flags TSRMLS_DC)
{
    zend_op *op;
    zval *tmpzval, *raw;
    int i = 0;
    long long address = 0;

    array_init(return_value);
    
    MAKE_STD_ZVAL(raw);
    array_init(raw);

    /* determine address from zend_op_array information */
    /* for now just increase */
    address = base_address;
    base_address += ops->last;
    base_address += (ops->last % 1024)==0 ? 0 : 1024 - (ops->last % 1024);

    MAKE_STD_ZVAL(tmpzval);
    array_init(tmpzval);
    for (op = ops->opcodes, i = 0; op && i < ops->last; op++, i++) {
        zval *zop;
        int oplines;

        MAKE_STD_ZVAL(zop);
        bytekit_disassemble_op(zop, ops, op, i, address+i, flags, &oplines TSRMLS_CC);
        add_next_index_zval(tmpzval, zop);
        
        /* handle multi opline opcodes correctly */
        i += oplines - 1;
        while (oplines > 1) {
            op++; oplines--;
        }
    }   
    add_assoc_zval(return_value, "code", tmpzval);
    bytekit_build_basic_blocks(return_value, ops, flags TSRMLS_CC);
    
    
    
    /********************************************************************************/
    /* Now fill in raw information about the op_array similar to what parsekit does */
    /********************************************************************************/
        
    add_assoc_long(raw, "type", (long)(ops->type));
    if (ops->function_name) {
        add_assoc_string(raw, "function_name", ops->function_name, 1);
    } else {
        add_assoc_null(raw, "function_name");
    }

    if (ops->scope && ops->scope->name) {
        add_assoc_stringl(raw, "scope", ops->scope->name, ops->scope->name_length, 1);
    } else {
        add_assoc_null(raw, "scope");
    }
    add_assoc_long(raw, "fn_flags", ops->fn_flags);
    
    add_assoc_long(raw, "num_args", ops->num_args);
    add_assoc_long(raw, "required_num_args", ops->required_num_args);
    add_assoc_bool(raw, "pass_rest_by_reference", ops->pass_rest_by_reference);


    if (ops->last_try_catch > 0) {
        MAKE_STD_ZVAL(tmpzval);
        array_init(tmpzval);
        for (i = 0; i < ops->last_try_catch; i++) {
            zval *tmp_zval;

            MAKE_STD_ZVAL(tmp_zval);
            array_init(tmp_zval);
            add_assoc_long(tmp_zval, "try_op", ops->try_catch_array[i].try_op);
            add_assoc_long(tmp_zval, "catch_op", ops->try_catch_array[i].catch_op);
            add_index_zval(tmpzval, i, tmp_zval);
        }
        add_assoc_zval(raw, "try_catch_array", tmpzval);
    } else {
        add_assoc_null(raw, "try_catch_array");
    }

/*  TODO: not supported by PHP 5.3 */
/*  add_assoc_bool(raw, "uses_this", ops->uses_this); */
    add_assoc_long(raw, "line_start", ops->line_start);
    add_assoc_long(raw, "line_end", ops->line_end);

    if (ops->doc_comment && ops->doc_comment_len) {
        add_assoc_stringl(raw, "doc_comment", ops->doc_comment, ops->doc_comment_len, 1);
    } else {
        add_assoc_null(raw, "doc_comment");
    }

    add_assoc_bool(raw, "return_reference", ops->return_reference);
    add_assoc_long(raw, "refcount", *(ops->refcount));
    add_assoc_long(raw, "last", ops->last);
    add_assoc_long(raw, "size", ops->size);
    add_assoc_long(raw, "T", ops->T);
    add_assoc_long(raw, "last_brk_cont", ops->last_brk_cont);
    add_assoc_long(raw, "current_brk_cont", ops->current_brk_cont);
    add_assoc_long(raw, "backpatch_count", ops->backpatch_count);
    add_assoc_bool(raw, "done_pass_two", ops->done_pass_two);

    if (ops->last_brk_cont > 0) {
        MAKE_STD_ZVAL(tmpzval);
        array_init(tmpzval);
        for (i = 0; i < ops->last_brk_cont; i++) {
            zval *tmp_zval;

            MAKE_STD_ZVAL(tmp_zval);
            array_init(tmp_zval);
            add_assoc_long(tmp_zval, "start", ops->brk_cont_array[i].start);
            add_assoc_long(tmp_zval, "cont", ops->brk_cont_array[i].cont);
            add_assoc_long(tmp_zval, "brk", ops->brk_cont_array[i].brk);
            add_assoc_long(tmp_zval, "parent", ops->brk_cont_array[i].parent);
            add_index_zval(tmpzval, i, tmp_zval);
        }
        add_assoc_zval(raw, "brk_cont_array", tmpzval);
    } else {
        add_assoc_null(raw, "brk_cont_array");
    }
    
    if (ops->vars) {
        MAKE_STD_ZVAL(tmpzval);
        array_init(tmpzval);
        for (i = 0; i < ops->last_var; i++) {
            add_index_stringl(tmpzval, i, ops->vars[i].name, ops->vars[i].name_len, 1);
        }       
        add_assoc_zval(raw, "cv", tmpzval);
    } else {
        add_assoc_null(raw, "cv");
    }

    if (ops->static_variables) {
        zval *tmp_zval;

        MAKE_STD_ZVAL(tmpzval);
        array_init(tmpzval);
        zend_hash_copy(HASH_OF(tmpzval), ops->static_variables, (copy_ctor_func_t) zval_add_ref, (void *) &tmp_zval, sizeof(zval *));
        add_assoc_zval(raw, "static_variables", tmpzval);
    } else {
        add_assoc_null(raw, "static_variables");
    }

    if (ops->start_op) {
        char sop[(sizeof(void *) * 2) + 1];

        snprintf(sop, sizeof(sop), "%X", (unsigned int)ops->start_op); 
        add_assoc_string(raw, "start_op", sop, 1);
    } else {
        add_assoc_null(raw, "start_op");
    }

    if (ops->filename) {
        add_assoc_string(raw, "filename", ops->filename, 1);
    } else {
        add_assoc_null(raw, "filename");
    }

    MAKE_STD_ZVAL(tmpzval);
    array_init(tmpzval);
    for (op = ops->opcodes, i = 0; op && i < ops->last; op++, i++) {
        zval *zop;

        MAKE_STD_ZVAL(zop);
        bytekit_raw_op(zop, ops, op, flags TSRMLS_CC);
        add_next_index_zval(tmpzval, zop);
    }   
    add_assoc_zval(raw, "opcodes", tmpzval);
    
    add_assoc_zval(return_value, "raw", raw);
}
/* }}} */

/* {{{ bytekit disassemble_functions */
static int bytekit_disassemble_functions(zval *return_value, char *scope, zval *methods, HashTable *function_table, int count, long flags TSRMLS_DC)
{
    HashPosition pos;
    
    zval **functions, **raw;
    HashTable *ht = HASH_OF(return_value);
    
    if (ht && zend_hash_find(ht, "functions", sizeof("functions"), (void **)&functions) == FAILURE) {
                        
    }

    zend_hash_internal_pointer_end_ex(function_table, &pos);
    while (count < zend_hash_num_elements(function_table)) {
        zend_function *function;
        zval *function_ops;
        char *name;

        if (zend_hash_get_current_data_ex(function_table, (void **)&function, &pos) == FAILURE) {
            php_error_docref(NULL TSRMLS_CC, E_ERROR, "bytekit_disassemble_functions: unable to traverse function table.");
            return FAILURE;
        }
        zend_hash_move_backwards_ex(function_table, &pos);
        count++;
        
        if (function->type == ZEND_INTERNAL_FUNCTION) {
            continue;
        } else if (function->type != ZEND_USER_FUNCTION) {
            php_error_docref(NULL TSRMLS_CC, E_ERROR, "bytekit_disassemble_functions: illegal function entry found");
            return FAILURE;
        }
        if (methods != NULL) {
            add_next_index_string(methods, function->common.function_name, 1);
        }
        
        MAKE_STD_ZVAL(function_ops);
        bytekit_disassemble_op_array(function_ops, &(function->op_array), flags TSRMLS_CC);
        if (scope == NULL) {
            name = estrdup(function->common.function_name);
        } else {
            spprintf(&name, 0, "%s::%s", scope, function->common.function_name);
        }
        
        /* NEW HANDLING OF ARG_INFO */
        
        if (zend_hash_find(HASH_OF(function_ops), "raw", sizeof("raw"), (void **)&raw) == SUCCESS) {
            zval *tmpzval;
            if (function->common.num_args && function->common.arg_info) {
                MAKE_STD_ZVAL(tmpzval);
                bytekit_raw_arginfo(tmpzval, function->common.num_args, function->common.arg_info, flags TSRMLS_CC);
                add_assoc_zval(*raw, "arg_info", tmpzval);
            } else {
                add_assoc_null(*raw, "arg_info");
            }
        }
        
        add_assoc_zval(*functions, name, function_ops);
        efree(name);
    }

    return SUCCESS;
}
/* }}} */

/* {{{ bytekit_disassemble_class_entry */
static int bytekit_disassemble_class_entry(zval *return_value, zval *raw_class, zend_class_entry *ce, long flags TSRMLS_DC)
{
    zval *tmpzval;
    int i;
    char *classname = estrndup(ce->name, ce->name_length);
    
    add_assoc_long(raw_class, "type", ce->type);
    add_assoc_stringl(raw_class, "name", ce->name, ce->name_length, 1);
    if (ce->parent) {
        add_assoc_stringl(raw_class, "parent", ce->parent->name, ce->parent->name_length, 1);
    } else {
        add_assoc_null(raw_class, "parent");
    }
    add_assoc_bool(raw_class, "constants_updated", ce->constants_updated);

    add_assoc_long(raw_class, "ce_flags", ce->ce_flags);

    if (ce->constructor) {
        add_assoc_string(raw_class, "constructor", ce->constructor->common.function_name, 1);
    } else {
        add_assoc_null(raw_class, "constructor");
    }

    if (ce->clone) {
        add_assoc_string(raw_class, "clone", ce->clone->common.function_name, 1);
    } else {
        add_assoc_null(raw_class, "clone");
    }

    if (ce->__get) {
        add_assoc_string(raw_class, "__get", ce->__get->common.function_name, 1);
    } else {
        add_assoc_null(raw_class, "__get");
    }

    if (ce->__set) {
        add_assoc_string(raw_class, "__set", ce->__set->common.function_name, 1);
    } else {
        add_assoc_null(raw_class, "__set");
    }

    if (ce->__call) {
        add_assoc_string(raw_class, "__call", ce->__call->common.function_name, 1);
    } else {
        add_assoc_null(raw_class, "__call");
    }

    if (zend_hash_num_elements(&(ce->properties_info)) > 0) {
        zend_property_info *property_info;

        MAKE_STD_ZVAL(tmpzval);
        array_init(tmpzval);
        for (zend_hash_internal_pointer_reset(&(ce->properties_info));
            zend_hash_get_current_data(&(ce->properties_info), (void **)&property_info) == SUCCESS;
            zend_hash_move_forward(&(ce->properties_info))) {
            zval *tmp_zval;

            MAKE_STD_ZVAL(tmp_zval);
            array_init(tmp_zval);
            add_assoc_long(tmp_zval, "flags", property_info->flags);
            add_assoc_stringl(tmp_zval, "name", property_info->name, property_info->name_length, 1);
            add_next_index_zval(tmpzval, tmp_zval);
        }
        add_assoc_zval(raw_class, "properties_info", tmpzval);
    } else {
        add_assoc_null(raw_class, "properties_info");
    }
    
    if (ce->static_members && zend_hash_num_elements(ce->static_members) > 0) {
        zval *tmp_zval;

        MAKE_STD_ZVAL(tmpzval);
        array_init(tmpzval);
        zend_hash_copy(HASH_OF(tmpzval), ce->static_members, (copy_ctor_func_t) zval_add_ref, (void *) &tmp_zval, sizeof(zval *));  
        add_assoc_zval(raw_class, "static_members", tmpzval);
    } else {
        add_assoc_null(raw_class, "static_members");
    }

    if (zend_hash_num_elements(&(ce->constants_table)) > 0) {
        zval *tmp_zval;

        MAKE_STD_ZVAL(tmpzval);
        array_init(tmpzval);
        zend_hash_copy(HASH_OF(tmpzval), &(ce->constants_table), (copy_ctor_func_t) zval_add_ref, (void *) &tmp_zval, sizeof(zval *));  
        add_assoc_zval(raw_class, "constants_table", tmpzval);
    } else {
        add_assoc_null(raw_class, "constants_table");
    }

    if (ce->num_interfaces > 0) {
        MAKE_STD_ZVAL(tmpzval);
        array_init(tmpzval);
        for (i = 0; i < ce->num_interfaces; i++) {
            add_next_index_stringl(tmpzval, ce->interfaces[i]->name, ce->interfaces[i]->name_length, 1);
        }
        add_assoc_zval(raw_class, "interfaces", tmpzval);
    } else {
        add_assoc_null(raw_class, "interfaces");
    }

    add_assoc_string(raw_class, "filename", ce->filename, 1);
    add_assoc_long(raw_class, "line_start", ce->line_start);
    add_assoc_long(raw_class, "line_end", ce->line_end);
    if (ce->doc_comment) {
        add_assoc_stringl(raw_class, "doc_comment", ce->doc_comment, ce->doc_comment_len, 1);
    } else {
        add_assoc_null(raw_class, "doc_comment");
    }

    add_assoc_long(raw_class, "refcount", ce->refcount);

    if (zend_hash_num_elements(&(ce->function_table)) > 0) {
        zval *methods;
        MAKE_STD_ZVAL(methods);
        array_init(methods);
        if (bytekit_disassemble_functions(return_value, classname, methods, &(ce->function_table), 0, flags TSRMLS_CC) == FAILURE) {
            efree(classname);
            return FAILURE;
        }
        add_assoc_zval(raw_class, "methods", methods);
    } else {
        add_assoc_null(raw_class, "methods");
    }
    efree(classname);

    if (zend_hash_num_elements(&(ce->default_properties)) > 0) {
        zval *tmp_zval;

        MAKE_STD_ZVAL(tmpzval);
        array_init(tmpzval);
        zend_hash_copy(HASH_OF(tmpzval), &(ce->default_properties), (copy_ctor_func_t) zval_add_ref, (void *) &tmp_zval, sizeof(zval *));   
        add_assoc_zval(raw_class, "default_properties", tmpzval);
    } else {
        add_assoc_null(raw_class, "default_properties");
    }

    return SUCCESS;
}
/* }}} */


/* {{{ bytekit_disassemble_classes */
static int bytekit_disassemble_classes(zval *return_value, HashTable *class_table, int count, long flags TSRMLS_DC)
{
    zval *classes_raw;
    
    MAKE_STD_ZVAL(classes_raw);
    array_init(classes_raw);
    
    zend_hash_internal_pointer_end(class_table);
    while (count < zend_hash_num_elements(class_table)) {
        zend_class_entry *class_entry, **pce;
        zval *class_data;

        if (zend_hash_get_current_data(class_table, (void **)&pce) == FAILURE || !pce || !(*pce)) {
            php_error_docref(NULL TSRMLS_CC, E_ERROR, "bytekit_disassemble_classes: unable to traverse class table.");
            return FAILURE;
        }
        count++;
        zend_hash_move_backwards(class_table);

        class_entry = *pce;

        if (class_entry->type != ZEND_USER_CLASS) {
            php_error_docref(NULL TSRMLS_CC, E_ERROR, "bytekit_disassemble_classes: illegal class entry found.");
            return FAILURE;
        }
        MAKE_STD_ZVAL(class_data);
        array_init(class_data);
        if (bytekit_disassemble_class_entry(return_value, class_data, class_entry, flags TSRMLS_CC) == FAILURE) {
            return FAILURE;
        }

        add_assoc_zval_ex(classes_raw, class_entry->name, class_entry->name_length + 1, class_data);
    }
    
    add_assoc_zval(return_value, "classes", classes_raw);
    return SUCCESS;
}
/* }}} */

/* {{{ bytekit_cleanup_functions_and_classes */
static void bytekit_disassemble_functions_and_classes(zval *return_value, int num_functions, int num_classes, long flags TSRMLS_DC)
{
    if (num_functions < zend_hash_num_elements(EG(function_table))) {
        bytekit_disassemble_functions(return_value, NULL, NULL, EG(function_table), num_functions, flags TSRMLS_CC);
    }
    
    if (num_classes < zend_hash_num_elements(EG(class_table))) {
        bytekit_disassemble_classes(return_value, EG(class_table), num_classes, flags TSRMLS_CC);
    }
}
/* }}} */

/* {{{ proto array bytekit_disassemble_file(string filename[, array &errors[, mixed options]])
   Return array of disassembled opcodes compiled from phpfile */
PHP_FUNCTION(bytekit_disassemble_file)
{
    int original_num_functions = zend_hash_num_elements(EG(function_table));
    int original_num_classes = zend_hash_num_elements(EG(class_table));
    int dependencies_num_functions;
    int dependencies_num_classes;
    zend_uchar original_handle_op_arrays;
    zend_op_array *ops = NULL;
    zval *zfilename, *zerrors = NULL, *zoptions = NULL;
    HashTable *dependencies = NULL;
    long flags = 0;
    zend_execute_data *orig_execute_data;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|zz", &zfilename, &zerrors, &zoptions) == FAILURE) {
        RETURN_FALSE;
    }

    /* handle the error array */
    if (zerrors) {
        zval_dtor(zerrors);
        ZVAL_NULL(zerrors);
        BYTEKIT_G(compile_errors) = zerrors;
    }
    
    /* handle the options */
    if (zoptions) {
        
        if (Z_TYPE_P(zoptions) == IS_LONG) {
            flags = Z_LVAL_P(zoptions);
        } else if (Z_TYPE_P(zoptions) == IS_ARRAY) {
            zval **option;
            HashTable *ht = HASH_OF(zoptions);
            
            if (ht && zend_hash_find(ht, "dependencies", sizeof("dependencies"), (void **)&option) == SUCCESS && Z_TYPE_PP(option) == IS_ARRAY) {
                dependencies = HASH_OF(*option);                
            } else if (ht && zend_hash_find(ht, "flags", sizeof("flags"), (void **)&option) == SUCCESS && Z_TYPE_PP(option) == IS_LONG) {
                flags = Z_LVAL_PP(option);
            }
        } /* ignore everything else */
    }

    convert_to_string(zfilename);
#ifdef ZEND_COMPILE_HANDLE_OP_ARRAY
    original_handle_op_arrays = CG(compiler_options) & ZEND_COMPILE_HANDLE_OP_ARRAY;
/*  CG(compiler_options) &= ~ZEND_COMPILE_HANDLE_OP_ARRAY; */
#else
    original_handle_op_arrays = CG(handle_op_arrays);
/*  CG(handle_op_arrays) = 0;~*/
#endif
    /* this is not 100% thread-safe but without
       a lock it is not possible to get it better */
    while (zend_error_cb == bytekit_error_cb) {}
    bytekit_original_error_cb = zend_error_cb;
    zend_error_cb = bytekit_error_cb;

    /* Handle all dependencies */
    if (dependencies) {
        
        zval **zdependency;
        
        for (zend_hash_internal_pointer_reset(dependencies);
            zend_hash_get_current_data(dependencies, (void **)&zdependency) == SUCCESS;
            zend_hash_move_forward(dependencies)) {
            
            orig_execute_data = EG(current_execute_data);
            
            zend_try {
                ops = compile_filename(ZEND_INCLUDE, *zdependency TSRMLS_CC);
            } zend_catch {
                ops = NULL;
            } zend_end_try();
            
            EG(current_execute_data) = orig_execute_data;
        
            /* in case of an error clean up and exit */
            if (ops == NULL || Z_TYPE_P(BYTEKIT_G(compile_errors)) != IS_NULL) {
                
                bytekit_cleanup_functions_and_classes(original_num_functions, original_num_classes TSRMLS_CC);
                
                /* do not forget to restore the error cb */
                if (zend_error_cb == bytekit_error_cb) {
                    zend_error_cb = bytekit_original_error_cb;
                }
                RETURN_FALSE;
            } else if (ops != NULL) {
                /* get rid of op_array */
                destroy_op_array(ops TSRMLS_CC);
                efree(ops);
            }
        }
    }

    dependencies_num_functions = zend_hash_num_elements(EG(function_table));
    dependencies_num_classes = zend_hash_num_elements(EG(class_table));

    orig_execute_data = EG(current_execute_data);
    
    zend_try {
        
        zend_file_handle file_handle;
        
        file_handle.filename = zfilename->value.str.val;
        file_handle.free_filename = 0;
        file_handle.type = ZEND_HANDLE_FILENAME;
        file_handle.opened_path = NULL;
        file_handle.handle.fp = NULL;
        
        ops = zend_compile_file(&file_handle, ZEND_INCLUDE TSRMLS_CC);
    
        zend_destroy_file_handle(&file_handle TSRMLS_CC);
        
    } zend_catch {
        ops = NULL;
    } zend_end_try();
    
    EG(current_execute_data) = orig_execute_data;
    
    
    /* this is again not 100% thread-safe but better than nothing */
    if (zend_error_cb == bytekit_error_cb) {
        zend_error_cb = bytekit_original_error_cb;
    }

    BYTEKIT_G(compile_errors) = NULL;
#ifdef ZEND_COMPILE_HANDLE_OP_ARRAY
    CG(compiler_options) |= original_handle_op_arrays;
#else
    CG(handle_op_arrays) = original_handle_op_arrays;
#endif
    if (ops) {
        zval *main, *functions;
        array_init(return_value);
        MAKE_STD_ZVAL(main);
        MAKE_STD_ZVAL(functions);
        array_init(functions);
        
        bytekit_disassemble_op_array(main, ops, flags TSRMLS_CC);
        destroy_op_array(ops TSRMLS_CC);
        efree(ops);
        add_assoc_zval(functions, "main", main);
        add_assoc_zval(return_value, "functions", functions);
        bytekit_disassemble_functions_and_classes(return_value, dependencies_num_functions, dependencies_num_classes, flags TSRMLS_CC);
        add_assoc_long(return_value, "base", base_address);
    } else {
        RETVAL_FALSE;
    }
    
    /* do not forget to cleanup our mess */
    bytekit_cleanup_functions_and_classes(original_num_functions, original_num_classes TSRMLS_CC);
}
/* }}} */

/* {{{ proto long bytekit_get_baseaddress()
   Return the current baseaddress */
PHP_FUNCTION(bytekit_get_baseaddress) 
{
    RETURN_LONG(base_address);
}
/* }}} */

/* {{{ proto bytekit_set_baseaddress(long baseaddress)
   Set the current baseaddress */
PHP_FUNCTION(bytekit_set_baseaddress) 
{
    zval **value;
    
    if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &value) == FAILURE) {
        WRONG_PARAM_COUNT;
    }

    convert_scalar_to_number_ex(value);
    
    if (Z_TYPE_PP(value) == IS_DOUBLE) {
        base_address = fabs(Z_DVAL_PP(value));
    } else if (Z_TYPE_PP(value) == IS_LONG) {
        base_address = Z_LVAL_PP(value);
    }
    RETURN_LONG(base_address);
}
/* }}} */

#if PHP_VERSION_ID < 50300
static
#endif
    ZEND_BEGIN_ARG_INFO(bytekit_second_arg_force_ref, 0)
        ZEND_ARG_PASS_INFO(0)
        ZEND_ARG_PASS_INFO(1)
    ZEND_END_ARG_INFO()

#if PHP_VERSION_ID < 50300
static
#endif
    ZEND_BEGIN_ARG_INFO(bytekit_arginfo_get_baseaddress, 0)
    ZEND_END_ARG_INFO()

#if PHP_VERSION_ID < 50300
static
#endif
    ZEND_BEGIN_ARG_INFO(bytekit_arginfo_set_baseaddress, 0)
        ZEND_ARG_INFO(0, baseaddress)
    ZEND_END_ARG_INFO()

/* {{{ function_entry
 */
function_entry bytekit_functions[] = {
    PHP_FE(bytekit_disassemble_file,            bytekit_second_arg_force_ref)
    PHP_FE(bytekit_get_baseaddress,             bytekit_arginfo_get_baseaddress)
    PHP_FE(bytekit_set_baseaddress,             bytekit_arginfo_set_baseaddress)
    {NULL, NULL, NULL}
};
/* }}} */

/* {{{ bytekit_module_entry
 */
zend_module_entry bytekit_module_entry = {
    STANDARD_MODULE_HEADER,
    "bytekit",
    bytekit_functions,
    PHP_MINIT(bytekit),
    PHP_MSHUTDOWN(bytekit),
    NULL, /* RINIT */
    NULL, /* RSHUTDOWN */
    PHP_MINFO(bytekit),
    BYTEKIT_VERSION,
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_BYTEKIT
ZEND_GET_MODULE(bytekit)
#endif

#define REGISTER_BYTEKIT_CONSTANTS(define_list)     \
    {   \
        char const_name[96];    \
        int const_name_len; \
        bytekit_define_list *defines = (define_list);   \
        while (defines->str) {  \
            /* the macros don't like variable constant names */ \
            const_name_len = snprintf(const_name, sizeof(const_name), "BYTEKIT_%s", defines->str);  \
            zend_register_long_constant(const_name, const_name_len+1, defines->val, CONST_CS | CONST_PERSISTENT, module_number TSRMLS_CC); \
            defines++;  \
        }   \
    }

/* {{{ bytekit_init_globals
 */
static void bytekit_init_globals(zend_bytekit_globals *bytekit_globals)
{
    bytekit_globals->compile_errors = NULL;
    bytekit_globals->in_error_cb = 0;
	bytekit_globals->op_array_opcodes = NULL;
	bytekit_globals->opcodes_base = NULL;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(bytekit)
{
    REGISTER_BYTEKIT_CONSTANTS(bytekit_constant_names);
    REGISTER_BYTEKIT_CONSTANTS(bytekit_opcode_names);
    REGISTER_BYTEKIT_CONSTANTS(bytekit_zend_constant_names);
    
    ZEND_INIT_MODULE_GLOBALS(bytekit, bytekit_init_globals, NULL);

    return SUCCESS;
}
/* }}} */

    /* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(bytekit)
{
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(bytekit)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "Bytekit support", "enabled");
    php_info_print_table_end();
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
