# The ARB probe server cgi perl library
# (C) 2003 by Ralf Westram

package probe_server;

use CGI;
$CGI::POST_MAX=1024 * 100;  # max 100K posts
$CGI::DISABLE_UPLOADS = 1;  # no uploads
# use CGI::Carp qw(fatalsToBrowser warningsToBrowser set_message);
use CGI::Carp qw(fatalsToBrowser warningsToBrowser set_message);

BEGIN {
  use Exporter ();

  our (@ISA, @EXPORT, @EXPORT_OK);

  @ISA       = qw(Exporter);
  @EXPORT    = qw( &print_header
                   &print_special_header
                   &wait_answer
                   &send_std_result
                   &write_std_request
                   &print_error
                   &print_critical_error
                   &generate_filenames
                 );
  @EXPORT_OK = qw( $q
                   %params
                   $requestdir
                   $datadir
                   $admin
                 );
}

our @EXPORT_OK;

my $header_printed;
my $root_dir='/home/westram/ARB/PROBE_SERVER';
# my $root_dir='/trance1/ARB/source/ARB/PROBE_SERVER';

sub print_header() {
  print $probe_server::q->header(-type=>'text/plain');
  $header_printed=1;
}
sub print_special_header($) {
  my ($type) = @_;
  print $probe_server::q->header(-type=>$type);
  $header_printed=1;
}

sub print_error($) {
  my ($msg) = @_;
  $msg =~ s/$root_dir/./ig;
  print "result=error\nmessage=$msg\n";
}
sub print_critical_error($) {
  my ($msg) = @_;
  $msg =~ s/$root_dir/./ig;
  print "result=error\nmessage=$msg (please contact $admin)\n";
}

#error handling:
BEGIN {
  sub handle_errors {
    my $msg = shift;
    if ($header_printed==0) { &probe_server::print_header(); }
    print_error($msg);
  }

  $header_printed = 0;
  $q              = new CGI;
  %params         = $q->Vars();

  $root_dir='/home/westram/ARB/PROBE_SERVER';
  # $root_dir='/trance1/ARB/source/ARB/PROBE_SERVER';

  $requestdir = $root_dir.'/ps_workerdir';
  $datadir    = $root_dir.'/ps_serverdata';
  $admin      = 'probeadmin@arb-home.de';
  set_message(\&handle_errors);
}

sub generate_filenames ($$) {
  my ($plength,$num) = @_;
  my $request_name = $probe_server::requestdir.'/'.$plength.'_'.$$.'_'.$num;
  my $result_name = $request_name.".res";
  $request_name.=".req";
  return ($request_name,$result_name);
}

sub write_std_request($$) {
  my ($request_name,$command) = @_;
  my $temp_req_name=$request_name.'.tmp';

  if (not open(REQUEST,">$temp_req_name")) { return "Can't write '$temp_req_name'"; }
  print REQUEST "command=$command\n"; # write command
  for (keys %probe_server::params) { # forward all parameters to request file
    print REQUEST "$_=$probe_server::params{$_}\n";
  }
  close REQUEST;
  if (not rename($temp_req_name,$request_name)) {
    return "Can't rename '$temp_req_name' to '$request_name'";
  }
  return;
}

sub wait_answer($) {
  my ($result_name) = @_;
  while (not -f $result_name) { sleep 1; }
}

sub send_std_result($) {
  my ($result_name) = @_;
  my $count         = 0;

  if (not open(RESULT,"<$result_name")) { return "Can't read '$result_name'"; }
  foreach (<RESULT>) { print $_; ++$count; }
  close RESULT;
  if ($count==0) { return "got empty result file from server"; }
  unlink $result_name;    # remove the result file
  return;
}

sub treefilename() {
  return $datadir.'/current.tree.gz';
}

END { }                         # module clean-up code here (global destructor)
1;                              # don't forget to return a true value from the file
