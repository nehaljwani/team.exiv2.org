#include <iostream>
#include <set>
#include <vector>
#include <string>

// crummy old c magic
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

typedef unsigned short int uint16_t ;
typedef          short int  int16_t ;
typedef unsigned char       byte    ;

enum type_e {
        unsignedByte       = 1, //!< Exif BYTE type, 8-bit unsigned integer.
        asciiString        = 2, //!< Exif ASCII type, 8-bit byte.
        unsignedShort      = 3, //!< Exif SHORT type, 16-bit (2-byte) unsigned integer.
        unsignedLong       = 4, //!< Exif LONG type, 32-bit (4-byte) unsigned integer.
        unsignedRational   = 5, //!< Exif RATIONAL type, two LONGs: numerator and denumerator of a fraction.
        signedByte         = 6, //!< Exif SBYTE type, an 8-bit signed (twos-complement) integer.
        undefined          = 7, //!< Exif UNDEFINED type, an 8-bit byte that may contain anything.
        signedShort        = 8, //!< Exif SSHORT type, a 16-bit (2-byte) signed (twos-complement) integer.
        signedLong         = 9, //!< Exif SLONG type, a 32-bit (4-byte) signed (twos-complement) integer.
        signedRational     =10, //!< Exif SRATIONAL type, two SLONGs: numerator and denumerator of a fraction.
        tiffFloat          =11, //!< TIFF FLOAT type, single precision (4-byte) IEEE format.
        tiffDouble         =12, //!< TIFF DOUBLE type, double precision (8-byte) IEEE format.
        tiffIfd            =13, //!< TIFF IFD type, 32-bit (4-byte) unsigned integer.
        unsignedLongLong   =16, //!< Exif LONG LONG type, 64-bit (8-byte) unsigned integer.
        signedLongLong     =17, //!< Exif LONG LONG type, 64-bit (8-byte) signed integer.
        tiffIfd8           =18, //!< TIFF IFD type, 64-bit (8-byte) unsigned integer.
        string        =0x10000, //!< IPTC string type.
        date          =0x10001, //!< IPTC date type.
        // time          =0x10002, //!< IPTC time type.
        comment       =0x10003, //!< %Exiv2 type for the Exif user comment.
        directory     =0x10004, //!< %Exiv2 type for a CIFF directory.
        xmpText       =0x10005, //!< XMP text type.
        xmpAlt        =0x10006, //!< XMP alternative type.
        xmpBag        =0x10007, //!< XMP bag type.
        xmpSeq        =0x10008, //!< XMP sequence type.
        langAlt       =0x10009, //!< XMP language alternative type.
        invalidTypeId =0x1fffe, //!< Invalid type id.
        lastTypeId    =0x1ffff  //!< Last type id.
}; 

enum error_e
{  kerCorruptedMetadata
,  kerTiffDirectoryTooLarge
,  kerInvalidTypeValue
,  kerInvalidMalloc
,  kerNotATiff
};

enum PrintStructureOption_e
{    kpsBasic
,    kpsXMP 
,    kpsRecursive
,    kpsIccProfile
};

enum seek_e
{    beg = SEEK_SET
,    cur = SEEK_CUR
,    end = SEEK_END
};

void Error (error_e error)
{
	std::cerr << "*** error *** ";
	switch ( error ) {
	    case   kerCorruptedMetadata      : std::cerr << "corrupted metadata"       ; break;
	    case   kerTiffDirectoryTooLarge  : std::cerr << "tiff directory too large" ; break;
	    case   kerInvalidTypeValue       : std::cerr << "invalid type"             ; break;
	    case   kerInvalidMalloc          : std::cerr << "invalid malloc"           ; break;
	    case   kerNotATiff               : std::cerr << "Not a tiff"               ; break;
        default                          : std::cerr << "unknown"                  ; break;
	}
	std::cerr << std::endl;
	_exit(1); // pull the plug!
}

std::string tagName(uint16_t /* tag */ )
{
	return "unknown tag";
}

class DataBuf
{
public:
	byte*   pData_;
	size_t  size_ ;
	DataBuf(size_t size)
	: pData_(NULL)
	, size_(size)
	{
		pData_ = new byte[size_];
	}
	virtual ~DataBuf()
	{
		if ( pData_ ) {
			delete pData_ ;
			pData_ = NULL ;
		}
		size_ = 0 ;
	}
};

