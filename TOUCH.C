/* touch.c
**
** Copyright (c) 1989, Christopher Laforet
** Released into the Public Domain
**
**
** (ver 1) turbo.c 2.0 version
** (ver 2) msc 5.1 os/2-dos bound version
** 1990.05.04 Reed Shilts: Minor mods to match Berkeley Unix 4.2
**      unix: -c prevent automatic creation, -f ignored.
**      C.Laf: -k confirm, -v verbose
** 1990.05.04 Reed Shilts: long filename support added (quick hack!)
**
**
*/

#define LINT_ARGS
#define __OS2_SDK_12__  1


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <os2.h>



#define MAJOR_VERSION		3
#define MINOR_VERSION		0


extern void get_time(FTIME *time,FDATE *date);
extern void show_usage(void);
extern void touch(unsigned char *filename,FTIME *time,FDATE *date);

short verbose_flag = 0;
short confirm_flag = 0;
short create_flag  = 1;         /* default TRUE - unix style */
short total_files  = 0;
short total_touched = 0;
short total_created = 0;


main(argc,argv)
int argc;
char *argv[];
	{
	FTIME start_time;
	FDATE start_date;
	short count;
	short files = 0;

	get_time(&start_time,&start_date);
	fprintf(stderr,"TOUCH (v %u.%02u of %s): A File Date and Time Modification Utility\n",MAJOR_VERSION,MINOR_VERSION,__DATE__);
	fprintf(stderr,"Copyright (c) 1989, Christopher Laforet.  Released into the Public Domain.\n\n");
	if (argc == 1)
		{
		show_usage();
		}
	else
		{
		for (count = 1; count < argc; count++)
			{
			if ((*argv[count] == '/') || (*argv[count] == '-')) 	/* check for flags */
				{
				switch (argv[count][1])
					{
					case '?':
						show_usage();
						break;
					case 'F':               /* force is currently ignored */
					case 'f':
						break;
					case 'K':
					case 'k':
						confirm_flag = 1;
						break;
					case 'V':
					case 'v':
						verbose_flag = 1;
						break;
					case 'C':
					case 'c':
						create_flag = 0;    /* yes - create defaults true */    
						break;
					default:
						printf("Warning: Invalid flag argument \"%s\".\n",&argv[count][1]);
						break;
					}
				}
			}
		for (count = 1; count < argc; count++)
			{
			if ((*argv[count] != '/') && (*argv[count] != '-')) 	/* check for files */
				{
				files = 1;
				touch(argv[count],&start_time,&start_date);
				}
			}
		if (!files)
			{
			printf("Error: No valid filename(s) specified on command line.\n");
			}
		else
			{
			printf("\nTotal Files Found: %u\n",total_files);
			if (create_flag)
				{
				printf("Total Files Created: %u\n",total_created);
				printf("Total Files Touched/Created: %u\n\n",total_touched);
				}
			else
				{
				printf("Total Files Touched: %u\n\n",total_touched);
				}
			}
		}
	return(0);
	}


void get_time(time,date)
FTIME *time;
FDATE *date;
	{
	DATETIME datetime;

	DosGetDateTime(&datetime);
	time->hours = datetime.hours;
	time->minutes = datetime.minutes;
	time->twosecs = datetime.seconds >> 1;	   /* get two-second interval */

	date->year = datetime.year - 1980;
	date->month = datetime.month;
	date->day = datetime.day;
	}


void show_usage()
	{
	printf("Usage:  TOUCH [flags] pathname [pathname(s)]\n");
	printf("    where flags are:\n");
	printf("\t-c   Prevent creation of file if not existant.\n");
	printf("\t-k   Confirm each file to be touched.\n");
	printf("\t-f   Force touch despite read/write permissions (currently ignored).\n");
	printf("\t-v   Verbose mode.  Show each file touched.\n");
	printf("\t-?   Show this usage screen.\n\n");
	}



/* Big vars made static */
#ifdef __OS2_SDK_12__
FILEFINDBUF findbuf;

