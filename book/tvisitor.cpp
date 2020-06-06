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
// types of data in Exif Specification
enum type_e
{   typeMin            = 0,
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
    typeNot1           =14,
    typeNot2           =15,
    unsignedLongLong   =16, //!< Exif LONG LONG type, 64-bit (8-byte) unsigned integer.
    signedLongLong     =17, //!< Exif LONG LONG type, 64-bit (8-byte) signed integer.
    tiffIfd8           =18, //!< TIFF IFD type, 64-bit (8-byte) unsigned integer.
    typeMax            =19,
};
const char* typeName(type_e tag)
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
        default               : result = "unknown"   ; break;
    }
    return result;
}

enum endian_e
{   keLittle
,   keBig
,   keImage // used by a field  to say "use image's endian"
};

// Error support
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
,   kerBigtiffNotSupported
,   kerFileDidNotOpen
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
        case   kerBigtiffNotSupported    : std::cerr << "bigtiff not supported"    ; break;
        case   kerFileDidNotOpen         : std::cerr << "file did not open"        ; break;
        default                          : std::cerr << "unknown error"            ; break;
    }
    if ( msg.size() ) std::cerr << " " << msg ;
    std::cerr << std::endl;
    _exit(1); // pull the plug!
}

void Error (error_e error)
{
    Error(error,"");
}

typedef unsigned char byte ;
class DataBuf
{
public:
    byte*   pData_;
    size_t  size_ ;
    DataBuf(size_t size,size_t size_max=0)
    : pData_(NULL)
    , size_(size)
    {
        if ( size_max && size > size_max ) {
            Error(kerInvalidMalloc);
        }
        pData_ = new byte[size_];
        if ( pData_) {
            std::memset(pData_, 0,size_);
        }
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

    std::string toString(size_t offset,type_e type,uint16_t count,endian_e endian,size_t max=40);
    std::string binaryToString(size_t start,size_t size);
};

// endian and byte swappers
bool isPlatformBigEndian()
{
    union {
        uint32_t i;
        char c[4];
    } e = { 0x01000000 };

    return e.c[0]?true:false;
}
bool   isPlatformLittleEndian() { return !isPlatformBigEndian(); }
endian_e platformEndian() { return isPlatformBigEndian() ? keBig : keLittle; }

bool isStringType(type_e type)
{
    return type == asciiString
        || type == unsignedByte
        || type == signedByte
        || type == undefined
        ;
}
bool isShortType(type_e type) {
     return type == unsignedShort
         || type == signedShort
         ;
}
bool isLongType(type_e type) {
     return type == unsignedLong
         || type == signedLong
         ;
}
bool isLongLongType(type_e type) {
    return type == unsignedLongLong
        || type == signedLongLong
        ;
}
bool isRationalType(type_e type) {
     return type == unsignedRational
         || type == signedRational
         ;
}
bool is2ByteType(type_e type)
{
    return isShortType(type);
}
bool is4ByteType(type_e type)
{
    return isLongType(type)
        || type == tiffFloat
        || type == tiffIfd
        ;
}
bool is8ByteType(type_e type)
{
    return  isRationalType(type)
         || isLongLongType(type)
         || type == tiffIfd8
         || type == tiffDouble
         ;
}
uint16_t typeSize(type_e type)
{
    return isStringType(type) ? 1
        :  is2ByteType(type)  ? 2
        :  is4ByteType(type)  ? 4
        :  is8ByteType(type)  ? 8
        :  1 ;
}
uint64_t byteSwap(uint64_t value,bool bSwap,uint16_t n)
{
    uint64_t result = 0;
    byte* source_value      = reinterpret_cast<byte *>(&value);
    byte* destination_value = reinterpret_cast<byte *>(&result);

    for (int i = 0; i < n; i++)
        destination_value[i] = source_value[n - i - 1];

    return bSwap ? result : value;
}

uint16_t getShort(const DataBuf& buf,size_t offset,endian_e endian)
{
    uint16_t v;
    char*    p = (char*) &v;
    p[0] = buf.pData_[offset+0];
    p[1] = buf.pData_[offset+1];
    bool bSwap = endian != ::platformEndian();
    return (uint16_t) byteSwap(v,bSwap,2);
}
type_e getType(const DataBuf& buf,size_t offset,endian_e endian)
{
    return (type_e) getShort(buf,offset,endian);
}
uint32_t getLong(const DataBuf& buf,size_t offset,endian_e endian)
{
    uint32_t v;
    char*    p = (char*) &v;
    p[0] = buf.pData_[offset+0];
    p[1] = buf.pData_[offset+1];
    p[2] = buf.pData_[offset+2];
    p[3] = buf.pData_[offset+3];
    bool bSwap = endian != ::platformEndian();
    return (uint32_t)byteSwap(v,bSwap,4);
}
uint64_t getLongLong(const DataBuf& buf,size_t offset,endian_e endian)
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
    bool bSwap = endian != ::platformEndian();
    return byteSwap (v,bSwap,8);
}

