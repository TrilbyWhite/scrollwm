/*NOTE: In the commit history of this file you will find many variations.
Most likely none of them will work perfectly as-is.  Instead these can
be used as templates for your own status programs.
Specifically some of the specifics of the audio "codec" file seem hardware
dependent, and the mail settings are based of my own maildir archive from
offline-imap. */


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
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>

/* input files */
static const char *CPU_FILE		= "/proc/stat";
static const char *MEM_FILE		= "/proc/meminfo";
static const char *AUD_FILE		= "/proc/asound/card0/codec#0";
static const char *WIFI_FILE	= "/proc/net/wireless";
static const char *BATT_NOW		= "/sys/class/power_supply/BAT1/charge_now";
static const char *BATT_FULL	= "/sys/class/power_supply/BAT1/charge_full";
static const char *BATT_STAT	= "/sys/class/power_supply/BAT1/status";
static const char *MAIL_CUR		= "/home/username/mail/INBOX/cur";
static const char *MAIL_NEW		= "/home/username/mail/INBOX/new";

/* colors			  			    R G B */
static const long int White		= 0xDDDDDD;
static const long int Grey		= 0x686868;
static const long int Blue		= 0x68B0E0;
static const long int Green		= 0x288428;
static const long int Yellow	= 0x997700;
static const long int Red		= 0x990000;

/* icon names */
enum {
	clock_icon, cpu_icon, mem_icon,
	speaker_hi_icon, speaker_mid_icon, speaker_low_icon, speaker_mute_icon,
	wifi_full_icon, wifi_hi_icon, wifi_mid_icon, wifi_low_icon
};

/* variables */
static long		j1,j2,j3,j4,ln1,ln2,ln3,ln4;
static int		n, loops = 100, mail = 0;
static char		c, clk[8], *aud_file, *mail_file;
static FILE		*in;
static time_t	current;

int mailcheck() {
	DIR *dir;
	struct dirent *de;
	struct stat info;
	time_t cur,new;
	char *dname;
	chdir(MAIL_CUR);
	if ( !(dir=opendir(MAIL_CUR)) ) return -1;
	while ( (de=readdir(dir)) ) dname = de->d_name;
	if (dname[0] == '.') { closedir(dir); return 0; }
	stat(dname,&info);
	cur = info.st_mtime;
	closedir(dir);
	chdir(MAIL_NEW);
	if ( !(dir=opendir(MAIL_NEW)) ) return -1;
	while ( (de=readdir(dir)) ) dname = de->d_name;
	if (dname[0] == '.') { closedir(dir); return 0; }
	stat(dname,&info);
	new = info.st_mtime;
	closedir(dir);
	if (new > cur) return 2;
	else return 1;
}

int main(int argc, const char **argv) {
	in = fopen(CPU_FILE,"r");
	fscanf(in,"cpu %ld %ld %ld %ld",&j1,&j2,&j3,&j4);
	fclose(in);
	/* main loop */
	for (;;) {
		if ( (in=fopen(CPU_FILE,"r")) ) {       /* CPU MONITOR */
			fscanf(in,"cpu %ld %ld %ld %ld",&ln1,&ln2,&ln3,&ln4);
			fclose(in);
			if (ln4>j4) n=(int)100*(ln1-j1+ln2-j2+ln3-j3)/(ln1-j1+ln2-j2+ln3-j3+ln4-j4);
			else n=0;
			j1=ln1; j2=ln2; j3=ln3; j4=ln4;
			if (n > 85) printf("{#%06X}{i %d} ",Red,cpu_icon);
			else if (n > 60) printf("{#%06X}{i %d} ",Yellow,cpu_icon);
			else if (n > 20) printf("{#%06X}{i %d} ",Blue,cpu_icon);
			else printf("{#%06X}{i %d} ",Grey,cpu_icon);
		}
		if ( (in=fopen(MEM_FILE,"r")) ) {		/* MEM USAGE MONITOR */
			fscanf(in,"MemTotal: %ld kB\nMemFree: %ld kB\nBuffers: %ld kB\nCached: %ld kB\n", &ln1,&ln2,&ln3,&ln4);
			fclose(in);
			n = 100*(ln2+ln3+ln4)/ln1;
			if (n > 80) printf("{#%06X}{i %d} ",Grey,mem_icon);
			else if (n > 65) printf("{#%06X}{i %d} ",Green,mem_icon);
			else if (n > 15) printf("{#%06X}{i %d} ",Yellow,mem_icon);
			else printf("{#%06X}{i %d} ",Red,mem_icon);
		}
		if ( (in=fopen(AUD_FILE,"r")) ) {       /* AUDIO VOLUME MONITOR */
			while ( fscanf(in," Amp-Out caps: ofs=0x%x",&ln1) !=1 )
				fscanf(in,"%*[^\n]\n");
			while ( fscanf(in," Amp-Out vals: [0x%x",&ln2) != 1 )
				fscanf(in,"%*[^\n]\n");
			while ( fscanf(in,"Node 0x14 [%c",&c) != 1 )
				fscanf(in,"%*[^\n]\n");
			while ( fscanf(in," Amp-Out vals: [0x%x",&ln3) != 1 )
				fscanf(in,"%*[^\n]\n");
			fclose(in);
			if (ln3 == 0) printf("{#%06X}{i %d} ",Red,speaker_mute_icon);
			else {
				n = 100*ln2/ln1;
				if (n > 95) printf("{#%06X}{i %d} ",Blue,speaker_hi_icon);
				else if (n > 75) printf("{#%06X}{i %d} ",Grey,speaker_hi_icon);
				else if (n > 50) printf("{#%06X}{i %d} ",Grey,speaker_mid_icon);
				else if (n > 30) printf("{#%06X}{i %d} ",Yellow,speaker_mid_icon);
				else if (n > 10) printf("{#%06X}{i %d} ",Yellow,speaker_low_icon);
				else printf("{#%06X}{i %d} ",Red,speaker_low_icon);
			}
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
		if ( (in=fopen(WIFI_FILE,"r")) ) {       /* WIFI MONITOR */
			n = 0;
			fscanf(in,"%*[^\n]\n%*[^\n]\n wlan0: %*d %d.",&n);
			fclose(in);
			if (n == 0) printf("{#%06X}{i %d} ",Red,wifi_low_icon);
			else if (n > 63) printf("{#%06X}{i %d} ",Green,wifi_full_icon);
			else if (n > 61) printf("{#%06X}{i %d} ",Grey,wifi_full_icon);
			else if (n > 56) printf("{#%06X}{i %d} ",Grey,wifi_mid_icon);
			else if (n > 51) printf("{#%06X}{i %d} ",Grey,wifi_low_icon);
			else printf("{#%06X}{i %d} ",Yellow,wifi_low_icon);
		}
		if ((loops % 20) == 0) mail = mailcheck();    /* MAIL */
			if (mail == 1) printf("{#%06X}%c  ",Blue,202);
			else if (mail == 2) printf("{#%06X}%c  ",Green,202);
			else printf("{#%06X}%c  ",Grey,203);
		if (loops++ > 60) {					/* TIME */
			time(&current);
			strftime(clk,6,"%H:%M",localtime(&current));
			loops = 0;
		}
		printf("{#%06X}{i %d}{#%06X} %s \n",Blue,clock_icon,White,clk);
		fflush(stdout);
		usleep(500000);
	}
	return 0;
}
