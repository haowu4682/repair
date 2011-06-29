#!/usr/bin/php
<?php

/* Simple script to test dbmgr. Runs and records a single db query */

require_once('pg_wrapper.php');
require_once('json.php');

function log_record($msg) {
    if (file_exists('/tmp/retro/retro_rerun')) {
        return;
    }
    $f = fopen("/tmp/retro/php.log", "a");
    fwrite($f, posix_getpid() . ":" . gettimestamp() . ":" .
               $msg . "\n");
    fclose($f);
}

$q = getenv('DB_QUERY');
log_record("call:" . json_serialize($q));

$db = pg2_connect(pg_connect_params());
$cur = pg2_query($db, $q);
$rows = pg2_fetch_all($cur);

log_record("ret:" . json_serialize($rows));

?>