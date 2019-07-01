#include "lib.h"

enum {PVAL_NUM, PVAL_DNUM, PVAL_STRING, PVAL_CHAR, PVAL_ERR, PVAL_SYM, PVAL_SEXPR};
enum {PERR_DIV_ZERO, PERR_BAD_OP, PERR_BAD_NUM};

typedef struct pval
{
    int type;
    long num;
    double dnum;
    char* err;
    char* sym;

    int count;
    struct pval** cell;
} pval;

long min(long* a, long* b) {

    long minimum = *a;

    if (*a == *b){
        return *a;
    } 
    else
    {
        if (*a > *b){
            minimum = *b;
        }
        else 
        {
            minimum = *a;
        }

    }
    return minimum;
}

long max(long* a, long* b) {
    long maximum = *a;

    if (*a == *b){
        return *a;
    } 
    else
    {
        if (*a > *b){
            maximum = *a;
        }
        else 
        {
            maximum = *b;
        }

    }
    return maximum;
}

double dmin(double* a, double* b)
{
    double minimum = *a;

    if (*a == *b){
        return *a;
    } 
    else
    {
        if (*a > *b){
            minimum = *b;
        }
        else 
        {
            minimum = *a;
        }

    }
    return minimum;
}

double dmax(double* a, double* b)
{
    double maximum = *a;
    if (*a == *b){
        return *a;
    } 
    else
    {
        if (*a > *b){
            maximum = *a;
        }
        else 
        {
            maximum = *b;
        }

    }
    return maximum;
}