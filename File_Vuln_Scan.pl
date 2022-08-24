#!/usr/bin/perl 
################################################
#      Must@f@ - http://www.mustafatopal.org   #
# Jdownloads File Upload Vulnerability Scanner #
################################################

use HTTP::Request;
use LWP::Simple;
use LWP::UserAgent;


if ($^O =~ /MSWin32/) {system("cls"); system("color c");
}else { system("clear"); }
print "\t[+]--------------------------------------------------------[+]\n";
print "\t[!]      Jdownloads File Upload Vulnerability Scanner      [!]\n";
print "\t[!]      Coded by Mustafa - http://www.mustafatopal.org    [!]\n";
print "\t[!]      Kayit Dosyasi ddz.txt                             [!]\n";
print "\t[+]--------------------------------------------------------[+]\n";
print "\n\n\t Mesaj: perl $0  list.txt \n\n";

open(tarrget,"<$ARGV[0]") or die "$!";
while(<tarrget>){
chomp($_);
$target = $_;
if($target !~ /http:\/\//)
{
$target = "http://$target";
}

$website = $target."/index.php?option=com_jdownloads&Itemid=0&view=upload";

$req=HTTP::Request->new(GET=>$website);
$ua=LWP::UserAgent->new();
$ua->timeout(30);
$response=$ua->request($req);
if($response->content=~ /Soumettre/ ||
   $response->content=~ /Submit/    

) 
 {
 $Messageee ="Acik Var";
open (TEXT, '>>ddz.txt');
print TEXT "$target => $Messageee\n";
close (TEXT);
}
else {
$Messageee = "Acik Yok";
}
print ">> $target => $Messageee\n";
}
