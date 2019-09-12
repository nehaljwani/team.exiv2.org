#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
    bool     bSwap = false;
    unsigned bytes = 2;

	if ( argc > 1 ) {
		for ( int i = 1 ; i < argc ; i++ ) {
			char* arg = argv[i] ;
			bool  dash = strlen(arg) >= 2 ;
			if ( dash ) dash =  arg[0] == '-' ;
			if ( dash ) switch (arg[1]) {
				case '1' : bytes = 1 ; break;
				case '2' : bytes = 2 ; break;
				case '4' : bytes = 4 ; break;
				case 's' : bSwap = true ; break;
				default : printf("illegal option %s\n",arg);
			} else {
			    if ( bytes == 2 ) {
			        unsigned short v = (unsigned short) atoi(arg);
			        unsigned char* p = (unsigned char*) &v;
			        if ( bSwap ) printf("0x%02x%02x ",p[0],p[1]);
			        else         printf("0x%02x%02x ",p[1],p[0]);
			    }
			    if ( bytes == 4 ) {
			        unsigned long  v = (unsigned long) atoi(arg);
			        unsigned char* p = (unsigned char*) &v;
			        if ( bSwap ) printf("0x%02x%02x%02x%02x ",p[0],p[1],p[2],p[3]);
			        else         printf("0x%02x%02x%02x%02x ",p[3],p[2],p[1],p[0]);
			    }
			    if ( bytes == 1 ) {
			        unsigned long  v = (unsigned long) atoi(arg);
			        printf("0x%02lx",v);
			    }
			}
		}
	} else {
	    printf("usage: %s [option] number]+   option: -1|-2|-4|-s",argv[0]);
	}
	printf("\n");
	return 0 ;
}