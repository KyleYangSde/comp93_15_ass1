#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "postgres.h"
#include "fmgr.h"

PG_MODULE_MAGIC;


/*
 * src/tutorial/pname.c
 *
 ******************************************************************************
*PersonName structure (variable size)
*header
******************************************************************************/

typedef struct Pname {
    int32 length;
    char FullName[1];
} Pname;

static char* standard_form(char *str);
static char* family_name(char* str);
static char* given_name(char* str);
static int invalid_words(char *str);
static int word_check(char *str);
static int check(char* str);
static int inter_compare(Pname *a, Pname *b);

/*****************************************************************************
 * Supported functions
 *****************************************************************************/

static char* standard_form(char *str){
    int len = strlen(str);
    char *std_name = (char *) palloc(sizeof(char) * len);
    int start = 0, end = len;
    for (int i = 0; i < len; i++){
        if (str[i] != ' ') {
            start = i;
            break;
        }
    }
    for (int i = len - 1; i >= 0; i--){
        if (str[i] != ' '){
            end = i;
            break;
        }
    }
    int pos = 0;
    for (int i = start; i <= end; i++){
        if (str[i] == ' '){
            if (str[i + 1] == ' '){
                continue;
            }
        }
        std_name[pos] = str[i];
        pos++;
    }
    std_name[pos] = '\0';
    return std_name;
}

static char* family_name(char* str){
    int len = strlen(str);
    int num_comma = 0, pos_comma = len;
    for (int i = 0; i < len; i++){
        if (str[i] == ','){
            pos_comma = i;
	    break;
        }
    }
    char * family_name = (char *) palloc(sizeof(char) * (pos_comma + 1));
    for (int i = 0; i < pos_comma; i++){
        family_name[i] = str[i];
    }
    family_name[pos_comma] = '\0';
    return family_name;
}

static char* given_name(char* str){
    int len = strlen(str);
    int num_comma = 0, pos_comma = 0;
    for (int i = 0; i < len; i++){
        if (str[i] == ','){
            pos_comma = i;
	    break;
        }
    }
    char *given_name = (char *) palloc(len - pos_comma);
    for (int i = pos_comma + 1; i < len; i++){
        given_name[i - pos_comma - 1] = str[i];
    }
    given_name[len - pos_comma - 1] = '\0';
    return given_name;
}

static int invalid_words(char *str){
    int len = strlen(str);
    if (len < 2){
        return 0;
    }
    //
    if (str[0] < 'A' || str[0] > 'Z'){
        return 0;
    }
    for (int i = 1; i < len; i++){
        if (str[i] >= 'a' && str[i] <= 'z'){
            continue;
        } else if (str[i] >= 'A' && str[i] <= 'Z'){
            //if (str[i - 1] != '\'' && str[i - 1] != '-'){
               // return 0;
            //}
	    continue;
        } else if (str[i] >= '0' && str[i] <= '9'){
            return 0;
        }else if (str[i] == '\'' || str[i] == '-'){
            continue;
        } else{
            return 0;
        }
    }
    // some titles
    if (strcmp(str, "Mr") == 0){
        return 0;
    }
    if (strcmp(str, "Dr") == 0){
        return 0;
    }
    if (strcmp(str, "Ms") == 0){
        return 0;
    }
    if (strcmp(str, "Prof") == 0){
        return 0;
    }
    if (strcmp(str, "Miss") == 0){
        return 0;
    }
    return 1;
}

static int word_check(char *str){
    int len = strlen(str);
    int len_name = 0, start = 0;
    char *name = (char *) palloc(sizeof(char) * len);
    for (int i = 0; i < len; i++){
        if(str[i] == ' ' || i == len - 1){
            if (i == len - 1){
                len_name++;
            }
            for (int j = 0; j < len_name; j++){
                name[j] = str[start + j];
            }
            name[len_name] = '\0';
            start = i + 1;
            len_name = 0;
            //printf("00%s00\n", name);
            if (invalid_words(name) == 0){
                return 0;
            }
        } else{
            len_name++;
        }

    }
    return 1;
}

