#!/usr/bin/perl 
use GD;
$image = GD::Image->new($ARGV[0]);
$width=  $image->width ;
print "int num_colors = $width;\n";
print "GLfloat colors[$width][3] = {\n";
for ($w=0;$w<$width;$w++)
{
 $index = $image->getPixel($w,0);
 ($r,$g,$b) = $image->rgb($index);
 print "{".($r/256).",".($g/256).",".($b/256)."}";
 print "," if (($w+1) < $width);
 print "\n" if ((($w+1)%3)==0) ;
}
print "};\n";
