/* *** Originally from postgres-source/contrib/spi -- Ramesh ***/
   
/* 
 * timetravel.c --      function to get time travel feature
 *                      using general triggers.
 */

/* Modified by B�JTHE Zolt�n, Hungary, mailto:urdesobt@axelero.hu */

#include "executor/spi.h"               /* this is what you need to work with SPI */
#include "commands/trigger.h"           /* -"- and triggers */
#include "miscadmin.h"                  /* for GetPgUserName() */
#include "utils/nabstime.h"
#include "utils/timestamp.h"            /* for 64-bit timestamps */
#include "catalog/pg_type.h"            /* for OIDs */

#include <ctype.h>                      /* tolower () */
#include <sys/types.h>
#include <unistd.h>

PG_MODULE_MAGIC;

Datum           timetravel(PG_FUNCTION_ARGS);
Datum           set_timetravel(PG_FUNCTION_ARGS);
Datum           get_timetravel(PG_FUNCTION_ARGS);

typedef struct {
    char       *ident;
    SPIPlanPtr splan;
} EPlan;

static EPlan *Plans = NULL;             /* for UPDATE/DELETE */
static int   nPlans = 0;

typedef struct _TTOffList {
    struct _TTOffList *next;
    char            name[1];
} TTOffList;

static TTOffList TTOff = {NULL, {0}};

static int   findTTStatus(char *name);
static EPlan *find_plan(char *ident, EPlan ** eplan, int *nplans);

/*
 * timetravel () --
 *              1.      IF an update affects tuple with stop_date eq INFINITY
 *                      then form (and return) new tuple with start_date eq current date
 *                      and stop_date eq INFINITY [ and update_user eq current user ]
 *                      and all other column values as in new tuple, and insert tuple
 *                      with old data and stop_date eq current date
 *                      ELSE - skip updation of tuple.
 *              2.      IF an delete affects tuple with stop_date eq INFINITY
 *                      then insert the same tuple with stop_date eq current date
 *                      [ and delete_user eq current user ]
 *                      ELSE - skip deletion of tuple.
 *              3.      On INSERT, if start_date is NULL then current date will be
 *                      inserted, if stop_date is NULL then INFINITY will be inserted.
 *                      [ and insert_user eq current user, update_user and delete_user
 *                      eq NULL ]
 *
 * In CREATE TRIGGER you are to specify start_date and stop_date column
 * names:
 * EXECUTE PROCEDURE
 * timetravel ('date_on', 'date_off' [,'insert_user', 'update_user', 'delete_user' ] ).
 */

#define MaxAttrNum      7
#define MinAttrNum      4

#define a_time_on       0
#define a_time_off      1
#define a_pid_on        2
#define a_pid_off       3
#define a_ins_user      4
#define a_upd_user      5
#define a_del_user      6

static
TimestampTz
d2tz(Datum d)
{
    return DatumGetTimestampTz(d);
}

static
Datum
tz2d(TimestampTz ts)
{
    return TimestampTzGetDatum(ts);
}

static
void
add_to_update_vals(int * idx, Datum * values, char * nulls, int * attrnums, 
                   Datum attrval, bool isnull, int attrnum)
{
    values[*idx]   = attrval;
    nulls[*idx]    = isnull ? 'n' : ' ';
    attrnums[*idx] = attrnum;
    (*idx)++;
}

static
bool
retro_rerun()
{
    return (access("/tmp/retro/retro_rerun", F_OK) == 0);
}

static
bool
read_tt_params_from_file(unsigned long long * ts, unsigned int * pid)
{
    FILE * fp;
    bool ret;
    
    fp = fopen("/tmp/retro/tt_params", "r");
    if (!fp) {
        return 0;
    }

    ret = (fscanf(fp, "%llu %u", ts, pid) == 2);
    fclose(fp);
    return ret;
}

