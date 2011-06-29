<?php
$stderr = fopen ("php://stderr", 'w');
function perr ($s) {
  global $stderr;
  fwrite ($stderr, $s);
  fflush($stderr);
}

$stats = array ();
$active = array ();
function stats_begin ($key) {
  global $active;
  $active[$key] = gettimeofday(true);
}
function stats_end ($key) {
  global $active, $stats;
  if (!array_key_exists ($key, $stats))
    $stats[$key] = array(0, 0);
  $stats[$key][0] += (gettimeofday(true) - $active[$key]);
  $stats[$key][1]++;
}

function stats_report () {
  global $stats;
  foreach ($stats as $k => $dat) {
    $usec = 1000000 * ($dat[0] / $dat[1]);
    perr (sprintf ("%-15s %7d %10.3f\n", 
                   $k, $dat[1], $usec));
  }
}
function stats_reset () {
  global $stats, $active;
  $stats = array ();
  $active = array ();
}

?>
