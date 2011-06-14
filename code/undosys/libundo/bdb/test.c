#include <assert.h>
#include <db.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#ifndef USE_TRANSACTION
#define USE_TRANSACTION 0
#endif

struct entry
{
	const char *op;
	void (*handler)(int, char **, DB *);
};

static void usage()
{
	fprintf(stderr, "Berkeley DB tester\n");
	fprintf(stderr, "  read   <key>           shows the value for given key\n");
	fprintf(stderr, "  write  <key> <value>   writes key\n");
	fprintf(stderr, "  delete <key>           deletes key\n");
	exit(1);
}

static DB * do_open()
{
	int ret;
	DB_ENV *envp = 0;
	DB *dbp = 0;
	const char *filename = "test.db";
#if USE_TRANSACTION
	const char *dirname = "db";
	u_int32_t env_flags = DB_CREATE | DB_INIT_MPOOL
		| DB_INIT_TXN | DB_INIT_LOCK | DB_INIT_LOG;
	u_int32_t db_flags = DB_CREATE | DB_AUTO_COMMIT;
#else
	u_int32_t db_flags = DB_CREATE;
#endif

#if USE_TRANSACTION
	mkdir(dirname, 0777);
	ret = db_env_create(&envp, 0);
	assert(ret == 0);
	ret = envp->open(envp, dirname, env_flags, 0);
	assert(ret == 0);
#endif
	ret = db_create(&dbp, envp, 0);
	assert(ret == 0);
	ret = dbp->open(dbp, NULL, filename, NULL, DB_BTREE, db_flags, 0);
	assert(ret == 0);

	return dbp;
}

static void do_close(DB *dbp)
{
	dbp->close(dbp, 0);
	// envp is freed automatically
}

static void do_read(int argc, char **argv, DB *dbp)
{
	if (argc < 1)
		usage();
	char buf[BUFSIZ];
	DBT key, data;
	bzero(&key, sizeof(key));
	bzero(&data, sizeof(data));
	data.data = buf;
	data.ulen = BUFSIZ;
	data.flags = DB_DBT_USERMEM;
	for (int i = 0; i < argc; ++i)
	{
		key.data = argv[i];
		key.size = strlen(key.data);
		int ret = dbp->get(dbp, NULL, &key, &data, 0);
		if (ret)
			dbp->err(dbp, ret, "read %s", (char *)key.data);
		else
			printf("%s => %s\n", (char *)key.data, (char *)data.data);
	}
}

static void do_write(int argc, char **argv, DB *dbp)
{
	if (argc != 2)
		usage();
	DBT key, data;
	bzero(&key, sizeof(key));
	bzero(&data, sizeof(data));
	key.data = argv[0];
	key.size = strlen(key.data);
	data.data = argv[1];
	data.size = strlen(data.data) + 1;
	int ret = dbp->put(dbp, NULL, &key, &data, 0);
	if (ret)
		dbp->err(dbp, ret, "write %s => %s", (char *)key.data, (char *)data.data);
}

static void do_delete(int argc, char **argv, DB *dbp)
{
	if (argc < 1)
		usage();
	DBT key;
	bzero(&key, sizeof(key));
	for (int i = 0; i < argc; ++i)
	{
		key.data = argv[i];
		key.size = strlen(key.data);
		int ret = dbp->del(dbp, NULL, &key, 0);
		if (ret)
			dbp->err(dbp, ret, "delete %s", (char *)key.data);
	}
}

static struct entry entries[] = {
	{"read"  , do_read},
	{"write" , do_write},
	{"delete", do_delete},
	{0       , 0}
};

int main(int argc, char **argv)
{
	if (argc < 3)
		usage();

	struct entry *e = entries;
	for (; ; ++e)
	{
		if (e->op == 0)
			usage();
		if (strcmp(e->op, argv[1]) == 0)
			break;
	}

	DB *dbp = do_open();
	e->handler(argc - 2, argv + 2, dbp);
	do_close(dbp);
}
