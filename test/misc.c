#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <u/libu.h>

int test_suite_misc_register (u_test_t *t);

static int test_u_path_snprintf (u_test_case_t *tc);
static int test_u_strtok (u_test_case_t *tc);
static int test_u_atoi (u_test_case_t *tc);

#ifdef HAVE_SETITIMER
static int test_u_rdwr (u_test_case_t *tc);

struct itimerval itv;
size_t file_size, buf_size;

static void setitv(struct itimerval *pitv)
{
    memset(pitv, 0, sizeof(struct itimerval));

    pitv->it_value.tv_sec = 0;
    pitv->it_value.tv_usec = 1; /* use maximum granularity */
}

static void onsigalrm(int s)
{
    u_unused_args(s);
    setitv(&itv);
    con_if(setitimer(ITIMER_REAL, &itv, NULL) < 0);
}

static int cat_file(const char *fn, unsigned int *phash)
{
    ssize_t tot = 0, c = 0;
    unsigned int hash = 0;
    int fd, i;
    struct stat st;
    char *buf = NULL;

    con_err_sif(stat(fn, &st) < 0);

    buf = u_malloc(buf_size);
    con_err_if(buf == NULL);

    memset(buf, 0, buf_size);

    fd = open(fn, O_NONBLOCK | O_RDONLY);
    con_err_if(fd < 0);

    for(tot = 0; ; tot += c)
    {
        c = u_read(fd, buf, buf_size);

        con_err_sif(c < 0);

        for(i = 0; i < c; ++i)
            hash += buf[i]; /* idiot hash */

        dbg_ifb(c == 0)
            break; /* eof */
    }

    close(fd);

    *phash = hash;

    con_err_ifm(st.st_size != tot, "file size differs (%d != %d)", 
            st.st_size, tot);

    u_free(buf);

    return 0;
err:
    if(buf)
        u_free(buf);
    return 1;
}

static int gen_file(const char *fn, unsigned int *phash)
{
    int fd = -1;
    char *buf = NULL;
    unsigned int hash = 0;
    size_t i, c, size = file_size;

    buf = u_malloc(buf_size);
    con_err_if(buf == NULL);

    memset(buf, 0, buf_size);

    fd = open(fn, O_CREAT | O_WRONLY | O_NONBLOCK, 0600);
    con_err_sif(fd < 0);

    while(size)
    {
        c = (size < buf_size ? size : buf_size);

        for(i = 0; i < c; ++i)
            buf[i] = (char) i; /* just fill buf somehow */

        con_err_if(u_write(fd, buf, c) < 0);

        for(i = 0; i < c; ++i)
            hash += buf[i]; /* idiot hash */

        size -= c;
    }

    close(fd);
    u_free(buf);

    *phash = hash;

    return 0;
err:
    if(fd >= 0)
        close(fd);
    if(buf)
        u_free(buf);
    return ~0;
}

static int tmp_u_rdwr(int rd, const char *fn, unsigned int *phash)
{
    int read_rc, write_rc;

    /* set a fast timer to generate EINTRs */
    signal(SIGALRM, onsigalrm);
    setitv(&itv);
    con_err_if(setitimer(ITIMER_REAL, &itv, NULL) < 0);

    if(rd)
        read_rc = cat_file(fn, phash);
    else
        write_rc = gen_file(fn, phash);

    /* disable timers */
    signal(SIGALRM, SIG_IGN);
    memset(&itv, 0, sizeof(itv));
    setitimer(ITIMER_REAL, &itv, &itv);
    signal(SIGALRM, SIG_DFL);

    if(rd)
        con_err_if(read_rc);
    else
        con_err_if(write_rc);
 
    return 0;
err:
    return ~0;
}

