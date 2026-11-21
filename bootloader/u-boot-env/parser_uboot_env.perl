use strict;
use warnings;

#my $file = 'uboot-env-g3hgu-ctc-yueme.txt';

my ($file, $ofilename) = @ARGV;

if (not defined $file) {
   die "Miss uboot env file!\n"
}
if (not defined $ofilename){
   die "Miss output file name!\n"
}

open my $info, $file or die "Could not open $file: $!";

#my $ofilename = 'uboot-env-extra.h';
open(my $fh, '>', $ofilename) or die "Could not open file '$ofilename' $!";


printf $fh  "#ifndef __UBOOT_ENV_H \n #define __UBOOT_ENV_H\n ";


printf $fh  "#undef CONFIG_EXTRA_ENV_SETTINGS\n";
printf $fh "#define CONFIG_EXTRA_ENV_SETTINGS  \\\n";

while( my $line = <$info>)  {

        if ($line =~ m/\/.*\//) {
         next;
        }

   next if $line=~/^\/\/.*/;
   next if $line=~/^\s*$/;

        chomp($line);
         printf $fh  "\"" ;
         printf $fh "%s\\0\" \\", $line;
         printf $fh  "\n" ;
}
printf $fh  "\"\"\n" ;
printf $fh  "#endif\n";
close $info;

close $fh;

