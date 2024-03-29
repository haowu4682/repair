<?php
// mailtemplate.inc -- HotCRP mail templates
// HotCRP is Copyright (c) 2006-2008 Eddie Kohler and Regents of the UC
// Distributed under an MIT-like license; see LICENSE

global $mailTemplates;
$mailTemplates = array
    ("createaccount" =>
     array("[%CONFSHORTNAME%] New account information",
	   "Greetings,

An account has been created for you at the %CONFNAME% submissions site, including an initial password.

        Site: %URL%/
       Email: %EMAIL%
    Password: %PASSWORD%

To sign in directly, either click the link below or paste it into your web browser's location field.

%LOGINURL%

If you already have an account under a different email address, you may merge this new account into that one.  Sign in to the site and select \"Merge accounts\".

Contact the site administrator, %ADMIN%, with any questions or concerns.

- %CONFSHORTNAME% Submissions\n"),

     "accountinfo" =>
     array("[%CONFSHORTNAME%] Account information",
	   "Dear %NAME%,

Here is your account information for the %CONFNAME% submissions site.

        Site: %URL%/
       Email: %EMAIL%
    Password: %PASSWORD%

To sign in directly, either click the link below or paste it into your web browser's location field.

%LOGINURL%

Contact the site administrator, %ADMIN%, with any questions or concerns.

- %CONFSHORTNAME% Submissions\n"),

     "mergeaccount" =>
     array("[%CONFSHORTNAME%] Merged account",
	   "Dear %NAME%,

Your account at the %CONFSHORTNAME% submissions site has been merged with the account of %OTHERCONTACT%.  From now on, you should log in using the %OTHEREMAIL% account.

Contact the site administrator, %ADMIN%, with any questions or concerns.

- %CONFSHORTNAME% Submissions\n"),

     "requestreview" =>
     array("[%CONFSHORTNAME%] Review request for paper #%NUMBER%",
	   "Dear %NAME%,

On behalf of the %CONFNAME% program committee, %OTHERCONTACT% would like to solicit your help with the review of %CONFNAME% paper #%NUMBER%.%IF(REASON)%  They supplied this note: %REASON%%ENDIF%

       Title: %TITLE%
     Authors: %OPT(AUTHORS)%
  Paper site: %URL%/paper%PHP%?p=%NUMBER%

If you are willing to review this paper, you may enter your review on the conference site or complete a review form offline and upload it.%IF(DEADLINE(extrev_soft))%  Your review is requested by %DEADLINE(extrev_soft)%.%ENDIF%

Once you've decided, please take a moment to accept or refuse this review request by using one of these links.  You may also contact %OTHERNAME% directly or refuse the request using the conference site.

      Accept: %URL%/review%PHP%?p=%NUMBER%&accept=1&%LOGINURLPARTS%
      Refuse: %URL%/review%PHP%?p=%NUMBER%&refuse=1&%LOGINURLPARTS%

For reference, your account information is as follows.

        Site: %URL%/
       Email: %EMAIL%
    Password: %PASSWORD%

To sign in, either click the link below or paste it into your web browser's location field.

%LOGINURL%

Contact the site administrator, %ADMIN%, with any questions or concerns.

Thanks for your help -- we appreciate that reviewing is hard work!
- %CONFSHORTNAME% Submissions\n"),

     "retractrequest" =>
     array("[%CONFSHORTNAME%] Retracting review request for paper #%NUMBER%",
	   "Dear %NAME%,

%OTHERNAME% has retracted a previous request that you review %CONFNAME% paper #%NUMBER%.  There's no need to complete your review.

       Title: %TITLE%
     Authors: %OPT(AUTHORS)%

Contact the site administrator, %ADMIN%, with any questions or concerns.

Thank you,
- %CONFSHORTNAME% Submissions\n"),

     "proposereview" =>
     array("[%CONFSHORTNAME%] Proposed reviewer for paper #%NUMBER%",
	   "Greetings,

%OTHERCONTACT% would like %CONTACT% to review %CONFNAME% paper #%NUMBER%.%IF(REASON)%  They supplied this note: %REASON%%ENDIF%

Visit the assignment page to approve or deny the request.

       Title: %TITLE%
     Authors: %OPT(AUTHORS)%
  Paper site: %URL%/assign%PHP%?p=%NUMBER%

- %CONFSHORTNAME% Submissions\n"),

     "denyreviewrequest" =>
     array("[%CONFSHORTNAME%] Proposed reviewer for paper #%NUMBER% denied",
	   "Dear %NAME%,

Your proposal that %OTHERCONTACT% review %CONFNAME% paper #%NUMBER% has been denied by an administrator.  You may want to propose someone else.

       Title: %TITLE%
     Authors: %OPT(AUTHORS)%
  Paper site: %URL%/paper%PHP%?p=%NUMBER%

Contact the site administrator, %ADMIN%, with any questions or concerns.

Thank you,
- %CONFSHORTNAME% Submissions\n"),

     "refusereviewrequest" =>
     array("[%CONFSHORTNAME%] Review request for paper #%NUMBER% refused",
	   "Dear %NAME%,

%OTHERCONTACT% cannot complete the review of %CONFNAME% paper #%NUMBER% that you requested.  %IF(REASON)%They gave the reason \"%REASON%\".  %ENDIF%You may want to find an alternate reviewer.

       Title: %TITLE%
     Authors: %OPT(AUTHORS)%
  Paper site: %URL%/paper%PHP%?p=%NUMBER%

- %CONFSHORTNAME% Submissions\n"),

     "authorwithdraw" =>
     array("[%CONFSHORTNAME%] Withdrawn paper #%NUMBER% %TITLEHINT%",
	   "Dear %NAME%,

An author of %CONFNAME% paper #%NUMBER% has withdrawn the paper from consideration.  The paper will not be reviewed.

       Title: %TITLE%
     Authors: %OPT(AUTHORS)%
  Paper site: %URL%/paper%PHP%?p=%NUMBER%

Contact the site administrator, %ADMIN%, with any questions or concerns.

Thank you,
- %CONFSHORTNAME% Submissions\n"),

     "adminwithdraw" =>
     array("[%CONFSHORTNAME%] Withdrawn paper #%NUMBER% %TITLEHINT%",
	   "Dear %NAME%,

%CONFNAME% paper #%NUMBER% has been withdrawn from consideration and will not be reviewed.

       Title: %TITLE%
     Authors: %OPT(AUTHORS)%
  Paper site: %URL%/paper%PHP%?p=%NUMBER%

%IF(REASON)%The paper was withdrawn by an administrator, who provided the following reason: %REASON%%ELSE%The paper was withdrawn by an administrator.%ENDIF%

Contact the site administrator, %ADMIN%, with any questions or concerns.

Thank you,
- %CONFSHORTNAME% Submissions\n"),     
     
     "withdrawreviewer" =>
     array("[%CONFSHORTNAME%] Withdrawn paper #%NUMBER% %TITLEHINT%",
	   "Dear %NAME%,

%CONFSHORTNAME% paper #%NUMBER%, which you reviewed or have been assigned to review, has been withdrawn from consideration for the conference.

Authors can voluntarily withdraw a submission at any time, as can the chair.  %REASON%

       Title: %TITLE%
     Authors: %OPT(AUTHORS)%
  Paper site: %URL%/paper%PHP%?p=%NUMBER%

You are not expected to complete your review; in fact the system will not allow it unless the paper is revived.

Contact the site administrator, %ADMIN%, with any questions or concerns.

- %CONFSHORTNAME% Submissions\n"),

     "deletepaper" =>
     array("[%CONFSHORTNAME%] Deleted paper #%NUMBER% %TITLEHINT%",
	   "Dear %NAME%,

Your %CONFNAME% paper #%NUMBER% has been removed from the submission database by an administrator.  This is usually done to remove duplicate papers.  %IF(REASON)%The following reason was provided for deleting the paper: %REASON%%ENDIF%

       Title: %TITLE%
     Authors: %OPT(AUTHORS)%

Contact the site administrator, %ADMIN%, with any questions or concerns.

- %CONFSHORTNAME% Submissions\n"),

     "reviewsubmit" =>
     array("[%CONFSHORTNAME%] Submitted review #%REVIEWNUMBER% %TITLEHINT%",
	   "Dear %NAME%,

Review #%REVIEWNUMBER% for %CONFNAME% paper #%NUMBER% has been submitted.  The review is available at the paper site.

  Paper site: %URL%/paper%PHP%?p=%NUMBER%
       Title: %TITLE%
     Authors: %OPT(AUTHORS)%
   Review by: %OPT(REVIEWAUTHOR)%

Contact the site administrator, %ADMIN%, with any questions or concerns.

- %CONFSHORTNAME% Submissions\n"),

     "reviewupdate" =>
     array("[%CONFSHORTNAME%] Updated review #%REVIEWNUMBER% %TITLEHINT%",
	   "Dear %NAME%,

Review #%REVIEWNUMBER% for %CONFNAME% paper #%NUMBER% has been updated.  The review is available at the paper site.

  Paper site: %URL%/paper%PHP%?p=%NUMBER%
       Title: %TITLE%
     Authors: %OPT(AUTHORS)%
   Review by: %OPT(REVIEWAUTHOR)%

Contact the site administrator, %ADMIN%, with any questions or concerns.

- %CONFSHORTNAME% Submissions\n"),

     "acceptnotify" =>
     array("[%CONFSHORTNAME%] Accepted paper #%NUMBER% %TITLEHINT%",
	   "Dear %NAME%,

The %CONFNAME% program committee is delighted to inform you that your paper #%NUMBER% has been accepted to appear in the conference.

       Title: %TITLE%
     Authors: %OPT(AUTHORS)%
  Paper site: %URL%/paper%PHP%?p=%NUMBER%

Your paper was one of %NUMACCEPTED% accepted out of %NUMSUBMITTED% submissions.  Congratulations!

Reviews and comments on your paper are appended to this email.  The submissions site also has the paper's reviews and comments, as well as more information about review scores.  To sign in, either click the link below or paste it into your web browser's location field.

%LOGINURL%

Contact the site administrator, %ADMIN%, with any questions or concerns.

- %CONFSHORTNAME% Submissions

%REVIEWS%
%COMMENTS%\n"),

     "rejectnotify" =>
     array("[%CONFSHORTNAME%] Rejected paper #%NUMBER% %TITLEHINT%",
	   "Dear %NAME%,

The %CONFNAME% program committee is sorry to inform you that your paper #%NUMBER% was rejected, and will not appear in the conference.

       Title: %TITLE%
     Authors: %OPT(AUTHORS)%
  Paper site: %URL%/paper%PHP%?p=%NUMBER%

%NUMACCEPTED% papers were accepted out of %NUMSUBMITTED% submissions.

Reviews and comments on your paper are appended to this email.  The submissions site also has the paper's reviews and comments, as well as more information about review scores.  To sign in, either click the link below or paste it into your web browser's location field.

%LOGINURL%

Contact the site administrator, %ADMIN%, with any questions or concerns.

- %CONFSHORTNAME% Submissions

%REVIEWS%
%COMMENTS%\n"),

     "commentnotify" =>
     array("[%CONFSHORTNAME%] Comment for #%NUMBER% %TITLEHINT%",
	   "A comment for %CONFNAME% paper #%NUMBER% is available as shown below.  For the most up-to-date comments, or to turn off comment emails, see the paper site.

  Paper site: %URL%/paper%PHP%?p=%NUMBER%

Contact the site administrator, %ADMIN%, with any questions or concerns.

- %CONFSHORTNAME% Submissions

%COMMENTS%\n"),

     "responsenotify" =>
     array("[%CONFSHORTNAME%] Response for #%NUMBER% %TITLEHINT%",
	   "The authors' response for %CONFNAME% paper #%NUMBER% is available as shown below.  The authors may still update their response; for the most up-to-date version, or to turn off comment emails, see the paper site.

  Paper site: %URL%/paper%PHP%?p=%NUMBER%

Contact the site administrator, %ADMIN%, with any questions or concerns.

- %CONFSHORTNAME% Submissions

%COMMENTS%\n"),
     
     "genericmailtool" =>
     array("[%CONFSHORTNAME%] Paper #%NUMBER% %TITLEHINT%",
	   "Dear %NAME%,

Your message here.

       Title: %TITLE%
  Paper site: %URL%/paper%PHP%?p=%NUMBER%

To sign in to the submissions site, either click the link below or paste it into your web browser's location field.

%LOGINURL%

Contact the site administrator, %ADMIN%, with any questions or concerns.

- %CONFSHORTNAME% Submissions\n"),

     "reviewremind" =>
     array("[%CONFSHORTNAME%] Review reminder for paper #%NUMBER% %TITLEHINT%",
	   "Dear %NAME%,

This is a reminder to finish your review for %CONFNAME% paper #%NUMBER%. %IF(REVIEWDEADLINE)% Reviews are requested by %REVIEWDEADLINE%. %ENDIF% If you are unable to complete the review, please \"refuse\" the review using the site or contact the person who requested the review directly.

       Title: %TITLE%
     Authors: %OPT(AUTHORS)%
  Paper site: %URL%/paper%PHP%?p=%NUMBER%

To sign in to the submissions site, either click the link below or paste it into your web browser's location field.

%LOGINURL%

Thank you for your help -- we appreciate that reviewing is hard work.

Contact the site administrator, %ADMIN%, with any questions or concerns.

- %CONFSHORTNAME% Submissions\n"),

     "myreviewremind" =>
     array("[%CONFSHORTNAME%] Review reminder for paper #%NUMBER% %TITLEHINT%",
	   "Dear %NAME%,

This is a reminder from %OTHERCONTACT% to finish your review for %CONFNAME% paper #%NUMBER%. %IF(REVIEWDEADLINE)% Reviews are requested by %REVIEWDEADLINE%. %ENDIF% If you are unable to complete the review, please \"refuse\" the review using the site or contact %OTHERNAME% directly.

       Title: %TITLE%
     Authors: %OPT(AUTHORS)%
  Paper site: %URL%/paper%PHP%?p=%NUMBER%

To sign in to the submissions site, either click the link below or paste it into your web browser's location field.

%LOGINURL%

Thank you for your help -- we appreciate that reviewing is hard work.

Contact the site administrator, %ADMIN%, with any questions or concerns.

- %CONFSHORTNAME% Submissions\n")

);


class Mailer {

    var $row;
    var $contact;
    var $otherContact;
    var $ifstack;
    var $text;
    var $textstart;
    var $hideSensitive;
    var $hideReviews;
    var $reason;
    var $rrow;
    var $reviewNumber;
    var $commentId;
    var $statistics;
    var $width;

    function Mailer($row, $contact, $otherContact = null, $rest = array()) {
	$this->row = $row;
	$this->contact = $contact;
	$this->otherContact = $otherContact;
	$this->ifstack = array();
	$this->text = "";
	$this->textstart = 0;
	$this->hideSensitive = defval($rest, "hideSensitive", false);
	$this->reason = defval($rest, "reason", "");
	$this->rrow = defval($rest, "rrow", null);
	$this->reviewNumber = defval($rest, "reviewNumber", "");
	$this->commentId = defval($rest, "commentId", null);
	$this->hideReviews = defval($rest, "hideReviews", false);
	$this->statistics = null;
	$this->width = 75;
    }
    
    function _pushIf($yes) {
	if ($yes !== false && $yes !== true && $yes !== null)
	    $yes = (bool) $yes;
	if ($yes === true || $yes === null)
	    array_push($this->ifstack, $yes);
	else
	    array_push($this->ifstack, $this->text);
    }

    function _popIf() {
	if (count($this->ifstack) == 0)
	    return null;
	else if (($pop = array_pop($this->ifstack)) === true || $pop === null)
	    return $pop;
	else {
	    $this->text = $pop;
	    if ($this->textstart > strlen($this->text))
		$this->textstart = strlen($this->text);
	    return false;
	}
    }
    
    function expandvar($what, $isbool = false) {
	global $Conf, $ConfSiteSuffix;
	$len = strlen($what);

	if ($len > 6 && substr($what, 0, 4) == "%IF(" && substr($what, $len - 2) == ")%") {
	    $inner = "%" . substr($what, 4, $len - 6) . "%";
	    $this->_pushIf(($yes = $this->expandvar($inner, true)));
	    return ($yes === null ? $what : "");
	} else if ($what == "%ELSE%") {
	    $yes = $this->_popIf();
	    $this->_pushIf($yes === null ? $yes : !$yes);
	    return ($yes === null ? $what : "");
	} if ($what == "%ENDIF%") {
	    $yes = $this->_popIf();
	    return ($yes === null ? $what : "");
	}
	
	if ($len > 7 && substr($what, 0, 5) == "%OPT(" && substr($what, $len - 2) == ")%") {
	    $inner = "%" . substr($what, 5, $len - 7) . "%";
	    if ($isbool)
		return $this->expandvar($inner, true);
	    else if (($yes = $this->expandvar($inner, true)))
		return $this->expandvar($inner, false);
	    else
		return ($yes === null ? $what : "");
	}
	
	if ($len > 10 && substr($what, 0, 8) == "%URLENC(" && substr($what, $len - 2) == ")%") {
	    $inner = "%" . substr($what, 8, $len - 10) . "%";
	    $yes = $this->expandvar($inner, true);
	    if ($isbool)
		return $yes;
	    else if ($yes)
		return urlencode($this->expandvar($inner, false));
	    else
		return ($yes === null ? $what : "");
	}

	if ($what == "%REVIEWDEADLINE%") {
	    $row_reviewType = isset($this->row->reviewType) ? $this->row->reviewType : null;
	    if ($row_reviewType <= 0 && $Conf->setting("pcrev_soft") != $Conf->setting("extrev_soft")) {
		if ($isbool && ($Conf->setting("pcrev_soft") > 0) == ($Conf->setting("extrev_soft") > 0))
		    return $Conf->setting("pcrev_soft") > 0;
		else
		    return ($isbool ? null : $what);
	    }
	    $what = "%DEADLINE(" . ($row_reviewType >= REVIEW_PC ? "pcrev_soft" : "extrev_soft") . ")%";
	    $len = strlen($what);
	}
	if ($len > 12 && substr($what, 0, 10) == "%DEADLINE(" && substr($what, $len - 2) == ")%") {
	    $inner = substr($what, 10, $len - 12);
	    if ($isbool)
		return $Conf->setting($inner) > 0;
	    else
		return $Conf->printableTimeSetting($inner);
	}

	if ($what == "%CONFNAME%") {
	    $t = $Conf->longName;
	    if ($Conf->shortName && $Conf->shortName != $Conf->longName)
		$t .= " ($Conf->shortName)";
	    return $t;
	}
	if ($what == "%CONFSHORTNAME%")
	    return $Conf->shortName;
	if ($what == "%CONFLONGNAME%")
	    return $Conf->longName;
	if ($what == "%ADMIN%")
	    return "$Conf->contactName <$Conf->contactEmail>";
	if ($what == "%URL%")
	    return $Conf->paperSite;
	if ($what == "%PHP%")
	    return $ConfSiteSuffix;
	if (($what == "%NUMACCEPTED%" || $what == "%NUMSUBMITTED%")
	    && $this->statistics === null) {
	    $this->statistics = array(0, 0);
	    $result = $Conf->q("select outcome, count(paperId) from Paper where timeSubmitted>0 group by outcome");
	    while (($row = edb_row($result))) {
		$this->statistics[0] += $row[1];
		if ($row[0] > 0)
		    $this->statistics[1] += $row[1];
	    }
	}
	if ($what == "%NUMSUBMITTED%")
	    return $this->statistics[0];
	if ($what == "%NUMACCEPTED%")
	    return $this->statistics[1];
	
	if ($what == "%OTHERCONTACT%" && $this->otherContact)
	    return contactText($this->otherContact);
	if ($what == "%OTHERNAME%" && $this->otherContact)
	    return contactNameText($this->otherContact);
	if ($what == "%OTHEREMAIL%" && $this->otherContact)
	    return $this->otherContact->email;

	// if no contact, this is a pre-expansion
	if (!$this->contact)
	    return ($isbool ? null : $what);

	if ($what == "%LOGINURL%")
	    return $Conf->paperSite . "/?email=" . urlencode($this->contact->email) . "&password=" . urlencode($this->hideSensitive ? "HIDDEN" : $this->contact->password);
	if ($what == "%LOGINURLPARTS%")
	    return "email=" . urlencode($this->contact->email) . "&password=" . urlencode($this->hideSensitive ? "HIDDEN" : $this->contact->password);
	if ($what == "%EMAIL%")
	    return $this->contact->email;
	if ($what == "%PASSWORD%")
	    return ($this->hideSensitive ? "HIDDEN" : $this->contact->password);
	if ($what == "%CONTACT%")
	    return contactText($this->contact);
	if ($what == "%NAME%")
	    return contactNameText($this->contact);
	if ($what == "%FIRST%")
	    return $this->contact->firstName;
	if ($what == "%LAST%")
	    return $this->contact->lastName;

	if ($what == "%REASON%")
	    return $this->reason;

	// rest is only there if we have a real paper
	if (!$this->row || defval($this->row, "paperId") <= 0)
	    return ($isbool ? false : $what);

	if ($what == "%TITLE%")
	    return $this->row->title;
	if ($what == "%TITLEHINT%") {
	    if (($tw = titleWords($this->row->title)))
		return "\"$tw\"";
	    else
		return "";
	}
	if ($what == "%NUMBER%" || $what == "%PAPER%")
	    return $this->row->paperId;
	if ($what == "%REVIEWNUMBER%")
	    return $this->reviewNumber;
	if ($what == "%AUTHOR%" || $what == "%AUTHORS%") {
	    if (paperBlind($this->row)
		&& defval($this->row, "conflictType") < CONFLICT_AUTHOR
		&& defval($this->contact, "conflictType") < CONFLICT_AUTHOR)
		return ($isbool ? false : "Hidden for blind review");
	    cleanAuthor($this->row);
	    return rtrim($this->row->authorInformation);
	}

	if ($what == "%SHEPHERD%" || $what == "%SHEPHERDNAME%"
	    || $what == "%SHEPHERDEMAIL%") {
	    $pc = pcMembers();
	    if (defval($this->row, "shepherdContactId") <= 0
		|| !defval($pc, $this->row->shepherdContactId, null))
		return ($isbool ? false : "(no shepherd assigned)");
	    $shep = $pc[$this->row->shepherdContactId];
	    if ($what == "%SHEPHERD%")
		return contactText($shep);
	    else if ($what == "%SHEPHERDNAME%")
		return contactNameText($shep);
	    else
		return $shep->email;
	}

	if ($what == "%REVIEWAUTHOR%" && $this->otherContact) {
	    if (reviewBlind($this->rrow)
		&& defval($this->contact, "privChair") <= 0
		&& (!isset($this->contact->canViewReviewerIdentity)
		    || !$this->contact->canViewReviewerIdentity($this->row, $this->rrow, $Conf, true)))
		return ($isbool ? false : "Hidden for blind review");
	    return contactText($this->otherContact);
	}
	
	if ($what == "%REVIEWS%")
	    return $this->getReviews($this->contact, false);
	if ($what == "%COMMENTS%")
	    return $this->getComments($this->contact);

	return ($isbool ? false : $what);
    }

    function getReviews($contact, $finalized) {
	global $Conf, $Me, $rf;
	if ($this->hideReviews)
	    return "[Reviews are hidden since you have incomplete reviews of your own.]";

	$result = $Conf->qe("select Paper.title, PaperReview.*,
		ContactInfo.firstName, ContactInfo.lastName, ContactInfo.email,
		conflictType, ContactReview.reviewType as myReviewType
 		from PaperReview
		join Paper using (paperId)
		join ContactInfo on (ContactInfo.contactId=PaperReview.contactId)
		left join PaperConflict on (PaperConflict.contactId=$contact->contactId and PaperConflict.paperId=PaperReview.paperId)
		left join PaperReview as ContactReview on (ContactReview.contactId=$contact->contactId and ContactReview.paperId=PaperReview.paperId)
		where PaperReview.paperId=" . $this->row->paperId . " order by reviewOrdinal", "while retrieving reviews");
	if (edb_nrows($result)) {
	    $text = "";
	    while (($row = edb_orow($result)))
		if ($row->reviewSubmitted)
		    $text .= $rf->prettyTextForm($row, $row, $contact, $Conf, true) . "\n";
	    return $text;
	} else
	    return "";
    }

    function getComments($contact) {
	global $Conf, $rf;
	if ($this->hideReviews)
	    return "";

	$q = "select PaperComment.*,
 		ContactInfo.firstName as reviewFirstName,
		ContactInfo.lastName as reviewLastName,
		ContactInfo.email as reviewEmail,
		PaperConflict.conflictType
		from PaperComment
 		join ContactInfo on (ContactInfo.contactId=PaperComment.contactId)
		left join PaperConflict on (PaperConflict.paperId=PaperComment.paperId and PaperConflict.contactId=PaperComment.contactId)";
	if (is_array($this->commentId))
	    $q .= "\n\t\twhere PaperComment.commentId in (" . join(", ", $this->commentId) . ")";
	else if ($this->commentId)
	    $q .= "\n\t\twhere PaperComment.commentId=$this->commentId";
	else
	    $q .= "\n\t\twhere PaperComment.paperId=" . $this->row->paperId;
	$text = "";
	// save old au_seerev setting, and reset it so authors can see them.
	$old_au_seerev = $Conf->setting("au_seerev");
	$Conf->settings["au_seerev"] = AU_SEEREV_ALWAYS;
	$crows = $Conf->commentRows($q . "\n\t\torder by commentId");
	foreach ($crows as $crow)
	    if ($contact->canViewComment($this->row, $crow, $Conf, $whyNot, true))
		$text .= $rf->prettyTextComment($this->row, $crow, $contact, $Conf) . "\n";
	$Conf->settings["au_seerev"] = $old_au_seerev;
	return $text;
    }

    function expand($text) {
	if (is_array($text)) {
	    $a = array();
	    foreach ($text as $t)
		$a[] = $this->expand($t);
	    return $a;
	}
	
	$lines = explode("\n", $text);
	if (strlen($text) && $text[strlen($text) - 1] == "\n")
	    array_pop($lines);
	$this->text = "";
	for ($i = 0; $i < count($lines); $i++) {
	    $line = rtrim($lines[$i]);
	    if ($line == "")
		$this->text .= "\n";
	    else if (preg_match('/^%[\w()]+%$/', $line)) {
		if (($m = $this->expandvar($line, false)) != "")
		    $this->text .= $m . "\n";
	    } else if (preg_match('/^([ \t][ \t]*.*?: )(%OPT\([\w()]+\)%)$/', $line, $m)) {
		
		if (($yes = $this->expandvar($m[2], true)))
		    $this->text .= wordWrapIndent($this->expandvar($m[2]), $m[1], tabLength($m[1], true), $this->width) . "\n";
		else if ($yes === null)
		    $this->text .= $line . "\n";
	    } else if (preg_match('/^([ \t][ \t]*.*?: )(%[\w()]+%)$/', $line, $m))
		$this->text .= wordWrapIndent($this->expandvar($m[2]), $m[1], tabLength($m[1], true), $this->width) . "\n";
	    else if (preg_match('/^([ \t][ \t]*.*?: )(%URL%\S*)$/', $line, $m)) {
		// do not wrap lines that start with %URL%
		$this->textstart = strlen($this->text);
		while (preg_match('/^(.*?)(%[\w()]+%)(.*)$/s', $line, $m)) {
		    $this->text .= $m[1];
		    $this->text .= $this->expandvar($m[2]);
		    $line = $m[3];
		}
		$this->text .= $line . "\n";
	    } else if (strpos($line, '%') !== false) {
		$this->textstart = strlen($this->text);
		while (preg_match('/^(.*?)(%[\w()]+%)(.*)$/s', $line, $m)) {
		    $this->text .= $m[1];
		    $this->text .= $this->expandvar($m[2]);
		    $line = $m[3];
		}
		$this->text .= $line;
		$this->text = substr($this->text, 0, $this->textstart) . wordWrapIndent(substr($this->text, $this->textstart), "", 0, $this->width) . "\n";
	    } else
		$this->text .= wordWrapIndent($line, "", 0, $this->width) . "\n";
	}
	return $this->text;
    }

    function expandTemplate($templateName, $expand = false, $default = false) {
	global $Conf, $mailTemplates;
	$m = $mailTemplates[$templateName];
	if (!$default && ($t = $Conf->settingText("mailsubj_" . $templateName)) !== false)
	    $m[0] = $t;
	if (!$default && ($t = $Conf->settingText("mailbody_" . $templateName)) !== false)
	    $m[1] = $t;
	if ($expand) {
	    $m[0] = $this->expand($m[0]);
	    $m[1] = $this->expand($m[1]);
	}
	return $m;
    }

    function prepareToSend($template, $row, $contact, $otherContact = null, $rest = array()) {
	global $Conf, $mailTemplates;

	if (is_string($template) && $template[0] == "@")
	    $template = self::expandTemplate(substr($template, 1));

	if (!isset($rest["emailTo"]) || !$rest["emailTo"])
	    $emailTo = $contact;
	else if (is_string($rest["emailTo"]))
	    $emailTo = (object) array("email" => $rest["emailTo"]);
	else
	    $emailTo = $rest["emailTo"];
	if (!$emailTo || !$emailTo->email)
	    return $Conf->errorMsg("no email in Mailer::send");
	$headers = "MIME-Version: 1.0\r\nContent-Type: text/plain; charset=utf-8\r\n" . defval($rest, "headers", "");
	if (substr($headers, strlen($headers) - 2) != "\r\n")
	    $headers .= "\r\n";
	$headers .= "To: " . contactEmailTo($emailTo) . "\r\n";
	
	$mailer = new Mailer($row, $contact, $otherContact, $rest);
	$m = $mailer->expand($template);
	$m[0] = rtrim($m[0]);
	if (preg_match("/[\x80-\xFF]/", $m[0]))
	    $m[0] = Mailer::mimeHeaderQuote($m[0], 73 - 21, false);
	$m["allowEmail"] = $Conf->allowEmailTo($emailTo->email);
	$m["to"] = $emailTo->email;
	$m["headers"] = $headers;
	return $m;
    }

    function sendPrepared($preparation) {
	global $Conf, $Opt;
	if ($preparation["allowEmail"]) {
	    // set sendmail parameters
	    $extra = defval($Opt, "sendmailParam", "");
	    if (isset($Opt["emailSender"])) {
		@ini_set("sendmail_from", $Opt["emailSender"]);
		if (!isset($Opt["sendmailParam"]))
		    $extra = "-f" . escapeshellarg($Opt["emailSender"]);
	    }

	    // try to extract a valid To: header
	    $to = $preparation["to"];
	    $headers = $preparation["headers"];
	    if (($topos = strpos($headers, "\r\nTo: ")) !== false
		&& ($nlpos = strpos($headers, "\r\n", $topos + 1)) !== false
		&& ($nlpos + 2 == strlen($headers) || !ctype_space($headers[$nlpos + 2]))) {
		$to = substr($headers, $topos + 6, $nlpos - $topos - 6);
		$headers = substr($headers, 0, $topos) . substr($headers, $nlpos);
	    } else if ($topos !== false)
		$to = "";

	    return mail($to, $preparation[0], $preparation[1], $headers . "From: $Conf->emailFrom", $extra);
	} else if (!$Opt["sendEmail"])
	    return $Conf->infoMsg("<pre>" . htmlspecialchars("To: " . $preparation["to"] . "\nSubject: $preparation[0]\n" . $preparation["headers"] . "\n$preparation[1]") . "</pre>");
    }
    
    function send($template, $row, $contact, $otherContact = null, $rest = array()) {
	$preparation = self::prepareToSend($template, $row, $contact, $otherContact, $rest);
	if ($preparation)
	    self::sendPrepared($preparation);
    }

    function sendContactAuthors($template, $row, $otherContact = null, $rest = array()) {
	global $Conf, $Me, $mailTemplates;

	$result = $Conf->qe("select ContactInfo.contactId,
		firstName, lastName, email, password, conflictType, 0 as myReviewType
		from ContactInfo join PaperConflict using (contactId)
		where paperId=$row->paperId and conflictType>=" . CONFLICT_AUTHOR . "
		group by ContactInfo.contactId", "while looking up contact authors to send email");

	// must set the current conflict type in $row for each contact
	$old_conflictType = $row->conflictType;
	
	$contacts = array();
	while (($contact = edb_orow($result))) {
	    $row->conflictType = $contact->conflictType;
	    Mailer::send($template, $row, Contact::makeMinicontact($contact), $otherContact, $rest);
	    $contacts[] = contactHtml($contact);
	}

	$row->conflictType = $old_conflictType;
	if ($row->conflictType < CONFLICT_AUTHOR && count($contacts) && $Me->privChair) {
	    $endmsg = (isset($rest["infoMsg"]) ? ", " . $rest["infoMsg"] : ".");
	    if (isset($rest["infoNames"]) && $Me->privChair)
		$contactsmsg = pluralx($contacts, "contact author") . ", " . commajoin($contacts);
	    else
		$contactsmsg = "contact author(s)";
	    $Conf->infoMsg("Sent email to paper #$row->paperId's $contactsmsg$endmsg");
	}
    }

    function sendReviewers($template, $row, $otherContact = null, $rest = array()) {
	global $Conf, $Me, $mailTemplates;

	$result = $Conf->qe("select ContactInfo.contactId,
		firstName, lastName, email, password,
		conflictType, reviewType as myReviewType
		from ContactInfo
		join PaperReview on (PaperReview.contactId=ContactInfo.contactId and PaperReview.paperId=$row->paperId)
		left join PaperConflict on (PaperConflict.contactId=ContactInfo.contactId and PaperConflict.paperId=$row->paperId)
		group by ContactInfo.contactId", "while looking up reviewers to send email");

	$rest["headers"] = "Cc: $Conf->contactName <$Conf->contactEmail>";

	// must set the current conflict type in $row for each contact
	$old_conflictType = $row->conflictType;
	
	$contacts = array();
	while (($contact = edb_orow($result))) {
	    $row->conflictType = $contact->conflictType;
	    Mailer::send($template, $row, Contact::makeMinicontact($contact), $otherContact, $rest);
	    $contacts[] = contactHtml($contact);
	}

	$row->conflictType = $old_conflictType;
	if ($row->conflictType < CONFLICT_AUTHOR && count($contacts) && $Me->privChair) {
	    $endmsg = (isset($rest["infoMsg"]) ? ", " . $rest["infoMsg"] : ".");
	    $Conf->infoMsg("Sent email to paper #$row->paperId's " . pluralx($contacts, "reviewer") . ", " . commajoin($contacts) . $endmsg);
	}
    }

    function mimeHeaderQuote($text, $len, $stringent) {
    	// minimum length
	if ($len < 30)
	    $len = 30;
	// replace all special characters used by the encoder
	$text = str_replace(array('=',   '_',   '?',   ' '),
			    array('=3D', '=5F', '=3F', '_'), $text);
	// replace nonsafe characters
	if ($stringent)
	    $matcher = '#[^-0-9a-zA-Z!*+/=_]#';
	else
	    $matcher = '#[\x80-\xFF]#';
	preg_match_all($matcher, $text, $m, PREG_OFFSET_CAPTURE);
	$ntext = "";
	$last = 0;
	foreach ($m[0] as $mx) {
	    $ntext .= substr($text, $last, $mx[1] - $last)
		. "=" . strtoupper(dechex(ord($mx[0])));
	    $last = $mx[1] + 1;
	}
	$ntext .= substr($text, $last);
	// create encoded-words
	$xpref = "";
	while (strlen($ntext) > $len) {
	    $z = $len;
	    if ($ntext[$z - 1] == '=')
		$z += 2;
	    else if ($ntext[$z - 2] == '=')
		$z += 1;
	    // skip intact UTF-8 characters
	    while ($z < strlen($ntext)
		   && $ntext[$z] == '='
		   && ($chr = hexdec(substr($ntext, $z + 1, 2))) >= 128
		   && $chr < 192)
		$z += 3;
	    $xpref .= "=?utf-8?q?" . substr($ntext, 0, $z) . "?=\r\n ";
	    $ntext = substr($ntext, $z);
	}
	return $xpref . "=?utf-8?q?" . $ntext . "?=";
    }

    function mimeHeaderUnquote($text) {
	if (strlen($text) > 2 && $text[0] == '=' && $text[1] == '?') {
	    $out = '';
	    while (preg_match('/\A=\?utf-8\?q\?(.*?)\?=(\r?\n )?/', $text, $m)) {
		$f = str_replace('_', ' ', $m[1]);
		$out .= preg_replace('/=([0-9A-F][0-9A-F])/e',
				     'chr(hexdec("\1"))', $f);
		$text = substr($text, strlen($m[0]));
	    }
	    return $out . $text;
	} else
	    return $text;
    }
    
}
