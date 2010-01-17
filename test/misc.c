#include <string.h>
#include <syslog.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <u/libu.h>

struct itimerval itv;
size_t file_size, buf_size;

U_TEST_SUITE(misc);

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
            buf[i] = i; /* just fill buf somehow */

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

static int test_u_rdwr(void)
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

        con_err_if(tmpnam(fn) == NULL);

        con_err_if(tmp_u_rdwr(0 /* write */, fn, &hash_write));

        con_err_if(tmp_u_rdwr(1 /* read */, fn, &hash_read));

        con_err_if(hash_read != hash_write);

        unlink(fn);
    }

    return 0;
err:
    u_con("failed. file: %s file_size: %d, buf_size: %d", fn, 
            file_size, buf_size);
    return 1;
}

static int test_u_strtok (void)
{
    char **tv = NULL;
    size_t nelems, i, j;
    enum { MAX_TOKENS = 100 };

    struct vt_s
    {
        const char *in; 
        const char *delim; 
        const char *exp[MAX_TOKENS];
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
        dbg_err_if (u_strtok(vt[i].in, vt[i].delim, &tv, &nelems));

        for (j = 0; j < nelems; j++)
        {
            con_err_ifm (strcmp(tv[j], vt[i].exp[j]), 
                    "%s != %s (tv idx=%zu)", tv[j], vt[i].exp[j], i);
        }

        con_err_ifm (vt[i].exp[j] != NULL, 
                "got %zu tokens from u_strtok, need some more (tv idx=%zu)", 
                nelems, i);

        u_strtok_cleanup(tv, nelems);
        tv = NULL;
    }

    return 0;
err:
    u_strtok_cleanup(tv, nelems);
    return ~0;
}

static int test_u_path_snprintf(void)
{
    struct vt_s
    {
        const char *src, *exp;
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
        con_err_ifm(strcmp(buf, vt[i].exp), "src: %s  exp: %s  got: %s",
                vt[i].src, vt[i].exp, buf);
    }

    return 0;
err:
    return ~0;
}

U_TEST_SUITE(misc)
{
    U_TEST_CASE_ADD( test_u_rdwr );
    U_TEST_CASE_ADD( test_u_path_snprintf );
    U_TEST_CASE_ADD( test_u_strtok );

    return 0;
}
