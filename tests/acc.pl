#!/usr/bin/perl -w
use strict;

# apt-get install libnet-handlersocket-perl
use Net::HandlerSocket;
use DBI;
use Time::HiRes qw/gettimeofday tv_interval/;
use Data::Dumper;

my $runs = 10000;


my $acc_cols = "method,from_tag,to_tag,callid,sip_code,sip_reason,time,time_hires,src_leg,dst_leg,dst_user,dst_ouser,dst_domain,src_user,src_domain";

my @acc = (
	'INVITE',
	'test_from_tag',
	'test_to_tag',
	'test_call_id',
	'200',
	'OK',
	'2017-04-12 12:00:00',
	Time::HiRes::time,
	'test_src_leg',
	'test_dst_leg',
	'test_dst_user',
	'test_dst_ouser',
	'test_dst_domain',
	'test_src_user',
	'test_src_domain',
);

my %prefs;
my $start;
my $duration;

###### Handlersocket version goes here: ######################
my $idx = 1;
my $hs = Net::HandlerSocket->new({
	host => '127.0.0.1',
	port => 9997,
}) or die "Failed to connect to HS port\n";

if($hs->open_index($idx, 'kamailio', 'acc', 'PRIMARY', $acc_cols)) {
	die "Failed to open index on kamailio.acc: " . $hs->get_error() . "\n";
}

$start = [gettimeofday];
for(my $i = 0; $i < $runs; ++$i) {
	my $res = $hs->execute_single($idx, '+', \@acc, -1, 0);
	my $status = shift @{ $res };
	if($status != 0) {
		die "Failed to execute query: " . $hs->get_error() . "\n";
	}
}
$duration = tv_interval($start, [gettimeofday]);
print "--------- handlersocket results --------------\n";
print "Duration: $duration sec for $runs interations\n";
print "Req/sec: " . ($runs/$duration) . "\n";
print "Dur/req: " . ($duration/$runs) . " sec\n";

###### SQL version goes here: ######################

my $dsn = "DBI:mysql:database=kamailio;host=localhost;port=3306";
my $dbh = DBI->connect($dsn, "kamailio", "XHNLjd3L4fWtdwE3ta7L");
my $valtmpl = join ",", (("?") x @acc);
my $q = "insert into acc($acc_cols) values($valtmpl)";
my $sth = $dbh->prepare($q);

for(my $i = 0; $i < $runs; ++$i) {
	%prefs = ();
	$sth->execute(@acc);
}
$duration = tv_interval($start, [gettimeofday]);
print "--------- sql results --------------\n";
print "Duration: $duration sec for $runs interations\n";
print "Req/sec: " . ($runs/$duration) . "\n";
print "Dur/req: " . ($duration/$runs) . " sec\n";