std::string indent(size_t s)
{
	std::string result ;
	while ( s-- ) result += "  ";
	return result ;
}

std::string stringFormat(const char* format, ...)
{
	std::string result;
	std::vector<char> buffer;
	size_t need = std::strlen(format)*8;  // initial guess
	int rc = -1;

	// vsnprintf writes at most size (2nd parameter) bytes (including \0)
	//           returns the number of bytes required for the formatted string excluding \0
	// the following loop goes through:
	// one iteration (if 'need' was large enough for the for formatted string)
	// or two iterations (after the first call to vsnprintf we know the required length)
	do {
		buffer.resize(need + 1);
		va_list args;            // variable arg list
		va_start(args, format);  // args start after format
		rc = vsnprintf(&buffer[0], buffer.size(), format, args);
		va_end(args);     // free the args
		assert(rc >= 0);  // rc < 0 => we have made an error in the format string
		if (rc > 0)
			need = static_cast<size_t>(rc);
	} while (buffer.size() <= need);

	if (rc > 0)
		result = std::string(&buffer[0], need);
	return result;
}

std::string binaryToString(DataBuf& dataBuf,size_t start,size_t size)
{
	std::string result;
	size_t i    = start;
	byte*  buff = dataBuf.pData_;
	while (i < start+size ) {
		result += (32 <= buff[i] && buff[i] <= 127) ? buff[i]: '.'; 
		i++ ;
	}
	return result;
}

class Io
{
public:
	Io(std::string path,std::string open)
	: path_(path)
	{ f_ = ::fopen(path.c_str(),open.c_str()); }
	virtual ~Io() { close() ; }
	
	std::string path() { return path_; }
	
	size_t read(unsigned char* buff,size_t size) { return fread(buff,size,1,f_);}
	size_t read(DataBuf& buff)                   { return read(buff.pData_,buff.size_); }
	int    eof()                                 { return feof(f_) ; }
    size_t tell()                                { return ftell(f_) ; }
	int    seek(size_t offset,seek_e from)       { return fseek(f_,offset,from) ; }
	void   close()                               { if ( good() ) { fclose(f_) ; f_ = NULL  ;} }
	size_t size()                                { struct stat st ; fstat(fileno(f_),&st) ; return st.st_size ; }
	bool   good()                                { return f_ ? true : false ; }

private:
	FILE*       f_;
	std::string path_;
};

class Visitor
{
public:
	Visitor();
	virtual ~Visitor() {};
	
	void directoryBegin();
	void directoryEnd();
};

class TiffImage
{
public:
	TiffImage(std::string path)
	: io_(path,"rb")
	, good_(false)
	{
		good_ = io_.good();
	}
	virtual ~TiffImage() { io_.close() ; }
	bool good() { return good_ && io_.good() ; }
	void printStructure(std::ostream& out, PrintStructureOption_e option,int depth = 0 ,size_t offset = 0 );

private:
    Io             io_;
	bool           good_;
    std::set<size_t> visits_; 

    bool isStringType(uint16_t type)
    {
        return type == asciiString
            || type == unsignedByte
            || type == signedByte
            || type == undefined
            ;
    }
    bool isShortType(uint16_t type) {
         return type == unsignedShort
             || type == signedShort
             ;
    }
    bool isLongType(uint16_t type) {
         return type == unsignedLong
             || type == signedLong
             ;
    }
    bool isLongLongType(uint16_t type) {
        return type == unsignedLongLong
            || type == signedLongLong
            ;
    }
    bool isRationalType(uint16_t type) {
         return type == unsignedRational
             || type == signedRational
             ;
    }
    bool is2ByteType(uint16_t type)
    {
        return isShortType(type);
    }
    bool is4ByteType(uint16_t type)
    {
        return isLongType(type)
            || type == tiffFloat
            || type == tiffIfd
            ;
    }
    bool is8ByteType(uint16_t type)
    {
        return isRationalType(type)
             || isLongLongType(type)
             || type == tiffIfd8
             || type == tiffDouble
            ;
    }
    bool isPrintXMP(uint16_t type, PrintStructureOption_e option)
    {
        return type == 700 && option == kpsXMP;
    }
    bool isPrintICC(uint16_t type, PrintStructureOption_e option)
    {
        return type == 0x8773 && option == kpsIccProfile;
    }