// Camera manufacturers
enum maker_e
{   kUnknown
,   kCanon
,   kNikon
,   kSony
};

// string formatting functions
std::string indent(size_t s)
{
    std::string result ;
    while ( s-- ) result += "  ";
    return result ;
}

// chop("a very long string",10) -> "a ver +++"
std::string chop(const std::string& a,size_t max=0)
{
    std::string result = a;
    if ( result.size() > max  && max > 4 ) {
        result = result.substr(0,max-4) + " +++";
    }
    return result;
}

// join("Exif.Nikon","PictureControl",22) -> "Exif.Nikon.PictureC.."
std::string join(const std::string& a,const std::string& b,size_t max=0)
{
    std::string c = a + "." +  b ;
    if ( max > 2 && c.size() > max ) {
        c = c.substr(0,max-2)+"..";
    }
    return c;
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

std::string binaryToString(const byte* b,size_t start,size_t size)
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

std::string DataBuf::binaryToString(size_t start,size_t size)
{
    return ::binaryToString(pData_,start,size);
}

std::string DataBuf::toString(size_t offset,type_e type,uint16_t count,endian_e endian,size_t max)
{
    std::ostringstream os;
    std::string        sp;
    uint16_t           size = typeSize(type);
    if ( isShortType(type) ){
        for ( size_t k = 0 ; k < count ; k++ ) {
            os << sp << ::getShort(*this,offset+k*size,endian);
            sp = " ";
        }
    } else if ( isLongType(type) ){
        for ( size_t k = 0 ; k < count ; k++ ) {
            os << sp << ::getLong(*this,offset+k*size,endian);
            sp = " ";
        }
    } else if ( isRationalType(type) ){
        for ( size_t k = 0 ; k < count ; k++ ) {
            uint32_t a = ::getLong(*this,offset+k*size+0,endian);
            uint32_t b = ::getLong(*this,offset+k*size+4,endian);
            os << sp << a << "/" << b;
            sp = " ";
        }
    } else if ( type == unsignedByte ) {
        for ( size_t k = 0 ; k < count ; k++ ) {
            os << sp << (int) pData_[offset+k];
            sp = " ";
        }
    } else if ( type == asciiString ) {
        bool bNoNull = true ;
        for ( size_t k = 0 ; bNoNull && k < count ; k++ )
            bNoNull = pData_[offset+k];
        if ( bNoNull )
            os << binaryToString(offset, (size_t)count);
        else
            os << (char*) pData_+offset ;
    } else {
        os << sp << binaryToString(offset, (size_t)count);
    }

    return chop(os.str(),max);
} // DataBuf::toString

// TagDict is use to map tag (uint16_t) to string (for printing)
typedef std::map<uint16_t,std::string> TagDict;
TagDict emptyDict ;
TagDict tiffDict  ;
TagDict exifDict  ;
TagDict canonDict ;
TagDict nikonDict ;
TagDict sonyDict  ;
TagDict gpsDict   ;

bool tagKnown(uint16_t tag,const TagDict& tagDict)
{
    return tagDict.find(tag) != tagDict.end();
}

std::string groupName(uint16_t tag,const TagDict& tagDict)
{
    std::string group = "Unknown";
    tag = 0xffff;
    if ( tagDict.find(tag) != tagDict.end() ) {
        group =  tagDict.find(tag)->second;
    }
    return "Exif." + group ;
}

std::string tagName(uint16_t tag,const TagDict& tagDict,const size_t max=0)
{
    std::string name ;
    if ( tag != 0xffff ) {
        if ( tagDict.find(tag) != tagDict.end() ) {
            name = tagDict.find(tag)->second;
        } else {
            name = stringFormat("%#x",tag);
        }
    }
    
    name =  groupName(tag,tagDict) + "." + name;
    if ( max && name.size() > max ){
        name = name.substr(0,max-2)+"..";
    }
    return name;
}

// Binary Records
class Field
{
public:
    Field
    ( std::string name
    , type_e      type
    , uint16_t    start
    , uint16_t    count
    , endian_e    endian = keImage
    )
    : name_  (name)
    , type_  (type)
    , start_ (start)
    , count_ (count)
    , endian_(endian)
    {};
    virtual ~Field() {}
    std::string name  () { return name_   ; }
    type_e      type  () { return type_   ; }
    uint16_t    start () { return start_  ; }
    uint16_t    count () { return count_  ; }
    endian_e    endian() { return endian_ ; }
private:
    std::string name_   ;
    type_e      type_   ;
    uint16_t    start_  ;
    uint16_t    count_  ;
    endian_e    endian_ ;
};
typedef std::vector<Field>   Fields;
typedef std::map<std::string,Fields>  MakerTags;

// global variable
MakerTags makerTags;

// IO supprt
enum seek_e
{   ksStart   = SEEK_SET
,   ksCurrent = SEEK_CUR
,   ksEnd     = SEEK_END
};
class Io
{
public:
    Io(std::string path,std::string open) // Io object from path
    : path_   (path)
    , start_  (0)
    , size_   (0)
    , restore_(0)
    , f_      (NULL)
    { f_ = ::fopen(path.c_str(),open.c_str()); if ( !f_ ) Error(kerFileDidNotOpen,path); }
    
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

// Options for ReportVisitor
enum PSopt_e
{   kpsBasic
,   kpsXMP
,   kpsRecursive
};

// 1.  declare types
class   Image; // forward
class   TiffImage;

// 2. Create abstract "visitor" base class with an element visit() method
class Visitor
{
public:
    Visitor(std::ostream& out,PSopt_e option)
    : out_   (out)
    , option_(option)
    {};
    virtual void visitBegin (Image& image,int depth=0) = 0 ;
    virtual void visitEnd   (Image& image,int depth=0) = 0 ;
    virtual void visitReport(std::ostringstream& out)=0  ;
    virtual void visitReport(std::ostringstream& out,bool& bLF)=0  ;
    virtual void visitTag   (Image& image,int depth,size_t address,const TagDict& tagDict)=0;
    PSopt_e option()    { return option_ ; }
    std::ostream& out() { return out_    ; }
public:
    PSopt_e       option_;
    std::ostream& out_;
};

// 3. Image has an accept(Visitor&) method
class Image
{
public:
    Image(std::string path)
    : io_(path,"rb")
    , start_(0)
    , maker_(kUnknown)
    , path_(path)
    , makerDict_(emptyDict)
    , bigtiff_(false)
    , endian_(keLittle)
    {}
    Image(Io io)
    : io_(io)
    , makerDict_(emptyDict)
    {
        good_ = io_.good();
        path_ = io.path();
    }
    virtual    ~Image()    { io_.close()   ; }
    bool        good()     { return good_ && io_.good() ; }
    bool        valid()    { return false  ; }
    std::string path()     { return path_  ; }
    endian_e    endian()   { return endian_; }
    Io&         io()       { return io_    ; }
    std::string format()   { return format_; }
    
    virtual void accept(class Visitor& v)=0;
    
    friend class TiffImage;
    friend class JpegImage;
    friend class ReportVisitor;

private:
    std::set<size_t>    visits_;
    size_t              start_;
    Io                  io_;
    bool                good_;
    uint16_t            magic_;
    maker_e             maker_;
    TagDict&            makerDict_;
    endian_e            endian_;
    std::string         path_;
    bool                bigtiff_;
    std::string         format_; // "TIFF" or "JPEG"
    
    bool isPrintXMP(uint16_t type, PSopt_e option)
    {
        return type == 700 && option == kpsXMP;
    }

    static bool typeValid(type_e type)
    {
        return type  > typeMin  && type <  typeMax
            && type != typeNot1 && type != typeNot2
        ;
    }
    
    void setMaker(DataBuf& buf)
    {
        maker_ = buf.strequals("Canon"            )? kCanon
               : buf.strequals("NIKON CORPORATION")? kNikon
               : buf.strequals("SONY")             ? kSony
               : maker_
               ;
        switch ( maker_ ) {
            case kCanon : makerDict_ = canonDict ; break;
            case kNikon : makerDict_ = nikonDict ; break;
            case kSony  : makerDict_ = sonyDict  ; break;
            default : /* do nothing */           ; break;
        }
    } // setMaker
};

// 4. Create concrete "visitors"
class ReportVisitor: public Visitor
{
public:
    ReportVisitor(std::ostream& out, PSopt_e option)
    : Visitor(out,option)
    {}

    void visitBegin(Image& image,int depth)
    {
        if ( option_ != kpsBasic && option_ != kpsRecursive ) return;

        char c = image.endian() == keBig ? 'M' : 'I';
        if ( image.format() == "TIFF" ) {
            out_ << indent(depth) << stringFormat("STRUCTURE OF %s FILE (%c%c): ",image.format().c_str(),c,c) <<  image.path() << std::endl;
            out_ << indent(depth)
                 << " address |    tag                              |     "
                 << " type |    count |    offset | value"
                 << std::endl
                 ;
        }
        if ( image.format() == "JPEG" ) {
            out_ << indent(depth) << "STRUCTURE OF JPEG FILE: " << image.path() << std::endl;
            out_ << "address | marker       |  length | data";
        }
    }
    virtual void visitReport(std::ostringstream& os)
    {
        out() << os.str();
        os.str("");// reset the string
        os.clear();// reset the good/bad/ugly flags
    }


    virtual void visitReport(std::ostringstream& os,bool& bLF)
    {
        if ( bLF ) os << std::endl;
        visitReport(os);
        bLF = false ;
    }

    void visitTag
    ( Image&                image
    , int                   depth
    , size_t                address
    , const TagDict&        tagDict
    ) {
        endian_e endian = image.endian();
        Io& io = image.io();

        size_t restore = io.tell(); // save io position
        io.seek(address);
        DataBuf tiffTag(12);
        io.read(tiffTag);
        uint16_t tag    = getShort(tiffTag,0,endian);
        type_e   type   = getType(tiffTag,2,endian);
        uint32_t count  = getLong(tiffTag,4,endian);
        size_t   offset = getLong(tiffTag,8,endian);
        uint16_t size   = typeSize(type);

        // allocate a buffer and read the data
        DataBuf buf(count*size);
        std::string offsetString ;
        if ( count*size > 4 ) {  // read into buffer
            io.seek(offset);     // position
            io.read(buf);        // read
            offsetString = stringFormat("%10u", offset);
        }
        io.seek(restore);                 // restore

        // format the output
        std::string name  = tagName(tag,tagDict,28);
        std::string value = buf.toString(0,type,count,image.endian(),40);

        out_ << indent(depth)
             << stringFormat("%8u | %#06x %-28s |%10s |%9u |%10s | "
                    ,address,tag,name.c_str(),typeName(type),count,offsetString.c_str())
             << value
             << std::endl
        ;
        if ( makerTags.find(name) != makerTags.end() ) {
            for (Field field : makerTags[name] ) {
                std::string n = join(groupName(tag,tagDict),field.name(),28);
                out_ << indent(depth)
                     << stringFormat("%8u | %#06x %-28s |%10s |%9u |%10s | "
                                     ,offset+field.start(),tag,n.c_str(),typeName(field.type()),field.count(),"")
                     << buf.toString(field.start(),field.type(),field.count(),field.endian()==keImage?image.endian():field.endian(),40)
                     << std::endl
                ;
            }
        }
    } // visitTag

    void visitEnd(Image& image,int depth)
    {
        if ( option_ != kpsBasic && option_ != kpsRecursive ) return;
        out_ << indent(depth) << "END: " << image.path() << std::endl;
    }
};

// Concrete Images
class TiffImage : public Image
{
public:
    TiffImage(std::string path) : Image(path) {}
    TiffImage(Io& io) : Image(io) {} ;
    void readTiff(Visitor& visitor,TagDict& tagDict=tiffDict,int depth = 0);
    void readIFD (Visitor& visitor,size_t start,endian_e endian,int depth,TagDict& tagDict,bool bHasNext=true);
    bool valid();

    virtual void accept(class Visitor& visitor) { readTiff(visitor); }
};

class JpegImage : public Image
{
public:
    JpegImage(std::string path)
    : Image(path)
    {}
    virtual void accept(class Visitor& v);
    bool valid();

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

void TiffImage::readIFD(Visitor& visitor,size_t start,endian_e endian,
    int depth/*=0*/,TagDict& tagDict/*=tiffDict*/,bool bHasNext/*=true*/)
{
    size_t   restore_at_start = io_.tell();

    if ( !depth++ ) visits_.clear();
    visitor.visitBegin(*this,depth);

    // buffer
    const size_t dirSize = 32;
    DataBuf  dir(dirSize);

    do {
        // Read top of directory
        io_.seek(start);
        io_.read(dir.pData_, 2);

        uint16_t dirLength = getShort(dir,0,endian_);
        if ( dirLength > 500 ) Error(kerTiffDirectoryTooLarge);

        // Read the dictionary
        for ( int i = 0 ; i < dirLength ; i ++ ) {
            const size_t address = start + 2 + i*12 ;

            if ( visits_.find(address) != visits_.end()  ) { // never visit the same place twice!
                Error(kerCorruptedMetadata);
            }
            visits_.insert(address);
            io_.seek(address);

            visitor.visitTag(*this,depth,address,tagDict);  // Tell the visitor

            // read the tag (we might want to modify tagDict before we tell the visitor)
            io_.read(dir.pData_, 12);
            uint16_t tag    = getShort(dir,0,endian_);
            type_e   type   = getType (dir,2,endian_);
            uint32_t count  = getLong (dir,4,endian_);
            uint32_t offset = getLong (dir,8,endian_);

            // Break for unknown tag types else we may segfault.
            if ( !typeValid(type) ) {
                Error(kerInvalidTypeValue);
            }

            uint16_t pad   = isStringType(type) ? 1 : 0;
            uint16_t size  = typeSize(type);
            size_t   alloc = size*count + pad+20;
            DataBuf  buf(alloc,io_.size());
            size_t   restore = io_.tell();
            io_.seek(offset);
            io_.read(buf);
            io_.seek(restore);
            if ( depth == 1 && tag == 0x010f /* Make */ ) setMaker(buf);

            // anybody for a recursion?
            if      ( tag  == 0x8769  ) readIFD(visitor,offset,endian,depth,exifDict); /* ExifTag   */
            else if ( tag  == 0x8825  ) readIFD(visitor,offset,endian,depth,gpsDict ); /* GPSTag    */
            else if ( type == tiffIfd ) readIFD(visitor,offset,endian,depth,tagDict );
            else if ( tag  == 0x014a  ) readIFD(visitor,offset,endian,depth,tagDict ); /* SubIFDs   */
            else if ( tag  == 0x927c  ) {                                        /* MakerNote */
                if ( maker_ == kNikon ) {
                    // MakerNote is not and IFD, it's an emabeded tiff `II*_.....`
                    size_t punt = 0 ;
                    if ( buf.strequals("Nikon")) {
                        punt = 10;
                    }
                    Io io(io_,offset+punt,count-punt);
                    TiffImage makerNote(io);
                    makerNote.readTiff(visitor,nikonDict,depth);
                } else if ( maker_ == kSony && buf.strequals("SONY DSC ") ) {
                    // Sony MakerNote IFD does not have a next pointer.
                    size_t punt   = 12 ;
                    readIFD(visitor,offset+punt,endian_,depth,sonyDict,false);
                } else {
                    readIFD(visitor,offset,endian_,depth,makerDict_);
                }
            }
        } // for i < dirLength

        start = 0; // !stop
        if ( bHasNext ) {
            io_.read(dir.pData_, 4);
            start = getLong(dir,0,endian_);
        }
    } while (start) ;
    visitor.visitEnd(*this,depth);
    depth--;

    io_.seek(restore_at_start); // restore
} // TiffImage::tourIFD

bool TiffImage::valid()
{
    size_t restore = io_.tell();
    io_.seek(0);
    // read header
    DataBuf      header(20);
    io_.seek(0);
    io_.read(header);

    char c   = (char) header.pData_[0] ;
    char C   = (char) header.pData_[1] ;
    endian_  = c == 'M' ? keBig : keLittle;
    magic_   = getShort(header,2,endian_);
    start_   = getLong (header,4,endian_);
    bool result = (magic_ == 42 && c == C) && ( c == 'I' || c == 'M' ) && start_ < io_.size() ;

    bigtiff_ = magic_ == 43;
    if ( bigtiff_ ) Error(kerBigtiffNotSupported);
    if ( result ) format_ = "TIFF";

    io_.seek(restore);
    return result;
}

void TiffImage::readTiff(Visitor& visitor,TagDict& tagDict,int depth)
{
    if ( valid() ) {
        readIFD(visitor,start_,endian_,depth,tagDict);
    }
} // TiffImage::tourTiff

bool JpegImage::valid()
{
    io_.seek(0,ksStart);
    byte   h[2];
    size_t n = io_.read(h,2);
    io_.seek(0,ksStart);
    bool result = n == 2 && h[0] == 0xff && h[1] == 0xd8;
//  std::cout << stringFormat("%ld %#x %#x result = %s\n",n,h[0],h[1],result?"true":"false");
    if ( result ) format_ = "JPEG";
    return result;
}

void JpegImage::accept(Visitor& v)
{
    if (!io_.good())
        Error(kerDataSourceOpenFailed, io_.path());
    // Ensure that this is the correct image type
    if (!valid()) {
        if (!io_.good() || io_.eof())
            Error(kerFailedToReadImageData);
        Error(kerNotAJpeg);
    }
    PSopt_e option = v.option();
    bool bPrint = option == kpsBasic || option == kpsRecursive;
    v.visitBegin((*this));

    // navigate the JPEG chunks
    std::ostringstream os; // string buffer for output to v.visitReport()

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

    const byte     dht_      = 0xc4;
    const byte     dqt_      = 0xdb;
    const byte     dri_      = 0xdd;
    const byte     sos_      = 0xda;
    const byte     eoi_      = 0xd9;
    const byte     app0_     = 0xe0;
    const byte     com_      = 0xfe;

    // Start of Frame markers
    const byte     sof0_     = 0xc0;        // start of frame 0, baseline DCT
    const byte     sof15_    = 0xcf;        // start of frame 15, differential lossless, arithmetic coding

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
    while (!done) {
        size_t current = io_.tell();
        bool bLF = true; // tell v.visitReport() to append a LF

        // Read size and signature
        bufRead = io_.read(buf.pData_, bufMinSize);
        const uint16_t size = bHasLength[marker] ? getShort(buf,0,keBig):0;

        if (bHasLength[marker])
            os << stringFormat(" | %7d ", size);

        // print signature for APPn
        if (marker >= app0_ && marker <= (app0_ | 0x0F)) {
            // http://www.adobe.com/content/dam/Adobe/en/devnet/xmp/pdfs/XMPSpecificationPart3.pdf p75
            const std::string signature = buf.binaryToString(2, buf.size_ - 2);

            // $ exiv2 -pS test/data/exiv2-bug922.jpg
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
                    os.str(""); // clear the buffer
                    os.write(reinterpret_cast<const char*>(&xmp.at(start)), size - start);// write the xmp
                    v.visitReport(os); // and tell the visitor

                    bufRead = size;
                    done = !bExtXMP;
                }
            } else {
                const size_t start = size > 0 ? 2 : 0;
                const size_t end = start + (size > 32 ? 32 : size);
                os << "| " << buf.binaryToString(start, end);
            }
            if (bLF && bPrint) v.visitReport(os,bLF);

            // std::cout << "app0_+1 = " << app0_+1 << " compare " << signature << " = " << signature.find("Exif") == 2 << std::endl;
            bool bExif = option == kpsRecursive && marker == (app0_ + 1) && signature.find("Exif") == 0;

            // Pure beauty.  Create a TiffImage and ask him to entertain the visitor
            if ( bExif ) {
                Io io(io_,current+2+6,size-2-6);
                TiffImage exif(io);
                exif.accept(v);
            }
        }

        // print COM marker
        if (marker == com_) {
            // size includes 2 for the two bytes for size!
            const int n = (size - 2) > 32 ? 32 : size - 2;
            // start after the two bytes
            os << "| " << buf.binaryToString(2, n + 2);
        }

        // Skip the segment if the size is known
        io_.seek(size - bufRead, ksCurrent);
        if (bLF && bPrint) v.visitReport(os,bLF);

        if (marker != sos_) {
            // Read the beginning of the next segment
            marker = advanceToMarker();
            os << stringFormat("%8ld | 0xff%02x %-5s",
                                io_.tell()-2,marker,nm[marker].c_str());
            if ( bPrint ) v.visitReport(os);
        }
        done |= marker == eoi_ || marker == sos_;
        if (done) {
            if ( bPrint) v.visitReport(os);
        }
    }

    v.visitEnd((*this));
}  // JpegImage::tourTiff

void init()
{
    tiffDict  [ 0xffff ] = "Image";
    tiffDict  [ 0x8769 ] = "ExifTag";
    tiffDict  [ 0x014a ] = "SubIFD";
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
    exifDict  [ 0x927c ] = "MakerNote";
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
    nikonDict [ 0x0023 ] = "PictureControl";

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

    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcVersion"         ,asciiString , 0, 4));
    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcName"            ,asciiString , 4,20));
    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcBase"            ,asciiString ,24,20));
    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcAdjust"          ,unsignedByte,48, 1));
    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcQuickAdjust"     ,unsignedByte,49, 1));
    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcSharpness"       ,unsignedByte,50, 1));
    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcContrast"        ,unsignedByte,51, 1));
    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcBrightness"      ,unsignedByte,52, 1));
    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcSaturation"      ,unsignedByte,53, 1));
    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcHueAdjustment"   ,unsignedByte,54, 1));
    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcFilterEffect"    ,unsignedByte,55, 1));
    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcFilterEffect"    ,unsignedByte,56, 1));
    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcToningSaturation",unsignedByte,57, 1));

}

int main(int argc,const char* argv[])
{
    init();

    int rc = 0;
    if ( argc == 2 || argc == 3 ) {
        const char* path = argv[argc-1];
        TiffImage tiff(path);
        JpegImage jpeg(path);

        PSopt_e option = kpsBasic;
        if ( argc == 3 ) {
            std::string arg(argv[1]);
            option = arg.find("R") != std::string::npos ? kpsRecursive
                   : arg.find("X") != std::string::npos ? kpsXMP
                   : arg.find("S") != std::string::npos ? kpsBasic
                   : option
                   ;
        }
        if ( tiff.valid() ) {
            ReportVisitor visitor(std::cout,option);
            tiff.accept(visitor);
        } else if ( jpeg.valid() ) {
            ReportVisitor visitor(std::cout,option);
            jpeg.accept(visitor);
        } else {
            std::cerr << "file type not recognised " << path << std::endl;
            rc=2;
        }
    } else {
        std::cout << "usage: [ {S | R | X} ] " << argv[0] << " path" << std::endl;
        rc = 1;
    }
    return rc;
}

// That's all Folks!
////
