<?php
// contactlist.inc -- HotCRP helper class for producing lists of contacts
// HotCRP is Copyright (c) 2006-2008 Eddie Kohler and Regents of the UC
// Distributed under an MIT-like license; see LICENSE

require_once("baselist.inc");

class ContactList extends BaseList {

    const FIELD_SELECTOR = 100;
    const FIELD_SELECTOR_ON = 101;
    
    const FIELD_NAME = 1;
    const FIELD_EMAIL = 2;
    const FIELD_AFFILIATION = 3;
    const FIELD_VISITS = 4;
    const FIELD_LASTVISIT = 5;
    const FIELD_HIGHTOPICS = 6;
    const FIELD_LOWTOPICS = 7;
    const FIELD_REVIEWS = 8;
    const FIELD_PAPERS = 9;
    const FIELD_REVIEW_PAPERS = 10;
    const FIELD_AFFILIATION_ROW = 11;
    const FIELD_REVIEW_RATINGS = 12;
    
    var $showHeader;
    var $sortField;
    var $reverseSort;
    var $sortable;
    var $count;
    var $anySelector;
    var $contact;
    var $scoreMax;
    var $authorMatch;
    var $collaboratorsMatch;
    var $limit;
    var $haveAffrow;
    var $haveTopics;
    var $rf;
    
    function ContactList($sortable = true) {
	global $contactListFields;
	$this->showHeader = true;

	$s = ($sortable ? defval($_REQUEST, "sort", "") : "");
	$x = (strlen($s) ? $s[strlen($s)-1] : "");
	$this->reverseSort = ($x == "R");
	if ($x == "R" || $x == "N")
	    $s = substr($s, 0, strlen($s) - 1);
	if (("x" . (int) $s) == ("x" . $s))
	    $this->sortField = (int) $s;
	else
	    $this->sortField = null;
	$this->sortable = $sortable;
	$this->haveAffrow = $this->haveTopics = null;
	$this->rf = reviewForm();
    }

    function _normalizeField($fieldId) {
	if ($fieldId >= self::FIELD_SCORE && $fieldId < self::FIELD_SCORE + self::FIELD_NUMSCORES)
	    return self::FIELD_SCORE;
	else
	    return $fieldId;
    }
    
    function selector($fieldId, &$queryOptions) {
	global $Conf, $reviewScoreNames;
	if (!$this->contact->isPC
	    && $fieldId != self::FIELD_NAME
	    && $fieldId != self::FIELD_AFFILIATION
	    && $fieldId != self::FIELD_AFFILIATION_ROW)
	    return false;
	if ($fieldId == self::FIELD_HIGHTOPICS || $fieldId == self::FIELD_LOWTOPICS) {
	    $queryOptions['topics'] = true;
	    $this->haveTopics = true;
	}
	if ($fieldId == self::FIELD_REVIEWS)
	    $queryOptions['reviews'] = true;
	if ($fieldId == self::FIELD_REVIEW_RATINGS) {
	    if ($Conf->setting("rev_ratings") == REV_RATINGS_NONE
		|| $Conf->setting("allowPaperOption") < 12)
		return false;
	    $queryOptions["revratings"] = $queryOptions["reviews"] = true;
	}
	if ($fieldId == self::FIELD_PAPERS)
	    $queryOptions['papers'] = true;
	if ($fieldId == self::FIELD_REVIEW_PAPERS)
	    $queryOptions['repapers'] = true;
	if ($fieldId == self::FIELD_AFFILIATION_ROW)
	    $this->haveAffrow = true;
	if (self::_normalizeField($fieldId) == self::FIELD_SCORE) {
	    // XXX scoresOk
	    $score = $reviewScoreNames[$fieldId - self::FIELD_SCORE];
	    $revViewScore = $this->contact->viewReviewFieldsScore(null, true, $Conf);
	    if ($this->rf->authorView[$score] <= $revViewScore)
		return false;
	    if (!isset($queryOptions['scores']))
		$queryOptions['scores'] = array();
	    $queryOptions['reviews'] = true;
	    $queryOptions['scores'][] = $score;
	    $this->scoreMax[$score] = $this->rf->maxNumericScore($score);
	}
	return true;
    }

