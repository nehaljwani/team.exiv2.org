#include <iostream>
#include <set>
#include <vector>
#include <string>
#include <sstream>
#include <map>

// crummy old c magic for fopen and gang
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

typedef unsigned char       byte    ;
enum endian_e
{   kEndianLittle
,   kEndianBig
};

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
//  time          =0x10002, //!< IPTC time type.
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

enum maker_e
{	kUnknown
,	kCanon
,	kNikon
,   kSony
};

enum error_e
{   kerCorruptedMetadata
,   kerTiffDirectoryTooLarge
,   kerInvalidTypeValue
,   kerInvalidMalloc
,   kerNotATiff
,   kerFailedToReadImageData
,   kerNotAJpeg
,   kerDataSourceOpenFailed
,   kerNoImageInInputData
};

enum PSopt_e
{   kpsBasic
,   kpsXMP
,   kpsRecursive
,   kpsIccProfile
};

enum seek_e
{   ksStart   = SEEK_SET
,   ksCurrent = SEEK_CUR
,   ksEnd     = SEEK_END
};

void Error (error_e error, std::string msg)
{
    switch ( error ) {
        case   kerCorruptedMetadata      : std::cerr << "corrupted metadata"       ; break;
        case   kerTiffDirectoryTooLarge  : std::cerr << "tiff directory too large" ; break;
        case   kerInvalidTypeValue       : std::cerr << "invalid type"             ; break;
        case   kerInvalidMalloc          : std::cerr << "invalid malloc"           ; break;
        case   kerNotATiff               : std::cerr << "Not a tiff"               ; break;
        case   kerFailedToReadImageData  : std::cerr << "failed to read image data"; break;
        case   kerNotAJpeg               : std::cerr << "not a jpeg"               ; break;
        case   kerDataSourceOpenFailed   : std::cerr << "data source open failed"  ; break;
        case   kerNoImageInInputData     : std::cerr << "not image in input data"  ; break;
        default                          : std::cerr << "unknown"                  ; break;
    }
    if ( msg.size() ) std::cerr << " " << msg ;  
    std::cerr << std::endl;
    _exit(1); // pull the plug!
}

