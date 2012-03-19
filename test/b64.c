#include <u/libu.h>

int test_suite_b64_register (u_test_t *t);

static int test_b64_codec (u_test_case_t *tc)
{
    struct 
    {
        const uint8_t *bin;
        size_t bin_sz;
        const char *str;
        size_t str_sz;
    } vt[] = {
        {
            .bin = (const uint8_t *) "f",
            .bin_sz = strlen("f"),
            .str = "Zg==",
            .str_sz = strlen("Zg==")
        },
        {
            .bin = (const uint8_t *) "fo",
            .bin_sz = strlen("fo"),
            .str = "Zm8=",
            .str_sz = strlen("Zm8=")
        },
        {
            .bin = (const uint8_t *) "foo",
            .bin_sz = strlen("foo"),
            .str = "Zm9v",
            .str_sz = strlen("Zm9v")
        },
        {
            .bin = (const uint8_t *) "foob",
            .bin_sz = strlen("foob"),
            .str = "Zm9vYg==",
            .str_sz = strlen("Zm9vYg==")
        },
        {
            .bin = (const uint8_t *) "fooba",
            .bin_sz = strlen("fooba"),
            .str = "Zm9vYmE=",
            .str_sz = strlen("Zm9vYmE=")
        },
        {
            .bin = (const uint8_t *) "foobar",
            .bin_sz = strlen("foobar"),
            .str = "Zm9vYmFy",
            .str_sz = strlen("Zm9vYmFy")
        },
        { NULL, 0, NULL, 0 }
    };

    int i;

    for (i = 0; vt[i].bin; i++)
    {
        char s[U_B64_LENGTH(vt[i].bin_sz) + 1]; /* +1 for '\0' */
        uint8_t b[vt[i].bin_sz];
        size_t b_sz = vt[i].bin_sz;

        u_test_err_ifm (u_b64_encode(vt[i].bin, vt[i].bin_sz, s, sizeof s),
                "error encoding test vector %zu", i);

        u_test_err_ifm (strcasecmp(s, vt[i].str),
                "expecting %s, got %s", vt[i].str, s);

        u_test_err_ifm (u_b64_decode(s, strlen(s), b, &b_sz),
                "error decoding test vector %zu", i);

        u_test_err_ifm (vt[i].bin_sz != b_sz ||
                memcmp(b, vt[i].bin, vt[i].bin_sz),
                "run \'gdb --args runtest -s\' and b somewhere before %s:%s",
                __FILE__, __LINE__);
    }

    return U_TEST_SUCCESS;
err:
    return U_TEST_FAILURE;
}

int test_suite_b64_register (u_test_t *t)
{
    u_test_suite_t *ts = NULL;

    con_err_if (u_test_suite_new("Base64", &ts));

    con_err_if (u_test_case_register("Base64 codec", test_b64_codec, ts));

    return u_test_suite_add(ts, t);
err:
    u_test_suite_free(ts);
    return ~0;
}
