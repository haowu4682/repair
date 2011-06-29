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

void store_pathid(const char* name, size_t nlen, struct inode* inode) {
	struct pathid_record rec;
    int cpu, size_to_write;

	cpu = smp_processor_id();
	memcpy(rec.parent_pathid, pathid_parent[cpu], PATHID_LEN);
	rec.len = varinode(inode, rec.buf);
    size_to_write = min(nlen, sizeof(rec.buf) - rec.len);
	memcpy(rec.buf + rec.len, name, size_to_write);
    rec.len += size_to_write;

	if(hmac_sha1(rec.parent_pathid, PATHID_LEN + rec.len, rec.pathid)) {
		memset(rec.pathid, 0, PATHID_LEN);
		err("failed to compute sha1");
	}
	__relay_write(pathidtable_chan, &rec, sizeof(rec));
	memcpy(pathid_parent[cpu], rec.pathid, PATHID_LEN);
}
