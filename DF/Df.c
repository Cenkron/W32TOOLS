/********************************************************************\
                                DF
 --------------------------------------------------------------------
            Prints available space on a disk drive.
 --------------------------------------------------------------------
   Copyright (c) 1985-2018 Miller Micro Systems - All Rights Reserved
\********************************************************************/

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fwild.h>

/*------------------------------------------------------------------*/

int	v_flag = 0;
int	unc;
int	result = 0;
char	PathName [1024];

/*------------------------------------------------------------------*/

typedef __int64 INT64;
typedef INT64 *PINT64;

int	process  (void);
char   *km_bytes (INT64 count);
char   *percent  (INT64, INT64);

/*------------------------------------------------------------------*/
	char *
usagedoc[] = {
	"Usage:  df [-?] [-v[v[v]]] [d]",
	"",
	"Report available bytes on drive",
	"",
	"    -v    report total, used and available bytes on drive",
	"    -vv   report above plus clusters information",
	"    -vvv  report above plus sector information",
//	"    -vvvv report above plus physical disk information",     //bwj
	"    d     report for drive 'd' (else use current drive)",
	"",
	"Version 7.0 Copyright (c) 2000 J & M Software, Dallas TX - All Rights Reserved",
	"",
	NULL};

/*------------------------------------------------------------------*/
	int
main (
	int argc,
	char *argv[])

	{
	int  c;


	optenv = getenv("DF");

	while ((c=getopt(argc, argv, "vV?")) != EOF)
	switch (tolower(c))
		{
		case 'v':
			++v_flag;
			break;

		case '?':
			help();

		default:
			fprintf(stderr, "invalid option '%c'\n", optchar);
			usage();
		}


	if (optind >= argc)
		{
		sprintf(PathName, "%c:\\", (char)(((int)('A') + getdrive() - 1)));
		unc = FALSE;
		result |= process();
		}
	else
		{
		while (optind < argc)
			{
			if ( ! fnchkunc(argv[optind]))
				{
				sprintf(PathName, "%c:\\", (char)(toupper(*argv[optind])));
				unc = FALSE;
				}
			else
				{
				strcpy(PathName, argv[optind]);
				strcat(PathName, "\\");
				fnreduce(PathName);
				unc = TRUE;
				}
			result |= process();
			++optind;
			}
		}
	return (0);
	}

/**********************************************************************/

	int
process (void)

	{
	INT64   dummy2;
	INT64   userfreespace;
	INT64   usermaxspace;
	char   *pszVolumeName;	// = NULL;
	char    fileSystem [32];

	DWORD   SectorsPerCluster = 0;
	DWORD   BytesPerSector    = 0;
	DWORD   FreeClusters      = 0;
	DWORD   Clusters          = 0;


// This code now does comprehend UNC pathnames

	char *pszAbsolutePath = fnabspth(PathName);
	if (pszAbsolutePath != NULL)
		{
		pszVolumeName = vol_name(pszAbsolutePath);
		if (pszVolumeName == NULL)
			pszVolumeName = "(unknown)";
		free(pszAbsolutePath);
		}
	else
		pszVolumeName = "(unknown)";


	if ( ! GetVolumeInformation(PathName,
			NULL,
			0,
			NULL,
			NULL,
			NULL,
			&fileSystem[0],
			sizeof(fileSystem)))
		{
		fprintf(stderr, "\007DF: Error getting file system type info\n");
		return (1);
		}

	const int fatDrive = _stricmp(fileSystem, "FAT32") != 0
					  || _stricmp(fileSystem, "NTFS" ) == 0;

	if (fatDrive)
		{
		if ( ! GetDiskFreeSpace(PathName,
				&SectorsPerCluster,
				&BytesPerSector,
				&FreeClusters,
				&Clusters))
			{
			fprintf(stderr, "\007DF: Error getting file system info\n");
			return (1);
			}

		userfreespace = (INT64)(SectorsPerCluster)
					  * (INT64)(BytesPerSector)
					  * (INT64)(FreeClusters);

		usermaxspace  = (INT64)(SectorsPerCluster)
					  * (INT64)(BytesPerSector)
					  * (INT64)(Clusters);
		}
	else
		{
		if ( ! GetDiskFreeSpaceEx(PathName,
				(PULARGE_INTEGER)(&userfreespace),
				(PULARGE_INTEGER)(&usermaxspace),
				(PULARGE_INTEGER)(&dummy2)))
			{
			fprintf(stderr, "\007DF: Error getting file system info\n");
			return (1);
			}
		}

	const INT64 usedspace = usermaxspace - userfreespace;

	if (v_flag > 0)
		{
		if ( ! unc)
			strcpy(&PathName[1], ":");
		printf("\nDisk usage for %s  %s  (%s)\n", PathName, pszVolumeName, fileSystem);

		if (fatDrive)
			{
			if (v_flag > 2)
				{
				printf("%12u  Bytes   per sector\n", BytesPerSector);
				printf("%12lu  Sectors per cluster\n", SectorsPerCluster);
				printf("%12lu  Bytes   per cluster\n", (long)BytesPerSector * (long)SectorsPerCluster);
				}

			if (v_flag > 1)
				{
				printf("%12ld  Total     clusters\n", Clusters);
				printf("%12ld  Available clusters\n", FreeClusters);
				}
			}

		printf("%s  Total bytes\n", 
				km_bytes(usermaxspace));
		printf("%s  Used  bytes (%s)\n", 
				km_bytes(usedspace), percent(usedspace, usermaxspace));
		}

	printf("%s  Free  bytes (%s)\n", 
	km_bytes(userfreespace), percent(userfreespace, usermaxspace));

	return (1);
	}

