#!/usr/bin/perl
foreach $p ( `ls -1 *.html` )
{
 $_=$p;
 /(.*)/;
 $p=$1;  
 open( i, $p );
 @t=<i>;
 $_="@t";
 close( i );
 
 s/(\/usr\/local\/lib\/latex2html\/icons.png)/./gi;
  
 open( o, ">$p");
 print o ($_);
 close( o);
}
