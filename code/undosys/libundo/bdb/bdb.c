#include <assert.h>
#include <db.h>
#include <dlfcn.h>
#include <resolv.h>
#include "undocall.h"

static int db_fd(DB *dbp)
{
	int fd;
	int ret = dbp->fd(dbp, &fd);
	assert(ret == 0);
	return fd;
}

static void db_depend(int fd, DBT *key, int proc_to_obj, int obj_to_proc)
{
	size_t namesize = key->size * 2;
	char name[namesize];
	b64_ntop(key->data, key->size, name, namesize);
	undo_depend(fd, name, "bdb_mgr", proc_to_obj, obj_to_proc);
}

UNDO_WRAP(__db_get_pp, int, (DB *dbp, DB_TXN *txn, DBT *key, DBT *data, u_int32_t flags))
{
	int fd = db_fd(dbp);
	undo_mask_start(fd);
	db_depend(fd, key, 0, 1);
	int ret = UNDO_ORIG(__db_get_pp)(dbp, txn, key, data, flags);
	undo_mask_end(fd);
	return ret;
}

UNDO_WRAP(__db_put_pp, int, (DB *dbp, DB_TXN *txn, DBT *key, DBT *data, u_int32_t flags))
{
	int fd = db_fd(dbp);
	undo_mask_start(fd);
	db_depend(fd, key, 1, 0);
	int ret = UNDO_ORIG(__db_put_pp)(dbp, txn, key, data, flags);
	undo_mask_end(fd);
	return ret;
}

UNDO_WRAP(__db_del_pp, int, (DB *dbp, DB_TXN *txn, DBT *key, u_int32_t flags))
{
	int fd = db_fd(dbp);
	undo_mask_start(fd);
	db_depend(fd, key, 1, 0);
	int ret = UNDO_ORIG(__db_del_pp)(dbp, txn, key, flags);
	undo_mask_end(fd);
	return ret;
}

UNDO_WRAP(__db_exists, int, (DB *dbp, DB_TXN *txn, DBT *key, u_int32_t flags))
{
	int fd = db_fd(dbp);
	undo_mask_start(fd);
	db_depend(fd, key, 0, 1);
	int ret = UNDO_ORIG(__db_exists)(dbp, txn, key, flags);
	undo_mask_end(fd);
	return ret;
}
