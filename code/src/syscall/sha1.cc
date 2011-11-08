// Author: Hao Wu <haowu@cs.utexas.edu>

#include <openssl/sha.h>

#include <common/common.h>
#include "sha1.h"

//
// verify this in python
// h = hmac.new("retro", "checkbuffer", hashlib.sha1)
// h.hexdigest()
// => "17f920e14ab76541da56c622a59c1e89a6c54309"
//
// in kernel, (assert len(hmac) == SHA1_SIZE == 40)
//   hmac_sha1("checkbuffer", strlen("checkbuffer"), hmac);
// 

int hmac_sha1(char *data_in , size_t dlen, unsigned char *hash_out) {
    // XXX: We won't need sha1 in the future probably
#if 0
    SHA1((unsigned char *)data_in, dlen, hash_out);
#endif
    return 0;
}
