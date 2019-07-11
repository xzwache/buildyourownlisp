#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"
//#include "lib.h"
#include "lib.c"

#ifdef _WIN32
#include <string.h>

static char buffer[2048];

char* readline(char* prompt)
{
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char* cpy = malloc(strlen(buffer)+1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy)-1] = '\0';
    return cpy;
}

void add_history(char* unused) {}
#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

pval eval_op(pval x, char* op, pval y);
pval eval(mpc_ast_t* t);
pval* pval_num(long x);
pval* pval_dnum(double x);
pval* pval_err(char* m);
void pval_print(pval* x);
void pval_println(pval* v);
pval* pval_read_num(mpc_ast_t* t);
pval* pval_read_dnum(mpc_ast_t* t);
void pval_expr_print(pval* v, char open, char close);
pval* pval_add(pval* v, pval* x);
pval* pval_read(mpc_ast_t* t);
pval* pval_sexpr(void);
pval* pval_eval(pval* v);
pval* pval_eval_sexpr(pval* v);
pval* pval_take(pval* v, int i);
pval* pval_pop (pval* v, int i);
pval* builtin_op(pval* v, char* op);

#define PASSERT(args, cond, err) if (!(cond)) {pval_del(args); return pval_err(err);}

void pval_print(pval* v) {
  switch (v->type) {
    case PVAL_NUM:   printf("%li", v->num); break;
    case PVAL_DNUM:   printf("%lf", v->dnum); break;
    case PVAL_ERR:   printf("Error: %s", v->err); break;
    case PVAL_SYM:   printf("%s", v->sym); break;
    case PVAL_SEXPR: pval_expr_print(v, '(', ')'); break;
    case PVAL_QEXPR: pval_expr_print(v, '{', '}'); break;
  }
}

void pval_println(pval* v) {pval_print(v); putchar('\n');}

pval* pval_num(long x)
{
    pval* v = malloc(sizeof(pval));
    v->type = PVAL_NUM;
    v->num = x;
    return v;
}

pval* pval_dnum(double x)
{
    pval* v = malloc(sizeof(pval));
    v->type = PVAL_DNUM;
    v->dnum = x;
    return v;
}

pval* pval_err(char* m)
{
    pval* v = malloc(sizeof(pval));
    v->type = PVAL_ERR;
    v->err = malloc(sizeof(m)+1);
    strcpy(v->err, m);
    return v;
}

pval* pval_sym(char* s)
{
    pval* v = malloc(sizeof(pval));
    v->type = PVAL_SYM;
    v->sym = malloc(sizeof(s)+1);
    strcpy(v->sym, s);
    return v;
}

