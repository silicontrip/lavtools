
FILENAME [^/ ]+

EDITMODE V|A|VA|B|v|a|va|b

TRANSITION C

TIMECODE [0-9]+[;:][0-9]+[;:][0-9]+[;:][0-9]+


%%

"#".*"\n" /* remove comments */

{FILENAME} {EDITMODE} {TRANSITION} {TIMECODE} {TIMECODE} "\n"  { printf ("a line is read %s\n",yytext); }

%%
main( argc, argv )
int argc;
char **argv;
{
	++argv, --argc;  /* skip over program name */
	if ( argc > 0 )
		yyin = fopen( argv[0], "r" );
	else
		yyin = stdin;

	yylex();
}