static int check(char* str){
    int len = strlen(str);
    int num_comma = 0, pos_comma = 0;
    for (int i = 0; i < len; i++){
        if (str[i] == ','){
            num_comma++;
            pos_comma = i;
        }
    }
    if (str[0] == ' ' || str[len - 1] == ' '){ return 0;}
    // no single-word names && only one comma
    if (num_comma != 1){ return 0;}
    // comma appears in the middle of string
    if (pos_comma < 2 || len - pos_comma < 3){ return 0;}
    // a single space after the comma and no space before the comma
    if (str[pos_comma - 1] == ' ' || str[pos_comma + 2] == ' '){ return 0;}
    char * f_name = (char *) palloc(sizeof(char) * (pos_comma + 1));
    for (int i = 0; i < pos_comma; i++){
        f_name[i] = str[i];
    }
    f_name[pos_comma] = '\0';
    //printf("00%s00\n", f_name);
    char *g_name = (char *) palloc(len - pos_comma);
    for (int i = pos_comma + 1; i < len; i++){
        g_name[i - pos_comma - 1] = str[i];
    }
    g_name[len - pos_comma - 1] = '\0';
    //printf("00%s00\n", g_name);
    //grammar check
    if (word_check(standard_form(f_name)) != 1){ return 0;}
    if (word_check(standard_form(g_name)) != 1){ return 0;}
    return 1;
}

// compare name, same result as strcmp
static int inter_compare(Pname *a, Pname *b) {
    char *a_f = family_name(a->FullName);
    char *a_g = given_name(a->FullName);
    char *b_f = family_name(b->FullName);
    char *b_g = given_name(b->FullName);
    if (strcmp(a_f, b_f) != 0){
        return strcmp(a_f, b_f);
    } else {
        return strcmp(a_g, b_g);
    }
}

/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/

PG_FUNCTION_INFO_V1(pname_in);

Datum
pname_in(PG_FUNCTION_ARGS)
{
    char *str = PG_GETARG_CSTRING(0);
    if (check(str) == 0){
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                        errmsg("invalid input syntax for type %s: \"%s\"",
                               "PersonName", str)));
    }
    int len = strlen(str), comma, blank = 0;
    for (int i = 0; i < len; i++){
        if (str[i] == ','){
            comma = i;
            if (str[i + 1] == ' '){
                blank++;
            }
            break;
        }
    }
    if (blank != 0){
        for (int i = comma + 1; i < len - 1; i++){
            str[i] = str[i + 1];
        }
        str[len - 1] = '\0';
        len--;
    }
    Pname *result = (Pname *) palloc(VARHDRSZ + len + 1);
    SET_VARSIZE(result, VARHDRSZ + len + 1);
    memcpy(result->FullName, str, len + 1);
    PG_RETURN_POINTER(result);
}


PG_FUNCTION_INFO_V1(pname_out);

Datum
pname_out(PG_FUNCTION_ARGS)
{
    Pname    *pname = (Pname *) PG_GETARG_POINTER(0);
    char       *result;

    result = psprintf("%s", pname->FullName);
    PG_RETURN_CSTRING(result);
}


/*****************************************************************************
 * Binary Input/Output functions(optional)
 *why is blank?
*beacuse i don't know how to do it
 *****************************************************************************/

/*****************************************************************************
 * Operator functions
 *****************************************************************************/

// =
PG_FUNCTION_INFO_V1(pname_eq);