void Error (error_e error)
{
    Error(error,"");
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
    int  strcmp   (const char* str) { return ::strcmp((const char*)pData_,str);}
    bool strequals(const char* str) { return strcmp(str)==0                   ;}
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

std::string binaryToString(byte* b,size_t start,size_t size)
{
    std::string result;
    size_t i    = start;
    while (i < start+size ) {
        result += (32 <= b[i] && b[i] <= 127) ? b[i]
                : ( 0 == b[i]               ) ? '_'
                : '.'
                ;
        i++ ;
    }
    return result;
}

std::string binaryToString(DataBuf& dataBuf,size_t start,size_t size)
{
    return binaryToString(dataBuf.pData_,start,size);
}

// TagDict is use to map tag (uint16_t) to string (for printing)
typedef std::map<uint16_t,std::string> TagDict;

TagDict tiffDict  ;
TagDict exifDict  ;
TagDict canonDict ;
TagDict nikonDict ;
TagDict sonyDict  ;
TagDict gpsDict   ;

TagDict copyDict(TagDict old)
{
    TagDict result;
    for ( TagDict::iterator it = old.begin() ; it != old.end() ; it++ )
        result[it->first] = it->second;
    return result;
}

bool tagKnown(uint16_t tag,const TagDict& tagDict)
{
    return tagDict.find(tag) != tagDict.end();
}

std::string tagName(uint16_t tag,const TagDict& tagDict)
{
    std::string name ;
    if ( tag != 0xffff ) {
        if ( tagDict.find(tag) != tagDict.end() ) {
            name = tagDict.find(tag)->second;
        } else {
            name = stringFormat("%#x",tag);
        }
    }
    std::string family = "Unknown";
    tag = 0xffff;
    if ( tagDict.find(tag) != tagDict.end() ) {
        family =  tagDict.find(tag)->second;
    }
    
    std::string result =  "Exif." + family ;
    if ( name.size() ) result += "." + name;
    return result;
}

class Io
{
public:
    Io(std::string path,std::string open) // Io object from path
    : path_   (path)
    , start_  (0)
    , size_   (0)
    , restore_(0)
    , f_      (NULL)
    { f_ = ::fopen(path.c_str(),open.c_str()); }
    
    Io(Io& io,size_t start,size_t size) // Io object is a substream
    : path_   (io.path())
    , start_  (start+io.start_)
    , size_   (size)
    , restore_(ftell(f_))
    , f_      (io.f_)
    {
        std::ostringstream os;
        os << path_ << ":" << start << "->" << size_;
        path_=os.str();
        seek(0);
    };

    virtual ~Io() { close(); }
    
    std::string path() { return path_; }
    
    size_t read(void* buff,size_t size)              { return fread(buff,1,size,f_);}
    size_t read(DataBuf& buff)                       { return read(buff.pData_,buff.size_); }
    byte   getb()                                    { byte b; if (read(&b,1)==1) return b ; else return -1; }
    int    eof()                                     { return feof(f_) ; }
    size_t tell()                                    { return ftell(f_)-start_ ; }
    void   seek(size_t offset,seek_e whence=ksStart) { fseek(f_,offset+start_,whence) ; }
    size_t size()                                    { if ( size_ ) return size_ ; struct stat st ; fstat(fileno(f_),&st) ; return st.st_size-start_ ; }
    bool   good()                                    { return f_ ? true : false ; }
    void   close()
    {
        if ( !f_ ) return ;
        if ( start_ == 0 && size_ == 0 && restore_ == 0 ) {
            fclose(f_) ;
        } else {
            fseek(f_,restore_,ksStart);
        }
        f_ = NULL  ;
    }

private:
    FILE*       f_;
    std::string path_;
    size_t      start_;
    size_t      size_;
    size_t      restore_;
};

class Visitor
{
public:
    Visitor();
    virtual ~Visitor() {};
    
    void directoryBegin();
    void directoryEnd();
};

class Image
{
public:
    Image(std::string path)
    : io_(path,"rb")
    , good_(false)
    , start_(0)
    , maker_(kUnknown)
    {
        good_ = io_.good();
    }
    Image(Io io)
    : io_(io)
    {
        good_ = io_.good();
    }
    virtual ~Image() { io_.close() ; }
    bool good()  { return good_ && io_.good() ; }
    bool valid() { return false ; }
    
    virtual void printStructure(std::ostream& out, PSopt_e option,const TagDict& tagDict,int depth = 0) =0;
    virtual void printIFD      (std::ostream& out, PSopt_e option,size_t start,endian_e endian,int depth,const TagDict& tagDict,bool bHasNext=true)=0;
    friend class TiffImage;
    friend class JpegImage;

private:
    std::set<size_t>    visits_;
    size_t              start_;
    Io                  io_;
    bool                good_;
    uint16_t            magic_;
    maker_e             maker_;
    TagDict             makerDict_;
    endian_e            endian_;
    
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
    bool isPrintXMP(uint16_t type, PSopt_e option)
    {
        return type == 700 && option == kpsXMP;
    }
    bool isPrintICC(uint16_t type, PSopt_e option)
    {
        return type == 0x8773 && option == kpsIccProfile;
    }

    bool isPlatformBigEndian()
    {
        union {
            uint32_t i;
            char c[4];
        } e = { 0x01000000 };

        return e.c[0]?true:false;
    }
    bool   isPlatformLittleEndian() { return !isPlatformBigEndian(); }
    endian_e platformEndian() { return isPlatformBigEndian() ? kEndianBig : kEndianLittle; }

    uint64_t byteSwap(uint64_t value,bool bSwap,uint16_t n) const
    {
        uint64_t result = 0;
        byte* source_value      = reinterpret_cast<byte *>(&value);
        byte* destination_value = reinterpret_cast<byte *>(&result);

        for (int i = 0; i < n; i++)
            destination_value[i] = source_value[n - i - 1];

        return bSwap ? result : value;
    }

    uint16_t getShort(const DataBuf& buf,size_t offset,bool bSwap) const
    {
        uint16_t v;
        char*    p = (char*) &v;
        p[0] = buf.pData_[offset+0];
        p[1] = buf.pData_[offset+1];
        return (uint16_t) byteSwap(v,bSwap,2);
    }

    uint32_t getLong(const DataBuf& buf,size_t offset,bool bSwap) const
    {
        uint32_t v;
        char*    p = (char*) &v;
        p[0] = buf.pData_[offset+0];
        p[1] = buf.pData_[offset+1];
        p[2] = buf.pData_[offset+2];
        p[3] = buf.pData_[offset+3];
        return (uint32_t)byteSwap(v,bSwap,4);
    }

    uint64_t getLongLong(const DataBuf& buf,size_t offset,bool bSwap) const
    {
        uint64_t v;
        byte*    p = reinterpret_cast<byte *>(&v);
        p[0] = buf.pData_[offset+0];
        p[1] = buf.pData_[offset+1];
        p[2] = buf.pData_[offset+2];
        p[3] = buf.pData_[offset+3];
        p[4] = buf.pData_[offset+4];
        p[5] = buf.pData_[offset+5];
        p[6] = buf.pData_[offset+6];
        p[7] = buf.pData_[offset+7];
        return byteSwap (v,bSwap,8);
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

};

class TiffImage : public Image
{
public:
    TiffImage(std::string path) : Image(path) {}
    TiffImage(Io& io) : Image(io) {} ;
    void printStructure(std::ostream& out, PSopt_e option,const TagDict& tagDict,int depth = 0);
    void printIFD      (std::ostream& out, PSopt_e option,size_t start,endian_e endian,int depth,const TagDict& tagDict,bool bHasNext=true);
    bool valid();
};

class JpegImage : public Image
{
public:
    JpegImage(std::string path) : Image(path) {}
    void printStructure(std::ostream& out, PSopt_e option,const TagDict& tagDict,int depth = 0);
    void printIFD      (std::ostream& out, PSopt_e option,size_t start,endian_e endian,int depth,const TagDict& tagDict,bool bHasNext=true)
    {
        TiffImage* p((TiffImage*)this);
        p->printIFD(out,option,start,endian,depth,tagDict);
    };

    bool valid();
private:
    const byte     dht_      = 0xc4;
    const byte     dqt_      = 0xdb;
    const byte     dri_      = 0xdd;
    const byte     sos_      = 0xda;
    const byte     eoi_      = 0xd9;
    const byte     app0_     = 0xe0;
    const byte     app1_     = 0xe1;
    const byte     app2_     = 0xe2;
    const byte     app13_    = 0xed;
    const byte     com_      = 0xfe;

    // Start of Frame markers, nondifferential Huffman-coding frames
    const byte     sof0_     = 0xc0;        // start of frame 0, baseline DCT
    const byte     sof1_     = 0xc1;        // start of frame 1, extended sequential DCT, Huffman coding
    const byte     sof2_     = 0xc2;        // start of frame 2, progressive DCT, Huffman coding
    const byte     sof3_     = 0xc3;        // start of frame 3, lossless sequential, Huffman coding

    // Start of Frame markers, differential Huffman-coding frames
    const byte     sof5_     = 0xc5;        // start of frame 5, differential sequential DCT, Huffman coding
    const byte     sof6_     = 0xc6;        // start of frame 6, differential progressive DCT, Huffman coding
    const byte     sof7_     = 0xc7;        // start of frame 7, differential lossless, Huffman coding

    // Start of Frame markers, nondifferential arithmetic-coding frames
    const byte     sof9_     = 0xc9;        // start of frame 9, extended sequential DCT, arithmetic coding
    const byte     sof10_    = 0xca;        // start of frame 10, progressive DCT, arithmetic coding
    const byte     sof11_    = 0xcb;        // start of frame 11, lossless sequential, arithmetic coding

    // Start of Frame markers, differential arithmetic-coding frames
    const byte     sof13_    = 0xcd;        // start of frame 13, differential sequential DCT, arithmetic coding
    const byte     sof14_    = 0xce;        // start of frame 14, progressive DCT, arithmetic coding
    const byte     sof15_    = 0xcf;        // start of frame 15, differential lossless, arithmetic coding

    int advanceToMarker()
    {
        int c = -1;
        // Skips potential padding between markers
        while ((c=io_.getb()) != 0xff) {
            if (io_.eof() )
                return -1;
        }

        // Markers can start with any number of 0xff
        while ((c=io_.getb()) == 0xff) {
            if (io_.eof() )
                return -2;
        }
        return c;
    }
};

bool TiffImage::valid()
{
    bool         result  = false ;
    size_t       restore = io_.tell();
    io_.seek(0);
    // read header
    DataBuf      header(16);
    io_.seek(0);
    io_.read(header);
    io_.seek(restore);

    char c      = (char) header.pData_[0] ;
    char C      = (char) header.pData_[1] ;
    endian_ = c == 'M' ? kEndianBig : kEndianLittle;
    
    start_  = getLong (header,4,endian_);
    magic_  = getShort(header,2,endian_);
    result  = magic_ == 42 && c == C && ( c == 'I' || c == 'M' ) && start_ < io_.size() ;

    return result;
}

void TiffImage::printIFD(std::ostream& out, PSopt_e option,size_t start,endian_e endian,int depth,const TagDict& tagDict,bool bHasNext)
{
    bool     bFirst  = true  ;
    size_t   restore_at_start = io_.tell();

    depth++;
    if ( depth == 1 ) visits_.clear();
    // buffer
    const size_t dirSize = 32;
    DataBuf  dir(dirSize);
    bool bPrint = option == kpsBasic || option == kpsRecursive;

    do {
        // Read top of directory
        io_.seek(start);
        const long bytesRead  = io_.read(dir.pData_, 2);
        if ( bytesRead != 2) {
            Error(kerCorruptedMetadata);
        }

        uint16_t dirLength = getShort(dir,0,endian_);
        bool tooBig = dirLength > 500;
        if ( tooBig ) Error(kerTiffDirectoryTooLarge);

        if ( bFirst && bPrint ) {
            char c = endian_ == kEndianBig ? 'M' : 'I' ;
            out << indent(depth) << stringFormat("STRUCTURE OF TIFF FILE (%c%c): ",c,c) << io_.path()
                << " dirLength = " << dirLength << " tagDict = " << tagName(0xffff,tagDict)
                << std::endl;
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
            uint16_t tag    = getShort(dir,0,endian_);
            uint16_t type   = getShort(dir,2,endian_);
            uint32_t count  = getLong (dir,4,endian_);
            uint32_t offset = getLong (dir,8,endian_);

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

            size_t allocate = size*count + pad+20;
            if ( allocate > io_.size() ) {
                Error(kerInvalidMalloc);
            }
            DataBuf  buf(allocate);              // allocate a buffer
            std::memset(buf.pData_, 0, buf.size_);
            std::memcpy(buf.pData_,dir.pData_+8,4);  // copy dir[8:11] into buffer (short strings)
            const bool bOffsetIsPointer = count*size > 4;

            if ( bOffsetIsPointer ) {            // read into buffer
                size_t   restore = io_.tell();   // save
                io_.seek(offset);                // position
                io_.read(buf.pData_,count*size); // read
                io_.seek(restore);               // restore
            }
            
            if ( depth == 1 && tag == 0x010f /* Make */ ) {
                maker_ = buf.strequals("Canon"            )? kCanon
            	       : buf.strequals("NIKON CORPORATION")? kNikon
            	       : buf.strequals("SONY")             ? kSony
            	       : maker_
            	       ;
                switch ( maker_ ) {
                    case kCanon : makerDict_ = copyDict(canonDict) ; break;
                    case kNikon : makerDict_ = copyDict(nikonDict) ; break;
                    case kSony  : makerDict_ = copyDict(sonyDict ) ; break;
                    default : /* do nothing */                     ; break;
                }
            }

            if ( bPrint ) {
                const size_t address = start + 2 + i*12 ;
                const std::string offsetString = bOffsetIsPointer?
                    stringFormat("%10u", offset):
                    "";
                out << indent(depth)
                << stringFormat("%8u | %#06x %-28s |%10s |%9u |%10s | "
                                          ,address,tag,tagName(tag,tagDict).c_str(),typeName(type),count,offsetString.c_str());
                if ( isShortType(type) ){
                    for ( size_t k = 0 ; k < kount ; k++ ) {
                        out << sp << getShort(buf,k*size,endian_);
                        sp = " ";
                    }
                } else if ( isLongType(type) ){
                    for ( size_t k = 0 ; k < kount ; k++ ) {
                        out << sp << getLong(buf,k*size,endian_);
                        sp = " ";
                    }
                } else if ( isRationalType(type) ){
                    for ( size_t k = 0 ; k < kount ; k++ ) {
                        uint32_t a = getLong(buf,k*size+0,endian_);
                        uint32_t b = getLong(buf,k*size+4,endian_);
                        out << sp << a << "/" << b;
                        sp = " ";
                    }
                } else if ( isStringType(type) ) {
                    out << sp << binaryToString(buf, 0, kount);
                }

                sp = kount == count ? "" : " ...";
                out << sp << std::endl;
                
                // these tags are IFDs, not a embedded TIFF
                if ( option == kpsRecursive && (tag == 0x8769 /* ExifTag */ || tag == 0x014a /*SubIFDs*/  || tag == 0x8825 /* GPSTag */ || type == tiffIfd) ) {
                    TagDict  useDict = tag == 0x8769 ? copyDict(exifDict) // joinDict( tagDict,exifDict  )
                                     : tag == 0x8825 ? copyDict(gpsDict)  // joinDict( tagDict,gpsDict   )
                                     : tag == 0x927c ? copyDict(makerDict_) // joinDict( tagDict,makerDict_)
                                     :                 copyDict(tagDict)
                                     ;
                    for ( size_t k = 0 ; k < count ; k++ ) {
                        uint32_t offset  = getLong(buf,k*size,endian_);
                        printIFD(out,option,offset,endian_,depth,useDict);
                    }
                } else if ( option == kpsRecursive && tag == 0x83bb /* IPTCNAA */ ) {
                    // This is an IPTC tag
                } else if ( option == kpsRecursive && tag == 0x927c /* MakerNote */ && count > 10) {
                    // Nikon MakerNote is not and IFD, it's an emabedd tiff `II*_.....`
                    if ( maker_ == kSony && buf.strequals("SONY DSC ") ) {
                        // Sony MakerNote IFD does not have a next pointer.
                        size_t punt   = 12 ;
                        printIFD(out,option,offset+punt,endian_,depth,sonyDict,false);
                    } else {
                        size_t punt = buf.strequals("Nikon") ? 10 : 0 ;
                        Io io(io_,offset+punt,count-punt);
                        TiffImage makerNote(io);
                        makerNote.printStructure(out,option,makerDict_,depth);
                    }
                }
            }
            
            if ( isPrintXMP(tag,option) ) {
                buf.pData_[count]=0;
                out << (char*) buf.pData_;
            }
            if ( isPrintICC(tag,option) ) {
                out.write((const char*)buf.pData_,count);
            }
        }
        if ( start ) {
            if ( bHasNext ) {
                io_.read(dir.pData_, 4);
                start = tooBig ? 0 : getLong(dir,0,endian_);
            } else {
                start=0;
            }
        }
    } while (start) ;

    if ( bPrint ) {
        out << indent(depth) << "END " << io_.path() << std::endl;
    }
    out.flush();
    depth--;
    
    io_.seek(restore_at_start); // restore
} // print IFD

void TiffImage::printStructure(std::ostream& out, PSopt_e option,const TagDict& tagDict,int depth)
{
    if ( option == kpsBasic || option == kpsXMP || option == kpsRecursive || option == kpsIccProfile ) {
        if ( valid() ) {
            printIFD(out,option,start_,endian_,depth,tagDict);
        }
    }
}

bool JpegImage::valid()
{
    io_.seek(0,ksStart);
    byte   h[2];
    size_t n = io_.read(h,2);
    io_.seek(0,ksStart);
    bool result = n == 2 && h[0] == 0xff && h[1] == 0xd8;
//  std::cout << stringFormat("%ld %#x %#x result = %s\n",n,h[0],h[1],result?"true":"false");
    return result;
}

#define REPORT_MARKER if ( (option == kpsBasic||option == kpsRecursive) ) \
     out << stringFormat("%8ld | 0xff%02x %-5s", \
     io_.tell()-2,marker,nm[marker].c_str())

#define FLUSH(bLF) if (bLF) { out << std::endl; bLF = false;}

void JpegImage::printStructure(std::ostream& out, PSopt_e option,const TagDict& tagDict,int depth)
{
    if (!io_.good())
        Error(kerDataSourceOpenFailed, io_.path());
    // Ensure that this is the correct image type
    if (!valid()) {
        if (!io_.good() || io_.eof())
            Error(kerFailedToReadImageData);
        Error(kerNotAJpeg);
    }

    bool bPrint = option == kpsBasic || option == kpsRecursive;

    if (bPrint || option == kpsXMP) {
        // nmonic for markers
        std::string nm[256];
        nm[0xd8] = "SOI";
        nm[0xd9] = "EOI";
        nm[0xda] = "SOS";
        nm[0xdb] = "DQT";
        nm[0xdd] = "DRI";
        nm[0xfe] = "COM";

        // 0xe0 .. 0xef are APPn
        // 0xc0 .. 0xcf are SOFn (except 4)
        nm[0xc4] = "DHT";
        for (int i = 0; i <= 15; i++) {
            char MN[16];
            sprintf(MN, "APP%d", i);
            nm[0xe0 + i] = MN;
            if (i != 4) {
                sprintf(MN, "SOF%d", i);
                nm[0xc0 + i] = MN;
            }
        }

        // which markers have a length field?
        bool bHasLength[256];
        for (int i = 0; i < 256; i++)
            bHasLength[i] = (i >= sof0_ && i <= sof15_) || (i >= app0_ && i <= (app0_ | 0x0F)) ||
                            (i == dht_  || i == dqt_    || i == dri_   || i == com_  ||i == sos_);

        // Container for the signature
        bool bExtXMP = false;
        long bufRead = 0;
        const long bufMinSize = 36;
        DataBuf buf(bufMinSize);

        // Read section marker
        int marker = advanceToMarker();
        if (marker < 0)
            Error(kerNotAJpeg);

        bool done = false;
        bool first = true;
        while (!done) {
            size_t current = io_.tell();
            // print marker bytes
            if (first && bPrint) {
                out << "STRUCTURE OF JPEG FILE: " << io_.path() << std::endl;
                out << " address | marker       |  length | data" << std::endl;
                REPORT_MARKER;
            }
            first = false;
            bool bLF = bPrint;

            // Read size and signature
            std::memset(buf.pData_, 0x0, buf.size_);
            bufRead = io_.read(buf.pData_, bufMinSize);
            if (!io_.good())
                Error(kerFailedToReadImageData);
            if (bufRead < 2)
                Error(kerNotAJpeg);
            const uint16_t size = bHasLength[marker] ? getShort(buf,0,kEndianBig):0;
            
            if (bPrint && bHasLength[marker])
                out << stringFormat(" | %7d ", size);

            // print signature for APPn
            if (marker >= app0_ && marker <= (app0_ | 0x0F)) {
                // http://www.adobe.com/content/dam/Adobe/en/devnet/xmp/pdfs/XMPSpecificationPart3.pdf p75
                const std::string signature = binaryToString(buf,2, buf.size_ - 2);

                // 728 rmills@rmillsmbp:~/gnu/exiv2/ttt $ exiv2 -pS test/data/exiv2-bug922.jpg
                // STRUCTURE OF JPEG FILE: test/data/exiv2-bug922.jpg
                // address | marker     | length  | data
                //       0 | 0xd8 SOI   |       0
                //       2 | 0xe1 APP1  |     911 | Exif..MM.*.......%.........#....
                //     915 | 0xe1 APP1  |     870 | http://ns.adobe.com/xap/1.0/.<x:
                //    1787 | 0xe1 APP1  |   65460 | http://ns.adobe.com/xmp/extensio
                if (option == kpsXMP && signature.find("http://ns.adobe.com/x") == 0) {
                    // extract XMP
                    if (size > 0) {
                        io_.seek(-bufRead, ksCurrent);
                        std::vector<byte> xmp(size + 1);
                        io_.read(&xmp[0], size);
                        int start = 0;

                        // http://wwwimages.adobe.com/content/dam/Adobe/en/devnet/xmp/pdfs/XMPSpecificationPart3.pdf
                        // if we find HasExtendedXMP, set the flag and ignore this block
                        // the first extended block is a copy of the Standard block.
                        // a robust implementation allows extended blocks to be out of sequence
                        // we could implement out of sequence with a dictionary of sequence/offset
                        // and dumping the XMP in a post read operation similar to kpsIptcErase
                        // for the moment, dumping 'on the fly' is working fine
                        if (!bExtXMP) {
                            while (xmp.at(start)) {
                                start++;
                            }
                            start++;
                            const std::string xmp_from_start = binaryToString(
                                reinterpret_cast<byte*>(&xmp.at(0)),start, size - start);
                            if (xmp_from_start.find("HasExtendedXMP", start) != xmp_from_start.npos) {
                                start = size;  // ignore this packet, we'll get on the next time around
                                bExtXMP = true;
                            }
                        } else {
                            start = 2 + 35 + 32 + 4 + 4;  // Adobe Spec, p19
                        }

                        out.write(reinterpret_cast<const char*>(&xmp.at(start)), size - start);
                        bufRead = size;
                        done = !bExtXMP;
                    }
                } else if (bPrint) {
                    const size_t start = size > 0 ? 2 : 0;
                    const size_t end = start + (size > 32 ? 32 : size);
                    out << "| " << binaryToString(buf, start, end);
                }
                FLUSH(bLF)

                // std::cout << "app0_+1 = " << app0_+1 << " compare " << signature << " = " << signature.find("Exif") == 2 << std::endl;
				bool bExif = option == kpsRecursive && marker == (app0_ + 1) && signature.find("Exif") == 0;

				if ( bExif ) {
                    Io io(io_,current+2+6,size-2-6);
                    TiffImage exif(io);
                    exif.printStructure(out,option,tiffDict,depth);
				}
            }

            // print COM marker
            if (bPrint && marker == com_) {
                // size includes 2 for the two bytes for size!
                const int n = (size - 2) > 32 ? 32 : size - 2;
                // start after the two bytes
                out << "| " << binaryToString(buf, 2, n + 2);
            }

            // Skip the segment if the size is known
            io_.seek(size - bufRead, ksCurrent);
            FLUSH(bLF)

            if (marker != sos_) {
                // Read the beginning of the next segment
                marker = advanceToMarker();
                REPORT_MARKER;
            }
            done |= marker == eoi_ || marker == sos_;
            if (done && bPrint)
                out << std::endl;
        }
    }
}  // JpegImage::printStructure

void init()
{
    tiffDict  [ 0xffff ] = "Image";
    tiffDict  [ 0x8769 ] = "ExifTag";
    tiffDict  [ 0x014a ] = "SubIFD";
    tiffDict  [ 0x927c ] = "MakerNote";
    tiffDict  [ 0x83bb ] = "IPTCNAA";
    tiffDict  [ 0x02bc ] = "XMLPacket";
    tiffDict  [ 0x8773 ] = "InterColorProfile";
    tiffDict  [ 0x010e ] = "ImageDescription";
    tiffDict  [ 0x010f ] = "Make";
    tiffDict  [ 0x0110 ] = "Model";
    tiffDict  [ 0x0112 ] = "Orientation";
    tiffDict  [ 0x011a ] = "XResolution";
    tiffDict  [ 0x011b ] = "YResolution";
    tiffDict  [ 0x0128 ] = "ResolutionUnit";
    tiffDict  [ 0x0131 ] = "Software";
    tiffDict  [ 0x0132 ] = "DateTime";
    tiffDict  [ 0x0213 ] = "YCbCrPositioning";

    exifDict  [ 0xffff ] = "Photo";
    exifDict  [ 0x829a ] = "ExposureTime";
    exifDict  [ 0x829d ] = "FNumber";
    exifDict  [ 0x8822 ] = "ExposureProgram";
    exifDict  [ 0x8827 ] = "ISOSpeedRatings";
    exifDict  [ 0x8830 ] = "SensitivityType";
    exifDict  [ 0x9000 ] = "ExifVersion";
    exifDict  [ 0x9003 ] = "DateTimeOriginal";
    exifDict  [ 0x9004 ] = "DateTimeDigitized";
    exifDict  [ 0x9101 ] = "ComponentsConfiguration";
    exifDict  [ 0x9102 ] = "CompressedBitsPerPixel";
    exifDict  [ 0x9204 ] = "ExposureBiasValue";
    exifDict  [ 0x9205 ] = "MaxApertureValue";
    exifDict  [ 0x9207 ] = "MeteringMode";
    exifDict  [ 0x9208 ] = "LightSource";
    exifDict  [ 0x9209 ] = "Flash";
    exifDict  [ 0x920a ] = "FocalLength";
    exifDict  [ 0xa000 ] = "FlashpixVersion";
    exifDict  [ 0xa001 ] = "ColorSpace";
    exifDict  [ 0xa002 ] = "PixelXDimension";
    exifDict  [ 0xa003 ] = "PixelYDimension";
    exifDict  [ 0xa005 ] = "InteropTag";
    exifDict  [ 0x0001 ] = "InteropIndex";
    exifDict  [ 0x0002 ] = "InteropVersion";
    exifDict  [ 0xa300 ] = "FileSource";
    exifDict  [ 0xa301 ] = "SceneType";
    exifDict  [ 0xa401 ] = "CustomRendered";
    exifDict  [ 0xa402 ] = "ExposureMode";
    exifDict  [ 0xa403 ] = "WhiteBalance";
    exifDict  [ 0xa406 ] = "SceneCaptureType";
    exifDict  [ 0xa408 ] = "Contrast";
    exifDict  [ 0xa409 ] = "Saturation";
    exifDict  [ 0xa40a ] = "Sharpness";
    exifDict  [ 0xc4a5 ] = "PrintImageMatching";
    
    nikonDict [ 0xffff ] = "Nikon";
    nikonDict [ 0x0001 ] = "Version";
    nikonDict [ 0x0002 ] = "ISOSpeed";
    nikonDict [ 0x0004 ] = "Quality";
    nikonDict [ 0x0005 ] = "WhiteBalance";
    nikonDict [ 0x0007 ] = "Focus";
    nikonDict [ 0x0008 ] = "FlashSetting";
    nikonDict [ 0x0009 ] = "FlashDevice";
    nikonDict [ 0x000b ] = "WhiteBalanceBias";
    nikonDict [ 0x000c ] = "WB_RBLevels";
    nikonDict [ 0x000d ] = "ProgramShift";
    nikonDict [ 0x000e ] = "ExposureDiff";
    nikonDict [ 0x0012 ] = "FlashComp";
    nikonDict [ 0x0013 ] = "ISOSettings";
    nikonDict [ 0x0016 ] = "ImageBoundary";
    nikonDict [ 0x0017 ] = "FlashExposureComp";
    nikonDict [ 0x0018 ] = "FlashBracketComp";
    nikonDict [ 0x0019 ] = "ExposureBracketComp";
    nikonDict [ 0x001b ] = "CropHiSpeed";
    nikonDict [ 0x001c ] = "ExposureTuning";
    nikonDict [ 0x001d ] = "SerialNumber";
    nikonDict [ 0x001e ] = "ColorSpace";
    
    canonDict [ 0xffff ] = "Canon";
    canonDict [ 0x0001 ] = "Macro";
    canonDict [ 0x0002 ] = "Selftimer";
    canonDict [ 0x0003 ] = "Quality";
    canonDict [ 0x0004 ] = "FlashMode";
    canonDict [ 0x0005 ] = "DriveMode";
    canonDict [ 0x0007 ] = "FocusMode";
    canonDict [ 0x000a ] = "ImageSize";
    canonDict [ 0x000b ] = "EasyMode";
    canonDict [ 0x000c ] = "DigitalZoom";
    canonDict [ 0x000d ] = "Contrast";
    canonDict [ 0x000e ] = "Saturation";
    canonDict [ 0x000f ] = "Sharpness";
    canonDict [ 0x0010 ] = "ISOSpeed";
    canonDict [ 0x0011 ] = "MeteringMode";
    canonDict [ 0x0012 ] = "FocusType";
    
    gpsDict   [ 0xffff ] = "GPSInfo";
    gpsDict   [ 0x0000 ] = "GPSVersionID";
    gpsDict   [ 0x0001 ] = "GPSLatitudeRef";
    gpsDict   [ 0x0002 ] = "GPSLatitude";
    gpsDict   [ 0x0003 ] = "GPSLongitudeRef";
    gpsDict   [ 0x0004 ] = "GPSLongitude";
    gpsDict   [ 0x0005 ] = "GPSAltitudeRef";
    gpsDict   [ 0x0006 ] = "GPSAltitude";
    gpsDict   [ 0x0007 ] = "GPSTimeStamp";
    gpsDict   [ 0x0008 ] = "GPSSatellites";
    gpsDict   [ 0x0012 ] = "GPSMapDatum";
    gpsDict   [ 0x001d ] = "GPSDateStamp";

    sonyDict  [ 0xffff ] = "Sony";
    sonyDict  [ 0x0001 ] = "Offset";
    sonyDict  [ 0x0002 ] = "ByteOrder";
    sonyDict  [ 0xb020 ] = "ColorReproduction";
    sonyDict  [ 0xb040 ] = "Macro";
    sonyDict  [ 0xb041 ] = "ExposureMode";
    sonyDict  [ 0xb042 ] = "FocusMode";
    sonyDict  [ 0xb043 ] = "AFMode";
    sonyDict  [ 0xb044 ] = "AFIlluminator";
    sonyDict  [ 0xb047 ] = "JPEGQuality";
    sonyDict  [ 0xb048 ] = "FlashLevel";
    sonyDict  [ 0xb049 ] = "ReleaseMode";
    sonyDict  [ 0xb04a ] = "SequenceNumber";
    sonyDict  [ 0xb04b ] = "AntiBlur";
    sonyDict  [ 0xb04e ] = "LongExposureNoiseReduction";
}

int main(int argc,const char* argv[])
{
    init();
    
    int rc = 0;
    if ( argc == 2 || argc == 3 ) {
        const char* path = argv[argc-1];
        Image* image = NULL ;
        TiffImage tiff(path);
        JpegImage jpeg(path);
        
        PSopt_e opt = kpsBasic;
        if ( argc == 3 ) {
            const char*      arg = argv[1];
            char c = tolower(arg[0]);
            opt = c == 's' ? kpsBasic
                : c == 'r' ? kpsRecursive
                : c == 'x' ? kpsXMP
                : c == 'i' ? kpsIccProfile
                : opt
                ;
        }
        if ( tiff.valid() ) image = &tiff ;
        if ( jpeg.valid() ) image = &jpeg ;
        if ( image ) {
            image->printStructure(std::cout,opt,tiffDict);
        } else {
            std::cerr << "file type not recognised " << path << std::endl;
            rc=2;
        }
    } else {
        std::cout << "usage: [ {S | R | X | I} ] " << argv[0] << " path" << std::endl;
        rc = 1;
    }
    return rc;
}

// That's all Folks!
////