/**********************************************************************/
	char *
percent (
	INT64 numerator,
	INT64 denominator)

	{
static char buffer [80];
	int	   pct = 0;
	char  *p;


//  printf("num = %I64d  den = %I64d\n", numerator, denominator);

	if      (denominator <= 0)
		p = "<n/0 err>";
	else if (numerator <= 0)
		p = "  0%%";
	else if (numerator >= denominator)
		p = "100%%";
	else
		{
		pct = ((int) ((((200I64 * numerator) / denominator) + 1I64) / 2I64));

		if (pct <= 0)
			p = "< 1%%";
		else if (pct > 99)
			p = ">99%%";
		else /* (1 <= pct <= 99) */
			p = " %2d%%";
		}

	sprintf(buffer, p, pct);
	return (buffer);
	}

/**********************************************************************/
	char *
km_bytes (
	INT64 count)

	{
static char buffer [80];
	INT64  trial   = count;
	INT64  divisor = 5I64;
	char   unit    = '?';
	int    mode    = 0;


	if (count >= 1000)
		{
		do  {
			++mode;
			trial = (count / divisor);
			if (trial < 1999I64)
				{
//  printf("mode = %d  trial = %I64d\n", mode, trial);
				trial = (count + divisor) / (divisor + divisor);
//  printf("mode = %d  trial = %I64d\n", mode, trial);
				break;
				}
			divisor *= 10I64;
			} while (mode < 12);
		}

	if (mode == 0)
		{
		sprintf(buffer, "%12I64d         ", count);
		return (buffer);
		}

	switch ((mode - 1) / 3)
		{
		case 0:	unit = 'K';	break;
		case 1:	unit = 'M';	break;
		case 2:	unit = 'G';	break;
		case 3:	unit = 'T';	break;
		default: ;
		}

	switch (mode % 3)
		{
		default:  // (xxx U)
			sprintf(buffer, "%12I64d ( %03I64d %c)", 
				count,
				trial,
				unit);
			break;

		case 1:   // (x.yy U)
			sprintf(buffer, "%12I64d (%1I64d.%02I64d %c)", 
					count,
					(trial / 100I64),
					(trial % 100I64),
					unit);
			break;

		case 2:   // (xx.y U)
			sprintf(buffer, "%12I64d (%2I64d.%1I64d %c)", 
					count,
					(trial / 10I64),
					(trial % 10I64),
					unit);
					break;
		}

	return (buffer);
	}

/**********************************************************************/

#if 0 //bwj old method used for OS/2
	if ((v_flag > 3)
	&&  ((_osmode == OS2_MODE) || (_osmajor >= 10)))
		{
		if (drivechar == '\0')
			{
			fprintf(stderr, "\007DF: Drive not specified\n");
			return (1);
			}

// Note: PathName cannot be UNC; it must be "X:\"

		if ((rc=DosOpen(PathName, &handle, &dummy, 0L, FILE_NORMAL, FILE_OPEN, 
				OPEN_ACCESS_READWRITE|OPEN_SHARE_DENYREADWRITE|OPEN_FLAGS_DASD, 
				0L)) != 0)
			{
			fprintf(stderr, "\007DF: Error %d getting drive handle\n", rc);
			return (1);
			}

		command = 1;
		rc = DosDevIOCtl((PVOID)&bpb, &command, DSK_GETDEVICEPARAMS, 
				IOCTL_DISK, handle);
		DosClose(handle);
		if (rc != 0)
			{
			fprintf(stderr, "\007DF: Error %d getting drive info\n", rc);
			return (1);
			}

		printf("%9u Heads\n", bpb.cHeads);
		printf("%9u Cylinders\n", bpb.cCylinders);
		printf("%9u Sectors per track\n", bpb.usSectorsPerTrack);
		printf("\n");
		}
#endif //bwj old method used for OS/2

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
