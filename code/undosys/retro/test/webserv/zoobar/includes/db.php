<?php

// txt-db-api library: http://www.c-worker.ch/txtdbapi/index_eng.php
if (file_exists('/tmp/retro/use_postgres')) {
    require_once('pgsql/txt_db_to_pg.php');
    $db = new PG_Database();
} else {
    require_once("txt-db-api/txt-db-api.php");
    $db = new Database("zoobar");
}

?>
