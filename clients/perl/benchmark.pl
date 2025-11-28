use strict;
use warnings;
use FindBin ();
use lib "$FindBin::Bin";
use Dumbbench;
use Redis;
use List::Util qw(first);
use Melian;

use constant PACKED_ID => pack('V', 5);

my $bench = Dumbbench->new(
    'target_rel_precision' => 0.005, # seek ~0.5%
    'initial_runs'         => 20,    # the higher the more reliable
);

my $redis = Redis->new('server' => '127.0.0.1:6379');

my $melian = Melian->new(
    'dsn'         => 'unix:///tmp/melian.sock',
    'schema_spec' => 'table1#0|60|id:int,table2#1|60|id:int;hostname:string',
    'timeout'     => 1,
);

my $schema = $melian->{'schema'};
my ($table1) = grep { $_->{'name'} eq 'table1' } @{ $schema->{'tables'} };
my ($id_index) = first { $_->{'column'} eq 'id' } @{ $table1->{'indexes'} };

$bench->add_instances(
    Dumbbench::Instance::PerlSub->new(
        'name' => 'Melian',
        'code' => sub {
            for (1 .. 1e5) {
                $melian->fetch_raw($table1->{'id'}, $id_index->{'id'}, PACKED_ID);
            }
        }
    ),

    Dumbbench::Instance::PerlSub->new(
        'name' => 'Redis',
        'code' => sub {
            for (1 .. 1e5) {
                $redis->get('t1:id:5');
            }
        }
    ),
);

$bench->run;
$bench->report;