    bool isBigEndianPlatform()
    {
        union {
            uint32_t i;
            char c[4];
        } e = { 0x01000000 };

        return e.c[0]?true:false;
    }
    bool isLittleEndianPlatform() { return !isBigEndianPlatform(); }

    uint64_t byteSwap(uint64_t value,bool bSwap) const
    {
        uint64_t result = 0;
        byte* source_value = reinterpret_cast<byte *>(&value);
        byte* destination_value = reinterpret_cast<byte *>(&result);

        for (int i = 0; i < 8; i++)
            destination_value[i] = source_value[8 - i - 1];

        return bSwap ? result : value;
    }

    uint32_t byteSwap(uint32_t value,bool bSwap) const
    {
        uint32_t result = 0;
        result |= (value & 0x000000FF) << 24;
        result |= (value & 0x0000FF00) << 8;
        result |= (value & 0x00FF0000) >> 8;
        result |= (value & 0xFF000000) >> 24;
        return bSwap ? result : value;
    }

    uint16_t byteSwap(uint16_t value,bool bSwap) const
    {
        uint16_t result = 0;
        result |= (value & 0x00FF) << 8;
        result |= (value & 0xFF00) >> 8;
        return bSwap ? result : value;
    }

    uint16_t byteSwap2(const DataBuf& buf,size_t offset,bool bSwap) const
    {
        uint16_t v;
        char*    p = (char*) &v;
        p[0] = buf.pData_[offset];
        p[1] = buf.pData_[offset+1];
        return byteSwap(v,bSwap);
    }

    uint32_t byteSwap4(const DataBuf& buf,size_t offset,bool bSwap) const
    {
        uint32_t v;
        char*    p = (char*) &v;
        p[0] = buf.pData_[offset];
        p[1] = buf.pData_[offset+1];
        p[2] = buf.pData_[offset+2];
        p[3] = buf.pData_[offset+3];
        return byteSwap(v,bSwap);
    }

    uint64_t byteSwap8(const DataBuf& buf,size_t offset,bool bSwap) const
    {
        uint64_t v;
        byte*    p = reinterpret_cast<byte *>(&v);

        for(int i = 0; i < 8; i++)
            p[i] = buf.pData_[offset + i];

        return byteSwap(v,bSwap);
    }

    const char* typeName(uint16_t tag) const
    {
        //! List of TIFF image tags
        const char* result = NULL;
        switch (tag ) {
            case unsignedByte     : result = "BYTE"      ; break;
            case asciiString      : result = "ASCII"     ; break;
            case unsignedShort    : result = "SHORT"     ; break;
            case unsignedLong     : result = "LONG"      ; break;
            case unsignedRational : result = "RATIONAL"  ; break;
            case signedByte       : result = "SBYTE"     ; break;
            case undefined        : result = "UNDEFINED" ; break;
            case signedShort      : result = "SSHORT"    ; break;
            case signedLong       : result = "SLONG"     ; break;
            case signedRational   : result = "SRATIONAL" ; break;
            case tiffFloat        : result = "FLOAT"     ; break;
            case tiffDouble       : result = "DOUBLE"    ; break;
            case tiffIfd          : result = "IFD"       ; break;
            default            : result = "unknown"   ; break;
        }
        return result;
    }

    static bool typeValid(uint16_t type)
    {
        return type >= 1 && type <= 13 ;
    }

