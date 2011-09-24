#ifndef _STATS_H_
#define _STATS_H_

/* This structure is used to abstract the different ways of handling
 * first pass statistics.
 */
typedef struct
{
    vpx_fixed_buf_t buf;
    int             pass;
    FILE           *file;
    char           *buf_ptr;
    size_t          buf_alloc_sz;
} stats_io_t;

int stats_open_file(stats_io_t *stats, const char *fpf, int pass);
int stats_open_mem(stats_io_t *stats, int pass);
void stats_close(stats_io_t *stats);
void stats_write(stats_io_t *stats, const void *pkt, size_t len);
vpx_fixed_buf_t stats_get(stats_io_t *stats);




#endif
