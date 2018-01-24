/********************************************************************\
                                DS
 --------------------------------------------------------------------
              Prints disk space required for a file set.
 --------------------------------------------------------------------
 Copyright (c) 1985-2018 Miller Micro Systems - All Rights Reserved
 Copyright (c) 2007 Miller Micro Systems and B Johnson - All Rights Reserved
\********************************************************************/

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fwild.h>

/*------------------------------------------------------------------*/

//#define       XT
#define         AT
#define         PS2
#define         CUR

/*------------------------------------------------------------------*/
    char *
usagedoc[] = {
    "Usage:  ds  [-?] [-dx:] [-bfhqsv] [-x[@]xspec] [path\\][file_spec]",
    "",
    "Displays the total amount of space required on various",
    "sized disks (or a particular disk) to store the specified files.",
    "",
    "-dx:     get disk type from drive x:",
    "-f       include estimated floppy diskette counts",
    "-q       query: determine if fileset will fit on default drive",
    "               (or drive x: if -dx: specified)",
    "               return errorlevel == 1 if fileset will not fit",
    "-h       hidden: include hidden files",
    "-s       system: include system files",
    "-b       brief: no output to screen",
    "-v       verbose: output directory name also",
    "-x xspec exclude: don't count files which match xspec",
    "-x@xspec exclude: don't count files which match xspec(s) in xfile",
    "",
    "If 'file_spec' is not specified, then '*.*' is assumed.",
    "The recursive path name '**' may be used to specifiy the inclusion",
    "of all subdirectories.  If '[path\\]' is not specified, then the",
    "current directory is assumed.",
    "",
    "Version 6.5 Copyright (c) 1995 J & M Software, Dallas TX - All Rights Reserved",
    "",
    NULL};

/*------------------------------------------------------------------*/

void    process         (char *fw);
void    proc_file       (char *dta);
int     get_drive_info  (int d);
char    plural          (UINT64 i);
char *  km_bytes        (UINT64 count);
UINT64  DiskCount       (UINT64 TotalBytes, UINT64 DiskSize);

/*------------------------------------------------------------------*/

BOOL            bBflag          = FALSE;
BOOL            bDflag          = FALSE;
BOOL            bFflag          = FALSE;
BOOL            bHflag          = FALSE;
BOOL            bQflag          = FALSE;
BOOL            bSflag          = FALSE;
BOOL            bVflag          = FALSE;

int             iDrive          = 0;

UINT64          Files           = 0;
UINT64          FreeSpace       = 0L;
UINT64          TotalData       = 0;

char            szLegend [1024] = {0};

/*------------------------------------------------------------------*/

#define         K       ((UINT64)(1000))
#define         M       ((K)*(K))
#define         G       ((K)*(M))

#if defined (XT)
#define XT_BLK          1024    /* dsdd 5.25" xt floppy */
#define XT_MASK         (1 + (~XT_BLK))
#define XT_DISK         362496L
UINT64  xt_total        = 0;
#endif

#if defined (AT)
#define AT_BLK          512     /* dshd 5.25" at floppy */
#define AT_MASK         (1 + (~AT_BLK))
#define AT_DISK         1228800L
UINT64  at_total        = 0;
#endif

#if defined (PS2)
#define MFD_BLK         1024    /* ds 3.5" microfloppy */
#define MFD_MASK        (1 + (~MFD_BLK))
#define MFD_DISK        1474560L
UINT64  mfd_total       = 0;
#endif

#if defined (CUR)
#define DR_BLK          dr_blk
UINT64  dr_blk;
#define DR_MASK         (1 + (~DR_BLK))
#define DR_DISK         dr_disk
UINT64  dr_disk;
UINT64  dr_total        = 0;
#endif

/**********************************************************************/
    int
