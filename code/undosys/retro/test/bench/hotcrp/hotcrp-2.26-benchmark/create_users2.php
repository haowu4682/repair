<?php
$custom_sessions = true;
require_once ('Code/header.inc');

assert ($argc == 8);

$dbname = $argv[1];
$chairs = $argv[2];
$pcmembers = $argv[3];
$authors = $argv[4];
$papersize = $argv[5];

$use_policy = $argv[6];
$sessname = $argv[7];


$link = mysql_connect();
if (!$link){
    perr2("mysql error: " . mysql_error () . "\n");
}
mysql_select_db($dbname, $link);
mysql_query("set names 'utf8'", $link);

$stderr = fopen ("php://stderr", 'w');
function perr2 ($s) {
  global $stderr;
  fwrite ($stderr, $s);
  fflush($stderr);
}

function query ($q) {
  global $link, $use_policy;
  $r = $use_policy ? resin_mysql_query ($q, $link) : mysql_query($q, $link);
  if (!$r) {
    perr2("mysql error: " . mysql_error () . "\n");
    perr2(" query: $q\n");
  }
  return $r;
}

function create_user ($usertype, $usertypeindex, $contactId) {
  global $use_policy;
  $email = "$usertype$usertypeindex@foo.edu";
  $pw = 'pw';
  if ($use_policy) {
    $p = make_password_policy($email);
    taint_set_policy ($pw, $p);
  }

  query ("INSERT INTO ContactInfo (firstName, email, password, creationTime, contactId) ".
         "VALUES ('$usertype$usertypeindex', '$email', '$pw', 1235849447, $contactId)");
  query ("INSERT INTO ActionLog (ipaddr, contactId, paperId, action) VALUES ('127.0.0.1', $contactId, null, 'Account created')");
}

function open_submissions () {
  query ("DELETE FROM Settings ".
         "WHERE name='sub_open' OR name='sub_blind' OR name='sub_reg' OR ".
         "name='sub_sub' OR name='sub_grace' OR name='sub_pcconf' OR ".
         "name='sub_pcconfsel' OR name='sub_collab' OR name='sub_banal' OR ".
         "name='sub_freeze' OR name='pc_seeall' OR name='sub_update' OR ".
         "name='revform_update'");
  query ("INSERT INTO Settings (name, value, data) ".
         "VALUES ('sub_open', '1', null), ('sub_blind', '2', null), ".
         "('sub_pcconf', '1', null), ('sub_freeze', '0', null), ".
         "('papersub', '1', null), ".
         "('revform_update', '1235940344', null)");
  query ("INSERT INTO ActionLog (ipaddr, contactId, paperId, action) VALUES ".
         "('127.0.0.1', 1, null, 'Updated settings group \'sub\'')");
}

function open_reviewing () {
  query ("delete from Settings where name='rev_open' or name='cmt_always' or name='rev_blind' or name='rev_notifychair' or name='pcrev_any' or name='pcrev_soft' or name='pcrev_hard' or name='rev_roundtag' or name='pc_seeallrev' or name='extrev_chairreq' or name='tag_chair' or name='tag_vote' or name='tag_rank' or name='tag_seeall' or name='extrev_soft' or name='extrev_hard' or name='extrev_view' or name='mailbody_requestreview' or name='rev_ratings' or name='revform_update'");
  query ("insert into Settings (name, value, data) values ('rev_open', '1', null), ('rev_blind', '2', null), ('pc_seeallrev', '0', null), ('tag_chair', '3', 'accept pcpaper reject'), ('tag_vote', '0', ''), ('tag_rank', '0', ''), ('extrev_view', '0', null), ('rev_ratings', '0', null), ('revform_update', '1235870787', null)");
  query ("insert into ActionLog (ipaddr, contactId, paperId, action) values ('127.0.0.1', 1, null, 'Updated settings group \'rev\'')");
}

function close_submissions () {
  query ("delete from Settings where name='sub_open' or name='sub_blind' or name='sub_reg' or name='sub_sub' or name='sub_grace' or name='sub_pcconf' or name='sub_pcconfsel' or name='sub_collab' or name='sub_banal' or name='sub_freeze' or name='pc_seeall' or name='sub_update' or name='revform_update'");
  query ("insert into Settings (name, value, data) values ('sub_blind', '2', null), ('sub_pcconf', '1', null), ('sub_freeze', '0', null), ('revform_update', '1235939104', null)");
  query ("insert into ActionLog (ipaddr, contactId, paperId, action) values ('127.0.0.1', 1, null, 'Updated settings group \'sub\'')");
}

