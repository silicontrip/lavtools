
%union {
	char *filename;
	char audio;
	char video;
	int isec;
	int iframe;
	int osec;
	int oframe;
}

%token <filename> STRING

%%

valid: filename editmode transitiontype srcin srcout '\n'

filename: STRING

editmode: 'V'
	| 'A'
	| 'VA'
	| 'B'
	| 'v'
	| 'a'
	| 'va'
	| 'b'

transitiontype: 'C'

srcin: timecode

srcout: timecode

timecode: 
