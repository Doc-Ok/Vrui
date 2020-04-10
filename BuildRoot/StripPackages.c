#include <string.h>
#include <stdio.h>

int main(int argc,char* argv[])
	{
	/* Strip duplicate command line arguments from the back: */
	for(int stripIndex=1;argc-stripIndex>1;++stripIndex)
		{
		/* Get the next argument to strip out: */
		char* strip=argv[argc-stripIndex];
		
		/* Remove the argument from earlier in the command line: */
		int insert=1;
		int newArgc=argc;
		for(int test=1;test<argc-stripIndex;++test)
			{
			if(strcmp(strip,argv[test])!=0)
				{
				/* Keep the argument: */
				argv[insert]=argv[test];
				++insert;
				}
			else
				{
				/* Remove the argument: */
				--newArgc;
				}
			}
		
		/* Keep the rest of the command line: */
		for(int i=argc-stripIndex;i<argc;++i,++insert)
			argv[insert]=argv[i];
		argc=newArgc;
		}
	
	/* Print the stripped command line: */
	for(int i=1;i<argc;++i)
		{
		printf("%s%s",i>1?" ":"",argv[i]);
	  }
	printf("\n");
	
	return 0;
	}
