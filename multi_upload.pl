#!/usr/bin/perl -w
use strict;
my @builds = sort { -M $a <=> -M $b } glob "/tmp/build*.tmp";
my $hex = (glob "$builds[0]/*.hex")[0];

my @pids;
for my $port (glob "/dev/ttyUSB*") {
    if (my $pid = fork) {
        push @pids, $pid;
    } else {
        system avrdude => (
            -c => "arduino",
            -b => 115200,
            -p => "m328p",
            -P => $port,
            -U => "flash:w:$hex"
        );
        exit;
    }
}

waitpid $_, 0 for @pids;