    function _sortBase($a, $b) {
	$x = strcasecmp($a->lastName, $b->lastName);
	$x = $x ? $x : strcasecmp($a->firstName, $b->firstName);
	return $x ? $x : strcasecmp($a->email, $b->email);
    }
    
    function _sortEmail($a, $b) {
	return strcasecmp($a->email, $b->email);
    }
    
    function _sortAffiliation($a, $b) {
	$x = strcasecmp($a->affiliation, $b->affiliation);
	return $x ? $x : self::_sortBase($a, $b);
    }

    function _sortVisits($a, $b) {
	$x = $b->visits - $a->visits;
	return $x ? $x : self::_sortBase($a, $b);
    }
 
    function _sortLastVisit($a, $b) {
	$x = $b->lastLogin - $a->lastLogin;
	return $x ? $x : self::_sortBase($a, $b);
    }

    function _sortReviews($a, $b) {
	$x = $b->numReviewsSubmitted - $a->numReviewsSubmitted;
	$x = $x ? $x : $b->numReviews - $a->numReviews;
	return $x ? $x : self::_sortBase($a, $b);
    }

    function _sortReviewRatings($a, $b) {
	global $Conf;
	if ((int) $a->sumRatings != (int) $b->sumRatings)
	    return ($a->sumRatings > $b->sumRatings ? -1 : 1);
	if ((int) $a->numRatings != (int) $b->numRatings)
	    return ($a->numRatings > $b->numRatings ? 1 : -1);
	return self::_sortBase($a, $b);
    }

    function _sortPapers($a, $b) {
	$x = (int) $a->paperIds - (int) $b->paperIds;
	$x = $x ? $x : strcmp($a->paperIds, $b->paperIds);
	return $x ? $x : self::_sortBase($a, $b);
    }

    function _sort($rows) {
	global $Conf, $reviewScoreNames;
	switch (self::_normalizeField($this->sortField)) {
	case self::FIELD_EMAIL:
	    usort($rows, array("ContactList", "_sortEmail"));
	    break;
	case self::FIELD_AFFILIATION:
	case self::FIELD_AFFILIATION_ROW:
	    usort($rows, array("ContactList", "_sortAffiliation"));
	    break;
	case self::FIELD_VISITS:
	    usort($rows, array("ContactList", "_sortVisits"));
	    break;
	case self::FIELD_LASTVISIT:
	    usort($rows, array("ContactList", "_sortLastVisit"));
	    break;
	case self::FIELD_REVIEWS:
	    usort($rows, array("ContactList", "_sortReviews"));
	    break;
	case self::FIELD_REVIEW_RATINGS:
	    usort($rows, array("ContactList", "_sortReviewRatings"));
	    break;
	case self::FIELD_PAPERS:
	case self::FIELD_REVIEW_PAPERS:
	    usort($rows, array("ContactList", "_sortPapers"));
	    break;
	case self::FIELD_SCORE:
	    $scoreName = $reviewScoreNames[$this->sortField - self::FIELD_SCORE];
	    $scoreMax = $this->scoreMax[$scoreName];
	    $this->sortInfo = array();
	    $this->sortAverage = array();
	    $scoresort = defval($_SESSION, "pplscoresort", "A");
	    if ($scoresort != "A" && $scoresort != "V" && $scoresort != "D")
		$scoresort = 1;
	    foreach ($rows as $row)
		$this->_setSortRow($row, $scoreName, $scoreMax, $scoresort);
	    usort($rows, array($this, "_sortScoreNum"));
	}
	if ($this->reverseSort)
	    return array_reverse($rows);
	else
	    return $rows;
    }
    
