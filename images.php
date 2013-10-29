<html>
<head>
<title></title>
<link rel="stylesheet" href="/trip.css" type="text/css" media="screen" />

</head>
<body>

<div id="outer">
<?php

$parser_version = phpversion();

if ($parser_version > "4.1.0") {
        $scriptname=basename($_SERVER['SCRIPT_NAME']);
} else {
        $scriptname=basename($SCRIPT_NAME);
}
if ($parser_version > "4.1.0") {
        $thisdir=$_SERVER['PATH_INFO'];
} else {
        $thisdir=$PATH_INFO;
}
if ($parser_version > "4.1.0") {
        $uri = $_SERVER['REQUEST_URI'];
} else {
        $uri = $REQUEST_URI;
}


$basedir = "/Users/mark/Sites/lavtools";


$baseuri = $uri;
if ($thisdir) {
        $baseuri = preg_replace(",$thisdir\$,","","$baseuri");
}
$baseuri = preg_replace(",$scriptname,","","$baseuri");


$endfile = basename($thisdir);
$title= $endfile;
if (!$title) { $title = basename(getcwd()); }

#print "$scriptname $thisdir $uri $baseuri $title";

?>
<div id="head"><h1>
<?=$title?></h1>
</div>
<?
if ($dir = @opendir("$basedir/$thisdir")) {
	while (($file = readdir($dir)) !== false) {
		list ($base,$ext)=split("\.",$file);
		if ( ($ext=="gif" || $ext=="jpg" || $ext=="png")  && $base) {
			$extra = "";
			if (is_file ("$basedir/$title/$base.$ext")) {
?>
<div id=photo> <p>
<img src="<?=$baseuri?>/<?=$title?>/<?=$base?>.<?=$ext?>" ><br>
<?
				if (is_file("$basedir/$title/$base.txt")) {
					include	("$basedir/$title/$base.txt");
				} else {
					print "$base";
				}
?>
</p> </div>
<?
			}
		}
	}
closedir($dir);
}

?>
</body>
</html>
