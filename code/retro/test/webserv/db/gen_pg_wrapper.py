#!/usr/bin/python

## php postgres fns we support (based on the fns used by
## mediawiki and zoobar). pg_ignored_fns.txt contains the
## list of fns we don't support yet.
pg_funcs     = ['pg_affected_rows',
                'pg_close',
                'pg_connect',
                'pg_escape_bytea',
                'pg_escape_string',
                'pg_fetch_all',
                'pg_fetch_array',
                'pg_fetch_object',
                'pg_fetch_result',
                'pg_field_name',
                'pg_field_type',
                'pg_free_result',
                'pg_last_error',
                'pg_num_fields',
                'pg_num_rows',
                'pg_parameter_status',
                'pg_query',
                'pg_result_seek',
                'pg_unescape_bytea',
                'pg_version']

pg_query_fns = ['pg_query']

header = '''
require_once("json.php");

$fid = 0;
function get_fid() {
    global $fid;
    return $fid++;
}

function pg_connect_params() {
    return file_get_contents(realpath(dirname(__FILE__)) . "/connect_params");
}

function gettimestamp() {
    $t = gettimeofday();
    return $t["sec"] * 1000000 + $t["usec"];
}

function pg2_log($str) {
    $f = fopen("/tmp/retro/db.log", "a");
    if (flock($f, LOCK_EX)) {
        fwrite($f, posix_getpid() . ":" .
                   gettimestamp() . ":" .
                   $str . "\\n");
        flock($f, LOCK_UN);
    } else {
        throw new Exception("error locking /tmp/retro/db.log");
    }
    fclose($f);
}

// XXX: this only matches simple select queries and does not
// deal with multiple tables in join queries
function rewrite_select($query) {
    $query = trim($query);
    if (stripos($query, "SELECT") === 0) {
        $query = str_ireplace("WHERE ", "WHERE end_ts='infinity' AND ", $query);
    }

    return $query;
}

function retro_redo($args) {
    echo "pg_wrapper: calling: $args";
    $f = fopen(getenv("RETRO_LOG_FILE"), "w");
    fwrite($f, $args);
    fclose($f);

    $f = fopen(getenv("RETRO_GET_FILE"), "r");
    $ret = fread($f, 10240);
    fclose($f);
    echo "pg_wrapper: returning: $ret\\n";
    return $ret;
}

'''

fn_template = '''
function __FUNC2__() {
    $my_fid = get_fid();

    // log the func call
    $args = Array();
    for ($i = 0; $i < func_num_args(); $i++) {
        $arg = func_get_arg($i);
        array_push($args, $arg);
    }

    if (file_exists("/tmp/retro/retro_rerun")) {
        $ret = retro_redo("__FUNC__:$my_fid:" . json_serialize($args) . "\\n");
        $ret = json_deserialize($ret);
    } else {
        pg2_log("__FUNC__:$my_fid:call:" . json_serialize($args));

        // call function
        __CALL_FUNC__

        // record return value
        pg2_log("__FUNC__:$my_fid:ret:" . json_serialize($ret));
    }

    return $ret;
}
'''

fn_call_template = '''
    switch (func_num_args()) {
        case 0:
            $ret = __FUNC__();
            break;
        case 1:
            $arg0 = func_get_arg(0);
            $ret  = __FUNC__($arg0);
            break;
        case 2:
            $arg0 = func_get_arg(0);
            $arg1 = func_get_arg(1);
            $ret = __FUNC__($arg0, $arg1);
            break;
        case 3:
            $arg0 = func_get_arg(0);
            $arg1 = func_get_arg(1);
            $arg2 = func_get_arg(2);
            $ret = __FUNC__($arg0, $arg1, $arg2);
            break;
        case 4:
            $arg0 = func_get_arg(0);
            $arg1 = func_get_arg(1);
            $arg2 = func_get_arg(2);
            $arg3 = func_get_arg(3);
            $ret = __FUNC__($arg0, $arg1, $arg2, $arg3);
            break;
        case 5:
            $arg0 = func_get_arg(0);
            $arg1 = func_get_arg(1);
            $arg2 = func_get_arg(2);
            $arg3 = func_get_arg(3);
            $arg4 = func_get_arg(4);
            $ret = __FUNC__($arg0, $arg1, $arg2, $arg3, $arg4);
            break;
        default:
            throw new Exception("function __FUNC__ has too many args: " . func_num_args());
    }
'''

query_fn_call_template = '''
    if (func_num_args() == 1) {
        pg2_log("__FUNC__:$my_fid:warn: no connection arg, so cannot fetch server pid");
        $query = func_get_arg(0);
        $ret = __FUNC__(rewrite_select($query));
    } else {
        $conn = func_get_arg(0);
        $server_pid = pg_get_pid($conn);
        pg2_log("__FUNC__:$my_fid:serverpid:$server_pid");
        $query = func_get_arg(1);
        $ret = __FUNC__($conn, rewrite_select($query));
    }
'''

## main code
import sys
def main():
    if len(sys.argv) != 2:
        print "Usage: %s outfile" % sys.argv[0]
        exit(1)

    fp = open(sys.argv[1], 'w')
    fp.write('<?php\n')
    fp.write(header)
    
    for f in pg_funcs:
        f2 = f.replace('pg_', 'pg2_')
        s = fn_template.replace('__FUNC__', f).replace('__FUNC2__', f2)
        if f in pg_query_fns:
            s2 = query_fn_call_template.replace('__FUNC__', f)
        else:
            s2 = fn_call_template.replace('__FUNC__', f)
        fp.write(s.replace('__CALL_FUNC__', s2))

    fp.write('?>\n')
        
    fp.close()
    
if __name__ == '__main__':
    main()