static
Datum
ts_to_dt(unsigned long long ts)
{
    TimestampTz result;
    unsigned long long secs  = ts / USECS_PER_SEC;
    unsigned long long usecs = ts - (secs * USECS_PER_SEC);

    /* from GetCurrentTimestamp() */
    result = (TimestampTz) secs - ((POSTGRES_EPOCH_JDATE - UNIX_EPOCH_JDATE) * SECS_PER_DAY);
                
#ifdef HAVE_INT64_TIMESTAMP
    result = (result * USECS_PER_SEC) + usecs; 
#else
    result = result + ((unsigned long double) usecs / 1000000.0); 
#endif
    PG_RETURN_TIMESTAMPTZ (result);
}

static
void
get_tt_params(Datum *ts_dt, Datum *pid_dt) {
    if (retro_rerun()) {        
        unsigned long long ts;
        unsigned int pid;
        if (read_tt_params_from_file(&ts, &pid)) {
            *ts_dt  = ts_to_dt(ts);
            *pid_dt = Int32GetDatum(pid);
            return;
        }
    }

    *ts_dt = clock_timestamp(0);
    *pid_dt = Int32GetDatum(getpid());
}

PG_FUNCTION_INFO_V1(timetravel);

Datum                                   /* have to return HeapTuple to Executor */
timetravel(PG_FUNCTION_ARGS)
{
    TriggerData *trigdata = (TriggerData *) fcinfo->context;
    Trigger    *trigger;                /* to get trigger name */
    int        argc;
    char       **args;                  /* arguments */
    int        attnum[MaxAttrNum];      /* fnumbers of start/stop columns */
    Datum      oldtimeon, oldtimeoff;
    Datum      newtimeon, newtimeoff, newuser, nulltext;
    Datum      oldpidon, oldpidoff, newpidon, newpidoff;
    Datum      *cvals;                  /* column values */
    char       *cnulls;                 /* column nulls */
    char       *relname;                /* triggered relation name */
    Relation   rel;                     /* triggered relation */
    HeapTuple  trigtuple;
    HeapTuple  newtuple = NULL;
    HeapTuple  rettuple;
    TupleDesc  tupdesc;                 /* tuple description */
    int        natts;                   /* # of attributes */
    EPlan      *plan;                   /* prepared plan */
    char       ident[2 * NAMEDATALEN];
    bool       isnull;                  /* to know is some column NULL or not */
    bool       isinsert = false;
    int        ret;
    int        i;
    Datum      mypid;
    Datum      current_ts;

    get_tt_params(&current_ts, &mypid);

    /*
     * Some checks first...
     */

    /* Called by trigger manager ? */
    if (!CALLED_AS_TRIGGER(fcinfo))
        elog(ERROR, "timetravel: not fired by trigger manager");

    /* Should be called for ROW trigger */
    if (TRIGGER_FIRED_FOR_STATEMENT(trigdata->tg_event))
        elog(ERROR, "timetravel: cannot process STATEMENT events");

    /* Should be called BEFORE */
    if (TRIGGER_FIRED_AFTER(trigdata->tg_event))
        elog(ERROR, "timetravel: must be fired before event");

    /* INSERT ? */
    if (TRIGGER_FIRED_BY_INSERT(trigdata->tg_event))
        isinsert = true;

    if (TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event))
        newtuple = trigdata->tg_newtuple;

    trigtuple = trigdata->tg_trigtuple;

    rel = trigdata->tg_relation;
    relname = SPI_getrelname(rel);

    /* check if TT is OFF for this relation */
    if (0 == findTTStatus(relname)) {
        /* OFF - nothing to do */
        pfree(relname);
        return PointerGetDatum((newtuple != NULL) ? newtuple : trigtuple);
    }

    trigger = trigdata->tg_trigger;

    argc = trigger->tgnargs;
    if (argc != MinAttrNum && argc != MaxAttrNum)
        elog(ERROR, "timetravel (%s): invalid (!= %d or %d) number of arguments %d",
             relname, MinAttrNum, MaxAttrNum, trigger->tgnargs);

    args = trigger->tgargs;
    tupdesc = rel->rd_att;
    natts = tupdesc->natts;

    for (i = 0; i < MinAttrNum; i++) {
        attnum[i] = SPI_fnumber(tupdesc, args[i]);
        if (attnum[i] < 0)
            elog(ERROR, "timetravel (%s): there is no attribute %s", relname, args[i]);
        if (i <= a_time_off) {
            if (SPI_gettypeid(tupdesc, attnum[i]) != TIMESTAMPTZOID)
                elog(ERROR, "timetravel (%s): attribute %s must be of timestamptz type",
                     relname, args[i]);
        } else {
            if (SPI_gettypeid(tupdesc, attnum[i]) != INT4OID)
                elog(ERROR, "timetravel (%s): attribute %s must be of int type",
                     relname, args[i]);
        }
    }
    for (; i < argc; i++) {
        attnum[i] = SPI_fnumber(tupdesc, args[i]);
        if (attnum[i] < 0)
            elog(ERROR, "timetravel (%s): there is no attribute %s", relname, args[i]);
        if (SPI_gettypeid(tupdesc, attnum[i]) != TEXTOID)
            elog(ERROR, "timetravel (%s): attribute %s must be of text type",
                 relname, args[i]);
    }

    /* create fields containing name */
    newuser = DirectFunctionCall1(textin, CStringGetDatum(GetUserNameFromId(GetUserId())));

    nulltext = (Datum) NULL;

    if (isinsert) {                 /* INSERT */
        int     chnattrs = 0;
        int     chattrs[MaxAttrNum];
        Datum   newvals[MaxAttrNum];
        char    newnulls[MaxAttrNum];

        oldtimeon = SPI_getbinval(trigtuple, tupdesc, attnum[a_time_on], &isnull);
        if (isnull) {
            add_to_update_vals(&chnattrs, newvals, newnulls, chattrs,
                               current_ts, false, attnum[a_time_on]);
        }

        oldtimeoff = SPI_getbinval(trigtuple, tupdesc, attnum[a_time_off], &isnull);
        if (isnull) {
            if ((chnattrs == 0 && TIMESTAMP_IS_NOEND(d2tz(oldtimeon))) ||
                (chnattrs > 0 && TIMESTAMP_IS_NOEND(d2tz(newvals[a_time_on]))))
                elog(ERROR, "timetravel (%s): %s is infinity", relname, args[a_time_on]);
            add_to_update_vals(&chnattrs, newvals, newnulls, chattrs,
                               tz2d(DT_NOEND), false, attnum[a_time_off]);
        } else {
            if ((chnattrs == 0 && timestamp_cmp_internal(d2tz(oldtimeon), d2tz(oldtimeoff)) > 0) ||
                (chnattrs > 0 && timestamp_cmp_internal(d2tz(newvals[a_time_on]), d2tz(oldtimeoff)) > 0))
                elog(ERROR, "timetravel (%s): %s gt %s", relname, args[a_time_on], args[a_time_off]);
        }

        /* Check that pid values were not set by the client.
         *
         * Since this trigger does an INSERT as part of a DELETE
         * or UPDATE, which would result in a recursive call to this
         * trigger. Those INSERTs will already have pid values set.
         * Sanity check those pid values.
         *
         * If pid values were not set, set start_pid to mypid and
         * end_pid to -1. 
         */
        oldpidon = SPI_getbinval(trigtuple, tupdesc, attnum[a_pid_on], &isnull);
        if (isnull) {
            add_to_update_vals(&chnattrs, newvals, newnulls, chattrs,
                               mypid, false, attnum[a_pid_on]);
        } else {
            /* oldpidon should not be -ve */
            if (DatumGetInt32(oldpidon) < 0)
                elog(ERROR, "timetravel (%s): %s is negative", relname, args[a_pid_on]);
        }
        
        oldpidoff = SPI_getbinval(trigtuple, tupdesc, attnum[a_pid_off], &isnull);
        if (isnull) {
            /* oldpidon should be null */
            if (chnattrs == 0)
                elog(ERROR, "timetravel (%s): %s is not null when %s is null",
                     relname, args[a_pid_on], args[a_pid_off]);
            
            add_to_update_vals(&chnattrs, newvals, newnulls, chattrs,
                               Int32GetDatum(-1), false, attnum[a_pid_off]);
        } else {
            /* oldpidon cannot be null, and oldpidoff should be
               the same as mypid, as this INSERT resulted from a
               DELETE or UPDATE in the same trigger process */
            if (chnattrs > 0 || DatumGetInt32(oldpidoff) != DatumGetInt32(mypid))
                elog(ERROR, "timetravel (%s): %s is null or %s is not same as mypid",
                     relname, args[a_pid_on], args[a_pid_off]);
        }
        
        pfree(relname);
        if (chnattrs <= 0)
            return PointerGetDatum(trigtuple);

        if (argc == MaxAttrNum) {
            /* clear update_user value */
            add_to_update_vals(&chnattrs, newvals, newnulls, chattrs,
                               nulltext, true, attnum[a_upd_user]);
            /* clear delete_user value */
            add_to_update_vals(&chnattrs, newvals, newnulls, chattrs,
                               nulltext, true, attnum[a_del_user]);
            /* set insert_user value */
            add_to_update_vals(&chnattrs, newvals, newnulls, chattrs,
                               newuser, false, attnum[a_ins_user]);
        }
        rettuple = SPI_modifytuple(rel, trigtuple, chnattrs, chattrs, newvals, newnulls);
        return PointerGetDatum(rettuple);
        /* end of INSERT */
    }

    /* UPDATE/DELETE: */
    oldtimeon = SPI_getbinval(trigtuple, tupdesc, attnum[a_time_on], &isnull);
    if (isnull)
        elog(ERROR, "timetravel (%s): %s must be NOT NULL", relname, args[a_time_on]);

    oldtimeoff = SPI_getbinval(trigtuple, tupdesc, attnum[a_time_off], &isnull);
    if (isnull)
        elog(ERROR, "timetravel (%s): %s must be NOT NULL", relname, args[a_time_off]);

    oldpidon = SPI_getbinval(trigtuple, tupdesc, attnum[a_pid_on], &isnull);
    if (isnull)
        elog(ERROR, "timetravel (%s): %s must be NOT NULL", relname, args[a_pid_on]);

    oldpidoff = SPI_getbinval(trigtuple, tupdesc, attnum[a_pid_off], &isnull);
    if (isnull)
        elog(ERROR, "timetravel (%s): %s must be NOT NULL", relname, args[a_pid_off]);

    /*
     * If DELETE/UPDATE of tuple with stop_date neq INFINITY then say upper
     * Executor to skip operation for this tuple
     */
    if (newtuple != NULL) {             /* UPDATE */
        newtimeon = SPI_getbinval(newtuple, tupdesc, attnum[a_time_on], &isnull);
        if (isnull)
            elog(ERROR, "timetravel (%s): %s must be NOT NULL", relname, args[a_time_on]);

        newtimeoff = SPI_getbinval(newtuple, tupdesc, attnum[a_time_off], &isnull);
        if (isnull)
            elog(ERROR, "timetravel (%s): %s must be NOT NULL", relname, args[a_time_off]);

        if ((timestamp_cmp_internal(d2tz(oldtimeon), d2tz(newtimeon)) != 0) ||
            (timestamp_cmp_internal(d2tz(oldtimeoff), d2tz(newtimeoff)) != 0))
            elog(ERROR, "timetravel (%s): you cannot change %s and/or %s columns (use set_timetravel)",
                 relname, args[a_time_on], args[a_time_off]);

        newpidon = SPI_getbinval(newtuple, tupdesc, attnum[a_pid_on], &isnull);
        if (isnull)
            elog(ERROR, "timetravel (%s): %s must be NOT NULL", relname, args[a_pid_on]);

        newpidoff = SPI_getbinval(newtuple, tupdesc, attnum[a_pid_off], &isnull);
        if (isnull)
            elog(ERROR, "timetravel (%s): %s must be NOT NULL", relname, args[a_pid_off]);

        if ((DatumGetInt32(oldpidon) != DatumGetInt32(newpidon)) ||
            (DatumGetInt32(oldpidoff) != DatumGetInt32(newpidoff))) 
            elog(ERROR, "timetravel (%s): you cannot change %s and/or %s columns (use set_timetravel)",
                 relname, args[a_pid_on], args[a_pid_off]);
        
    }
    if (! TIMESTAMP_IS_NOEND(d2tz(oldtimeoff)) ||
        DatumGetInt32(oldpidoff) != -1) {  /* current record is a deleted/updated record */
        pfree(relname);
        return PointerGetDatum(NULL);
    }

    newtimeoff = current_ts;
    newpidoff = mypid;

    /* Connect to SPI manager */
    if ((ret = SPI_connect()) < 0)
        elog(ERROR, "timetravel (%s): SPI_connect returned %d", relname, ret);

    /* Fetch tuple values and nulls */
    cvals = (Datum *) palloc(natts * sizeof(Datum));
    cnulls = (char *) palloc(natts * sizeof(char));
    for (i = 0; i < natts; i++) {
        cvals[i] = SPI_getbinval(trigtuple, tupdesc, i + 1, &isnull);
        cnulls[i] = (isnull) ? 'n' : ' ';
    }

    /* change date column(s) */
    cvals[attnum[a_time_off] - 1] = newtimeoff; /* stop_date eq current date */
    cnulls[attnum[a_time_off] - 1] = ' ';

    /* set stop_pid of oldtuple to mypid */
    cvals[attnum[a_pid_off] - 1] = newpidoff;
    cnulls[attnum[a_pid_off] - 1] = ' ';

    if (!newtuple) {                    /* DELETE */
        if (argc == MaxAttrNum) {
            cvals[attnum[a_del_user] - 1] = newuser;  /* set delete user */
            cnulls[attnum[a_del_user] - 1] = ' ';
        }
    }

    /*
     * Construct ident string as TriggerName $ TriggeredRelationId and try to
     * find prepared execution plan.
     */
    snprintf(ident, sizeof(ident), "%s$%u", trigger->tgname, rel->rd_id);
    plan = find_plan(ident, &Plans, &nPlans);

    /* if there is no plan ... */
    if (plan->splan == NULL) {
        SPIPlanPtr  pplan;
        Oid         *ctypes;
        char        sql[8192];
        char        separ = ' ';

        /* allocate ctypes for preparation */
        ctypes = (Oid *) palloc(natts * sizeof(Oid));

        /*
         * Construct query: INSERT INTO _relation_ VALUES ($1, ...)
         */
        snprintf(sql, sizeof(sql), "INSERT INTO %s VALUES (", relname);
        for (i = 1; i <= natts; i++) {
            ctypes[i - 1] = SPI_gettypeid(tupdesc, i);
            if (!(tupdesc->attrs[i - 1]->attisdropped)) { /* skip dropped columns */
                snprintf(sql + strlen(sql), sizeof(sql) - strlen(sql), "%c$%d", separ, i);
                separ = ',';
            }
        }
        snprintf(sql + strlen(sql), sizeof(sql) - strlen(sql), ")");

        elog(DEBUG4, "timetravel (%s) update: sql: %s", relname, sql);

        /* Prepare plan for query */
        pplan = SPI_prepare(sql, natts, ctypes);
        if (pplan == NULL)
            elog(ERROR, "timetravel (%s): SPI_prepare returned %d", relname, SPI_result);

        /*
         * Remember that SPI_prepare places plan in current memory context -
         * so, we have to save plan in Top memory context for latter use.
         */
        pplan = SPI_saveplan(pplan);
        if (pplan == NULL)
            elog(ERROR, "timetravel (%s): SPI_saveplan returned %d", relname, SPI_result);

        plan->splan = pplan;
    }

    /*
     * Ok, execute prepared plan.
     */
    ret = SPI_execp(plan->splan, cvals, cnulls, 0);

    if (ret < 0)
        elog(ERROR, "timetravel (%s): SPI_execp returned %d", relname, ret);

    /* Tuple to return to upper Executor ... */
    if (newtuple) {                     /* UPDATE */
        int     chnattrs = 0;
        int     chattrs[MaxAttrNum];
        Datum   newvals[MaxAttrNum];
        char    newnulls[MaxAttrNum];

        add_to_update_vals(&chnattrs, newvals, newnulls, chattrs,
                           newtimeoff, false, attnum[a_time_on]);
        add_to_update_vals(&chnattrs, newvals, newnulls, chattrs,
                           tz2d(DT_NOEND), false, attnum[a_time_off]);

        /* set start_pid of newtuple to mypid */
        add_to_update_vals(&chnattrs, newvals, newnulls, chattrs,
                           newpidoff, false, attnum[a_pid_on]);
        add_to_update_vals(&chnattrs, newvals, newnulls, chattrs,
                           Int32GetDatum(-1), false, attnum[a_pid_off]);

        if (argc == MaxAttrNum) {
            /* set update_user value */
            add_to_update_vals(&chnattrs, newvals, newnulls, chattrs,
                               newuser, false, attnum[a_upd_user]);
            /* clear delete_user value */
            add_to_update_vals(&chnattrs, newvals, newnulls, chattrs,
                               nulltext, true, attnum[a_del_user]);
            /* set insert_user value */
            add_to_update_vals(&chnattrs, newvals, newnulls, chattrs,
                               nulltext, true, attnum[a_ins_user]);
        }

        rettuple = SPI_modifytuple(rel, newtuple, chnattrs, chattrs, newvals, newnulls);

        /*
         * SPI_copytuple allocates tmptuple in upper executor context - have
         * to free allocation using SPI_pfree
         */
        /* SPI_pfree(tmptuple); */
    } else {
        /* DELETE case */
        rettuple = trigtuple;
    }

    SPI_finish();                           /* don't forget say Bye to SPI mgr */

    pfree(relname);
    return PointerGetDatum(rettuple);
}

