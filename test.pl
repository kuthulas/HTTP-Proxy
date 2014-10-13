#!/usr/bin/perl
if($#ARGV<1){ print "Usage:$0 <ip_addr> <port>\n"; exit;}
$ip=$ARGV[0];
$port=$ARGV[1];
@list=("yahoo.com","google.com","cnn.com","tamu.edu","httpbin.org/cache");
#@list=("yahoo.com");
@range=(1..3);
foreach $url(@list)
{
 $file=$url;
 if($url!~/\//){$file=$url."_";}
 else{$file=~s/\//\_/;}
 system("rm $file\n");

 foreach(@range){
 system("./client $ip $port $url\n");
 if(-e $file){ print "Pass for $url File $file found\n"; }
 else {print "Fail for $url  No $file found\n"; exit;}
 }

}
