<?php
// options.inc -- HotCRP conference options
// Placed in the public domain

/*****************************************************************************
 * HotCRP User Settings
 * Required for all installations.
 * Set up this file before running HotCRP for the first time.
 *****************************************************************************/

// CONFERENCE NAMES
//
//   dbName          Database name.  Use the name you supplied to
//                   "Code/createdb.sh".  NO SPACES ALLOWED.
//   dbUser	     Database user name.  Defaults to $Opt['dbName'].
//   dbPassword      Password for database user.  Defaults to $Opt['dbName'].
//   shortName       Short name of the conference.  Examples: "SIGCOMM 2007",
//                   "HotNets V".
//   longName        Longer name of the conference.  Example: "ACM SIGCOMM
//                   2007 Conference".
//   downloadPrefix  Prefix for downloaded files, such as papers.  Should
//                   end in a dash.  Example: "hotnets5-".
//   If your mysql database isn't running on localhost, see $Opt['dsn'] below.

$Opt["dbName"] = "orig";
$Opt["dbPassword"] = "orig";
$Opt["shortName"] = "YourConf I";
$Opt["longName"] = "Your Conference I";
$Opt["downloadPrefix"] = "yourconf1-";


// EMAIL
//
//   contactName     Full name for site administrator (the person to contact
//                   about site problems).  Usually the PC chair(s).
//   contactEmail    Email address for site administrator.
//   sendEmail       Boolean.  If false, HotCRP will send no email.  This
//                   should almost always be set to true.
//   emailFrom       "From:" address for email sent by HotCRP.  Should be a
//                   plausible email address for mail originating at the
//                   conference server, so things like SPF don't mistake
//                   the mail for spam.
//   emailSender     Envelope sender address for email sent by HotCRP.  Usually
//                   defaults to something like "www-data@yourservername".
//                   This email address should be connected to a valid mailbox
//                   or certain spam filters will reject email from HotCRP.
//   sendmailParam   Extra parameters to be passed to PHP's mail() function.
//                   If your system's mailer is not sendmail, you may need to
//                   change the default value; see Code/mailtemplate.inc,
//                   sendPrepared().

$Opt["contactName"] = "Your Name";
$Opt["contactEmail"] = "your@name.edu";
$Opt["sendEmail"] = true;
$Opt["emailFrom"] = "from@your.site.here.com";


// SUBMISSIONS
//
//   emailSubmissions  If this is non-zero, then it should be an email address.
//                   All submissions are MIME-encoded and sent to this address
//                   as a backup.  Note that this will increase server load.

$Opt["emailSubmissions"] = 0;


// URLS
//
//   conferenceSite  Conference site URL (e.g., CFP, registration).
//   paperSite       URL for this HotCRP installation.  Used in emails.

$Opt["conferenceSite"] = "http://your.site.here.com/";
$Opt["paperSite"] = "http://your.site.here.com/";


// OPTIONAL SETTINGS (the defaults are reasonable)
//
//   dsn             Database configuration information in the format
//                   "mysql://DBUSER:DBPASSWORD@DBHOST/DBNAME".
//                   The default is derived from $Opt["dbName"], assuming
//                   that DBUSER, DBPASSWORD, and DBNAME are all the same
//                   and that DBHOST is "localhost".
//                   Example: "mysql://HotNetsV:HotNetsV@localhost/HotNetsV".
//   sessionName     Internal name used to distinguish conference sessions
//                   running on the same server.  NO SPACES ALLOWED.  Defaults
//                   to $Opt["dbName"].
//   sessionLifetime Number of seconds a user may be idle before their session
//                   is garbage collected and they must log in again.  Defaults
//                   to 86400 (24 hours).  Should be less than or equal to the
//                   system-wide setting for 'session.gc_maxlifetime' in
//                   the PHP initialization file, 'php.ini'.
//   memoryLimit     Maximum amount of memory a PHP script can use.  Defaults
//                   to 128MB.
//   disablePDF      Set to disable PDF format submissions.
//   disablePS       Set to disable PostScript format submissions.
//   disableSHA1     Set to disable storing SHA-1 hashes of submissions.
//   dbDumpDir       Directory where user-requested database dumps are stored.
//   pdftohtml       Pathname to pdftohtml executable (used only by the "banal"
//                   paper format checker).
//   banalLimit      Limit on number of parallel paper format checker
//                   executions.  Defaults to 8.

$Opt["disablePS"] = true;