void touch(filename,time,date)
unsigned char *filename;
FTIME *time;
FDATE *date;
	{
	unsigned char *cptr;
	HDIR hdir = 0xffff;
	FILESTATUS filestatus;
	short num = 1;
	HFILE fh;
	unsigned short action;
	short key;
	int rtn;
	short confirm;
	short found = 0;

	rtn = DosFindFirst( filename, &hdir, 0x20, (PFILEFINDBUF)&findbuf, 
                        sizeof(FILEFINDBUF), &num, 0L);
	while (!rtn)
		{
		++total_files;
		++found;
		confirm = 0;

		if (confirm_flag)
			{
			fprintf(stderr,"Is it OK to touch \"%s\"? ", findbuf.achName);
			do
				{
				key = getch();
				key = toupper(key);
				}
			while ((key != 'N') && (key != 'Y'));
			fprintf(stderr,"%c\n",key);
			if (key == 'Y')
				{
				confirm = 1;
				}
			}
        
        if (!confirm_flag || (confirm_flag && confirm))
			{
			if (DosOpen( (char far *)findbuf.achName, (PHFILE)&fh, 
                         (PUSHORT)&action, 0L, 0x20, 0x1, 
                         _osmajor < 3 ? 0x2 : 0x42, 0L)
                       ) 
				{
				printf("Error: Unable to open \"%s\"!\n", findbuf.achName);
				}
			else
				{
				if (verbose_flag)
					{
					printf("Touching %-12.12s -> %02u:%02u:%02u on %02u/%02u/%04u\n",
                            findbuf.achName,
                            time->hours,time->minutes,time->twosecs << 1,
                            date->month,date->day,date->year + 1980);
					}
				memset(&filestatus, 0, sizeof(FILESTATUS));
				memcpy(&filestatus.ftimeLastWrite, time, sizeof(FTIME));
				memcpy(&filestatus.fdateLastWrite, date, sizeof(FDATE));

                if (DosSetFileInfo(fh, 0x01, (PBYTE)&filestatus, sizeof(FILESTATUS)))
					{
					printf("Error: Unable to change date on \"%s\"!\n", findbuf.achName);
					}
				else
					{
					++total_touched;
					}
				DosClose(fh);
				}
			}
		num = 1;
		rtn = DosFindNext(hdir,(PFILEFINDBUF)&findbuf,sizeof(FILEFINDBUF),&num);
		}
	if (!found && create_flag)
		{
		cptr = filename;
		while (*cptr)
			{
			found = 1;
			if ((*cptr == '?') || (*cptr == '*'))	/* no wild cards */
				{
				found = 0;
				break;
				}
			++cptr;
			}
		if (found)
			{
			confirm = 0;
			if (confirm_flag)
				{
				strupr(filename);
				fprintf(stderr,"Is it OK to create \"%s\"? ",filename);
				do
					{
					key = getch();
					key = toupper(key);
					}
				while ((key != 'N') && (key != 'Y'));
				fprintf(stderr,"%c\n",key);
				if (key == 'Y')
					{
					confirm = 1;
					}
				}
			if (!confirm_flag || (confirm_flag && confirm))
				{
				if (DosOpen( (char far *)filename, (PHFILE)&fh, (PUSHORT)&action, 
                             0L, 0x20, 0x11, _osmajor <= 4 ? 0x2 : 0x42, 0L)
                           )
					{
					printf("Error: Unable to create \"%s\"!\n",filename);
					}
				else
					{
					if (verbose_flag)
						{
						printf("Creating %-12.12s -> %02u:%02u:%02u on %02u/%02u/%04u\n",
                                findbuf.achName,
                                time->hours,time->minutes,time->twosecs << 1,
                                date->month,date->day,date->year + 1980);
						}
					memset(&filestatus, 0, sizeof(FILESTATUS));
					memcpy(&filestatus.ftimeLastWrite, time, sizeof(FTIME));
					memcpy(&filestatus.fdateLastWrite, date, sizeof(FDATE));
					DosSetFileInfo(fh, 0x01, (PBYTE)&filestatus, sizeof(FILESTATUS));
					++total_created;
					++total_touched;
					DosClose(fh);
					}
				}
			}
		}
	if (!found)
		{
		printf("Error: Unable to find \"%s\"!\n",filename);
		}
	}

#else

