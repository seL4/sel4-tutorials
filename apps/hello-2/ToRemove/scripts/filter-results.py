#!/usr/bin/python
#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(NICTA_BSD)
#

# 
# A simple script to extract the results that you care
# about from the benchmarking output into nicely parsed xml.
# Yes it is horrible
# and depends totally on the format of that output. 
#
#
# Usage:
# ./fitler-results file-with-results-to-select file-with-results
#
# Example entry for a result to select:
# 
# Name of result 98 -> 100 1 wait_func send_func
#
# Format of file-with-results-to-select:
#
#  $0    $1      $2      $3       $4       $5             $6
# (.*) ([0-9]+) (->|<-) ([0-9]+) ([0-9]+) ([A-Za-z_]+) ([A-Za-z_]+)
#
#$0: the name of this result (you invent it)
#$1 The first priority
#$2 the direction of the call 
#$3 the second priority
#$4 the length of the message
#$5 the name of the first priorities function
#$6 the name of the second priorities function
#
# Format of the file-with-results
#
# Entries should look like this (spaces don't matter):
# 
# 98 -> 100, Length 1:
#     wait_func:
#         CCNT: 3667 
#         PMC0: 0
#         PMC1: 47
#     send_func:
#         CCNT: 3667 
#         PMC0: 0
#         PMC1: 47
#
# Currently only the first result is extracted. 

import re
import sys
import pdb

selectionsFile = open(sys.argv[1], 'r')
results = open(sys.argv[2], 'r');

selectionRegexp = re.compile('(.*) ([0-9]+) (->|<-) ([0-9]+) ([0-9]+) ([A-Za-z_]+) ([A-Za-z_]+)')

class Selection:
    name = ""
    firstPriority = ""
    direction = ""
    secondPriority = ""
    length = 0
    firstFunction = ""
    secondFunction = ""

    def match(self, other):
        return (self.firstPriority == other.firstPriority and
                self.direction == other.direction and
                self.secondPriority == other.secondPriority and
                self.length == other.length and
                self.firstFunction == other.firstFunction and
                self.secondFunction == other.secondFunction)
		        

#this is the list of all of the results we want to select
selections = []

#parse the selections
for line in selectionsFile:
    match = selectionRegexp.match(line.rstrip())
    if match is None:
        print "Invalid selection on line {0} of file {1}".format(line, sys.argv[1]);
        exit

    selection = Selection()
    selection.name = match.group(1)
    selection.firstPriority = match.group(2)
    selection.direction = match.group(3)
    selection.secondPriority = match.group(4)
    selection.length = match.group(5)
    selection.firstFunction = match.group(6)
    selection.secondFunction = match.group(7)
   # pdb.set_trace()

    selections.append(selection)

print "<results>"

entryStartRegexp = re.compile('([0-9]+) (<-|->) ([0-9]+), Length ([0-9]+):')

# iterate through every result and look for our selections. Bail early if
# we find them all. 
#
# complexity of this loop is O(nxm) where n is the number of selections and
# m is the number of entries in the results file. 
line = results.readline()
while len(line) > 0 and len(selections) > 0:
    match = entryStartRegexp.match(line.rstrip())
    if match is not None:
        candidate = Selection()
        candidate.firstPriority = match.group(1)
        candidate.direction = match.group(2)
        candidate.secondPriority = match.group(3)
        candidate.length = match.group(4)
        candidate.firstFunction = results.readline().strip().strip(':')
        count = int(results.readline().strip().split(' ')[1])
        results.readline() #PMC0
        results.readline() #PMC1
        candidate.secondFunction = results.readline().strip().strip(':')
        # try to match each of our selections against the one we have just parsed
        matched = None
        for selection in selections:
           # pdb.set_trace()
            if (selection.match(candidate)):
                print "<result name=\"{0}\">".format(selection.name)
                print count
                print "</result>"
                matched = selection
                break

        # we found this selection, don't bother searching for it anymore
        if matched is not None:
            selections.remove(matched)

    line = results.readline()

print "</results>"


