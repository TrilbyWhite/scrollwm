/**********************************************************\
* SCOROLLER:
* Author: Jesse McClure, 2012
* License: released to public domain
* Example status input for scrollwm
*
* Displays and colors icons for the following:
* CPU, MEM, VOLUME, BATTERY, and a CLOCK
* 
* Requires terminusmod,icons font to be used as is.
* Also requires a file at ~/.audio_volume that contains
* a number from 0-100 as a percent of max volume, or a
* -1 for muted speakers.
*
* COMPILE: gcc -o scroller scroller.c
\**********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>

/* input files */
static const char *CPU_FILE		= "/proc/stat";
static const char *MEM_FILE		= "/proc/meminfo";
static const char *BATT_NOW		= "/sys/class/power_supply/BAT1/charge_now";
static const char *BATT_FULL	= "/sys/class/power_supply/BAT1/charge_full";
static const char *BATT_STAT	= "/sys/class/power_supply/BAT1/status";

/* colors			  			    R G B */
static const long int White		= 0xDDDDDD;
static const long int Grey		= 0x686868;
static const long int Blue		= 0x122864;
static const long int Green		= 0x308030;
static const long int Yellow	= 0x997700;
static const long int Red		= 0x990000;

/* variables */
static long		j1,j2,j3,j4,ln1,ln2,ln3,ln4;
static int		n, loops = 100;
static char		c, clk[8], *aud_file;
static FILE		*in;
static time_t	current;

int main(int argc, const char **argv) {
	in = fopen(CPU_FILE,"r");
	fscanf(in,"cpu %ld %ld %ld %ld",&j1,&j2,&j3,&j4);
	fclose(in);
	char *homedir = getenv("HOME");
	if (homedir != NULL) {
		aud_file = (char *) calloc(strlen(homedir)+15,sizeof(char));
		strcpy(aud_file,homedir);
		strcat(aud_file,"/.audio_volume");
	}
	else aud_file = NULL;
	/* main loop */
	for (;;) {
		loops ++;
		if ( (in=fopen(CPU_FILE,"r")) ) {       /* CPU MONITOR */
			fscanf(in,"cpu %ld %ld %ld %ld",&ln1,&ln2,&ln3,&ln4);
			fclose(in);
			if (ln4>j4) n=(int)100*(ln1-j1+ln2-j2+ln3-j3)/(ln1-j1+ln2-j2+ln3-j3+ln4-j4);
			else n=0;
			j1=ln1; j2=ln2; j3=ln3; j4=ln4;
			if (n > 85) printf("{#%06X}%c  ",Red,209);
			else if (n > 60) printf("{#%06X}%c  ",Yellow,209);
			else if (n > 20) printf("{#%06X}%c  ",Blue,209);
			else printf("{#%06X}%c  ",Grey,209);
		}
		if ( (in=fopen(MEM_FILE,"r")) ) {		/* MEM USAGE MONITOR */
			fscanf(in,"MemTotal: %ld kB\nMemFree: %ld kB\nBuffers: %ld kB\nCached: %ld kB\n", &ln1,&ln2,&ln3,&ln4);
			fclose(in);
			n = 100*(ln2+ln3+ln4)/ln1;
			if (n > 80) printf("{#%06X}%c  ",Grey,206);
			else if (n > 65) printf("{#%06X}%c  ",Green,206);
			else if (n > 15) printf("{#%06X}%c  ",Yellow,206);
			else printf("{#%06X}%c  ",Red,206);
		}
		if ( (in=fopen(aud_file,"r")) ) {       /* AUDIO VOLUME MONITOR */
			fscanf(in,"%d",&n);
			fclose(in);
			if (n == -1) printf("{#%06X}%c  ",Red,235);
			else if (n == 100) printf("{#%06X}%c  ",Blue,237);
			else if (n < 15) printf("{#%06X}%c  ",Yellow,236);
			else printf("{#%06X}%c  ",Grey,237);
		}
		if ( (in=fopen(BATT_NOW,"r")) ) {       /* BATTERY MONITOR */
			fscanf(in,"%ld\n",&ln1); fclose(in);
			if ( (in=fopen(BATT_FULL,"r")) ) { fscanf(in,"%ld\n",&ln2); fclose(in); }
			if ( (in=fopen(BATT_STAT,"r")) ) { fscanf(in,"%c",&c); fclose(in); }
			n = (ln1 ? ln1 * 100 / ln2 : 0);
			if (c == 'C') printf("{#%06X}%c  ",Yellow,181);
			else if (n < 10) printf("{#%06X}%c  ",Red,238);
			else if (n < 20) printf("{#%06X}%c  ",Yellow,239);
			else if (n > 95) printf("{#%06X}%c  ",Green,240);
			else printf("{#%06X}%c  ",Grey,240);
		}
		if (loops++ > 90) {
			time(&current);
			strftime(clk,6,"%H:%M",localtime(&current));
			loops = 0;
		}
		printf("{#%06X}%c %s \n",White,182,clk);
		fflush(stdout);
		usleep(500000);
	}
	return 0;
}
