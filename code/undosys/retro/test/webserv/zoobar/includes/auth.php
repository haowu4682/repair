<?php

// Cookie-based authentication logic

class User {
  var $db = null;
  var $username = null;
  var $cookieName = "ZoobarLogin";

  function stderr($msg) {
      $f = fopen('php://stderr', 'w+');
      fwrite($f, $msg . "\n");
      fclose($f);
  }

  function log_rerun() {
      return (getenv('RETRO_RERUN') !== false);
  }

  function log_record($type, $subtype, $data) {
      $t = gettimeofday();
      $ts = $t['sec'] * 1000000 + $t['usec'];
      $logpath = "/tmp/retro/httpd.log";
      #$this->stderr('about to dump record to logfile ' . $logpath);
      $f = fopen($logpath, "a");
      $rec = posix_getpid() . " " . $ts . " " . $type . " " . $subtype . " " . rawurlencode($data) . "\n";
      fwrite($f, $rec, strlen($rec));
      fclose($f);
  }
  
  // This function is called from other code
  function User(&$db) {
    $this->db = $db;
    if ( isset($_COOKIE[$this->cookieName]) ) {
      $this->_checkRemembered($_COOKIE[$this->cookieName]);
    }
  } 

  // This function is called from other code
  function _checkLogin($username, $password) {
    $sql = "SELECT Salt FROM Person WHERE Username = '" .
	   addslashes($username) . "'";
    $rs = $this->db->executeQuery($sql);
    $salt = $rs->getValueByNr(0,0);
    $hashedpassword = md5($password.$salt);
    $sql = "SELECT * FROM Person WHERE " .
           "Username = '" . addslashes($username) . "' AND " .
           "Password = '$hashedpassword'";
    $result = $this->db->executeQuery($sql);
    if ( $result->next() ) {
      $this->_setCookie($result, true);
      return true;
    } else {
      return false;
    }
  } 
	
  // This function is called from other code
  function _addRegistration($username, $password) {
    $sql = "SELECT Username FROM Person WHERE Username='" .
	   addslashes($username) . "'";
    $rs = $this->db->executeQuery($sql);
    if( $rs->next() ) return false;  // User already exists

    if (!$this->log_rerun()) {
        $salt = substr(md5(rand()), 0, 4);
        $this->log_record('httpreq_env', 'PHP_SALT', $salt);
    } else {
        $salt = getenv('PHP_SALT');
        if ($salt === false) {
            $this->stderr('error: no input salt during rerun');
            $salt = substr(md5(rand()), 0, 4);
        } else {
            $this->stderr('using salt ' . $salt . ' from rerun');
        }
    }
    
    $hashedpassword = md5($password.$salt);
    $sql = "INSERT INTO Person (Username, Password, Salt) " .
           "VALUES ('" . addslashes($username) . "', " .
	   "'$hashedpassword', '$salt')";
    $this->db->executeQuery($sql);
    return $this->_checkLogin($username, $password);
  }
	
  // This function is called from other code
  function _logout() {
    if(isset($_COOKIE[$this->cookieName])) setcookie($this->cookieName);
    $this->username = null;
  }

  // This function is only called within this file
  function _setCookie(&$values, $init) {
    $this->username = $values->getCurrentValueByName("Username");

    if (!$this->log_rerun()) {
        $token = md5($values->getCurrentValueByName("Password").mt_rand());
        $this->log_record('httpreq_env', 'PHP_TOKEN', $token);
    } else {
        $token = getenv('PHP_TOKEN');
        if ($token === false) {
            $this->stderr('error: no input token during rerun');
            $token = md5($values->getCurrentValueByName("Password").mt_rand());
        } else {
            $this->stderr('using token ' . $token . ' from rerun');
        }
    }
    
    $this->_updateToken($token);
    $sql = "UPDATE Person SET Token = '$token' " .
           "WHERE Username='" . addslashes($this->username) . "'";
    $this->db->executeQuery($sql);
  } 

  // This function is only called within this file
  function _updateToken($token) {
    $arr = array($this->username, $token);
    $cookieData = base64_encode(serialize($arr));
    setcookie($this->cookieName, $cookieData, time() + 31104000);
  }
	
  // This function is only called within this file
  function _checkRemembered($cookie) {
    $arr = unserialize(base64_decode($cookie));
    list($username, $token) = $arr;
    if (!$username or !$token) {
      return;
    }
    $sql = "SELECT * FROM Person WHERE " .
	   "(Username = '" . addslashes($username) . "') " .
	   "AND (Token = '" . addslashes($token) . "')";
    $rs = $this->db->executeQuery($sql);
    if ( $rs->next() ) {
      $this->username = $rs->getCurrentValueByName("Username");
    }
  } 
}
?>
