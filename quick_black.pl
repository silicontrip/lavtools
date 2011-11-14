#!/usr/bin/perl

print "YUV4MPEG2 W720 H576 F25:1 It A128:117 C420mpeg2\n";

$lumaline = "";
for $x (0..719) { $lumaline = sprintf "%s%c",$lumaline,16; }

$chromaline = "";
for $x (0..359) { $chromaline = sprintf "%s%c",$chromaline,128; }

for $f (1..125) {
print "FRAME\n";
for $y (0..575) { print $lumaline; }
for $y (0..287) { print $chromaline; }
for $y (0..287) { print $chromaline; }
}