main (int argc, char *argv[])
    {
    int         retval  = 0;
    UINT64      i;
    int         c;

    optenv = getenv("DS");

    while ((c=getopt(argc, argv, "?bBd:D:fFhHqQsSvVx:X:")) != EOF)
        switch (tolower(c))
            {
            case 'x':
                if (fexclude(optarg))
                    {
                    fprintf(stderr, "Error excluding '%s'\n", optarg);
                    exit (1);
                    }
                break;

            case 'd':
                ++bDflag;
                if (*optarg)
                    iDrive = tolower(*optarg) - 'a' + 1;
                break;

            case 'b':		++bBflag;	break;
            case 'f':		++bFflag;	break;
            case 'h':		++bHflag;	break;
            case 'q':		++bQflag;	break;
            case 's':		++bSflag;	break;
            case 'v':		++bVflag;	break;

            case '?':
                help();

            default:
                fprintf(stderr, "invalid option '%c'\n", optchar);
                usage();
            }

    if (iDrive == 0)
	iDrive = getdrive();
    get_drive_info(iDrive);

    if (argv[optind])
        {
        while (argv[optind])
            {
            strncat(szLegend, argv[optind], sizeof(szLegend));
            strncat(szLegend, " ", sizeof(szLegend));
            process(argv[optind]);
            ++optind;
            }
        }
    else
        {
        strcpy(szLegend, "*.*");
        process("*.*");
        }

    if (!bBflag)
        {
        if (bVflag)
            printf("%s:  ", szLegend);
        printf("%s byte%c used in %I64u file%c\n",
            km_bytes(TotalData), plural(TotalData), Files, plural(Files));
        }

    if (bFflag && !bBflag && !bDflag)
        {
#if defined (PS2)
        if (bVflag)
            printf("%s:  ", szLegend);
        i = DiskCount(mfd_total, MFD_DISK);
        printf("%s byte%c free required on ", km_bytes(mfd_total), plural(mfd_total));
        printf("%3I64u 3.5\"  HD diskette%c\n", i, plural(i));
#endif

#if defined (AT)
        if (bVflag)
            printf("%s:  ", szLegend);
        i = DiskCount(dr_total, AT_DISK);
        printf("%s byte%c free required on ", km_bytes(at_total), plural(at_total));
        printf("%3I64u 5.25\" HD diskette%c\n", i, plural(i));
#endif

#if defined (XT)
        if (bVflag)
            printf("%s:  ", szLegend);
        i = DiskCount(dr_total, XT_DISK);
        printf("%s byte%c free required on ", km_bytes(xt_total), plural(xt_total));
        printf("%3I64u 5.25\" DD diskette%c\n", i, plural(i));
#endif
        }

#if defined (CUR)
    if (!bBflag && !bDflag)
        {
        if (bVflag)
            printf("%s:  ", szLegend);
        printf("%s byte%c consumed on current disk\n", km_bytes(dr_total), plural(dr_total));
        }
#endif

#if defined (CUR)
    if (!bBflag && bDflag)
        {
        if (bVflag)
            printf("%s:  ", szLegend);
        i = DiskCount(dr_total, DR_DISK);
        printf("%s bytes free required on ", km_bytes(dr_total));
        printf("%3I64u diskette%c\n", i, plural(i));
        }
#endif

#if defined (CUR)
    if (bQflag)
        {
        if (FreeSpace < dr_total)
            retval = 1;
        if (!bBflag)
            printf("Fileset will probably%s fit\n", retval ? " not" : "");
        }
#endif

    return (retval);
    }

/**********************************************************************/
/**********************************************************************/
    void
process (char *fw)
    {
    int attrib = FW_FILE;
    if (bHflag)	attrib |= FW_HIDDEN;
    if (bSflag)	attrib |= FW_SYSTEM;

    char *      dta = fwinit(fw, attrib);

    if (!dta)
        {
        printf("\nfinit fail\n");
        _exit(1);
        }

    while (fwildexcl(dta) != NULL)
        proc_file(dta);
    }

/**********************************************************************/
/**********************************************************************/
    void
proc_file (char *dta)
    {
    const UINT64      size = fwsize(dta);

    TotalData +=  size;

#if defined (XT)
    xt_total   += (size + XT_BLK -1) & XT_MASK;
#endif

#if defined (PS2)
    mfd_total  += (size + MFD_BLK -1) & MFD_MASK;
#endif

#if defined (AT)
    at_total   += (size + AT_BLK -1) & AT_MASK;
#endif

#if defined (CUR)
    dr_total   += (size + DR_BLK -1) & DR_MASK;
#endif

    ++Files;
    }

/**********************************************************************/
    int
get_drive_info (int d)
    {
    DWORD    SectorsPerCluster = 0;
    DWORD    BytesPerSector    = 0;
    DWORD    FreeClusters      = 0;
    DWORD    Clusters          = 0;
    char     drivebuf [4];

    drivebuf[0] = 'A' + (d - 1);
    drivebuf[1] = ':';
    drivebuf[2] = '\\';
    drivebuf[3] = '\0';

    if ( ! GetDiskFreeSpace(drivebuf,
		&SectorsPerCluster,
		&BytesPerSector,
		&FreeClusters,
		&Clusters))
	{
        fprintf(stderr, "\007DS: Error getting file system info\n");
	return (1);
	}

    dr_blk     = (UINT64)(SectorsPerCluster)     // bytes per cluster
               * (UINT64)(BytesPerSector);

    FreeSpace  = (UINT64)(SectorsPerCluster)     // free space on disk
	       * (UINT64)(BytesPerSector)
	       * (UINT64)(FreeClusters);

    dr_disk    = (UINT64)(SectorsPerCluster)     // total space on disk
               * (UINT64)(BytesPerSector)
               * (UINT64)(Clusters);

    return (0);
    }

/**********************************************************************/
    char *
km_bytes (UINT64 count)
    {
static char buffer [80];
    UINT64   rounded = count;

    if (count > (100*M))
	{
	rounded += 500*K;
	sprintf(buffer, "%9I64u (%I64u M)", count, rounded/M);
	}
    else if (count > M)
	{
	rounded += 50*K;
	sprintf(buffer, "%9I64u (%I64u.%I64u M)",
	    count, rounded/(1L*M), (rounded%(M))/(100*K));
	}
    else if (count > K)
	{
	rounded += K/2;
	sprintf(buffer, "%9I64u (%I64u K)", count, rounded/(K));
	}

    else
	{
	sprintf(buffer, "%9I64u (<1K)", count);
	}

    return (buffer);
    }

/**********************************************************************/
    char
plural (UINT64 i)
    {

    return (char) ( (i == 1) ? ' ' : 's' );
/*
    return (char)(
        (i > 1) ? 's' : 
        (i==0)  ? 's' :
        ' ');
*/
    }

/**********************************************************************/
    UINT64
DiskCount (UINT64 TotalBytes, UINT64 DiskSize)
    {
    UINT64  Count = TotalBytes/DiskSize;

    if ((TotalBytes % DiskSize) != 0)
        ++Count;

    return (Count);
    }

/**********************************************************************/
/*                              EOF                                   */
/**********************************************************************/