    function header($fieldId, $ordinal, $row = null) {
	global $reviewScoreNames;
	switch (self::_normalizeField($fieldId)) {
	case self::FIELD_NAME:
	    return "Name";
	case self::FIELD_EMAIL:
	    return "Email";
	case self::FIELD_AFFILIATION:
	case self::FIELD_AFFILIATION_ROW:
	    return "Affiliation";
	case self::FIELD_VISITS:
	    return "Visits";
	case self::FIELD_LASTVISIT:
	    return "Last visit";
	case self::FIELD_HIGHTOPICS:
	    return "High-interest topics";
	case self::FIELD_LOWTOPICS:
	    return "Low-interest topics";
	case self::FIELD_REVIEWS:
	    return "<span class='hastitle' title='\"1/2\" means 1 complete review out of 2 assigned reviews'>Reviews</span>";
	case self::FIELD_REVIEW_RATINGS:
	    return "<span class='hastitle' title='Ratings of reviews'>Rating</a>";
	case self::FIELD_SELECTOR:
	    return "";
	case self::FIELD_PAPERS:
	case self::FIELD_REVIEW_PAPERS:
	    return "Papers";
	case self::FIELD_SCORE: {
	    $scoreName = $reviewScoreNames[$fieldId - self::FIELD_SCORE];
	    return $this->rf->webFieldAbbrev($scoreName);
	}
	default:
	    return "&lt;$fieldId&gt;?";
	}
    }
    
