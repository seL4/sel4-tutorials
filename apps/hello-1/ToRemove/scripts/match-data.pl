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
use Switch;

my $setcount = -1;
my @sets = [];

while(<>) {
	if (m/^SB&#Sample, Cycles, PMC0, PMC1, FN Mode, FN Name, FN line, Extra\s*$/) {
		$setcount++;
#		printf "new dataset %d\n", $setcount;
		$sets[$setcount] = [];
	}
	elsif(m/^SB&\s*(\d+)\/(\d+)\s+-\s+(\d+), (\d+), (\d+), (\w+), (\w+), (\d+), ([^[:cntrl:]]*)[[:cntrl:]]*$/) {
#apparently there are random control characters at the ends of lines. yay.
		${$sets[$setcount]}[$1] = {
			'Time' => $3,
			'PMC0' => $4,
			'PMC1' => $5,
			'Mode' => $6,
			'Func' => $7,
			'Desc' => $8
		};
#		printf "datapoint %d/%d: %d %s-%s\n", $1 + 1, $2, $3, $4, $5;
	}
}

#print "\nData aggregation complete.\n\n";
#
#printf "setcount = %d\n", $setcount + 1;
#print "sets = @sets\n";
#foreach (@sets) {
#	print;
#	print ":\n";
#	foreach(@{$_}) {
#		print "\t";
#		print join("\t", %{$_});
#		print "\n";
#	}
#}
#
#print "\nBegin data processing.\n\n";

#datasets go: overhead, send/wait, send/replywait, send/reply+wait, [repeat in reverse]

#total dataset
my %phasedata = ();

for(my $set = 0; $set < (scalar @sets); $set++) {
	print "Processing set $set...\n";

	foreach(@{$sets[$set]}) {
		my $datapoint = $_;
		switch(${$datapoint}{'Mode'}) {
			case "enter" {
				my $starttime = ${$datapoint}{'Time'};
				my $phase = ${$datapoint}{'Func'};

				$phasedata{$phase} = \%{$datapoint};

				#print "$phase $starttime\n";
			}
			case "exit" {
				my $phase = ${$datapoint}{'Func'};

				if(defined $phasedata{$phase}) {
					#my $timetaken = ${$datapoint}{'Time'} - $phasedata{$phase}{'Time'};
					#if($timetaken < 0) {
					#	$timetaken = $timetaken + (2 ** 32);
					#}
					#print "$phase: $timetaken\n";
					my @perfdata = map { (${$datapoint}{$_} - $phasedata{$phase}{$_}) % (2 ** 32) } ('Time', 'PMC0', 'PMC1');
					print "$phase: ";
					print join ", ", @perfdata;
					print "\n";
					delete $phasedata{$phase};
				} else {
					print "phase $phase never started...\n";
					print join("\t", %{$datapoint});
					print "\n";
					exit -1;
				}

#				print "$phase: ";
#				print join(" ", %{$phasedata{$phase}});
#				print "\n";
			}
#			case "none" {
#			}
#			else {
#				print "invalid mode in datapoint: ";
#				printf "\t%s\n", ${$datapoint}{'Mode'};
#				printf "\t%s\n", ${$datapoint}{'Func'};
#				printf "\t%s\n", ${$datapoint}{'Desc'};
#				printf "\t%d\n", ${$datapoint}{'Time'};
#				printf "\n";
#				exit -1;
#			}
		}
	}
}
