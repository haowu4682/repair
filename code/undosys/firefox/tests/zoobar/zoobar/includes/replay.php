<?php

// XXX: both these functions have race conditions 
function mkdir_if_not_exists($dir) {
    if (!file_exists($dir)) {
        mkdir($dir, 0777, true);
    }
}

function unlink_if_exists($file) {
    if (file_exists($file)) {
        unlink($file);
    }
}

$debug_on = true;
function debug_log($msg) {
    global $debug_on;
    
    if (!$debug_on) {
        return;
    }
    
    $h = fopen("/tmp/debug_log", "a");
    fwrite($h, $msg . "\n");
    fclose($h);
}

$request_id = false;
$record = false;
$replay = false;

$origdbdir = DB_DIR . "zoobar";
$backupdir = DB_DIR . "backup";
$replaydir = DB_DIR . "replay";
mkdir_if_not_exists ($origdbdir);
mkdir_if_not_exists ($backupdir);
mkdir_if_not_exists ($replaydir);

function recordDBBeforeReq($table) {
    global $request_id, $record, $replay, $origdbdir, $backupdir, $replaydir;

    debug_log("recordDBBeforeReq: headers=" . print_r($_SERVER, true));
    
    // Don't do anything if X-REQ-ID header is not set
    if (!isset($_SERVER['HTTP_X-REQ-ID'])) {
        debug_log("recordDBBeforeReq: X-REQ-ID header not set");
        return false;
    }
    
    if ($request_id === false) {
        $request_id = $_SERVER['HTTP_X-REQ-ID'];
        $record = true;
        // XXX: should you send a different response id?
        // header("X-REQ-ID: $request_id"); 
    }

    debug_log("creating backup dir: $backupdir/$request_id");

    // Create backup dir if necessary
    $reqdbdir = "$backupdir/$request_id.before";
    if (!file_exists($reqdbdir)) {
        rename($origdbdir, $reqdbdir);
        mkdir_if_not_exists ($origdbdir);
        foreach (glob("$reqdbdir/*") as $f) {
            $base = basename($f);
            unlink_if_exists("$origdbdir/$base");
            link("$f", "$origdbdir/$base");
        }    
    }

    // Copy table if necessary
    if ($table) {
        debug_log("copying table: $table to $origdbdir");
        if (!file_exists("$reqdbdir/$table.copied") &&
            file_exists("$reqdbdir/$table")) {
            
            unlink_if_exists("$origdbdir/$table");
            copy("$reqdbdir/$table", "$origdbdir/$table");
            touch("$reqdbdir/$table.copied");
        }
    }
    
    return true;
}

function recordDBBeforeResp() {
    global $request_id, $record, $replay, $origdbdir, $backupdir, $replaydir;
    
    if (!$record) {
        debug_log("recordDBBeforeResp: not in recording mode");
        return;
    }
    
    $reqdbdir = "$backupdir/$request_id.after";
    debug_log("recordDBBeforeResp: recording after request: $reqdbdir");
    if (file_exists($reqdbdir)) {
        // Delete dir
        foreach (glob("$reqdbdir/*") as $f) {
            unlink_if_exists($f);
        }
        rmdir($reqdbdir);
    }
    mkdir_if_not_exists($reqdbdir);
    foreach (glob("$origdbdir/*") as $f) {
        $base = basename($f);
        link("$f", "$reqdbdir/$base");
    }
    
    $request_id = false; // Done with recording DB for this request
}

function restoreDB() {
    global $request_id, $record, $replay, $origdbdir, $backupdir, $replaydir;

    debug_log("restoreDB: headers=" . print_r($_SERVER, true));
    
    // Don't do anything if X-REPLAY header is not set
    if (!isset($_SERVER['HTTP_X-REPLAY'])) {
        return false;
    }
    
    /**
     * Don't do anything if the DB has already been restored. It is ok to use
     * $request_id for both backup and restore since only one of backup or
     * restore is going to happen in any session.
     **/
    if ($request_id !== false) {
        return true;
    }

    $request_id = $_SERVER['HTTP_X-REPLAY'];
    $replay = true;
    
    /**
     * If replay diverged, reuse old replay dir
     **/
    if (isset($_SERVER['HTTP_X-REPLAY-DIVERGED'])) {
        debug_log("restoreDB: replay diverged");
        return true;
    }
    
    /**
     * To restore a DB for replay, create a separate replay/req_id dir, where
     * req_id is the request id in X-REPLAY header to replay. Copy the files
     * from the backed up req_id dir into replay/req_id.
     **/
    $reqdbdir = "$replaydir/$request_id.before";
    debug_log("creating replay dir: $reqdbdir");
    if (file_exists($reqdbdir)) {
        // Delete dir
        foreach (glob("$reqdbdir/*") as $f) {
            unlink_if_exists($f);
        }
        rmdir($reqdbdir);
    }
    
    mkdir_if_not_exists ($reqdbdir);
    $origdir = "$backupdir/$request_id.before";
    foreach (glob("$origdir/*") as $f) {
        $fname = basename($f);
        copy($f, "$reqdbdir/$fname");
    }
    
    return true;
}

function testDBBeforeResp() {
    global $request_id, $record, $replay, $origdbdir, $backupdir, $replaydir;

    if (!$replay) {
        debug_log("testDBBeforeResp: replay flag is false");
        return;
    }
    
    header("X-REPLAY: $request_id");
    
    /**
     * If replay already diverged, just pass it along
     **/
    if (isset($_SERVER['HTTP_X-REPLAY-DIVERGED'])) {
        debug_log("testDBBeforeResp: replay already diverged. setting: $request_id");
        header("X-REPLAY-DIVERGED: $request_id");
        return;
    }    
    
    $dbdir1 = "$replaydir/$request_id.before";
    $dbdir2 = "$backupdir/$request_id.after";
    foreach (glob("$dbdir1/*.txt") as $f) {
        $fname = basename($f);
        
        // Do a diff and see whether the db changed
        // XXX: really you should do a diff of the HTTP response
        $data1 = file_get_contents($f);
        $data2 = file_get_contents("$dbdir2/$fname");
        if ($data1 != $data2) {
            debug_log("testDBBeforeResp: files $f and $dbdir2/$fname");
            debug_log("file1=\n%%$data1%%\n");
            debug_log("file2=\n%%$data2%%\n");
            header("X-REPLAY-DIVERGED: $request_id");
            break;
        }
    }
}

function getDBDir() {
    global $request_id, $record, $replay, $origdbdir, $backupdir, $replaydir;

    if (restoreDB()) {
        return "replay/$request_id.before";
    }
    
    if (recordDBBeforeReq(null)) {
        return "zoobar";
    }
    
    return "zoobar";
}

?>