Datum
pname_eq(PG_FUNCTION_ARGS) {
    Pname *a = (Pname *) PG_GETARG_POINTER(0);
    Pname *b = (Pname *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(inter_compare(a, b) == 0);
}

// !=
PG_FUNCTION_INFO_V1(pname_ne);

Datum
pname_ne(PG_FUNCTION_ARGS) {
    Pname *a = (Pname *) PG_GETARG_POINTER(0);
    Pname *b = (Pname *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(inter_compare(a, b) != 0);
}

// <
PG_FUNCTION_INFO_V1(pname_lt);

Datum
pname_lt(PG_FUNCTION_ARGS) {
    Pname *a = (Pname *) PG_GETARG_POINTER(0);
    Pname *b = (Pname *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(inter_compare(a, b) < 0);
}

// <=
PG_FUNCTION_INFO_V1(pname_le);

Datum
pname_le(PG_FUNCTION_ARGS) {
    Pname *a = (Pname *) PG_GETARG_POINTER(0);
    Pname *b = (Pname *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(inter_compare(a, b) <= 0);
}

// >
PG_FUNCTION_INFO_V1(pname_gt);

Datum
pname_gt(PG_FUNCTION_ARGS) {
    Pname *a = (Pname *) PG_GETARG_POINTER(0);
    Pname *b = (Pname *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(inter_compare(a, b) > 0);
}

// >=
PG_FUNCTION_INFO_V1(pname_ge);

Datum
pname_ge(PG_FUNCTION_ARGS) {
    Pname *a = (Pname *) PG_GETARG_POINTER(0);
    Pname *b = (Pname *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(inter_compare(a, b) >= 0);
}

PG_FUNCTION_INFO_V1(pname_cmp);

Datum
pname_cmp(PG_FUNCTION_ARGS) {
    Pname *a = (Pname *) PG_GETARG_POINTER(0);
    Pname *b = (Pname *) PG_GETARG_POINTER(1);

    PG_RETURN_INT32(inter_compare(a, b));
}


//hash function
PG_FUNCTION_INFO_V1(pname_hash);

Datum
pname_hash(PG_FUNCTION_ARGS) {
    Datum result;
    Pname *pname = (Pname *) PG_GETARG_POINTER(0);
    char *a_f = family_name(pname->FullName);
    char *a_g = given_name(pname->FullName);
    char *str = (char *) palloc (strlen(a_f) + strlen(a_g) + 2);
    strcpy(str, a_f);
    strcat(str, " ");
    strcat(str, a_g);
    result = hash_any((unsigned char*) str, strlen(a_f) + strlen(a_g) + 2);
    pfree(str);
    PG_RETURN_DATUM(result);
}


/*****************************************************************************
 * others
 *****************************************************************************/

PG_FUNCTION_INFO_V1(family);

Datum
family(PG_FUNCTION_ARGS) {
    Pname *pname = (Pname *) PG_GETARG_POINTER(0);
    char *FamilyName = family_name(pname->FullName);
    int len = strlen(FamilyName);
    Pname *result = (Pname *) palloc(VARHDRSZ + len + 1);
    SET_VARSIZE(result, VARHDRSZ + len + 1);
    memcpy(result->FullName, FamilyName, len + 1);
    PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(given);

Datum
given(PG_FUNCTION_ARGS) {
    Pname *pname = (Pname *) PG_GETARG_POINTER(0);
    char *GivenName = given_name(pname->FullName);
    int len = strlen(GivenName);
    Pname *result = (Pname *) palloc(VARHDRSZ + len + 1);
    SET_VARSIZE(result, VARHDRSZ + len + 1);
    memcpy(result->FullName, GivenName, len + 1);
    PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(show);

Datum
show(PG_FUNCTION_ARGS) {
    Pname *pname = (Pname *) PG_GETARG_POINTER(0);
    char *a_f = family_name(pname->FullName);
    char *a_g = given_name(pname->FullName);
    for (int i = 0; i < strlen(a_g); i++){
        if (a_g[i] == ' '){
            a_g[i] = '\0';
            break;
        }
    } 
    char *str = (char *) palloc0 (strlen(a_f) + strlen(a_g) + 2);
    strcpy(str, a_g);
    strcat(str, " ");
    strcat(str, a_f);
    str[strlen(a_f) + strlen(a_g) + 1] = '\0';
    int len = strlen(str);
    Pname *result = (Pname *) palloc(VARHDRSZ + len + 1);
    SET_VARSIZE(result, VARHDRSZ + len + 1);
    memcpy(result->FullName, str, len + 1);
    PG_RETURN_POINTER(result);
}




