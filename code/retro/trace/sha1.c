#include <linux/crypto.h>
#include <linux/err.h>
#include <linux/scatterlist.h>
#include <linux/gfp.h>
#include <crypto/hash.h>

#include "dbg.h"
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

struct hmac_sha1_result {
	struct completion completion;
	int err;
};

static void hmac_sha1_complete( struct crypto_async_request * req, int err ) {
	struct hmac_sha1_result *r = req->data;
	if (err == -EINPROGRESS)
		return;
	r->err = err;
	complete(&r->completion);
}

/* ref. http://andrei.fcns.eu/2010/03/hmac-inside-the-linux-kernel */
int hmac_sha1( char * data_in , size_t dlen, char * hash_out ) {
	int rc=0;
	struct crypto_ahash *tfm;
	struct scatterlist sg;
	struct ahash_request *req;
	struct hmac_sha1_result tresult;
	void *hash_buf;
	char *hash_res = hash_out;
	int len = SHA1_SIZE;

	memset(hash_out, 0, SHA1_SIZE);

	init_completion(&tresult.completion);
	tfm = crypto_alloc_ahash("hmac(sha1)", 0, 0);
	if ( IS_ERR(tfm) ) {
		dbg(sha1, "%s", "hmac_sha1: crypto_alloc_ahash failed.");
		rc = PTR_ERR(tfm);
		goto err_tfm;
	}
	
	if ( !(req = ahash_request_alloc(tfm, GFP_KERNEL)) ) {
		dbg(sha1, "%s", "hmac_sha1: failed to allocate request for hmac(sha1)");
		rc = -ENOMEM;
		goto err_req;
	}
	
	if ( crypto_ahash_digestsize(tfm) > len ) {
		dbg(sha1, "%s", "hmac_sha1: tfm size > result buffer.");
		rc = -EINVAL;
		goto err_req;
	}
	
	ahash_request_set_callback( req, CRYPTO_TFM_REQ_MAY_BACKLOG,
				    hmac_sha1_complete, &tresult );

	if ( !(hash_buf = kzalloc(dlen, GFP_KERNEL)) ) {
		dbg(sha1, "%s", "hmac_sha1: failed to kzalloc hash_buf");
		rc = -ENOMEM;
		goto err_hash_buf;
	}
	
	memcpy(hash_buf, data_in, dlen);
	sg_init_one(&sg, hash_buf, dlen);

	crypto_ahash_clear_flags(tfm, -0);
	if ( (rc = crypto_ahash_setkey(tfm, RETRO_KEYS, RETRO_LKEY)) ) {
		dbg(sha1, "%s", "hmac_sha1: crypto_ahash_setkey failed");
		goto err_setkey;
	}
	ahash_request_set_crypt(req, &sg,hash_res, dlen);
	rc = crypto_ahash_digest(req);
	if (rc != 0) {
		rc = wait_for_completion_interruptible(&tresult.completion);
		if ( !rc && !(rc = tresult.err) ) {
			INIT_COMPLETION(tresult.completion);
		} else {
			dbg(sha1, "%s", "hmac_sha1: wait_for_completion_interruptible failed");
		}
	}
	
 err_setkey:
	kfree(hash_buf);
 err_hash_buf:
	ahash_request_free(req);
 err_req:
	crypto_free_ahash(tfm);
 err_tfm:
	return rc;
}