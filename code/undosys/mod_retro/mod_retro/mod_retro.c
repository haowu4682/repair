/*
 * mod_retro.c:
 *   Record HTTP requests/responses, modified from mod_dumpio.
 *
 */

/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Originally written @ Covalent by Jim Jagielski
 */

/*
 * mod_dumpio.c:
 *  Think of this as a filter sniffer for Apache 2.x. It logs
 *  all filter data right before and after it goes out on the
 *  wire (BUT right before SSL encoded or after SSL decoded).
 *  It can produce a *huge* amount of data.
 */


#include "httpd.h"
#include "http_connection.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"

module AP_MODULE_DECLARE_DATA retro_module ;

typedef struct retro_conf_t {
    int loglevel;
    int rerun;
} retro_conf_t;

/*
 * Workhorse function: simply log to the current error_log
 * info about the data in the bucket as well as the data itself
 */

/* XXX */
/* let's try to make it compatible with httpd for zoobar */
static void log_record(const char *type, const char *subtype, const char *data)
{
    int i;
    struct timeval tv;
    uint64_t ts;
    const char * outfile = "/tmp/httpdtest.dump";

    FILE *f = fopen(outfile, "a");
    if (!f)
        return;

    /* time stamp */
    gettimeofday(&tv, 0);
    ts = ((uint64_t)tv.tv_sec) * 1000000 + tv.tv_usec;

    fprintf(f, "%d %lld %s %s ", getpid(), ts, type, subtype);
    for (i = 0; i < (data ? strlen(data) : 0); i++) {
        if (isalnum(data[i]))
            fprintf(f, "%c", data[i]);
        else
            fprintf(f, "%%%02x", ((unsigned)data[i]));
    }
    fprintf(f, "\n");

    fclose(f);
}

/* XXX */
static void log_env(ap_filter_t *f, apr_bucket *b, retro_conf_t *conf)
{
    const char *buf;
    apr_size_t nbytes;

    if (conf->rerun || APR_BUCKET_IS_METADATA(b))
        return;

    /* data & recording */
    if (apr_bucket_read(b, &buf, &nbytes, APR_BLOCK_READ) == APR_SUCCESS) {
        if (nbytes >= 0) {
            int i;
            char *sp;
            char * obuf = malloc(nbytes+1);    /* use pool? */
            if (!obuf) {
                return;
            }
            memcpy(obuf, buf, nbytes);
            obuf[nbytes] = '\0';

            /* XXX. parse GET/POST */

            /* tokenizing */
            if (!(sp = strchr(obuf, ':'))) {
                free(obuf);
                return;
            }
            *sp = '\0';
            sp ++;

            /* skip one space */
            if (*sp != ' ') {
                free(obuf);
                return;
            }
            sp ++;

            /* assert `sp < buf + nbytes' */

            /* capitalized http header */
            for (i = 0 ; i < strlen(obuf) ; i ++) {
                obuf[i] = toupper(obuf[i]);
            }

            /* bite last \r\n */
            for (i = nbytes - 1 ; i >= 0 ; i --) {
                if (obuf[i] == '\r' || obuf[i] == '\n') {
                    obuf[i] = '\0';
                    continue;
                }
                break;
            }

            /* header */
            ap_log_error(APLOG_MARK, conf->loglevel, 0, f->c->base_server,
                         "mod_retro HTTP_%s: %s",
                         obuf,
                         sp);

            /* logging */
            {
                /* append HTTP_ */
                char subtype[128];
                snprintf(subtype, sizeof(subtype), "HTTP_%s", obuf);

                /* log */
                log_record("httpreq_env", subtype, sp);
            }

            free(obuf);
        }
    }
}

static void dumpit(ap_filter_t *f, apr_bucket *b)
{
    conn_rec *c = f->c;
    retro_conf_t *ptr =
    (retro_conf_t *) ap_get_module_config(c->base_server->module_config,
                                           &retro_module);

    ap_log_error(APLOG_MARK, ptr->loglevel, 0, c->base_server,
        "mod_retro:  %s (%s-%s): %" APR_SIZE_T_FMT " bytes",
                f->frec->name,
                (APR_BUCKET_IS_METADATA(b)) ? "metadata" : "data",
                b->type->name,
                b->length) ;

    if (!(APR_BUCKET_IS_METADATA(b))) {
        const char *buf;
        apr_size_t nbytes;
        char *obuf;
        if (apr_bucket_read(b, &buf, &nbytes, APR_BLOCK_READ) == APR_SUCCESS) {
            if (nbytes) {
                obuf = malloc(nbytes+1);    /* use pool? */
                memcpy(obuf, buf, nbytes);
#if APR_CHARSET_EBCDIC
                ap_xlate_proto_from_ascii(obuf, nbytes);
#endif
                obuf[nbytes] = '\0';
                ap_log_error(APLOG_MARK, ptr->loglevel, 0, c->base_server,
                     "mod_retro:  %s (%s-%s): %s",
                     f->frec->name,
                     (APR_BUCKET_IS_METADATA(b)) ? "metadata" : "data",
                     b->type->name,
                     obuf);
                free(obuf);
            }
        } else {
            ap_log_error(APLOG_MARK, ptr->loglevel, 0, c->base_server,
                 "mod_retro:  %s (%s-%s): %s",
                 f->frec->name,
                 (APR_BUCKET_IS_METADATA(b)) ? "metadata" : "data",
                 b->type->name,
                 "error reading data");
        }
    }
}

