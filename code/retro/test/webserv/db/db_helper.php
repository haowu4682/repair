#!/usr/bin/php
<?php

/* php helper script to execute pg_* database functions */

require_once("json.php");

$resources = Array();
$pipein  = getenv("PIPE_IN") ? getenv("PIPE_IN") : "/tmp/retro/db_helper.in";
$pipeout = getenv("PIPE_OUT") ? getenv("PIPE_OUT") : "/tmp/retro/db_helper.out";

// request is of the form:
// pg_func json_serialized_args
function decode_req($reqstr) {
    global $resources;

    $req = explode(" ", $reqstr);
    $req[1] = json_deserialize($req[1], $resources);
    return $req;
}

function call_fn($reqstr) {
    $req = decode_req($reqstr);
    if (!$req) {
        return "php_helper error: invalid request $reqstr";
    }

    $fn = $req[0];
    $a  = $req[1];
    switch (count($a)) {
    case 0:
        return $fn();
        break;
    case 1:
        return $fn($a[0]);
        break;
    case 2:
        return $fn($a[0], $a[1]);
        break;
    case 3:
        return $fn($a[0], $a[1], $a[2]);
        break;
    case 4:
        return $fn($a[0], $a[1], $a[2], $a[3]);
        break;
    case 5:
        return $fn($a[0], $a[1], $a[2], $a[3], $a[4]);
        break;
    default:
        return "db_helper error: request has more than 5 args";
    }
}

function get_req() {
    global $pipein;
    $fp = fopen($pipein, "r");
    $req = fread($fp, 10240);
    fclose($fp);
    return trim($req);
}

function put_resp($resp) {
    global $pipeout;
    $fp = fopen($pipeout, "w");
    fwrite($fp, $resp);
    fclose($fp);
}

while (true) {
    global $resources;
    
    $req = get_req();
    echo "db_helper: got request: $req\n";
    $resp = json_serialize(call_fn($req), $resources);
    echo "db_helper: sending response: $resp\n";
    put_resp($resp . "\n");
}

?>