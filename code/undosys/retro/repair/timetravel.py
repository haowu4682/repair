import psycopg2, psycopg2.extensions

def enable(db, table):
    ## create and drop triggers need this isolation level to work
    db.set_isolation_level(psycopg2.extensions.ISOLATION_LEVEL_AUTOCOMMIT)
    q = """CREATE TRIGGER timetravel BEFORE INSERT OR DELETE OR UPDATE \
           ON %s FOR EACH ROW EXECUTE PROCEDURE \
           timetravel (start_ts, end_ts, start_pid, end_pid) """ % table
    print 'enable timetravel query:', q
    db.cursor().execute(q)

def disable(db, table):
    db.set_isolation_level(psycopg2.extensions.ISOLATION_LEVEL_AUTOCOMMIT)
    q = """DROP TRIGGER timetravel ON %s""" % table
    print 'disable timetravel query:', q
    db.cursor().execute(q)
    