static int test_u_rdwr (u_test_case_t *tc)
{
    char fn[U_FILENAME_MAX + 1] = { 0 };
    unsigned int hash_read, hash_write;
    int i;

    buf_size = 1;
    file_size = 1013;

    /* try with diferent buffer size and file size */
    for(i = 0; i < 10; ++i)
    {
        /* add 1 to avoid multiples of 2 */
        buf_size = (buf_size << 1) + 1;
        file_size = (file_size << 1) + 1;

        u_test_err_if(tmpnam(fn) == NULL);

        u_test_err_if(tmp_u_rdwr(0 /* write */, fn, &hash_write));

        u_test_err_if(tmp_u_rdwr(1 /* read */, fn, &hash_read));

        u_test_err_if(hash_read != hash_write);

        unlink(fn);
    }

    return U_TEST_SUCCESS;
err:
    u_test_case_printf(tc, "failed. file: %s file_size: %d, buf_size: %d", 
            fn, file_size, buf_size);
    return U_TEST_FAILURE;
}
#endif  /* HAVE_SETITIMER */

static int test_u_strtok (u_test_case_t *tc)
{
    char **tv = NULL;
    size_t nelems, i, j;
    enum { MAX_TOKENS = 100 };

    struct vt_s
    {
        const char *in; 
        const char *delim; 
        const char *ex[MAX_TOKENS];
    } vt[] = {
        { 
            /* tv idx 0 */
            "this . is , a : test ; string |", 
            " \t", 
            { 
                "this", 
                ".", 
                "is", 
                ",", 
                "a", 
                ":", 
                "test", 
                ";", 
                "string", 
                "|", 
                NULL 
            }
        },
        {
            /* tv idx 1 */
            "this . is , a : test ; string |", 
            ".",
            {
                "this ",
                " is , a : test ; string |",
                NULL 
            }
        },
        {
            /* tv idx 2 */
            "this . is , a : test ; string |", 
            ",",
            {
                "this . is ",
                " a : test ; string |",
                NULL 
            }
        },
        {
            /* tv idx 3 */
            "this .. is ,, a : test ; string |", 
            ",.:",
            {
                "this ",
                " is ",
                " a ",
                " test ; string |",
                NULL 
            }
        },
        {
            /* tv idx 4 */
            "is .. this ,, a : test ; string ||? |", 
            ",.:;|",
            {
                "is ",
                " this ",
                " a ",
                " test ",
                " string ",
                "? ",
                NULL 
            }
        },
        {
            /* tv idx 5 */
            "       is .. this ,, a : test ; string ||? |", 
            " ,.:;|",
            {
                "is",
                "this",
                "a",
                "test",
                "string",
                "?",
                NULL 
            }
        },
        {
            /* tv idx 6 */
            "       is .. this ,, a : test ; string ||? |", 
            "-",
            {
                "       is .. this ,, a : test ; string ||? |",
                NULL 
            }
        },
        {
            /* tv idx 7 (string containing separator chars only) */
            "|,,,  | ,",
            "|, ",
            { NULL }
        },
        {
            /* tv idx 8 (empty string) */
            "",
            "|, ",
            { NULL }
        },
        { 
             NULL, 
             NULL, 
             { NULL }
        }
    };

    for (i = 0; vt[i].in; i++)
    {
        u_test_err_ifm (u_strtok(vt[i].in, vt[i].delim, &tv, &nelems),
                "u_strtok failed on input: %s", vt[i].in);

        for (j = 0; j < nelems; j++)
        {
            u_test_err_ifm (strcmp(tv[j], vt[i].ex[j]), 
                    "%s != %s (tv idx=%zu)", tv[j], vt[i].ex[j], i);
        }

        u_test_err_ifm (vt[i].ex[j] != NULL, 
                "got %zu tokens from u_strtok, need some more (tv idx=%zu)", 
                nelems, i);

        u_strtok_cleanup(tv, nelems);
        tv = NULL;
    }

    return U_TEST_SUCCESS;
err:
    u_strtok_cleanup(tv, nelems);
    return U_TEST_FAILURE;
}