pval* pval_sexpr(void)
{
    pval* v = malloc(sizeof(pval));
    v->type = PVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

pval* pval_qexpr(void)
{
    pval* v = malloc(sizeof(pval));
    v->type = PVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

pval* pval_read_num(mpc_ast_t* t)
{
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? pval_num(x) : pval_err("Invalid number");
}

pval* pval_read_dnum(mpc_ast_t* t)
{
    double x = atof(t->contents);
    return errno !=ERANGE ? pval_dnum(x) : pval_err("Invalid decimal number");
}

void pval_expr_print(pval* v, char open, char close)
{
    putchar(open);
    for (int i = 0; i < v->count; i++)
    {
        pval_print(v->cell[i]);

        if (i != (v->count-1))
        {
            putchar(' ');
        }
    }
    putchar(close);
}

pval* pval_add(pval* v, pval* x)
{
    v->count++;
    v->cell = realloc(v->cell, sizeof(pval*) * v->count);
    v->cell[v->count-1] = x;
    return v;
}

pval* pval_read(mpc_ast_t* t)
{
    if (strstr(t->tag, "int")) {return pval_read_num(t);}
    if (strstr(t->tag, "decimal")) {return pval_read_dnum(t);}
    if (strstr(t->tag, "symbol")) {return pval_sym(t->contents);}

    pval* x = NULL;
    if (strcmp(t->tag, ">") == 0) {x = pval_sexpr();}
    if (strstr(t->tag, "sexpr")) {x = pval_sexpr();}
    if (strstr(t->tag, "qexpr")) {x = pval_qexpr();}

    for (int i = 0; i < t->children_num; i++)
    {
        if (strcmp(t->children[i]->contents, "(") == 0) {continue;}
        if (strcmp(t->children[i]->contents, ")") == 0) {continue;}
        if (strcmp(t->children[i]->contents, "{") == 0) {continue;}
        if (strcmp(t->children[i]->contents, "}") == 0) {continue;}
        if (strcmp(t->children[i]->tag, "regex") == 0) {continue;}
        x = pval_add(x, pval_read(t->children[i]));
    }
    return x;
}

void pval_del(pval* v)
{
    switch (v->type)
    {
    case PVAL_NUM: break;
    case PVAL_DNUM: break;
    case PVAL_ERR: free(v->err); 
        break;
    case PVAL_SYM: free(v->sym); 
        break;
    case PVAL_QEXPR:
    case PVAL_SEXPR:
        for (int i = 0; i < v->count; i++){
            free(v->cell[i]);
        } 
        free(v->cell);
        break;
    default:
        break;
    }
    free(v);
}

pval num_to_dnum(pval v)
{
    pval t;
    t.type = PVAL_DNUM;
    t.dnum = (double)v.num;
    return t;
}

pval* builtin_op(pval* a, char* op)
{
    /* Ensure all arguments are numbers */
    for (int i = 0; i < a->count; i++) {
        if (a->cell[i]->type != PVAL_NUM && a->cell[i]->type != PVAL_DNUM) {
            pval_del(a);
            return pval_err("Cannot operate on non-number!");
        }
    }

      /* Pop the first element */
    pval* x = pval_pop(a, 0);

    if (x->type == PVAL_NUM)
    {
          /* If no arguments and sub then perform unary negation */
        if ((strcmp(op, "-") == 0) && a->count == 0) {
            x->num = -x->num;
        }

    /* While there are still elements remaining */
        while (a->count > 0) {
            /* Pop the next element */
            pval* y = pval_pop(a, 0);

            if (strcmp(op, "+") == 0) { x->num += y->num; }
            if (strcmp(op, "-") == 0) { x->num -= y->num; }
            if (strcmp(op, "*") == 0) { x->num *= y->num; }
            if (strcmp(op, "/") == 0) {
                if (y->num == 0) {
                    pval_del(x);
                    pval_del(y);
                    x = pval_err("Division By Zero!"); break;
                }
                x->num /= y->num;
            }
            if (strcmp(op, "%") == 0){
                if (y->num == 0){
                    pval_del(x);
                    pval_del(y);
                    x = pval_err("Error: Attempt to get the remainder of the division with double floating point values.");
                }
                x->num %= y->num;
            }
            pval_del(y);
        }
    }

    // Decimal values
    if (x->type == PVAL_DNUM)
    {
        if (strcmp(op, "-")==0 && a->count==0) { x->dnum = -x->dnum; }
        while (a->count > 0) {

            pval* y = pval_pop(a ,0);

            if (strcmp(op, "+") == 0) { x->dnum += y->dnum; }
            if (strcmp(op, "-") == 0) { x->dnum -= y->dnum; }
            if (strcmp(op, "*") == 0) { x->dnum *= y->dnum; }
            if (strcmp(op, "/") == 0) {
                if (y->dnum == 0) {
                    pval_del(x);
                    pval_del(y);
                    x = pval_err("Division By Zero!"); break;
                }
                x->dnum /= y->dnum;
            }
            if (strcmp(op, "%") == 0){
                x = pval_err("Error: Attempt to get the remainder of the division by zero.");
            }
            pval_del(y);
        }
    }
    pval_del(a);
    return x;
}

pval* builtin_head(pval* a)
{
    PASSERT(a, a->count == 1, "Function 'head' passed too many arguments!");
    PASSERT(a, a->cell[0]->type == PVAL_QEXPR, "Function 'head' passed incorrect type!");
    PASSERT(a, a->cell[0]->count != 0, "Function 'head' passed {}!");

    pval* v = pval_take(a,0);
    while(v->count > 1) { pval_del(pval_pop(v, 1));}
    return v;
}

pval* builtin_tail(pval* a)
{
    PASSERT(a, a->count == 1, "Function 'tail' passed too many arguments!");
    PASSERT(a, a->cell[0]->type == PVAL_QEXPR, "Function 'tail' passed incorrect type!");
    PASSERT(a, a->cell[0]->count != 0, "Function 'tail' passed {}!");

    pval* v = pval_take(a,0);
    pval_del(pval_pop(v, 0));
    return v;
}

pval* pval_eval(pval* v)
{
    if (v->type == PVAL_SEXPR) { return pval_eval_sexpr(v); }
    return v;
}

pval* pval_eval_sexpr(pval* v)
{
    // Evaluate Children.
    for (int i = 0; i < v->count; i++ )
    {
        v->cell[i] = pval_eval(v->cell[i]);
    }

    // Error Checking.
    for (int i = 0; i < v->count; i++)
    {
         if (v->cell[i]->type == PVAL_ERR) { return pval_take(v, i);}
    }

    // Epmty Expression
    if (v->count == 0) { return v; }

    // Single Expression
    if (v->count == 1) { return pval_take(v, 0); }

    // Ensure First Element is Symbol
    pval* f = pval_pop(v, 0);
    if (f->type != PVAL_SYM)
    {
        pval_del(f); 
        pval_del(v);
        return pval_err("S-expression does not start with symbol!");
    }

    pval* result = builtin_op(v, f->sym);
    pval_del(f);
    return result;

}

pval* pval_pop(pval* v, int i) {
  /* Find the item at "i" */
  pval* x = v->cell[i];

  /* Shift memory after the item at "i" over the top */
  memmove(&v->cell[i], &v->cell[i+1],
    sizeof(pval*) * (v->count-i-1));

  /* Decrease the count of items in the list */
  v->count--;

  /* Reallocate the memory used */
  v->cell = realloc(v->cell, sizeof(pval*) * v->count);
  return x;
}

pval* pval_take(pval* v, int i) {
  pval* x = pval_pop(v, i);
  pval_del(v);
  return x;
}

int main(int argc, char const *argv[])
{
    puts("Pied 0.1.2");

   //mpc_parser_t* Value = mpc_new("value");
    mpc_parser_t* Int = mpc_new("int");
    mpc_parser_t* Decimal = mpc_new("decimal");
    //mpc_parser_t* String = mpc_new("string");
    //mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Sexpr = mpc_new("sexpr");
    mpc_parser_t* Qexpr = mpc_new("qexpr");
    mpc_parser_t* Pied = mpc_new("pied");

    /*mpca_lang(MPCA_LANG_DEFAULT, 
        "                                                                                                                                \
        int   : /-?[0-9]+/ ;                                                                                                             \
        decimal  : /-?[0-9]+\\.[0-9]+/ ;                                                                                                 \
        string  : /\"(\\\\.|[^\"])*\"/ ;                                                                                                 \
        value: <decimal> | <int> | <string> ;                                                                                            \
        operator : '+' | '-' | '*' | '/' | '%' | '^' | \"add\" | \"sub\" | \"mul\" | \"div\" | \"min\" | \"max\" ;                       \
        expr     : <value> | '(' <operator> <expr>+ ')' ;                                                                                \
        sexpr    : '(' <expr>* ')';                                                                                                      \
        pied    : /^/ <expr>* /$/ | /^/ <operator> <expr>+ /$/  | '(' <value> <operator> <value> ')'  | /^/ <operator> <value>+ ;        \
        ",
    Int, Decimal,String, Value, Operator, Expr, Sexpr, Pied);*/

    mpca_lang(MPCA_LANG_DEFAULT, 
        "                                                                                                                                \
        int   : /-?[0-9]+/ ;                                                                                                             \
        decimal  : /-?[0-9]+\\.[0-9]+/ ;                                                                                                 \
        symbol : \"list\" | \"head\" | \"tail\" | \"join\" | \"eval\" | '+' | '-' | '*' | '/' | '%';                       \
        sexpr    : '(' <expr>* ')';                                                                                                      \
        qexpr    : '{' <expr>* '}';                                                                                                      \
        expr     : <decimal> | <int> | <symbol> | <sexpr> | <qexpr>;                                                                                \
        pied    : /^/ <expr>* /$/ ;        \
        ",
    Int, Decimal, Symbol, Sexpr, Qexpr, Expr, Pied);

    while (1)
    {
        char* input = readline(">>> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>",input, Pied, &r)){
            pval* x = pval_eval(pval_read(r.output));
            pval_println(x);
            pval_del(x);
            //mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        free(input);
    }

    mpc_cleanup(6, Int, Decimal, Symbol, Sexpr, Qexpr, Expr, Pied);
    return 0;
}
