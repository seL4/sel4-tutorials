#!/usr/bin/env perl
#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(NICTA_BSD)
#

use warnings;
use strict;

my $prio = 98;
my $order = 0;
my $length = 0;
my %state = ();

my @overhead = ();

my $run_length = 10000;

#initial run is the calibration loop
exit -1 if not (<> =~ m/^Processing set 0\.\.\./); #match the first set
exit -1 if not (<> =~ m/^\s*(.*):\s+(\d+), (\d+), (\d+)/); #match its only member
@overhead = ($2, $3, $4);
exit -1 if not ($1 =~ m/measure_bench_overhead/); #make sure it's a bench overhead measurement
@overhead = map { int($_ / $run_length) } @overhead;

print "Priority,Length,Benchmark,CCNT,PMC0,PMC1\n";

#process IPC data
while (<>) {
	if (m/^Processing set \d+\.\.\./) {
		$prio = 98;
		$order = 0;
		$length = 0;
		%state = ();
		next;
	}
	elsif (m/^\s*(.*):\s+(\d+), (\d+), (\d+)/) {
		if (exists $state{$1}) { #we've come across a state we've seen before -- reset
			%state = ();
			$state{$1} = 1;

			if ($length == 10) {
				if($order == 1) {
					$prio++;
					$order = 0;
					$length = 0;
				} else {
					$order++;
					$length = 0;
				}
			} else {
				$length++;
			}
		} else {
			$state{$1} = 1;
		}

		printf "$prio %s 100,%d,$1,%d,%d,%d\n", $order ? "<-" : "->", $length, int( ($2 - (2 * $overhead[0]))/$run_length ), int( ($3 - (2 * $overhead[1]))/$run_length ), int( ($4 - (2 * $overhead[2]))/$run_length );
	}
	else {
		print STDERR "invalid line: ";
		print STDERR;
	}
}