    void printIFD(std::ostream& out, PrintStructureOption_e option,size_t start,bool bSwap,char c,int depth)
    {
        depth++;
        if ( depth == 1 ) visits_.clear();
        bool bFirst  = true  ;

        // buffer
        const size_t dirSize = 32;
        DataBuf  dir(dirSize);
        bool bPrint = option == kpsBasic || option == kpsRecursive;

        do {
            // Read top of directory
            const int seekSuccess = !io_.seek(start,beg);
            const long bytesRead = io_.read(dir.pData_, 2);
            if (!seekSuccess || bytesRead == 0) {
                Error(kerCorruptedMetadata);
            }
            uint16_t   dirLength = byteSwap2(dir,0,bSwap);

            bool tooBig = dirLength > 500;
            if ( tooBig ) Error(kerTiffDirectoryTooLarge);

            if ( bFirst && bPrint ) {
                out << indent(depth) << stringFormat("STRUCTURE OF TIFF FILE (%c%c): ",c,c) << io_.path() << std::endl;
                if ( tooBig ) out << indent(depth) << "dirLength = " << dirLength << std::endl;
            }

            // Read the dictionary
            for ( int i = 0 ; i < dirLength ; i ++ ) {
                if ( visits_.find(io_.tell()) != visits_.end()  ) { // never visit the same place twice!
                    Error(kerCorruptedMetadata);
                }
                visits_.insert(io_.tell());
                
                if ( bFirst && bPrint ) {
                    out << indent(depth)
                        << " address |    tag                              |     "
                        << " type |    count |    offset | value\n";
                }
                bFirst = false;

                io_.read(dir.pData_, 12);
                uint16_t tag    = byteSwap2(dir,0,bSwap);
                uint16_t type   = byteSwap2(dir,2,bSwap);
                uint32_t count  = byteSwap4(dir,4,bSwap);
                uint32_t offset = byteSwap4(dir,8,bSwap);

                // Break for unknown tag types else we may segfault.
                if ( !typeValid(type) ) {
                    std::cerr << "invalid type in tiff structure" << type << std::endl;
                    start = 0; // break from do loop
                    Error(kerInvalidTypeValue);
                }

                std::string sp  = "" ; // output spacer

                //prepare to print the value
                uint32_t kount  = isPrintXMP(tag,option) ? count // haul in all the data
                                : isPrintICC(tag,option) ? count // ditto
                                : isStringType(type)     ? (count > 32 ? 32 : count) // restrict long arrays
                                : count > 5              ? 5
                                : count
                                ;
                uint32_t pad    = isStringType(type) ? 1 : 0;
                uint32_t size   = isStringType(type) ? 1
                                : is2ByteType(type)  ? 2
                                : is4ByteType(type)  ? 4
                                : is8ByteType(type)  ? 8
                                : 1
                                ;

                // if ( offset > io_.size() ) offset = 0; // Denial of service?

                // #55 and #56 memory allocation crash test/data/POC8
                long long allocate = (long long) size*count + pad+20;
                if ( allocate > (long long) io_.size() ) {
                    Error(kerInvalidMalloc);
                }
                DataBuf  buf((long)allocate);  // allocate a buffer
                std::memset(buf.pData_, 0, buf.size_);
                std::memcpy(buf.pData_,dir.pData_+8,4);  // copy dir[8:11] into buffer (short strings)
                const bool bOffsetIsPointer = count*size > 4;

                if ( bOffsetIsPointer ) {         // read into buffer
                    size_t   restore = io_.tell();  // save
                    io_.seek(offset,beg);  // position
                    io_.read(buf.pData_,count*size);// read
                    io_.seek(restore,beg); // restore
                }

                if ( bPrint ) {
                    const uint32_t address = start + 2 + i*12 ;
                    const std::string offsetString = bOffsetIsPointer?
                        stringFormat("%10u", offset):
                        "";

                    out << indent(depth)
                    << stringFormat("%8u | %#06x %-28s |%10s |%9u |%10s | "
                                              ,address,tag,tagName(tag).c_str(),typeName(type),count,offsetString.c_str());
                    if ( isShortType(type) ){
                        for ( size_t k = 0 ; k < kount ; k++ ) {
                            out << sp << byteSwap2(buf,k*size,bSwap);
                            sp = " ";
                        }
                    } else if ( isLongType(type) ){
                        for ( size_t k = 0 ; k < kount ; k++ ) {
                            out << sp << byteSwap4(buf,k*size,bSwap);
                            sp = " ";
                        }

                    } else if ( isRationalType(type) ){
                        for ( size_t k = 0 ; k < kount ; k++ ) {
                            uint32_t a = byteSwap4(buf,k*size+0,bSwap);
                            uint32_t b = byteSwap4(buf,k*size+4,bSwap);
                            out << sp << a << "/" << b;
                            sp = " ";
                        }
                    } else if ( isStringType(type) ) {
                        out << sp << binaryToString(buf, 0, kount);
                    }

                    sp = kount == count ? "" : " ...";
                    out << sp << std::endl;

                    if ( option == kpsRecursive && (tag == 0x8769 /* ExifTag */ || tag == 0x014a/*SubIFDs*/  || type == tiffIfd) ) {
                        for ( size_t k = 0 ; k < count ; k++ ) {
                            size_t   restore = io_.tell();
                            uint32_t offset = byteSwap4(buf,k*size,bSwap);
                            printIFD(out,option,offset,bSwap,c,depth);
                            io_.seek(restore,beg);
                        }
                    } else if ( option == kpsRecursive && tag == 0x83bb /* IPTCNAA */ ) {

                        if (count + offset > io_.size() ) { // static_cast<size_t>(Safe::add(count, offset)) > io_.size()) {
                            Error(kerCorruptedMetadata);
                        }
#if 0
                        const size_t restore = io_.tell();
                        io_.seek(offset, beg);  // position
                        std::vector<byte> bytes(count) ;  // allocate memory
                        // TODO: once we have C++11 use bytes.data()
                        const long read_bytes = io_.read(&bytes[0], count);
                        io_.seek(restore, beg);
                        // TODO: once we have C++11 use bytes.data()
                        IptcData::printStructure(out, makeSliceUntil(&bytes[0], read_bytes), depth);
#endif
                    }  else if ( option == kpsRecursive && tag == 0x927c /* MakerNote */ && count > 10) {
#if 0      
                        size_t   restore = io_.tell();  // save

                        uint32_t jump= 10           ;
                        byte     bytes[20]          ;
                        const char* chars = (const char*) &bytes[0] ;
                        io_.seek(offset,beg);  // position
                        io_.read(bytes,jump    )     ;  // read
                        bytes[jump]=0               ;
                        if ( ::strcmp("Nikon",chars) == 0 ) {
                            // tag is an embedded tiff
                            byte* bytes=new byte[count-jump] ;  // allocate memory
                            io_.read(bytes,count-jump)        ;  // read
                            MemIo memIo(bytes,count-jump)    ;  // create a file
                            printTiffStructure(memIo,out,option,depth);
                            delete[] bytes                   ;  // free
                        } else {
                            // tag is an IFD
                            io_.seek(0,beg);  // position
                            printIFD(out,option,offset,bSwap,c,depth);
                        }

                        io_.seek(restore,beg); // restore
#endif
                    }
                }
#if 0
                if ( isPrintXMP(tag,option) ) {
                    buf.pData_[count]=0;
                    out << (char*) buf.pData_;
                }
                if ( isPrintICC(tag,option) ) {
                    out.write((const char*)buf.pData_,count);
                }
#endif
            }
            if ( start ) {
                io_.read(dir.pData_, 4);
                start = tooBig ? 0 : byteSwap4(dir,0,bSwap);
            }
        } while (start) ;

        if ( bPrint ) {
            out << indent(depth) << "END " << io_.path() << std::endl;
        }
        out.flush();
        depth--;
    }
};