/*
 * set_timetravel (relname, on) --
 *      turn timetravel for specified relation ON/OFF
 */
PG_FUNCTION_INFO_V1(set_timetravel);

Datum
set_timetravel(PG_FUNCTION_ARGS)
{
    Name       relname = PG_GETARG_NAME(0);
    int32      on = PG_GETARG_INT32(1);
    char       *rname;
    char       *d;
    char       *s;
    int32      ret;
    TTOffList  *p, *pp;

    for (pp = (p = &TTOff)->next; pp; pp = (p = pp)->next) {
        if (namestrcmp(relname, pp->name) == 0)
            break;
    }

    if (pp) {
        /* OFF currently */
        if (on != 0) {
            /* turn ON */
            p->next = pp->next;
            free(pp);
        }
        ret = 0;
    } else {
        /* ON currently */
        if (on == 0) {
            /* turn OFF */
            s = rname = DatumGetCString(DirectFunctionCall1(nameout, NameGetDatum(relname)));
            if (s) {
                pp = malloc(sizeof(TTOffList) + strlen(rname));
                if (pp) {
                    pp->next = NULL;
                    p->next = pp;
                    d = pp->name;
                    while (*s)
                        *d++ = tolower((unsigned char) *s++);
                    *d = '\0';
                }
                pfree(rname);
            }
        }
        ret = 1;
    }

    PG_RETURN_INT32(ret);
}