    function content($fieldId, $row) {
	global $Conf, $ConfSiteSuffix, $reviewTypeName, $reviewScoreNames;
	switch (self::_normalizeField($fieldId)) {
	case self::FIELD_NAME:
	    $t = contactHtml($row->firstName, $row->lastName);
	    if (trim($t) == "")
		$t = "[No name]";
	    if ($this->contact->privChair)
		$t = "<a href='account$ConfSiteSuffix?contact=$row->contactId'>$t</a>";
	    if ($row->chair)
		$t .= " <span class='pcrole'>(Chair)</span>";
	    else if ($row->assistant)
		$t .= " <span class='pcrole'>(Administrator)</span>";
	    else if ($row->pc && $this->limit != "pc")
		$t .= " <span class='pcrole'>(PC)</span>";
	    if ($this->contact->privChair && $row->email != $this->contact->email)
		$t .= " <a href=\"index$ConfSiteSuffix?viewContact=" . urlencode($row->email) . "\">"
		    . $Conf->cacheableImage("viewas.png", "[Act as]", "Act as " . htmlspecialchars(contactNameText($row)))
		    . "</a>";
	    return $t;
	case self::FIELD_EMAIL:
	    if (!$this->contact->isPC)
		return "";
	    return "<a href=\"mailto:" . htmlspecialchars($row->email) . "\">" . htmlspecialchars($row->email) . "</a>";
	case self::FIELD_AFFILIATION:
	case self::FIELD_AFFILIATION_ROW:
	    return htmlspecialchars($row->affiliation);
	case self::FIELD_VISITS:
	    return $row->visits;
	case self::FIELD_LASTVISIT:
	    if (!$row->lastLogin)
		return "Never";
	    return $Conf->printableTimeShort($row->lastLogin);
	case self::FIELD_SELECTOR:
	case self::FIELD_SELECTOR_ON:
	    $this->anySelector = 1;
	    $c = "";
	    if ($fieldId == self::FIELD_SELECTOR_ON)
		$c = " checked='checked'";
	    return "<input type='checkbox' name='pap[]' value='$row->contactId' tabindex='1' id='psel$this->count' onclick='pselClick(event, this, $this->count)' $c/>";
	case self::FIELD_HIGHTOPICS:
	case self::FIELD_LOWTOPICS:
	    if (!defval($row, "topicIds"))
		return "";
	    $want = ($fieldId == self::FIELD_HIGHTOPICS ? 2 : 0);
	    $topics = array_combine(explode(",", $row->topicIds), explode(",", $row->topicInterest));
	    $nt = array();
	    foreach ($topics as $k => $v)
		if ($v == $want)
		    $nt[] = $k;
	    if (count($nt))
		return join(", ", $this->rf->webTopicArray($nt, array_fill(0, count($nt), $want)));
	    else
		return "";
	case self::FIELD_REVIEWS:
	    if (!$row->numReviews && !$row->numReviewsSubmitted)
		return "";
	    $a1 = "<a href=\"search$ConfSiteSuffix?t=s&amp;q=re:" . urlencode($row->email) . "\">";
	    if ($row->numReviews == $row->numReviewsSubmitted)
		return "$a1<b>$row->numReviewsSubmitted</b></a>";
	    else
		return "$a1<b>$row->numReviewsSubmitted</b>/$row->numReviews</a>";
	case self::FIELD_REVIEW_RATINGS:
	    if ((!$row->numReviews && !$row->numReviewsSubmitted)
		|| !$row->numRatings)
		return "";
	    $a = array();
	    $b = array();
	    if ($row->sumRatings > 0) {
		$a[] = $row->sumRatings . " positive";
		$b[] = "<a href=\"search$ConfSiteSuffix?q=re:" . urlencode($row->email) . "+rate:%2B\">+" . $row->sumRatings . "</a>";
	    }
	    if ($row->sumRatings < $row->numRatings) {
		$a[] = ($row->numRatings - $row->sumRatings) . " negative";
		$b[] = "<a href=\"search$ConfSiteSuffix?q=re:" . urlencode($row->email) . "+rate:-\">&minus;" . ($row->numRatings - $row->sumRatings) . "</a>";
	    }
	    return "<span class='hastitle' title='" . join(", ", $a) . "'>" . join(" ", $b) . "</span>";
	case self::FIELD_PAPERS:
	    if (!$row->paperIds)
		return "";
	    $x = explode(",", $row->paperIds);
	    sort($x, SORT_NUMERIC);
	    $all = ($this->limit == "auuns" || $this->limit == "all");
	    $extra = "&amp;list=" . ($all ? "all:" : "s:") . join("+", $x);
	    $m = array();
	    foreach ($x as $v)
		$m[] = "<a href=\"paper$ConfSiteSuffix?p=$v$extra\">$v</a>";
	    return join(", ", $m);
	case self::FIELD_REVIEW_PAPERS:
	    if (!$row->paperIds)
		return "";
	    $x = array_combine(explode(",", $row->paperIds), explode(",", $row->reviewIds));
	    ksort($x, SORT_NUMERIC);
	    $extra = "&amp;list=s:" . join("+", array_keys($x));
	    $m = array();
	    foreach ($x as $k => $v)
		$m[] = "<a href=\"review$ConfSiteSuffix?r=$v$extra\">$k</a>";
	    return join(", ", $m);
	case self::FIELD_SCORE:
	    if (!$row->pc && !$this->contact->privChair
		&& $this->limit != "req")
		return "";
	    $scoreName = $reviewScoreNames[$fieldId - self::FIELD_SCORE];
	    $v = scoreCounts($row->$scoreName, $this->scoreMax[$scoreName]);
	    $m = "";
	    if ($v->n > 0)
		$m = $Conf->textValuesGraph($v, 0, 2, 0, $this->rf->reviewFields[$scoreName]);
	    return $m;
	default:
	    return "";
	}
    }

    function addScores($a) {
	if ($this->contact->isPC) {
	    $scores = defval($_SESSION, "pplscores", 1);
	    for ($i = 0; $i < self::FIELD_NUMSCORES; $i++)
		if ($scores & (1 << $i))
		    array_push($a, self::FIELD_SCORE + $i);
	    $this->scoreMax = array();
	}
	return $a;
    }

