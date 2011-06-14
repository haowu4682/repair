#pragma once

#include <sys/stat.h>
#include <sys/types.h>
#include <err.h>
#include <fcntl.h>

static inline int _creat(const char *filename, mode_t mode)
{
	int fd = creat(filename, mode);
        if (fd < 0)
		err(1, "creat `%s'", filename);
	return fd;
}

#if HAVE_ZLIB_H

#include <zlib.h>
typedef gzFile zfile_t;
static inline zfile_t zcreat(const char *filename, mode_t mode)
{
	return gzdopen(_creat(filename, mode), "wb");
}
#define zwrite gzwrite
#define zclose gzclose

#elif HAVE_BZLIB_H

#include <bzlib.h>
typedef BZFILE * zfile_t;
static inline zfile_t zcreat(const char *filename, mode_t mode)
{
	return BZ2_bzdopen(_creat(filename, mode), "wb");
}
#define zwrite BZ2_bzwrite
#define zclose BZ2_bzclose

#else

typedef int zfile_t;
#define zcreat _creat
#define zwrite write
#define zclose close

#endif