void touch(filename,time,date)
unsigned char *filename;
FTIME *time;
FDATE *date;
	{
    unsigned char buffer[100]; 
    unsigned char drive[_MAX_DRIVE];
    unsigned char dir[_MAX_DIR];    
    unsigned char file[_MAX_FNAME];
    unsigned char ext[_MAX_EXT];
    FILEFINDBUF findbuf;

	unsigned char *cptr;
	HDIR hdir = 0xffff;
	FILESTATUS filestatus;
	short num = 1;
	HFILE fh;
	unsigned short action;
	short key;
	int rtn;
	short confirm;
	short found = 0;

	_splitpath(filename,drive,dir,file,ext);	   /* split pathname */
	rtn = DosFindFirst( filename, &hdir, 0x20, (PFILEFINDBUF)&findbuf, sizeof(FILEFINDBUF), &num, 0L);
	while (!rtn)
		{
		++total_files;
		++found;
		confirm = 0;
		_splitpath(findbuf.achName,buffer,buffer,file,ext);  /* buffer is used as dummy areas */
		_makepath(buffer,drive,dir,file,ext);	  /* rebuild pathname */
		strlwr(buffer);
		if (confirm_flag)
			{
			strupr(buffer);
			fprintf(stderr,"Is it OK to touch \"%s\"? ",buffer);
			do
				{
				key = getch();
				key = toupper(key);
				}
			while ((key != 'N') && (key != 'Y'));
			fprintf(stderr,"%c\n",key);
			if (key == 'Y')
				{
				confirm = 1;
				}
			}
		if (!confirm_flag || (confirm_flag && confirm))
			{
			if (DosOpen( (char far *)buffer, (PHFILE)&fh, (PUSHORT)&action, 0L, 0x20, 0x1, _osmajor < 3 ? 0x2 : 0x42,0L))
				{
				printf("Error: Unable to open \"%s\"!\n",buffer);
				}
			else
				{
				if (verbose_flag)
					{
					printf("Touching %-12.12s -> %02u:%02u:%02u on %02u/%02u/%04u\n",
                            findbuf.achName,
                            time->hours,time->minutes,time->twosecs << 1,
                            date->month,date->day,date->year + 1980);
					}
				memset(&filestatus,0,sizeof(FILESTATUS));
				memcpy(&filestatus.ftimeLastWrite,time,sizeof(FTIME));
				memcpy(&filestatus.fdateLastWrite,date,sizeof(FDATE));

                if (DosSetFileInfo(fh, 0x1, (PBYTE)&filestatus, sizeof(FILESTATUS)))
					{
					printf("Error: Unable to change date on \"%s\"!\n",buffer);
					}
				else
					{
					++total_touched;
					}
				DosClose(fh);
				}
			}
		num = 1;
		rtn = DosFindNext(hdir,(PFILEFINDBUF)&findbuf,sizeof(FILEFINDBUF),&num);
		}
	if (!found && create_flag)
		{
		cptr = filename;
		while (*cptr)
			{
			found = 1;
			if ((*cptr == '?') || (*cptr == '*'))	/* no wild cards */
				{
				found = 0;
				break;
				}
			++cptr;
			}
		if (found)
			{
			confirm = 0;
			if (confirm_flag)
				{
				strupr(filename);
				fprintf(stderr,"Is it OK to create \"%s\"? ",filename);
				do
					{
					key = getch();
					key = toupper(key);
					}
				while ((key != 'N') && (key != 'Y'));
				fprintf(stderr,"%c\n",key);
				if (key == 'Y')
					{
					confirm = 1;
					}
				}
			if (!confirm_flag || (confirm_flag && confirm))
				{
				if (DosOpen((char far *)filename,(PHFILE)&fh,(PUSHORT)&action,0L,0x20,0x11,_osmajor <= 4 ? 0x2 : 0x42,0L))
					{
					printf("Error: Unable to create \"%s\"!\n",filename);
					}
				else
					{
					if (verbose_flag)
						{
						printf("Creating %-12.12s -> %02u:%02u:%02u on %02u/%02u/%04u\n",
                                findbuf.achName,
                                time->hours,time->minutes,time->twosecs << 1,
                                date->month,date->day,date->year + 1980);
						}
					memset(&filestatus,0,sizeof(FILESTATUS));
					memcpy(&filestatus.ftimeLastWrite,time,sizeof(FTIME));
					memcpy(&filestatus.fdateLastWrite,date,sizeof(FDATE));
					DosSetFileInfo(fh, 0x1, (PBYTE)&filestatus, sizeof(FILESTATUS));
					++total_created;
					++total_touched;
					DosClose(fh);
					}
				}
			}
		}
	if (!found)
		{
		printf("Error: Unable to find \"%s\"!\n",filename);
		}
	}

#endif