    function listFields($listname) {
	switch ($listname) {
	  case "pc":
	  case "admin":
	  case "pcadmin":
	    return $this->addScores(array($listname, self::FIELD_SELECTOR, self::FIELD_NAME, self::FIELD_EMAIL, self::FIELD_AFFILIATION, self::FIELD_VISITS, self::FIELD_LASTVISIT, self::FIELD_HIGHTOPICS, self::FIELD_LOWTOPICS, self::FIELD_REVIEWS, self::FIELD_REVIEW_RATINGS));
	  case "pcadminx":
	    return array($listname, self::FIELD_NAME, self::FIELD_EMAIL, self::FIELD_AFFILIATION, self::FIELD_VISITS, self::FIELD_LASTVISIT, self::FIELD_HIGHTOPICS, self::FIELD_LOWTOPICS);
	  case "re":
	  case "resub":
	    return $this->addScores(array($listname, self::FIELD_SELECTOR, self::FIELD_NAME, self::FIELD_EMAIL, self::FIELD_AFFILIATION, self::FIELD_VISITS, self::FIELD_LASTVISIT, self::FIELD_HIGHTOPICS, self::FIELD_LOWTOPICS, self::FIELD_REVIEWS, self::FIELD_REVIEW_RATINGS));
	  case "ext":
	  case "extsub":
	    return $this->addScores(array($listname, self::FIELD_SELECTOR, self::FIELD_NAME, self::FIELD_EMAIL, self::FIELD_AFFILIATION, self::FIELD_VISITS, self::FIELD_LASTVISIT, self::FIELD_HIGHTOPICS, self::FIELD_LOWTOPICS, self::FIELD_REVIEWS, self::FIELD_REVIEW_RATINGS, self::FIELD_REVIEW_PAPERS));
	  case "req":
	    return $this->addScores(array("req", self::FIELD_SELECTOR, self::FIELD_NAME, self::FIELD_EMAIL, self::FIELD_AFFILIATION, self::FIELD_VISITS, self::FIELD_LASTVISIT, self::FIELD_HIGHTOPICS, self::FIELD_LOWTOPICS, self::FIELD_REVIEWS, self::FIELD_REVIEW_RATINGS, self::FIELD_REVIEW_PAPERS));
	  case "au":
	  case "aurej":
	  case "auacc":
	  case "auuns":
	    return array($listname, self::FIELD_SELECTOR, self::FIELD_NAME, self::FIELD_EMAIL, self::FIELD_AFFILIATION_ROW, self::FIELD_VISITS, self::FIELD_LASTVISIT, self::FIELD_PAPERS);
	  case "all":
	    return array("all", self::FIELD_SELECTOR, self::FIELD_NAME, self::FIELD_EMAIL, self::FIELD_AFFILIATION_ROW, self::FIELD_VISITS, self::FIELD_LASTVISIT, self::FIELD_PAPERS);
	  default:
	    return null;
	}
    }

    function footer($ncol) {
	global $Conf;
	if ($this->count == 0)
	    return "";
	
	$t = "  <tr class='pl_footrow'>\n    <td class='pl_selector' style='vertical-align: top'>" . $Conf->cacheableImage("bendulft.png", "^^", null, "placthook") . "</td>\n";
	$t .= "    <td id='pplact' class='linelinks1' colspan='" . ($ncol - 1) . "'><b>Select people</b> (or <a href='javascript:void papersel(true)'>select all " . $this->count . "</a>), then ";

	// Begin linelinks
	$t .= "<span class='lll1'><a href='javascript:void crpfocus(\"pplact\",1)'>Download</a></span><span class='lld1'><b>:</b> &nbsp;"
	    . tagg_select("getaction", array("nameemail" => "Names and emails",
					     "address" => "Addresses"),
			  null, array("id" => "pplact1_d"))
	    . "&nbsp; <input type='submit' class='bsm' name='getgo' value='Go' /></span>";

	return $t . "</td></tr>\n";
    }

