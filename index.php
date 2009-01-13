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

<?php

function imageinline($thisdir, $max)
{

?>
<div id="thumb">
<p>
<a href="images.php/<?=$thisdir?>" >
<?
	if ($dir = @opendir("./$thisdir/")) {
		$count = 0;
		while ((($file = readdir($dir)) !== false) && ($count != $max)) {
			list ($base,$ext)=explode(".",$file);
			if ((strtolower($ext) == "jpg" ) || 
			(strtolower($ext) == "png" ) ) {
				if (is_file("./$thisdir/TN/$file")) {
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
include ("$blogdir/$base.html");
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
