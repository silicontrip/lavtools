<?php include ("../news.php"); ?>
<HTML> <HEAD> <TITLE> Mark's video filters </TITLE>

<link rel="stylesheet" href="/trip.css" type="text/css" media="screen" />
<link href="/~mark/lavtools.xml" rel="alternate" type="application/rss+xml" title="Mark video filters" />


</HEAD> <BODY>

<div id=head>


<h1>YUV Video filters</h1>

</div> <div id="side">

<p> Code for YUV video stream processing. This code extends code
from the MJPEG tools and requires the mjpeg libraries.  If you have
difficulty compiling these contact me, mjpeg0 at silicontrip dot org.
</p>
<p>
You will need the mjpeg libraries to compile the yuv tools.
<a href="http://mjpeg.sourceforge.net">information on the mjpeg libraries and download.</a>
</P>
<p>
The Libav tools require the libavcodec library which is part of the
<a href="http://ffmpeg.org">ffmpeg tools</a>
<p>

<?php

function imageinline($thisdir, $max)
{
$basepath="/Users/mark/Sites/";
$baseuri="/~mark/";
?>
<div id="thumb">
<p>
<a href="/~mark/images.php/<?=$thisdir?>" >
<?
	if ($dir = @opendir($basepath . $thisdir)) {
		$count = 0;
		while ((($file = readdir($dir)) !== false) && ($count != $max)) {
			list ($base,$ext)=explode(".",$file);
			if ((strtolower($ext) == "jpg" ) ||
			(strtolower($ext) == "png" ) ) {
				if (is_file("$basepath/$thisdir/TN/$file")) {
?>
<img src="<?=$baseuri?><?=$thisdir?>/TN/<?=$file?>" border="1">
<?
				$count ++;
				}
			}
		}
	}
?>
</a>
</p>
<br clear="right">
</div>
<?php
}


$blogdir="/Users/mark/Sites/lavtools/";
?>
<h1>Contents</h1>
<p>
<?php
if ($dir = @opendir("$blogdir")) {
        while (($file = readdir($dir)) !== false) {
                if ($file != "." && $file != ".." ) {
                        list ($base,$ext)=explode(".",$file);
                        if ($ext == "c" || $ext == "m") {
?>
<a href="#<?=$base?>"><?=$base?></a><br/>
<?php
			}
		}
	}
	closedir($dir);
}
?>
</p>
</div>
<?php

if ($dir = @opendir("$blogdir")) {
	while (($file = readdir($dir)) !== false) {
		if ($file != "." && $file != ".." ) {
			list ($base,$ext)=explode(".",$file);
                        if ($ext == "c" || $ext == "m") {
?>
<div id="text">
<a name="<?=$base?>">
<h2><?=$base?></h2>
</a>
<p><a href="<?=$file?>"><?=$file?></a>
<?php
if (file_exists("$blogdir/$base.html")) {
include ("$blogdir/$base.html");
} else {
$lines = file($file);
foreach ($lines as $line) if(preg_match('/^\*\*/', $line)) print preg_replace("/^\*\*/", " ", $line);
}
if (is_dir("$blogdir/${base}_images")) {
?><h3>Images</h3><?
news::imageinline("lavtools/${base}_images",-1);
}
?>
</div>
<?php

			}
		}
	}
	closedir($dir);
}

?>
<div id="text">
<p><em>Isn't Chyler Leigh a goddess?</em></p>
</div>
</body>
</html>
