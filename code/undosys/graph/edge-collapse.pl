#!/usr/bin/perl

sub print_buf {
    return unless defined $bufsrc;

    if ($buflo == $bufhi) {
	print "$bufsrc -> $bufdst [label=$buflo$bufcolor];\n";
    } else {
	print "$bufsrc -> $bufdst [label=\"$buflo--$bufhi\"$bufcolor];\n";
    }
    undef $bufsrc;
    undef $bufdst;
    undef $buflo;
    undef $bufhi;
    undef $bufcolor;
}

print "digraph G {\n";

while (<>) {
    next if /}$/;
    next if /{$/;

    if (not /->/) {
	print;
	next;
    }

    ($src, $arrow, $dst, $label) = split(/\s+/, $_);
    if (not $label =~ /\[label=(\d+)(,color=\w+)?\];/) {
	print;
	next;
    }
    $ts = $1;
    $color = $2;

    if (($src eq $bufsrc) &&
	($dst eq $bufdst) &&
	($color eq $bufcolor))
    {
	$buflo = $ts if $ts < $buflo;
	$bufhi = $ts if $ts > $bufhi;
    } else {
	&print_buf();

	$bufsrc = $src;
	$bufdst = $dst;
	$buflo = $ts;
	$bufhi = $ts;
	$bufcolor = $color;
    }
}

&print_buf();
print "}\n";

