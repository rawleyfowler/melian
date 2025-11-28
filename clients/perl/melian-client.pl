#!/usr/bin/env perl
use strict;
use warnings;
use v5.34;
use Getopt::Long qw(GetOptions);
use JSON::PP;
use FindBin ();
use lib "$FindBin::Bin";
use Melian;

sub usage {
    die <<'USAGE';
Usage:
  melian-client.pl [--dsn=unix:///tmp/melian.sock] --table table2 --index hostname --value host-00002
  melian-client.pl --dsn=tcp://127.0.0.1:8765 --table table1 --value 5
  melian-client.pl --describe

Options:
  --dsn         Melian DSN (unix:///path or tcp://host:port). Default: unix:///tmp/melian.sock
  --describe    Print schema metadata and exit
  --table       Table name (alternative to --table-id)
  --table-id    Numeric table id
  --index       Index column name (alternative to --index-id). Defaults to 'id'
  --index-id    Numeric index id
  --schema-file Path to schema specification (optional)
  --schema-spec Inline schema specification string (optional)
  --value       Lookup value (string or integer depending on index type)
  --raw         Print raw response bytes instead of JSON
  --pretty      Pretty-print JSON (default true)
USAGE
}

sub main {
    my %opts = (
        dsn    => 'unix:///tmp/melian.sock',
        pretty => 1,
        raw    => 0,
    );

    GetOptions(
        'dsn=s'        => \$opts{dsn},
        'describe!'    => \$opts{describe},
        'table=s'      => \$opts{table},
        'table-id=i'   => \$opts{'table-id'},
        'index=s'      => \$opts{index},
        'index-id=i'   => \$opts{'index-id'},
        'schema-file=s'=> \$opts{'schema-file'},
        'schema-spec=s'=> \$opts{'schema-spec'},
        'value=s'      => \$opts{value},
        'raw!'         => \$opts{raw},
        'pretty!'      => \$opts{pretty},
    ) or usage();

    my %ctor = (dsn => $opts{dsn});
    $ctor{'schema_file'} = $opts{'schema-file'} if $opts{'schema-file'};
    $ctor{'schema_spec'} = $opts{'schema-spec'} if $opts{'schema-spec'};
    my $client = Melian->new(%ctor);
    my $schema = $client->schema();

    if ($opts{describe}) {
        my $json = JSON::PP->new->utf8(1)->pretty(1);
        say $json->encode($schema);
        return;
    }

    usage() unless defined $opts{value};
    my $payload;
    if (defined $opts{'table-id'}) {
        my $table_meta = $client->table_by_id($opts{'table-id'})
            or die "Unknown table id $opts{'table-id'}\n";
        my $index_meta;
        if (defined $opts{'index-id'}) {
            $index_meta = $client->index_by_id($table_meta->{'id'}, $opts{'index-id'})
                or die "Unknown index id $opts{'index-id'} for table id $table_meta->{'id'}\n";
        } else {
            $index_meta = $client->index_by_id($table_meta->{'id'}, 0)
                or die "Table id $table_meta->{'id'} has no default index\n";
        }
        my $key = $index_meta->{'type'} eq 'int' ? pack('V', $opts{value}) : $opts{value};
        $payload = $client->fetch_raw($table_meta->{'id'}, $index_meta->{'index_id'}, $key);
    } else {
        my $table_name = $opts{table} // '';
        my $index_name = $opts{index} // 'id';
        $payload = $client->fetch_table($table_name, $index_name, $opts{value});
    }

    if ($opts{raw}) {
        print $payload;
        return;
    }

    if ($payload eq '') {
        say '<empty response>';
        return;
    }

    my $json = JSON::PP->new->utf8(1);
    $json = $json->pretty(1) if $opts{pretty};
    my $decoded = eval { $json->decode($payload) };
    if ($@) {
        warn "Failed to decode JSON response, showing raw payload\n";
        print $payload;
        return;
    }
    print $json->encode($decoded);
}

main();
