#include <ctype.h>
#include <u/libu.h>

U_TEST_SUITE(rb);

#define RB_SZ   4096

static int test_rw_simple (void);

static int test_rw_simple (void)
{
    u_rb_t *rb = NULL;
    size_t i;
    char ibuf[1024], obuf[1024], c;

    con_err_if (u_rb_create(RB_SZ, &rb));

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
        con_err_if (u_rb_read(rb, obuf, sizeof obuf) != sizeof obuf);

        /* u_rb_read is (4 * 1024) bytes behind */
        if (++i > 4)
            con_err_ifm (obuf[0] != (c - 4) || obuf[1023] != (c - 4), 
                    "expecting \'%c\', got \'%c\'", c - 4, obuf[0]);
        else
            con_err_ifm (obuf[0] != '*' || obuf[1023] != '*', 
                    "expecting \'%c\', got \'%c\'", '*', obuf[0]);

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

U_TEST_SUITE( rb )
{
    U_TEST_CASE_ADD( test_rw_simple );

    return 0;
}
