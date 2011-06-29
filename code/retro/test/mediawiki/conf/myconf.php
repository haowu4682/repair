#! /usr/bin/php

<?php
$_SERVER["REQUEST_METHOD"]="POST";
$_SERVER["PHP_SELF"]="";
$_SERVER["SCRIPT_NAME"]="";

$_POST["Sitename"   ]="test";
$_POST["DBtype"     ]="mysql";
$_POST["DBname"     ]="wikidb";
$_POST["DBuser"     ]="wikiuser";
$_POST["DBpassword" ]="wikiuser";
$_POST["DBpassword2"]="wikiuser";
$_POST["SysopName"  ]="admin";
$_POST["SysopPass"  ]="admin1234";
$_POST["SysopPass2" ]="admin1234";

require_once( "./index.php" );
?>