    function text($listname, $me, $url) {
	global $Conf, $contactListFields;

	$this->contact = $me;
	$this->count = 0;
	$this->anySelector = 0;

	// get paper list
	if (!($baseFieldId = $this->listFields($listname))) {
	    $Conf->errorMsg("There is no people list query named '" . htmlspecialchars($listname) . "'.");
	    return null;
	}
	$this->limit = array_shift($baseFieldId);

	if (!in_array($this->sortField, $baseFieldId))
	    $this->sortField = null;
	
	// get field array
	$fieldDef = array();
	$ncol = 0;
	$queryOptions = array();
	foreach ($baseFieldId as $fid) {
	    if ($this->selector($fid, $queryOptions) === false)
		continue;
	    $normFid = self::_normalizeField($fid);
	    $fieldDef[$fid] = $contactListFields[$normFid];
	    if ($contactListFields[$normFid][1] == 1)
		$ncol++;
	}

	// prepare query text
	$aulimit = (strlen($this->limit) >= 2 && $this->limit[0] == 'a' && $this->limit[1] == 'u');
	$pq = "select ContactInfo.contactId,
	ContactInfo.contactId as paperId,
	firstName, lastName, email, affiliation,
	voicePhoneNumber, faxPhoneNumber,
	ContactInfo.collaborators, lastLogin, visits, note,
	PCMember.contactId as pc, Chair.contactId as chair,
	ChairAssistant.contactId as assistant";
	if (isset($queryOptions['topics']))
	    $pq .= ",\n	topicIds, topicInterest";
	if (isset($queryOptions['reviews'])) {
	    $pq .= ",
	count(if(PaperReview.reviewNeedsSubmit<=0,PaperReview.reviewSubmitted,1)) as numReviews,
	count(PaperReview.reviewSubmitted) as numReviewsSubmitted";
	    if (isset($queryOptions["revratings"]))
		$pq .= ",\n	sum(PaperReview.numRatings) as numRatings,
	sum(PaperReview.sumRatings) as sumRatings";
	}
	if (isset($queryOptions['scores']))
	    foreach ($queryOptions['scores'] as $score)
		$pq .= ",\n\tgroup_concat(PaperReview.$score) as $score";
	if (isset($queryOptions['repapers']))
	    $pq .= ",\n\tgroup_concat(PaperReview.paperId) as paperIds,
	group_concat(PaperReview.reviewId) as reviewIds";
	else if (isset($queryOptions['papers']))
	    $pq .= ",\n\tgroup_concat(PaperConflict.paperId) as paperIds";
	
	$pq .= "\n	from ContactInfo\n";
	$pq .= ($this->limit == "pc" ? "\tjoin" : "\tleft join") . " PCMember on (PCMember.contactId=ContactInfo.contactId)\n";
	$pq .= "\tleft join Chair on (Chair.contactId=ContactInfo.contactId)
	left join ChairAssistant on (ChairAssistant.contactId=ContactInfo.contactId)\n";
	if (isset($queryOptions['topics']))
	    $pq .= "	left join (select contactId, group_concat(topicId) as topicIds, group_concat(interest) as topicInterest from TopicInterest group by contactId) as TopicInterests on (TopicInterests.contactId=ContactInfo.contactId)\n";
	if (isset($queryOptions['reviews'])) {
	    $j = "left join";
	    if ($this->limit == "re" || $this->limit == "req" || $this->limit == "ext" || $this->limit == "resub" || $this->limit == "extsub")
		$j = "join";
	    $pq .= "	$j (select PaperReview.*";
	    if (isset($queryOptions["revratings"]))
		$pq .= ", count(rating) as numRatings, sum(if(rating>0,1,0)) as sumRatings";
	    $pq .= " from PaperReview join Paper on (Paper.paperId=PaperReview.paperId)";
	    if (!$this->contact->privChair)
		$pq .= " left join PaperConflict on (PaperConflict.paperId=Paper.paperId and PaperConflict.contactId=" . $this->contact->contactId . ")";
	    if (isset($queryOptions["revratings"])) {
		require_once("search.inc");
		$badratings = PaperSearch::unusableRatings($me->privChair, $me->contactId);
		$pq .= " left join ReviewRating on (ReviewRating.reviewId=PaperReview.reviewId";
		if (count($badratings) > 0)
		    $pq .= " and not (ReviewRating.reviewId in (" . join(",", $badratings) . "))";
		$pq .= ")";
	    }
	    $jwhere = array();
	    if ($this->limit == "req" || $this->limit == "ext" || $this->limit == "extsub")
		$jwhere[] = "PaperReview.reviewType=" . REVIEW_EXTERNAL;
	    if ($this->limit == "req")
		$jwhere[] = "PaperReview.requestedBy=" . $this->contact->contactId;
	    if (!$this->contact->privChair)
		$jwhere[] = "(conflictType is null or conflictType=0 or PaperReview.contactId=" . $this->contact->contactId . ")";
	    if (count($jwhere))
		$pq .= " where " . join(" and ", $jwhere);
	    if (isset($queryOptions["revratings"]))
		$pq .= " group by PaperReview.reviewId";
	    $pq .= ") as PaperReview on (PaperReview.contactId=ContactInfo.contactId)\n";
	}
	if ($aulimit)
	    $pq .= "\tjoin PaperConflict on (PaperConflict.contactId=ContactInfo.contactId and PaperConflict.conflictType>=" . CONFLICT_AUTHOR . ")\n";
	if ($this->limit == "au")
	    $pq .= "\tjoin Paper on (Paper.paperId=PaperConflict.paperId and Paper.timeSubmitted>0)\n";
	if ($this->limit == "aurej")
	    $pq .= "\tjoin Paper on (Paper.paperId=PaperConflict.paperId and Paper.outcome<0)\n";
	if ($this->limit == "auacc")
	    $pq .= "\tjoin Paper on (Paper.paperId=PaperConflict.paperId and Paper.outcome>0)\n";
	if ($this->limit == "auuns")
	    $pq .= "\tjoin Paper on (Paper.paperId=PaperConflict.paperId and Paper.timeSubmitted<=0)\n";
	if ($this->limit == "all")
	    $pq .= "\tleft join PaperConflict on (PaperConflict.contactId=ContactInfo.contactId and PaperConflict.conflictType>=" . CONFLICT_AUTHOR . ")\n";
	if ($this->limit == "admin")
	    $pq .= "\twhere Chair.contactId is not null or ChairAssistant.contactId is not null\n";
	if ($this->limit == "pcadmin" || $this->limit == "pcadminx")
	    $pq .= "\twhere Chair.contactId is not null or ChairAssistant.contactId is not null or PCMember.contactId is not null";
	
	$pq .= "	group by ContactInfo.contactId
	order by lastName, firstName, email";

	// make query
	$result = $Conf->qe($pq, "while selecting people");
	if (!$result)
	    return NULL;

	// fetch data
	if (edb_nrows($result) == 0)
	    return "No matching people";
	$rows = array();
	while (($row = edb_orow($result)))
	    $rows[] = $row;

	// sort rows
	$srows = $this->_sort($rows);

	$anyData = array();
	$body = '';
	$extrainfo = false;
	foreach ($srows as $row) {
	    if (($this->limit == "resub" || $this->limit == "extsub")
		&& $row->numReviewsSubmitted == 0)
		continue;
	    
	    $trclass = "k" . ($this->count % 2);
	    $this->count++;

	    $t = "  <tr class=\"pl $trclass\">\n";
	    foreach ($fieldDef as $fieldId => $fdef)
		if ($fdef[1] == 1) {
		    $c = $this->content($fieldId, $row);
		    $t .= "    <td class=\"pl_$fdef[0]\">$c</td>\n";
		    if ($c != "")
			$anyData[$fieldId] = 1;
		}
	    $t .= "  </tr>\n";
	    
	    foreach ($fieldDef as $fieldId => $fdef)
		if ($fdef[1] >= 2 && ($d = $this->content($fieldId, $row))) {
		    $t .= "  <tr class=\"pl_$fdef[0] pl_callout $trclass";
		    if ($fdef[1] == 3 || $fdef[1] == 4)
			$t .= " extension" . ($fdef[1] - 2);
		    $t .= "\">\n";
		    $t .= "<td class=\"pl_$fdef[0] pl_callout\" colspan='$ncol'>";
		    $t .= "<span class='pl_callouthdr'>" . $this->header($fieldId, -1, $row) . ":</span>";
		    $t .= $d . "</td>\n  </tr>\n";
		}

	    $body .= $t;
	}

	$x = "<table class=\"ppltable plt_" . htmlspecialchars($listname);
	if ($this->haveAffrow !== null) {
	    $this->haveAffrow = (defval($_SESSION, "foldpplaff", 0) == 0);
	    $x .= ($this->haveAffrow ? " fold2o" : " fold2c");
	}
	if ($this->haveTopics !== null) {
	    $this->haveTopics = (defval($_SESSION, "foldppltopics", 1) == 0);
	    $x .= ($this->haveTopics ? " fold1o" : " fold1c");
	}
	$x .= "\" id=\"foldppl\">\n";

	if ($this->showHeader) {
	    $x .= "  <tr class=\"pl_headrow\">\n";
	    $ord = 0;

	    if ($this->sortable && $url) {
		$sortUrl = htmlspecialchars($url) . (strpos($url, "?") ? "&amp;" : "?") . "sort=";
		$q = "<a class='pl_sort' href=\"" . $sortUrl;
		foreach ($fieldDef as $fieldId => $fdef) {
		    if ($fdef[1] != 1)
			continue;
		    else if (!isset($anyData[$fieldId])) {
			$x .= "    <th class=\"pl_$fdef[0]\"></th>\n";
			continue;
		    }
		    $x .= "    <th class=\"pl_$fdef[0]\">";
		    $ftext = $this->header($fieldId, $ord++);
		    if ($this->sortField == null && $fieldId == 1)
			$this->sortField = $fieldId;
		    if ($fieldId == $this->sortField)
			$x .= "<a class='pl_sort_def" . ($this->reverseSort ? "_rev" : "") . "' href=\"" . $sortUrl . $fieldId . ($this->reverseSort ? "N" : "R") . "\">" . $ftext . "</a>";
		    else if ($fdef[2])
			$x .= $q . $fieldId . "\">" . $ftext . "</a>";
		    else
			$x .= $ftext;
		    $x .= "</th>\n";
		}
		
	    } else {
		foreach ($fieldDef as $fieldId => $fdef)
		    if ($fdef[1] == 1 && isset($anyData[$fieldId]))
			$x .= "    <th class=\"pl_$fdef[0]\">"
			    . $this->header($fieldId, $ord++) . "</th>\n";
		    else if ($fdef[1] == 1)
			$x .= "    <th class=\"pl_$fdef[0]\"></th>\n";
	    }
	    
	    $x .= "  </tr>\n";
	    $x .= "  <tr><td class='pl_headgap' colspan='$ncol'></td></tr>\n";
	}

	$x .= $body;

	$x .= "  <tr class='pl_footgap k" . (($this->count - 1) % 2) . "'><td class='pl_blank' colspan='$ncol'></td></tr>\n";
	reset($fieldDef);
	if (key($fieldDef) == self::FIELD_SELECTOR)
	    $x .= $this->footer($ncol);
	
	$x .= "</table>";

	return $x;
    }

}


global $contactListFields;
$contactListFields = array(
	ContactList::FIELD_SELECTOR => array('selector', 1, 0),
	ContactList::FIELD_SELECTOR_ON => array('selector', 1, 0),
	ContactList::FIELD_NAME => array('name', 1, 1),
	ContactList::FIELD_EMAIL => array('email', 1, 1),
	ContactList::FIELD_AFFILIATION => array('affiliation', 1, 1),
	ContactList::FIELD_AFFILIATION_ROW => array('affrow', 4, 0),
	ContactList::FIELD_VISITS => array('visits', 1, 1),
	ContactList::FIELD_LASTVISIT => array('lastvisit', 1, 1),
	ContactList::FIELD_HIGHTOPICS => array('topics', 3, 0),
	ContactList::FIELD_LOWTOPICS => array('topics', 3, 0),
	ContactList::FIELD_REVIEWS => array('revstat', 1, 1),
	ContactList::FIELD_REVIEW_RATINGS => array('revstat', 1, 1),
	ContactList::FIELD_PAPERS => array('papers', 1, 1),
	ContactList::FIELD_REVIEW_PAPERS => array('papers', 1, 1),
	ContactList::FIELD_SCORE => array('score', 1, 1)
	);
