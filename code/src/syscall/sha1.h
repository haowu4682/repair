#ifndef __SYSCALL_SHA1_H__
#define __SYSCALL_SHA1_H__

#define RETRO_KEYS ("retro")
#define RETRO_LKEY (5)
#define SHA1_SIZE  (20)

extern
int hmac_sha1(char *data_in , size_t dlen, unsigned char *hash_out );

#endif // __SYSCALL_SHA1_H__

