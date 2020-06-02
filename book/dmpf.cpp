#include <stdio.h>
#include <string.h>

static enum
{	errorOK = 0
, 	errorSyntax
,   errorProcessing
} error = errorOK ;

void syntax()
{
	printf("syntax: dmpf [-option]+ filename\n") ;
}

unsigned char print(unsigned char c) { return c >= 32 && c < 127 ? c : c==0 ? '_' : '.' ; }

int main(int argc, char* argv[])
{
	char* filename = argv[1] ;
	FILE* f = NULL ;

	if ( argc < 2 ) {
		syntax() ;
		error = errorSyntax ;
	}

	if ( !error ) {
		f = strcmp(filename,"-") ? fopen(filename,"rb") : stdin ;
		if ( !f ) {
			printf("unable to open file %s\n",filename) ;
			error = errorProcessing ;
		}
	}

	if ( !error  )
	{
		char line[1000] ;
		char buff[32]   ;
		int  n          ;
		int count = 0   ;
		while ( (n = fread(buff,1,sizeof buff,f)) > 0 )
		{
			// line number
			int l = sprintf(line,"%#8x %8d: ",count,count ) ;
			count += n ;

			// ascii print
			for ( int i = 0 ; i < n ; i++ )
			{
				char c = buff[i] ;
		        l += sprintf(line+l,"%c", print(c)) ;
			}
			// blank pad the ascii
			int save = n ;
			while ( n++ < sizeof(buff) ) {
			    l += sprintf(line+l," ") ;
			}
			n = save     ;

		    // hex code
		    l += sprintf(line+l,"  -> ") ;
			for ( int i = 0 ; i < n ; i++ )
			{
				unsigned char c = buff[i] ;
				l += sprintf(line+l," %02x" ,c) ;
			}

			line[l] = 0 ;
			printf("%s\n",line) ;
		}

        if ( f != stdin ) {
            fclose(f);
            f = NULL;
        }
	}

	return error ;
}