function make_chair ($contactId) {
  query ("insert into PCMember (contactId) values ($contactId)");
  query ("insert into ActionLog (ipaddr, contactId, paperId, action) ".
         "values ('127.0.0.1', $contactId, null, 'Added as PCMember by chair@foo.edu')");
  query ("insert into Chair (contactId) values ($contactId)");
  query ("insert into ActionLog (ipaddr, contactId, paperId, action) ".
         "values ('127.0.0.1', $contactId, null, 'Added as Chair by chair@foo.edu')");
  query ("insert into Settings (name, value) values ('pc', 1235855943) on duplicate key update value=1235855943");
  query ("update ContactInfo set roles='5', defaultWatch='2' where contactId='$contactId'");
  query ("delete from ContactAddress where contactId=$contactId");
  query ("insert into ActionLog (ipaddr, contactId, paperId, action) ".
         "values ('127.0.0.1', $contactId, null, 'Account updated by chair@foo.edu')");
}

function make_pcmember ($contactId) {
  query ("insert into PCMember (contactId) values ($contactId)");
  query ("insert into ActionLog (ipaddr, contactId, paperId, action) ".
         "values ('127.0.0.1', $contactId, null, 'Added as PCMember by pc@foo.edu')");
  query ("insert into Settings (name, value) values ('pc', 1235849728) on duplicate key update value=1235849728");
  query ("update ContactInfo set roles='1', defaultWatch='2' where contactId='$contactId'");
  query ("delete from ContactAddress where contactId=$contactId");
  query ("select ContactInfo.* from ContactInfo where email='alice@foo.com'");
  query ("insert into ActionLog (ipaddr, contactId, paperId, action) ".
         "values ('127.0.0.1', $contactId, null, 'Account updated by pc@foo.edu')");
}

function insert_paper ($paperId, $storageId, $usertype, $usertypeindex, $contactId) {
  global $papersize, $use_policy;
  $pdf = "%PDF-1.5%pdf\r\n" . str_repeat('X', $papersize);
  $sha = mysql_real_escape_string(sha1($pdf, true));
  $len = strlen($pdf);

  $u = "$usertype$usertypeindex";
  $coauthors = "$u-coauthor1\t$u-coauthor1@foo.edu\t\n";
  $coauthors .= "$u-coauthor2\t$u-coauthor2@foo.edu\t\n";
  $coauthors .= "$u-coauthor3\t$u-coauthor3@foo.edu\t\n";
  $coauthors .= "$u-coauthor4\t$u-coauthor4@foo.edu\t\n";

  $q = array ('title' => "'Paper$paperId'",
              'abstract' => "'AbstractContent'",
              'collaborators' => "''",
              'authorInformation' => "'$coauthors'",
              'blind' => 1,
              'paperStorageId' => "'$storageId'",
              'paperId' => "'$paperId'",
              'size' => "'$len'",
              'mimetype' => "'application/pdf'",
              'sha1' => "'$sha'");

  if ($use_policy) {
    foreach ($q as $k => $v) {
      switch ($k) {
      case 'authorInformation':
      case 'collaborators':
        taint_set_policy ($q[$k], make_author_policy ($paperId));
        break;
      case 'title':
      case 'abstract':
        taint_set_policy ($q[$k], make_paper_policy ($paperId));
        break;
      default:
        break;
      }
    }
  }

  query (array_to_insert ("Paper", $q));
  query ("insert into PaperConflict (paperId, contactId, conflictType) values ($paperId, $contactId, 10)");
  
  query ("delete from PaperTopic where paperId=$paperId");
  query ("delete from PaperConflict where paperId=$paperId and conflictType>=2 and conflictType<=7");
  query ("insert into PaperStorage (paperId, paperStorageId, timestampx, mimetype, paper) ".
         "VALUES ($paperId, $storageId, 1235851222, 'application/pdf', '$pdf')");

  //select length(paper) from PaperStorage where paperStorageId=4
  query ("update Paper set timeSubmitted=1235851222 where paperId=$paperId");
  query ("insert into ActionLog (ipaddr, contactId, paperId, action) values ('127.0.0.1', $contactId, $paperId, 'Submitted')");
}

function create_session ($contactId) {
  global $Conf, $sessname;
  session_id ("$sessname-contact$contactId");
  do_session ();
  $_SESSION["Me"]->lookupById($contactId, $Conf);
  session_write_close ();
}

function main () {
  global $chairs, $pcmembers, $authors;

  $contactId = $authors;
  $paperId = $authors;
  $storageId = $authors;

  for ($i=$authors; $i<=$authors+100; $i++) {
    create_user ("author", $i, ++$contactId);
    create_session ($contactId);
    insert_paper (++$paperId, ++$storageId, "author", $i, $contactId);
  }
}

main();
?>