static int test_u_path_snprintf (u_test_case_t *tc)
{
    struct vt_s
    {
        const char *src, *ex;
    } vt[] = {
        { "",           "" },
        { "/",          "/" },
        { "//",         "/" },
        { "///",        "/" },
        { "a",          "a" },
        { "ab",         "ab" },
        { "abc",        "abc" },
        { "a/b",        "a/b" },
        { "/a",         "/a" },
        { "//a",        "/a" },
        { "///a",       "/a" },
        { "////a",      "/a" },
        { "/a//",       "/a/" },
        { "/a///",      "/a/" },
        { "/a////",     "/a/" },
        { "a//b",       "a/b" },
        { "a///b",      "a/b" },
        { "a////b",     "a/b" },
        { "a/b//c",     "a/b/c" },
        { "a/b//c/",    "a/b/c/" },
        { "a//b//c//",  "a/b/c/" },
        { NULL,         NULL }
    };
    int i;
    char buf[4096];

    for(i = 0; vt[i].src; ++i)
    {
        u_path_snprintf(buf, sizeof(buf), '/', "%s", vt[i].src);
        u_test_err_ifm(strcmp(buf, vt[i].ex), "src: %s  exp: %s  got: %s",
                vt[i].src, vt[i].ex, buf);
    }

    return U_TEST_SUCCESS;
err:
    return U_TEST_FAILURE;
}

static int test_u_atoi (u_test_case_t *tc)
{
    int i, j, rc, dummy = 0;

    struct
    {
        const char *in;
        int ex, rc;
    } vt[] = {
        /* minimum value for INT_MIN (16-bit) */
        {   "-32767",   -32767, 0   },  
        /* minimum value for INT_MAX (16-bit) */
        {   "32767",    32767,  0   },  

        /* check mixed numeric/non-numeric strings
         * man strtol(3): the string may begin with an arbitrary amount of 
         * white space (as determined by isspace(3)) followed by a single 
         * optional `+' or `-' sign. conversion stop at the first character 
         * which is not a valid base-10 digit */
        {   "123abc",   123,    0   },  /* stop at 'a' */
        {   "  +1+1",   1,      0   },  /* stop at the second '+' */
        {   "abc123",   dummy,  ~0  },  /* stop at 'a' */
        {   "1b2c3",    1,      0   },  /* stop at 'b' */
        {   "bongo",    dummy,  ~0  },  /* stop at 'b' */

        /* check underflows */
#if (INT_MIN < (-2147483647 - 1))       /* less than 32-bit */
        {   "-2147483648",  dummy,      ~0  },
#elif (INT_MIN == (-2147483647 - 1))    /* 32-bit */
        {   "-2147483648", -2147483648, 0   },
        {   "-2147483649",  dummy,      ~0  },
#else                                   /* more than 32-bit */
        {   "-2147483649", -2147483649, 0   },
#endif

        /* check overflows */
#if (INT_MAX < 2147483647)              /* less than 32-bit */
        {   "2147483647",   dummy,      ~0  },
#elif (INT_MAX == 2147483647)           /* 32-bit */
        {   "2147483647",   2147483647, 0   },
        {   "2147483648",   dummy,      ~0  },
#else                                   /* more than 32-bit */
        {   "2147483648",   2147483648, 0   },
#endif

        {   NULL,   0,  0   }
    };

    for (i = 0; vt[i].in; ++i)
    {
        rc = u_atoi(vt[i].in, &j);

        u_test_err_ifm (rc != vt[i].rc, 
                "unexpected return code %d != %d (tested conversion was: %s)",
                rc, vt[i].rc, vt[i].in);

        if (rc == 0)
        {
            u_test_err_ifm (j != vt[i].ex, 
                    "unexpected conversion value %d != %d on  %s",
                    j, vt[i].ex, vt[i].in);
        }
    }

    return U_TEST_SUCCESS;
err:
    return U_TEST_FAILURE;
}

int test_suite_misc_register (u_test_t *t)
{
    u_test_suite_t *ts = NULL;

    con_err_if (u_test_suite_new("Miscellaneous Utilities", &ts));

#ifdef HAVE_SETITIMER
    con_err_if (u_test_case_register("Various I/O routines", test_u_rdwr, ts));
#endif  /* HAVE_SETITIMER */

    con_err_if (u_test_case_register("u_path_snprintf function", 
                test_u_path_snprintf, ts));
    con_err_if (u_test_case_register("u_strtok function", test_u_strtok, ts));
    con_err_if (u_test_case_register("u_atoi function", test_u_atoi, ts));

    return u_test_suite_add(ts, t);
err:
    u_test_suite_free(ts);
    return ~0;
}
