<?php // -*- mode: php -*-
// doc -- HotCRP paper download page
// HotCRP is Copyright (c) 2006-2007 Eddie Kohler and Regents of the UC
// Distributed under an MIT-like license; see LICENSE

// A PHP script where execution is specified in .htaccess, so the requested
// paper is specified as a suffix to the request.  It may help automatic file
// naming work for some browsers.

require_once("Code/header.inc");
$Me = $_SESSION['Me'];
$Me->goIfInvalid();

// Determine the intended paper

$final = (defval($_REQUEST, 'final', 0) != 0);

if (isset($_REQUEST["p"]))
    $paperId = rcvtint($_REQUEST["p"]);
else if (isset($_REQUEST["paperId"]))
    $paperId = rcvtint($_REQUEST["paperId"]);
else {
    $paper = preg_replace("|.*/doc(\.php)?/*|", "", $_SERVER["PHP_SELF"]);
    if (preg_match("/^(" . $Opt['downloadPrefix'] . ")?(paper-?)?(\d+).*$/", $paper, $match)
	&& $match[3] > 0)
	$paperId = $match[3];
    else if (preg_match("/^(" . $Opt['downloadPrefix'] . ")?(final-?)(\d+).*$/", $paper, $match)
	     && $match[3] > 0) {
	$paperId = $match[3];
	$final = true;
    } else
	$Error = "Invalid paper name '" . htmlspecialchars($paper) . "'.";
}


// Security checks - people who can download all paperss
// are assistants, chairs & PC members. Otherwise, you need
// to be a contact person for that paper.
if (!isset($Error) && !$Me->canDownloadPaper($paperId, $Conf, $whyNot))
    $Error = whyNotText($whyNot, "view");

// Actually download paper.
if (!isset($Error)) {
    $result = $Conf->downloadPaper($paperId, rcvtint($_REQUEST["save"]) > 0, $final);
    if (!PEAR::isError($result))
	exit;
}

// If we get here, there is an error.
$Conf->header("Download Paper" . (isset($paperId) ? " #$paperId" : ""));
if (isset($Error))
    $Conf->errorMsg($Error);
$Conf->footer();
