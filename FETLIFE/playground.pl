#!/bin/perl

# parsing fetlife user data into subroutine

require "/usr/local/lib/bclib.pl";

# fields from location sucking
# id,screenname,thumbnail,age,gender,role,loc1,loc2,page_number,scrape_time

# my(%res) = fetlife_user_data(join("",`bzcat /usr/local/etc/FETLIFE/user4608502.bz2`));

my(@order)=split(/\,/,"id,screenname,age,gender,role,city,state,country,source,mtime");

for $i (glob "/home/barrycarter/FETLIFE/FETLIFE-USER-PROFILES/464/*.bz2") {
  my(%res) = fetlife_user_data($i);
  unless ($res{id}) {next;}
  my(@l) = @order;
  map($_=$res{$_},@l);
  debug(join(",",@l));
}
