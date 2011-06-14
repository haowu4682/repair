#!/usr/bin/php
<?php 
    set_include_path(get_include_path() . PATH_SEPARATOR .
		     dirname($_SERVER['PHP_SELF']));
    require_once("txt-db-api/txt-db-api.php");

    $db_dir = getenv('DB_DIR');
    $q = getenv('DB_QUERY');
    putenv('RETRO_LOG_FILE=/dev/null');

    $db = new Database();
    $db->dbFolder = $db_dir;
    $rs = $db->executeQuery(rawurldecode($q));
    $data = serialize($rs);
    echo rawurlencode($data);
?>
