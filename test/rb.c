#include <ctype.h>
#include <u/libu.h>

U_TEST_SUITE(rb);

#define RB_SZ   4096

static int rw (int fast);
static int test_rw (void);
static int test_rw_fast (void);

static int rw (int fast)
{
    enum { BUF_SZ = 1024 };
    int opts;
    u_rb_t *rb = NULL;
    size_t i, obuf_sz;
    char ibuf[BUF_SZ], obuf[BUF_SZ], *obuf_ptr, c;

    opts = fast ? U_RB_OPT_USE_CONTIGUOUS_MEM : U_RB_OPT_NONE;

    con_err_if (u_rb_create(RB_SZ, opts, &rb));

    c = '*';
    memset(ibuf, c, sizeof ibuf);

    /* 4 * 1024 write's => full */
    for (i = 0; i < 4; i++)
        con_err_if (u_rb_write(rb, ibuf, sizeof ibuf) != sizeof ibuf);

    /* now make offsets advance in the second region to test pairing */
    for (i = 0, c = 0; c < 127; c++)
    {
        if (!isprint(c))
            continue;

        /* consume 1024 bytes and test whether the read bytes match what was
         * written */
        if (fast)
        {
            obuf_sz = BUF_SZ;
            obuf_ptr = u_rb_fast_read(rb, &obuf_sz);
            con_err_if (obuf_ptr == NULL || obuf_sz != BUF_SZ);
        }
        else
        {
            con_err_if (u_rb_read(rb, obuf, sizeof obuf) != sizeof obuf);
            obuf_ptr = obuf;
        }

        /* u_rb_read is (4 * 1024) bytes behind */
        if (++i > 4)
            con_err_ifm (obuf_ptr[0] != (c - 4) || 
                    obuf_ptr[BUF_SZ - 1] != (c - 4), 
                    "expecting \'%c\', got \'%c\'", c - 4, obuf_ptr[0]);
        else
            con_err_ifm (obuf_ptr[0] != '*' || obuf_ptr[BUF_SZ - 1] != '*', 
                    "expecting \'%c\', got \'%c\'", '*', obuf_ptr[0]);

        /* refill */
        memset(ibuf, c, sizeof ibuf);
        con_err_if (u_rb_write(rb, ibuf, sizeof ibuf) != sizeof ibuf);
    }

    u_rb_free(rb);

    return U_TEST_EXIT_SUCCESS;
err:
    if (rb)
        u_rb_free(rb);
    return U_TEST_EXIT_FAILURE;
}

static int test_rw (void) { return rw(0); }
static int test_rw_fast (void) { return rw(1); }

U_TEST_SUITE( rb )
{
    U_TEST_CASE_ADD( test_rw );
    U_TEST_CASE_ADD( test_rw_fast );

    return 0;
}
