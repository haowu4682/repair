<?php
set_include_path(get_include_path() . PATH_SEPARATOR .
                 realpath(dirname(__FILE__) . '/../../db/'));
require_once('pg_wrapper.php');

function pg_log($str) {
    $f = fopen('/tmp/pg_db.log', 'a');
    fwrite($f, $str . "\n");
    fclose($f);
}

// ini_set('display_errors', 1);

class PG_ResultSet {
    var $rows;
    var $idx;
    
    function PG_ResultSet($result) {
        $this->rows = pg2_fetch_all($result);
        $this->idx = -1;
    }
    
    function getValueByNr($rowNr, $colNr) {
        assert ($rowNr >= 0 && $colNr >= 0);
        if ($rowNr >= count($this->rows)) {
            pg_log("rowNr out of bounds ($rowNr, $colNr)");
            throw new Exception('out of bounds row access');
        }

        foreach ($this->rows[$rowNr] as $k => $v) {
            if ($colNr == 0) {
                pg_log("ResultSet.getValueByNr: ($rowNr, $colNr): " . serialize($v));
                return $v;
            }
            $colNr--;
        }
        
        pg_log("colrNr out of bounds ($rowNr, $colNr)");
        return false;
    }

    function next() {
        $ret = false;
        if ($this->rows !== false) {
            $this->idx++;
            $ret = ($this->idx < count($this->rows));
        }
        pg_log('ResultSet.next: ' . serialize($ret));
        return $ret;
    }

    function getCurrentValueByname($colName) {
        $colName = strtolower($colName);
        $ret = false;
        if ($this->rows !== false) {
            $ret = $this->rows[$this->idx][$colName];
        }
        pg_log('ResultSet.getCurrentValueByName (' . $colName . ") : " . serialize($ret));
        return $ret;
    }

    function getCurrentValues() {
        $a = array();
        foreach ($this->rows[$this->idx] as $k => $v) {
            array_push($a, $v);
        }
        pg_log("ResultSet.getCurrentValues: " . serialize($a));
        return $a;
    }
}

class PG_Database {
    var $db;

    function PG_Database() {
        $this->db = pg2_connect(pg_connect_params());
        if ($this->db === false) {
            throw new Exception('cannot connect to postgres db: ' . $connectstr);
        }
    }
    
    function executeQuery($query_str) {
        pg_log($query_str);
        $res = pg2_query($this->db, $query_str);
        if ($res === false) {
            pg_log('error executing query: ' . $query_str);
            return false;
        }

        return new PG_ResultSet($res);
    }
};

?>