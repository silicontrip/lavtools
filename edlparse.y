

token STRING
token NUM
token FRAMES

%%

line: filename editmode transition srcin srcout

filename: STRING

editmode: "V" {video = 1;}
	| "A" {audio = 1;}
	| "VA" {audio = 1; video = 1;}
	| "B" {audio = 1; video = 1;}
 
transition: 'C'

srcin: timecode

srcout: timecode

timecode: NUM
	| NUM':'NUM 
	| NUM';'NUM 
	| NUM':'NUM':'NUM
	| NUM':'NUM';'NUM
	| NUM':'NUM':'NUM':'NUM
	| NUM':'NUM':'NUM';'NUM
