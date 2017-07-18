#!/usr/bin/perl -w
use strict;

# apt-get install libnet-handlersocket-perl
use Net::HandlerSocket;
use DBI;
use Time::HiRes qw/gettimeofday tv_interval/;
use Data::Dumper;

my $uuid = 'fcd7f63c-a99e-4f13-befe-df0a9d1941f8';

my %prefs;
my $start;
my $duration;
#my $runs = 100000;
my $runs = 1;

###### Handlersocket version goes here: ######################
my $kam_usr_pref = 1;
my $hs = Net::HandlerSocket->new({
	host => '127.0.0.1',
	port => 9996,
}) or die "Failed to connect to HS port\n";

if($hs->auth("readsecret")) {
	die "Failed to auth\n";
}

if($hs->open_index($kam_usr_pref, 'kamailio', 'usr_preferences', 'ua_idx', 'attribute,value')) {
	die "Failed to open index on kamailio.usr_preferences: " . $hs->get_error() . "\n";
}

$start = [gettimeofday];
for(my $i = 0; $i < $runs; ++$i) {
	%prefs = ();
	my $res = $hs->execute_single($kam_usr_pref, '=', [$uuid], -1, 0);
	my $status = shift @{ $res };
	if($status != 0) {
		die "Failed to execute query: " . $hs->get_error() . "\n";
	}

	while(@{ $res }) {
		my $k = shift @{ $res };
		my $v = shift @{ $res };
		# we already had this key, so it's an attribute
		# with at least 2 values
		if(exists $prefs{$k}) {
			# if we already have >1 values, it's
			# converted to an array ref, so just
			# push our value at the end
			if(ref $prefs{$k} eq "ARRAY") {
				push @{ $prefs{$k} }, $v;
			# otherwise we have to convert the existing
			# value to an array ref and attach our new
			# value at the end
			} else {
				$prefs{$k} = [ $prefs{$k}, $v ];
			}
		} else {
			$prefs{$k} = $v;
		}
	}
}
$duration = tv_interval($start, [gettimeofday]);
print "--------- handlersocket results --------------\n";
print "Duration: $duration sec for $runs interations\n";
print "Req/sec: " . ($runs/$duration) . "\n";
print "Dur/req: " . ($duration/$runs) . " sec\n";
print "Result:\n";
print Dumper \%prefs;

exit;

###### SQL version goes here: ######################

my $dsn = "DBI:mysql:database=kamailio;host=localhost;port=3306";
my $dbh = DBI->connect($dsn, "kamailioro", "VvKthesty9cVziaVCidM");
my $sth = $dbh->prepare("select attribute,value from usr_preferences where uuid=?") or die "Failed to prepare select statement\n";

for(my $i = 0; $i < $runs; ++$i) {
	%prefs = ();
	$sth->execute($uuid);
	while(my $ref = $sth->fetchrow_hashref()) {
		my $k = $ref->{'attribute'};
		my $v = $ref->{'value'};
		# we already had this key, so it's an attribute
		# with at least 2 values
		if(exists $prefs{$k}) {
			# if we already have >1 values, it's
			# converted to an array ref, so just
			# push our value at the end
			if(ref $prefs{$k} eq "ARRAY") {
				push @{ $prefs{$k} }, $v;
			# otherwise we have to convert the existing
			# value to an array ref and attach our new
			# value at the end
			} else {
				$prefs{$k} = [ $prefs{$k}, $v ];
			}
		} else {
			$prefs{$k} = $v;
		}
	}
}
$duration = tv_interval($start, [gettimeofday]);
print "--------- sql results --------------\n";
print "Duration: $duration sec for $runs interations\n";
print "Req/sec: " . ($runs/$duration) . "\n";
print "Dur/req: " . ($duration/$runs) . " sec\n";
print "Result:\n";
print Dumper \%prefs;