/*
 * get_timetravel (relname) --
 *      get timetravel status for specified relation (ON/OFF)
 */
PG_FUNCTION_INFO_V1(get_timetravel);

Datum
get_timetravel(PG_FUNCTION_ARGS)
{
    Name       relname = PG_GETARG_NAME(0);
    TTOffList  *pp;

    for (pp = TTOff.next; pp; pp = pp->next) {
        if (namestrcmp(relname, pp->name) == 0)
            PG_RETURN_INT32(0);
    }
    PG_RETURN_INT32(1);
}

static int
findTTStatus(char *name)
{
    TTOffList  *pp;

    for (pp = TTOff.next; pp; pp = pp->next)
        if (pg_strcasecmp(name, pp->name) == 0)
            return 0;
    return 1;
}

static EPlan *
find_plan(char *ident, EPlan ** eplan, int *nplans)
{
    EPlan  *newp;
    int    i;

    if (*nplans > 0) {
        for (i = 0; i < *nplans; i++) {
            if (strcmp((*eplan)[i].ident, ident) == 0)
                break;
        }
        if (i != *nplans)
            return (*eplan + i);
        *eplan = (EPlan *) realloc(*eplan, (i + 1) * sizeof(EPlan));
        newp = *eplan + i;
    } else {
        newp = *eplan = (EPlan *) malloc(sizeof(EPlan));
        (*nplans) = i = 0;
    }

    newp->ident = (char *) malloc(strlen(ident) + 1);
    strcpy(newp->ident, ident);
    newp->splan = NULL;
    (*nplans)++;

    return (newp);
}
