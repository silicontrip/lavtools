#!/usr/bin/perl

use POSIX qw(strftime);

$srcdir="/Users/mark/Sites/lavtools";


$date=`date +"%a, %d %b %Y %T %Z"`;
chop($date);
@nummon=(undef,'Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec');

print << "EOF";
<?xml version="1.0"?>
<rss version="2.0">
  <channel>
    <title>Mark's Video Tools</title>
    <link>http://silicontrip.net/~mark/lavtools/</link>
    <description>Collection of yuv video tools.</description>
    <language>en-us</language>
    <pubDate>Fri, 26 Mar 2005 11:10:03 EST</pubDate>
    <lastBuildDate>$date</lastBuildDate>
    <docs>http://silicontrip.net/~mark/lavtools.xml</docs>
    <generator>mark's perl script</generator>
    <managingEditor>silicontrip.net</managingEditor>
    <webMaster>silicontrip.net</webMaster>
    <ttl>5</ttl>
EOF

opendir DIR,$srcdir || die "Cannot open dir $srcdir";

while ($direntry = readdir(DIR)) {

	($base,$ext) = split (/\./,$direntry) ;

	if ($ext eq 'c' || $ext eq 'm') {
($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
       $atime,$mtime,$ctime,$blksize,$blocks) = stat "$srcdir/$direntry";
		$filelist{$mtime} = $direntry;
	}

}



for $k (sort(keys(%filelist))) {
$art = $filelist{$k};
$art =~ s/\.[^\.][^\.]*$/.html/;
# article date
$date = strftime "%a, %b %e %Y %H:%M:%S %Z",localtime($k);
# article title
($title,$ext) = split ('.',$art) ;
$title = `grep -i h3 $art | head -n 1`;
chop($title);
$title =~ s/<h3>//i;
$title =~ s/<\/h3>//i;
# article link 
# article body
$body = `grep -v -i h3 $art`;
#$body =~ s/<\/p>.*//;
# find the end of the first paragraph
$body =~ tr/[A-Z]/[a-z]/;
$offset = index($body,"</p>");
$body = substr($body,0,$offset);

$body =~ s/<[^<]*>//g;
print << "EOF";
    <item>
      <title>$title</title>
      <link>http://silicontrip.net/~mark/lavtools/index.php#$art</link>
      <description>$body</description>
      <pubDate>$date</pubDate>
      <guid>http://silicontrip.net/~mark/lavtools/index.php#$art</guid>
    </item>
EOF

}

print << "EOF";
  </channel>
</rss>
EOF
