#include <time.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>


off_t progress_bytes;
time_t progress_start_time;

off_t progress_old_bytes = 0;
time_t progress_old_time = 0;

off_t progress_total_bytes;

#define THOUSAND_SEP ","
#define PROGRESS_WIDTH 40

inline void progress_init(off_t b, off_t t)
{

	//fprintf(stderr,"progress_init \n");

	progress_bytes = b;
	progress_old_bytes = b;
	progress_total_bytes = t;
	progress_start_time = time(NULL);
	progress_old_time = progress_start_time;
}


static inline void progress_comma_print (long n)
{

	char s[1024];
	int i,j;
	sprintf(s,"%ld",n);
	int skip = strlen(s) % 3;

	for (i=0; i<skip; i++)
		fprintf (stderr,"%c",s[i]);

	for (i=skip; i<strlen(s); i+=3)
	{
		if (i!=0)
			fprintf(stderr,THOUSAND_SEP);
		for (j=0;j<3;j++)
			fprintf(stderr,"%c",s[i+j]);
	}

}

static inline void progress_human_read_print_double(double n)
{

	char *suffix=" kMGTPEZY";
	int log=0;
	double base=n;
	while (base > 1000) {
		base /= 1000;
		log++;
	}

	fprintf(stderr,"%3.3f%c",base,suffix[log]);

}


static inline void progress_human_read_print(long n)
{

	char *suffix=" kMGTPEZY";
	int log=0;
	double base=n;
	while (base > 1000) {
		base /= 1000;
		log++;
	}

	fprintf(stderr,"%3.3f%c",base,suffix[log]);

}

// Process has done i out of n rounds,
// and we want a bar of width w and resolution r.
inline void progress_loadBar(off_t bytes)
{
    // Only update r times.
	time_t current =  time(NULL);
    if ( progress_old_time == current ) return;

	//fprintf (stderr,"progress_loadBar\n");


	off_t bps = (bytes-progress_old_bytes)/(current - progress_old_time);
	int w = PROGRESS_WIDTH;

	progress_old_bytes=bytes;
	progress_old_time = current;

    // Calculuate the ratio of complete-to-incomplete.
    double ratio = (double)bytes/(double)progress_total_bytes;
    int   c     = ratio * PROGRESS_WIDTH;

	//fprintf (stderr,"width: %d\n",c);

    // Show the percentage complete.
	// fprintf (stderr,"\033\n[F\033[J");

    fprintf(stderr,"%3d%% [", (int)(ratio*100) );

    // Show the load bar.
	int x;
    for (x=0; x<c; x++)
		fprintf(stderr,"=");

    for (x=c; x<w; x++)
		fprintf(stderr," ");

    // ANSI Control codes to go back to the
    // previous line and clear it.
    fprintf(stderr,"] ");

	progress_comma_print(bytes);

	fprintf(stderr," ");

	progress_human_read_print(bps);

	fprintf(stderr,"/s eta %llds",(progress_total_bytes - bytes) / bps);



	fprintf  (stderr,"\r");
	fflush(stderr);
}
