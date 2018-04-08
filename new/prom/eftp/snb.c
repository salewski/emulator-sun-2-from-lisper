/*#define	LOADPROG 1*/
/* Copyright (C) 1981 by Stanford University */

/*
 * snb.c
 *
 * network bootstrap program - minimal version.
 *
 * Jeffrey Mogul	13 February 1981
 *
 */

#define	BFN 1	/* # of boot file to request */
#define BFNAME "shasta:/usr/sun/monitor/pcmon/boot/test.r"

#ifdef	LOADPROG
#define	program	((char *)0x1D000)	/* starting point of program */
#else
char program[10000];
#endif

#ifdef	INTERACTIVE
char bfname[100];
#endif	INTERACTIVE

main()
{
	int plen;
	int (*fp)();
	int entry;
	char *bfnptr;

/*	plen = getbootfile(BFN,BFNAME,program,0);
	entry = setup(program,plen);
	
	fp = (int (*)())entry; */
#ifdef	INTERACTIVE
again:
	printf("Enter filename to boot:  ");
	gets(bfname);
	bfnptr = bfname;
#else
	bfnptr = BFNAME;
#endif	INTERACTIVE
#ifdef	LOADPROG
	fp = (int (*)())setup(program,getbootfile(BFN,bfnptr,program,0));
	fp();
#else
	printf("getbootfile returns %d\n",getbootfile(BFN,bfnptr,program,0));
	program[1000] = 0;
	printf(program);
	goto again;
#endif	LOADPROG
}