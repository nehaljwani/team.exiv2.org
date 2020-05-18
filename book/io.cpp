// ***************************************************************** -*- C++ -*-
// io.cpp
// Program to illustrate IMaEA.md Chapter 7 I/O in Exiv2

// g++ io.cpp -o io -lexiv2

#include <exiv2/exiv2.hpp>

#include <iostream>
#include <stdio.h>

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif//UNUSED

// sinargv - s in argv
bool sinargv(const char* s,const Exiv2::StringVector& a,int from,int to)
{
    bool    bResult = false ;
    int     i       = from  ;
    while ( !bResult && i < to ) {
        bResult = s == a[i]  ;
        if ( !bResult ) i++;
    }
    return bResult ? i : 0 ;
}

enum                        // keyword indices
{   kwHELP = 0
,   kwVERBOSE
,   kwGETB
,   kwVERB
,   kwCP
,   kwLS
,   kwVERSION
,   kwMAX                   // manages keyword array
};

class Options  {
public:
    bool        verbose;
    bool        help;
    bool        getb;
    int         verb;
    int         argc;
    int         arg0;

    Options(const Exiv2::StringVector& args,char const* keywords[])
    : verbose(false)
    , help(false)
    , getb(false)
    , verb(0)
    , arg0(0)
    {
        int argc = args.size();
        for ( int i = 0 ; i < argc && arg0 == 0 ; i++ ) {
            if ( args[i][0] != '-' ) {
                arg0=i;
            }
        }
        help    = sinargv(keywords[kwHELP   ],args,0,arg0) != 0 || argc < 2;
        verbose = sinargv(keywords[kwVERBOSE],args,0,arg0) != 0 ;
        getb    = sinargv(keywords[kwGETB   ],args,0,arg0) != 0 ;
        if ( arg0 < argc ) {
            for ( int i = kwVERB+1 ; i < kwMAX ; i++ ) {
                if (sinargv(keywords[i],args,arg0,arg0+1) != 0 ) {
                    verb = i;
                }
            }
        }
    }

    virtual ~Options() {} ;
} ;

int help(const char* program,char const* words[],int nWords,bool bVerbose)
{
    printf("usage: %s [options]+ verb [arg...]\n",program);
    printf("OPTIONS: ");
    for ( int i = 0 ; i < kwVERB ; i++ ) {
        if ( words[i] ) {
            printf(" %s",words[i]);
        }
    }
    printf("\n");
    printf("VERBS: ");
    for ( int i = kwVERB+1 ; i < nWords ; i++ ) {
        printf("%s ",words[i]);
    }
    printf("\n");
    return 0;

    UNUSED(bVerbose);
}

// Command dispatchers
typedef int command(int argc,const char** argv,const Options& options);

static int cp (int argc,const char** argv,const Options& options)
{
    // printf("cp argc = %d ",argc); for ( int i = 0 ; i < argc ; i++ ) printf("%s ", argv[i]); printf("\n");
    int rc = 0 ;
    Exiv2::BasicIo::AutoPtr pIn = Exiv2::ImageFactory::createIo(argv[0],true);

    if ( rc == 0 && pIn->open() != 0 ) {
        fprintf(stderr,"unable to open input file %s\n",argv[0]);
        rc = 1 ;
    }

    Exiv2::FileIo   out(argv[1]);
    if ( rc == 0 && out.open("w+b") != 0 ) {
        fprintf(stderr,"unable to open output file %s\n",argv[1]);
        rc = 1;
    }

    if ( rc == 0 ) {
        int  count = 0;
        char method[100];
        strcpy(method,"unknown");
        bool bEnd = pIn->eof() ;

        if ( options.getb ) {
            strcpy(method,"getb");
            while ( !bEnd ) {
                Exiv2::byte b = (Exiv2::byte) pIn->getb();
                bEnd = pIn->eof();
                if ( !bEnd ) {
                    out.putb(b);
                    count++;
                }
            }
        } else {
            strcpy(method,"read");
            Exiv2::byte buffer[128 * 1024];
            while (!bEnd) {
                int n = pIn->read(buffer, sizeof(buffer));
                out.write(buffer, n);
                count += n;
                bEnd = pIn->eof();
            }
        }
        printf("copied %d bytes by %s method\n",count,method);
    }
    pIn->close();

    return rc;
    UNUSED(options);
}

static int ls(int argc,const char** argv,const Options& options)
{
    printf("ls argc = %d ",argc);
    return 0;

    UNUSED(argv);
    UNUSED(options);
}

static  int version(int argc,const char** argv,const Options& options)
{
    exv_grep_keys_t                  keys;
    Exiv2::dumpLibraryInfo(std::cout,keys);

    return 0;

    UNUSED(argc);
    UNUSED(argv);
    UNUSED(options);
}

int main(int argc, const char* argv[])
{
    int         rc      = 0;
    const char* program = argv[0];

    command*    commands[kwMAX];
    char const* keywords[kwMAX];
    memset(keywords,0,sizeof(keywords));
    memset(commands,0,sizeof(commands));

    keywords[kwHELP    ] = "--help"   ;
    keywords[kwVERBOSE ] = "--verbose";
    keywords[kwGETB    ] = "--getb"   ;
    keywords[kwVERB    ] = ""         ;
    keywords[kwCP      ] = "cp"       ; commands[kwCP      ] = cp;
    keywords[kwLS      ] = "ls"       ; commands[kwLS      ] = ls;
    keywords[kwVERSION ] = "version"  ; commands[kwVERSION ] = version ;

    Exiv2::Dictionary shorts;
    shorts["-?"] = "--help";
    shorts["-h"] = "--help";
    shorts["-v"] = "--verbose";
    shorts["-b"] = "--getb";

    Exiv2::StringVector Args;
    for ( int i = 0 ; i < argc ; i++ ) {
        Args.push_back(argv[i]);
        if ( shorts.find(argv[i]) != shorts.end() ) {
            Args[i] = shorts.find(argv[i])->second;
        }
    }

    Options options(Args,keywords);
    if ( !options.verb ) {
        rc = 1 ;
        options.help=true;
        fprintf(stderr,"No verb\n");
    }

    if ( options.help ) {
        ::help(program,keywords,kwMAX,options.verbose);
    } else if ( rc == 0 ) {
        rc = commands[options.verb](argc-options.arg0-1,argv+options.arg0+1,options);
    }

    return rc;
}
