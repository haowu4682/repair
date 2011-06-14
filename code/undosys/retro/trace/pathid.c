#include <asm/unistd.h>
#include <linux/kernel.h>
#include <linux/relay.h>
#include <linux/string.h>
#include "dbg.h"
#include "fs.h"
#include "pathid.h"
#include "sha1.h"
#include "util.h"

static char pathid_parent[NR_CPUS][PATHID_LEN];

void reset_pathid(void) {
  int cpu = smp_processor_id();
  memset(pathid_parent[cpu], 0, PATHID_LEN);
}

const char* last_pathid(void) {
  int cpu = smp_processor_id();
  return pathid_parent[cpu];
}

void store_pathid(const char* name, unsigned int nlen, struct inode* inode) {
#pragma pack(push)
#pragma pack(1)
	struct {
		char pathid[PATHID_LEN];
		char parent_pathid[PATHID_LEN];
		char buf[128];
		size_t len;  // len of data in buf
	} rec;
#pragma pack(pop)
	int cpu = smp_processor_id();
	memcpy(rec.parent_pathid, pathid_parent[cpu], PATHID_LEN);
	rec.len = varinode(inode, rec.buf);
	if (rec.len + nlen > sizeof(rec.buf)) {
		memcpy(rec.buf + rec.len, name, sizeof(rec.buf) - rec.len);
		rec.len = sizeof(rec.buf);
	} else {
		memcpy(rec.buf + rec.len, name, nlen);
		rec.len += nlen;
	}

	if(hmac_sha1(rec.parent_pathid, PATHID_LEN + rec.len, rec.pathid)) {
		memset(rec.pathid, 0, PATHID_LEN);
		err("failed to compute sha1");
	}
	__relay_write(pathidtable_chan, &rec, sizeof(rec));
	memcpy(pathid_parent[cpu], rec.pathid, PATHID_LEN);
}
