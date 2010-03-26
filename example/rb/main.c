#include <ctype.h>
#include <u/libu.h>

int facility = LOG_LOCAL0;

static void dumpbuf (const char *b, size_t blen);

static int test_full (int opts);
static int test_empty (int opts);
static int test_advance (int opts);
static int test_full_advance (int opts);

static int readc (u_rb_t *rb, char *pexpected);

static int writec (u_rb_t *rb, char c);
static int write1 (u_rb_t *rb) { return writec(rb, '1'); }
static int write2 (u_rb_t *rb) { return writec(rb, '2'); }
static int write3 (u_rb_t *rb) { return writec(rb, '3'); }
static const char *bp (const char *s) { return s; }

int main (void)
{
    int opts = U_RB_OPT_IMPL_MALLOC | U_RB_OPT_USE_CONTIGUOUS_MEM;

    con_err_if (test_full(opts));
    con_err_if (test_empty(opts));
    con_err_if (test_advance(opts));
    con_err_if (test_full_advance(opts));

    return 0;
err:
    return 1;
}

static int test_full (int opts)
{
    enum { SZ = 2 };
    u_rb_t *rb = NULL;

    con_err_if (u_rb_create(SZ, opts, &rb));
    con_err_if (write1(rb));
    con_err_if (write2(rb));
    con_err_if (write3(rb) == 0);   /* overflow, write3 must fail */

    return 0;
err:
    return 1;
}

static int test_empty (int opts)
{
    enum { SZ = 3 };
    u_rb_t *rb = NULL;

    con_err_if (u_rb_create(SZ, opts, &rb));
    con_err_if (readc(rb, NULL) == 0);   /* underflow, readc() must fail */

    return 0;
err:
    return 1;
}

static int test_advance (int opts)
{
    char c;
    enum { SZ = 3 };
    u_rb_t *rb = NULL;

    con_err_if (u_rb_create(SZ, opts, &rb));

    for (c = 0; c < 127; c++)
    {
        if (!isprint(c))
            continue;
        con_err_if (writec(rb, c));
        con_err_if (readc(rb, &c));
    }

    return 0;
err:
    return 1;
}

static int test_full_advance (int opts)
{
    int i;
    char c, prev, star = '*';
    enum { SZ = 3 };
    u_rb_t *rb = NULL;

    con_err_if (u_rb_create(SZ, opts, &rb));

    /* fill the rb */
    for (i = 0; i < SZ; i++)
        con_err_if (writec(rb, star));

    for (c = 32; c < 126; c++)
    {
        if (c < (SZ + 32))
            con_err_if (readc(rb, &star));
        else
        {
            prev = c - SZ; 
            con_err_if (readc(rb, &prev));
        }

        con_err_if (writec(rb, c));
    }

    return 0;
err:
    return 1;
}

static int writec (u_rb_t *rb, char c)
{
    ssize_t rc;
    u_con("writing \'%c\'", c);
    rc = u_rb_write(rb, &c, 1);
    return (rc != 1);
}

static int readc (u_rb_t *rb, char *pexpected)
{
    char c;
    ssize_t rc = u_rb_read(rb, &c, 1);

    if (rc != 1)
        return ~0;
    else if (pexpected && *pexpected != c)
    {
        u_con("expect \'%c\', got \'%c\'", *pexpected, c);
        return ~0;
    }

    u_con("read \'%c\'", pexpected ? *pexpected : '?');
    return 0;
}

static void dumpbuf (const char *b, size_t blen)
{
    size_t i;

    printf("(%p)[%zu]: ", b, blen);
    for (i = 0; i < blen; i++)
        printf("\'%c\' ", b[i]);
    printf("\n");

    return;
}
