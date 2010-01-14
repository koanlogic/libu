#include <u/libu.h>

int facility = LOG_LOCAL0;

#define RB_SZ   1024

int main (int ac, char *av[])
{
    u_rb_t *rb = NULL;
    size_t rb_sz, i;
    ssize_t io_rc;
    char ibuf[1024], obuf[512], c;

    /* create the ring buffer */
    con_err_if (u_rb_create(RB_SZ, &rb));

    con("rb created at %p with size %zu", rb, (rb_sz = u_rb_size(rb)));
    con("bytes available: %zu", u_rb_avail(rb));
    con("bytes ready to be consumed: %zu", u_rb_ready(rb));

    c = 'A';
    memset(ibuf, c, sizeof ibuf);

    while (u_rb_avail(rb))
    {
        io_rc = u_rb_write(rb, ibuf, sizeof ibuf);
        con("write %zu '%c' to %p.  ret val: %d", sizeof ibuf, c, rb, io_rc);
    }

    while (u_rb_ready(rb))
    {
        io_rc = u_rb_read(rb, obuf, sizeof obuf); 
        con("read %zu bytes from %p.", io_rc, rb, io_rc);
        con_err_if (memcmp(obuf, ibuf, sizeof obuf));
    }

    u_rb_free(rb);

    return 0;
err:
    if (rb)
        u_rb_free(rb);
    return 1;
}