#define whichmode( mode ) \
 ( (( mode ) == AP_MODE_READBYTES) ? "readbytes" : \
   (( mode ) == AP_MODE_GETLINE) ? "getline" : \
   (( mode ) == AP_MODE_EATCRLF) ? "eatcrlf" : \
   (( mode ) == AP_MODE_SPECULATIVE) ? "speculative" : \
   (( mode ) == AP_MODE_EXHAUSTIVE) ? "exhaustive" : \
   (( mode ) == AP_MODE_INIT) ? "init" : "unknown" \
 )

static int retro_input_filter (ap_filter_t *f, apr_bucket_brigade *bb,
    ap_input_mode_t mode, apr_read_type_e block, apr_off_t readbytes)
{
    retro_conf_t *conf =
        (retro_conf_t *) ap_get_module_config(f->c->base_server->module_config,
                                           &retro_module);

    if (ap_get_brigade(f->next, bb, mode, block, readbytes) == APR_SUCCESS) {
        apr_bucket *b;
        for (b  = APR_BRIGADE_FIRST(bb);
             b != APR_BRIGADE_SENTINEL(bb);
             b  = APR_BUCKET_NEXT(b)) {
            log_env(f, b, conf);
        }
    }

    return APR_SUCCESS ;
}

static int retro_output_filter (ap_filter_t *f, apr_bucket_brigade *bb)
{
    apr_bucket *b;
    conn_rec *c = f->c;
    retro_conf_t *ptr =
    (retro_conf_t *) ap_get_module_config(c->base_server->module_config,
                                           &retro_module);

    ap_log_error(APLOG_MARK, ptr->loglevel, 0, c->base_server, ">> output filter");

    ap_log_error(APLOG_MARK, ptr->loglevel, 0, c->base_server, "mod_retro: %s", f->frec->name) ;

    for (b = APR_BRIGADE_FIRST(bb); b != APR_BRIGADE_SENTINEL(bb); b = APR_BUCKET_NEXT(b)) {
        /*
         * If we ever see an EOS, make sure to FLUSH.
         */
        if (APR_BUCKET_IS_EOS(b)) {
            apr_bucket *flush = apr_bucket_flush_create(f->c->bucket_alloc);
            APR_BUCKET_INSERT_BEFORE(b, flush);
        }
        dumpit(f, b);
    }

    ap_log_error(APLOG_MARK, ptr->loglevel, 0, c->base_server, "<< output filter");

    return ap_pass_brigade(f->next, bb) ;
}

static int retro_pre_conn(conn_rec *c, void *csd)
{
    retro_conf_t *ptr =
    (retro_conf_t *) ap_get_module_config(c->base_server->module_config,
                                           &retro_module);

    ap_log_error(APLOG_MARK, ptr->loglevel, 0, c->base_server, ">> reg");

    ap_add_input_filter("RETRO_IN", NULL, NULL, c);
    ap_add_output_filter("RETRO_OUT", NULL, NULL, c);
    return OK;
}

static void retro_register_hooks(apr_pool_t *p)
{
/*
 * We know that SSL is CONNECTION + 5
 */
  ap_register_output_filter("RETRO_OUT", retro_output_filter,
        NULL, AP_FTYPE_CONNECTION + 3) ;

  ap_register_input_filter("RETRO_IN", retro_input_filter,
        NULL, AP_FTYPE_CONNECTION + 3) ;

  ap_hook_pre_connection(retro_pre_conn, NULL, NULL, APR_HOOK_MIDDLE);
}

static void *retro_create_sconfig(apr_pool_t *p, server_rec *s)
{
    retro_conf_t *ptr = apr_pcalloc(p, sizeof *ptr);
    ptr->loglevel = APLOG_WARNING;
    ptr->rerun    = FALSE;
    return ptr;
}

module AP_MODULE_DECLARE_DATA retro_module = {
        STANDARD20_MODULE_STUFF,
        NULL,
        NULL,
        retro_create_sconfig,
        NULL,
        NULL,
        retro_register_hooks
};