void TiffImage::printStructure(std::ostream& out, PrintStructureOption_e option,int depth  ,size_t offset  )
{
	int rc = 3;
	if ( option == kpsBasic || option == kpsXMP || option == kpsRecursive || option == kpsIccProfile ) {
		// buffer
		const size_t dirSize = 32;
		DataBuf  dir(dirSize);

		// read header
		io_.seek(offset,beg);
		io_.read(dir.pData_,  8);

		char c = (char) dir.pData_[0] ;
		bool bSwap   = ( c == 'M' && isLittleEndianPlatform() )
					|| ( c == 'I' && isBigEndianPlatform()    )
					;
		uint32_t start = byteSwap4(dir,4,bSwap);
		
    	uint16_t magic = byteSwap2(dir,2,bSwap);
    	if ( magic != 42 || dir.pData_[0] != dir.pData_[1]
    	|| ( dir.pData_[0] != 'I' && dir.pData_[0] != 'M' )
    	){
    		Error(kerNotATiff);
    	}
		
		printIFD(out,option,start+offset,bSwap,c,depth);
		rc = 0;
	} 
}

int main(int argc,const char* argv[])
{
	int rc = 0;
	if ( argc == 2 ) {
		TiffImage tiff(argv[1]) ;
		// Visitor   visitor       ;
	
		if ( tiff.good() ) {
			tiff.printStructure(std::cout,kpsRecursive);
		} else {
			std::cerr << "not good!" << std::endl;
			rc=2;
		}
	} else {
		std::cout << "usage: " << argv[0] << " path" << std::endl;
		rc = 1;
	}
	return rc;
}

// That's all Folks!
////
