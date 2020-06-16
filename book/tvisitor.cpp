#include <iostream>
#include <set>
#include <vector>
#include <string>
#include <sstream>
#include <map>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

typedef std::set<size_t> Visits;

// this prevents unknown tags being reported!
// return true if you want to see unknown tags
bool printTag(std::string& name)
{
    return name.find(".0x") == std::string::npos;
}

// types of data in Exif Specification
enum type_e
{    kttMin             = 0
,    kttUByte           = 1 //!< Exif BYTE type, 8-bit unsigned integer.
,    kttAscii           = 2 //!< Exif ASCII type, 8-bit byte.
,    kttUShort          = 3 //!< Exif SHORT type, 16-bit (2-byte) unsigned integer.
,    kttULong           = 4 //!< Exif LONG type, 32-bit (4-byte) unsigned integer.
,    kttURational       = 5 //!< Exif RATIONAL type, two LONGs: numerator and denumerator of a fraction.
,    kttSByte           = 6 //!< Exif SBYTE type, an 8-bit signed (twos-complement) integer.
,    kttUndefined       = 7 //!< Exif UNDEFINED type, an 8-bit byte that may contain anything.
,    kttSShort          = 8 //!< Exif SSHORT type, a 16-bit (2-byte) signed (twos-complement) integer.
,    kttSLong           = 9 //!< Exif SLONG type, a 32-bit (4-byte) signed (twos-complement) integer.
,    kttSRational       =10 //!< Exif SRATIONAL type, two SLONGs: numerator and denumerator of a fraction.
,    kttFloat           =11 //!< TIFF FLOAT type, single precision (4-byte) IEEE format.
,    kttDouble          =12 //!< TIFF DOUBLE type, double precision (8-byte) IEEE format.
,    kttIfd             =13 //!< TIFF IFD type, 32-bit (4-byte) unsigned integer.
,    kttNot1            =14
,    kttNot2            =15
,    kttULongLong       =16 //!< Exif LONG LONG type, 64-bit (8-byte) unsigned integer.
,    kttSLongLong       =17 //!< Exif LONG LONG type, 64-bit (8-byte) signed integer.
,    kttIfd8            =18 //!< TIFF IFD type, 64-bit (8-byte) unsigned integer.
,    kttMax             =19
};
const char* typeName(type_e tag)
{
    //! List of TIFF image tags
    const char* result = NULL;
    switch (tag ) {
        case kttUByte      : result = "BYTE"      ; break;
        case kttAscii      : result = "ASCII"     ; break;
        case kttUShort     : result = "SHORT"     ; break;
        case kttULong      : result = "LONG"      ; break;
        case kttURational  : result = "RATIONAL"  ; break;
        case kttSByte      : result = "SBYTE"     ; break;
        case kttUndefined  : result = "UNDEFINED" ; break;
        case kttSShort     : result = "SSHORT"    ; break;
        case kttSLong      : result = "SLONG"     ; break;
        case kttSRational  : result = "SRATIONAL" ; break;
        case kttFloat      : result = "FLOAT"     ; break;
        case kttDouble     : result = "DOUBLE"    ; break;
        case kttIfd        : result = "IFD"       ; break;
        case kttULongLong  : result = "LONGLONG"  ; break;
        case kttSLongLong  : result = "SLONGLONG" ; break;
        case kttIfd8       : result = "IFD8"      ; break;
        default            : result = "unknown"   ; break;
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
,   kerUnknownFormat
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
        case   kerUnknownFormat          : std::cerr << "unknown format"           ; break;
        default                          : std::cerr << "unknown error"            ; break;
    }
    if ( msg.size() ) std::cerr << " " << msg ;
    std::cerr << std::endl;
    _exit(1); // pull the plug!
}

void Error (error_e error, size_t n)
{
    std::ostringstream os ;
    os << n ;
    Error(error,os.str());
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
    bool is       (const char* str) {
        size_t l      = ::strlen(str);
        bool   result = l == size_;
        size_t i = 0 ;
        while ( result && i < l ) {
            result = str[i]==pData_[i];
            i++;
        }
        return result;
    }

    std::string toString(size_t offset,type_e type,uint16_t count,endian_e endian);
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
uint64_t byteSwap(uint64_t value,bool bSwap,uint16_t n)
{
    uint64_t result = 0;
    byte* source_value      = reinterpret_cast<byte *>(&value);
    byte* destination_value = reinterpret_cast<byte *>(&result);

    for (int i = 0; i < n; i++)
        destination_value[i] = source_value[n - i - 1];

    return bSwap ? result : value;
}

uint16_t getShort(const byte b[],size_t offset,endian_e endian)
{
    bool bSwap = endian != ::platformEndian();
    return (uint16_t) byteSwap((uint64_t)*(b+offset),bSwap,2);
}
uint32_t getLong(const byte b[],size_t offset,endian_e endian)
{
    bool bSwap = endian != ::platformEndian();
    return (uint32_t) byteSwap((uint64_t)*(b+offset),bSwap,2);
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

// Tiff Data Functions
bool isByteType(type_e type)
{
    return type == kttAscii
        || type == kttUByte
        || type == kttSByte
        || type == kttUndefined
        ;
}
bool isShortType(type_e type) {
     return type == kttUShort
         || type == kttSShort
         ;
}
bool isLongType(type_e type) {
     return type == kttULong
         || type == kttSLong
         ;
}
bool isLongLongType(type_e type) {
    return type == kttULongLong
        || type == kttSLongLong
        ;
}
bool isRationalType(type_e type) {
     return type == kttURational
         || type == kttSRational
         ;
}
bool is2ByteType(type_e type)
{
    return isShortType(type);
}
bool is4ByteType(type_e type)
{
    return isLongType(type)
        || type == kttFloat
        || type == kttIfd
        ;
}
bool is8ByteType(type_e type)
{
    return  isRationalType(type)
         || isLongLongType(type)
         || type == kttIfd8
         || type == kttDouble
         ;
}
uint16_t typeSize(type_e type)
{
    return isByteType (type) ? 1
        :  is2ByteType(type) ? 2
        :  is4ByteType(type) ? 4
        :  is8ByteType(type) ? 8
        :  1 ;
}
type_e getType(const DataBuf& buf,size_t offset,endian_e endian)
{
    return (type_e) getShort(buf,offset,endian);
}

bool typeValid(type_e type)
{
    return type  > kttMin  && type <  kttMax
        && type != kttNot1 && type != kttNot2
    ;
}

// string formatting functions
std::string indent(size_t s)
{
    std::string result ;
    while ( s-- > 1) result += "  ";
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

std::string DataBuf::binaryToString(size_t start=0,size_t size=0)
{
    return ::binaryToString(pData_,start,size?size:size_);
}

std::string DataBuf::toString(size_t offset,type_e type,uint16_t count,endian_e endian)
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
    } else if ( type == kttUByte ) {
        os << binaryToString(offset, (size_t)count);
    } else if ( type == kttAscii ) {
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

    return os.str();
} // DataBuf::toString

// Camera manufacturers
enum maker_e
{   kUnknown
,   kCanon
,   kNikon
,   kSony
};

// Canon magic
enum kCanonHeap
{   kStg_InHeapSpace    = 0
,   kStg_InRecordEntry  = 0x4000
};

#define kcAscii 0x0800
#define kcWord  0x1000
#define kcDword 0x1000
#define kcHTP1  0x2800
#define kcHTP2  0x3000
enum kCanonType
{   kDT_BYTE            = 0x0000
,   kDT_ASCII           = kcAscii
,   kDT_WORD            = kcWord
,   kDT_DWORD           = kcDword
,   kDT_BYTE2           = 0x2000
,   kDT_HeapTypeProp1   = kcHTP1
,   kDT_HeapTypeProp2   = kcHTP1
,   kTC_WildCard        = 0xffff
,   kTC_Null            = 0x0000
,   kTC_Free            = 0x0001
,   kTC_ExUsed          = 0x0002
,   kTC_Description     = 0x0005|kcAscii
,   kTC_ModelName       = 0x000a|kcAscii
,   kTC_FirmwareVersion = 0x000b|kcAscii
,   kTC_ComponentVersion= 0x000c|kcAscii
,   kTC_ROMOperationMode= 0x000d|kcAscii
,   kTC_OwnerName       = 0x0010|kcAscii
,   kTC_ImageFileName   = 0x0016|kcAscii
,   kTC_ThumbnailFileName=0x001c|kcAscii
,   kTC_TargetImageType = 0x000a|kcWord
,   kTC_SR_ReleaseMethod= 0x0010|kcWord
,   kTC_SR_ReleaseTiming= 0x0011|kcWord
,   kTC_ReleaseSetting  = 0x0016|kcWord
,   kTC_BodySensitivity = 0x001c|kcWord
,   kTC_ImageFormat     = 0x0003|kcDword
,   kTC_RecordID        = 0x0004|kcDword
,   kTC_SelfTimerTime   = 0x0006|kcDword
,   kTC_SR_TargetDistanceSetting = 0x0007|kcDword
,   kTC_BodyID          = 0x000b|kcDword
,   kTC_CapturedTime    = 0x000e|kcDword
,   kTC_ImageSpec       = 0x0010|kcDword
,   kTC_SR_EF           = 0x0013|kcDword
,   kTC_MI_EV           = 0x0014|kcDword
,   kTC_SerialNumber    = 0x0017|kcDword
,   kTC_SR_Exposure     = 0x0018|kcDword
,   kTC_CameraObject    = 0x0007|kcHTP1
,   kTC_ShootingRecord  = 0x0002|kcHTP2
,   kTC_MeasuredInfo    = 0x0003|kcHTP2
,   kTC_CameraSpecificaiton= 0x0004|kcHTP2
};

// TagDict is used to map tag (uint16_t) to string
typedef std::map<uint16_t,std::string> TagDict;
TagDict emptyDict ;
TagDict tiffDict  ;
TagDict exifDict  ;
TagDict canonDict ;
TagDict nikonDict ;
TagDict sonyDict  ;
TagDict gpsDict   ;
TagDict crwDict   ;

enum ktSpecial
{   ktMakerNote = 0x927c
,   ktGps       = 0x8825
,   ktExif      = 0x8769
,   ktSubIFD    = 0x014a
,   ktMake      = 0x010f
,   ktXMLPacket = 0x02bc
,   ktGroup     = 0xffff
};

bool tagKnown(uint16_t tag,const TagDict& tagDict)
{
    return tagDict.find(tag) != tagDict.end();
}

std::string groupName(const TagDict& tagDict)
{
    std::string group = tagKnown(ktGroup,tagDict)
                      ? tagDict.find(ktGroup)->second
                      : "Unknown"
                      ;
    return "Exif." + group ;
}

std::string tagName(uint16_t tag,const TagDict& tagDict,const size_t max=0)
{
    std::string name = tagKnown(tag,tagDict)
                     ? tagDict.find(tag)->second
                     : stringFormat("%#x",tag)
                     ;

    name =  groupName(tagDict) + "." + name;
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

    Io(Io& io,size_t start,size_t size=0) // Io object is a substream
    : path_   (io.path())
    , start_  (start+io.start_)
    , size_   (size?size:io.size()-start)
    , restore_(ftell(f_))
    , f_      (io.f_)
    {
        std::ostringstream os;
        os << path_ << ":" << start << "->" << size_;
        path_=os.str();
        seek(0);
    };

    uint32_t getLong(endian_e endian)
    {
        DataBuf buf(4);
        read   (buf);
        return ::getLong(buf,0,endian);
    }
    uint16_t getShort(endian_e endian)
    {
        byte b[2];
        read(b,2);
        return ::getShort(b,0,endian);
    }

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

class IoSave // restore Io when function ends
{
public:
    IoSave(Io& io,size_t address)
    : io_(io)
    , restore_(io.tell())
    { io_.seek(address); }
    virtual ~IoSave() {io_.seek(restore_);}
private:
    Io&    io_;
    size_t restore_;
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
class   CrwImage ;
class   IFD;
class   CIFF;

// 2. Create abstract "visitor" base class with an element visit() method
class Visitor
{
public:
    Visitor(std::ostream& out,PSopt_e option)
    : out_   (out)
    , option_(option)
    {};
    virtual ~Visitor() {};

    virtual void visitBegin   (Image& image) = 0 ;
    virtual void visitEnd     (Image& image) = 0 ;
    virtual void visitDirBegin(Image& image,size_t dirLength) = 0 ;
    virtual void visitDirEnd  (Image& image,size_t start    ) = 0 ;
    virtual void visitTag     (Io& io,Image& image,size_t address,const TagDict& tagDict)=0;
    virtual void visitReport  (std::ostringstream& out) {} ;
    virtual void visitReport  (std::ostringstream& out,bool& bLF) {} ;

    PSopt_e       option() { return option_ ; }
    std::ostream& out()    { return out_    ; }
private:
    PSopt_e       option_;
    std::ostream& out_   ;
};

// 3. Image has an accept(Visitor&) method
class Image
{
public:
    Image(std::string path)
    : io_(path,"rb")
    , start_    (0)
    , maker_    (kUnknown)
    , makerDict_(emptyDict)
    , bigtiff_  (false)
    , endian_   (keLittle)
    , depth_    (0)
    {};
    Image(Io io)
    : io_       (io)
    , makerDict_(emptyDict)
    , depth_    (0)
    {};
    virtual    ~Image()        { io_.close()      ; }
    bool        valid()        { return false     ; }
    std::string path()         { return io_.path(); }
    endian_e    endian()       { return endian_   ; }
    Io&         io()           { return io_       ; }
    std::string format()       { return format_   ; }
    Visits&     visits()       { return visits_   ; }
    size_t      depth()        { return depth_    ; }

    virtual void accept(class Visitor& v)=0;

    maker_e     maker_;
    TagDict&    makerDict_;
    void setMaker(DataBuf& buf)
    {
        maker_ = buf.strequals("Canon"            ) ? kCanon
               : buf.strequals("NIKON CORPORATION") ? kNikon
               : buf.strequals("SONY")              ? kSony
               : maker_
               ;
        switch ( maker_ ) {
            case kCanon : makerDict_ = canonDict ; break;
            case kNikon : makerDict_ = nikonDict ; break;
            case kSony  : makerDict_ = sonyDict  ; break;
            default : /* do nothing */           ; break;
        }
    } // setMaker

    friend class TiffImage;
    friend class JpegImage;
    friend class CrwImage ;
    friend class ReportVisitor;
    friend class IFD      ;
    friend class CIFF     ;

private:
    Visits      visits_;
    size_t      start_;
    Io          io_;
    bool        good_;
    uint16_t    magic_;
    endian_e    endian_;
    bool        bigtiff_;
    size_t      depth_;
    std::string format_; // "TIFF", "JPEG" etc...

    bool isPrintXMP(uint16_t type, PSopt_e option)
    {
        return type == ktXMLPacket && option == kpsXMP;
    }

};

// 4. Create concrete "visitors"
class ReportVisitor: public Visitor
{
public:
    ReportVisitor(std::ostream& out, PSopt_e option)
    : Visitor(out,option)
    , indent_(0)
    {}

    std::string indent() { return ::indent(indent_); }

    virtual void visitBegin(Image& image)
    {
        indent_++;
        if ( option() == kpsBasic || option() == kpsRecursive ) {
            char c = image.endian() == keBig ? 'M' : 'I';
            out() << indent() << stringFormat("STRUCTURE OF %s FILE (%c%c): ",image.format().c_str(),c,c) <<  image.io().path() << std::endl;
            out() << indent() << " address |    tag type      count | value" << std::endl;
        }
    }
    virtual void visitDirBegin(Image& image,size_t dirLength)
    {
        //size_t depth = image.depth();
        //out() << indent(depth) << stringFormat("+%d",dirLength) << std::endl;
    };
    virtual void visitDirEnd(Image& image,size_t start)
    {
        // if ( start ) out() << std::endl;
    };

    virtual void visitReport(std::ostringstream& os)
    {
        out() << os.str();
        os.str("");// reset the string
        os.clear();// reset the good/bad/ugly flags
    }
    virtual void visitReport(std::ostringstream& os,bool& bLF)
    {
        if ( bLF ) {
            os << std::endl;
            visitReport(os);
        }
        bLF = false ;
    }

    virtual void visitTag
    ( Io&                   io
    , Image&                image
    , size_t                address
    , const TagDict&        tagDict
    ) {
        IoSave  restore(io,address);
        DataBuf tiffTag(12);
        io.read(tiffTag);
        endian_e endian = image.endian();

        uint16_t tag    = getShort(tiffTag,0,endian);
        type_e   type   = getType (tiffTag,2,endian);
        uint32_t count  = getLong (tiffTag,4,endian);
        size_t   offset = getLong (tiffTag,8,endian);
        uint16_t size   = typeSize(type);

        // allocate a buffer and read the data
        DataBuf buf(count*size);
        std::string offsetString ;
        std::string value ;
        if ( count*size > 4 ) {  // read into buffer
            io.seek(offset);     // position
            io.read(buf);        // read
            value = buf.toString(0,type,count,endian);
            offsetString = stringFormat("%10u", offset);
        } else {
            value = tiffTag.toString(8,type,count,endian);
        }

        // format the output
        std::string name  = tagName(tag,tagDict,28);

        if ( printTag(name) ) {
            out() << indent()
                  << stringFormat("%8u | %#06x %-28s |%10s |%9u |%10s | "
                        ,address,tag,name.c_str(),typeName(type),count,offsetString.c_str())
                  << chop(value,40)
                  << std::endl
            ;
            if ( makerTags.find(name) != makerTags.end() ) {
                for (Field field : makerTags[name] ) {
                    std::string n = join(groupName(tagDict),field.name(),28);
                    out() << indent()
                          << stringFormat("%8u | %#06x %-28s |%10s |%9u |%10s | "
                                         ,offset+field.start(),tag,n.c_str(),typeName(field.type()),field.count(),"")
                          << chop(buf.toString(field.start(),field.type(),field.count(),field.endian()==keImage?image.endian():field.endian()),40)
                          << std::endl
                    ;
                }
            }
        }
    } // visitTag

    virtual void visitEnd(Image& image)
    {
        if ( option() == kpsBasic || option() == kpsRecursive ) {
            out() << indent() << "END: " << image.path() << std::endl;
        }
        indent_--;
    } // visitEnd
//
private:
    size_t indent_;
};

class CIFF
{
public:
    CIFF(Image& image,size_t start)
    : image_  (image)
    , io_     (image.io())
    , parent_ (image.io())
    {
        // IoSave save(parent_,start);
        byte              u[2];
        image_.io()  .read(u,2);
        length_ = getShort(u,0,image.endian());
        io_ = Io(parent_,start,2+10*length_);
    };
    virtual ~CIFF() {};
public:
    Image&   image_  ;
    Io&      io_     ;
    Io       parent_ ;
    uint16_t length_ ;
    Io&      parent() { return parent_ ;}
public:
    void accept(Visitor& visitor);
};

class IFD
{
public:
    IFD(Image& image,size_t start,bool hasNext=true)
    : image_  (image)
    , start_  (start)
    , io_     (image.io())
    , hasNext_(hasNext)
    {};

    void     visit(Visitor& visitor,const TagDict& tagDict=tiffDict);
    Visits&  visits()    { return image_.visits()  ; }
    maker_e  maker()     { return image_.maker_    ; }
    TagDict& makerDict() { return image_.makerDict_; }
    endian_e endian()    { return image_.endian()  ; }

private:
    bool     hasNext_;
    Image&   image_  ;
    size_t   start_  ;
    Io&      io_     ;
};

// Concrete Images
class TiffImage : public Image
{
public:
    TiffImage(std::string path)
    : Image(path)
    , bHasNext_(true)
    {}
    TiffImage(Io& io)
    : Image(io)
    , bHasNext_(false)
    {}

    void visit(Visitor& visitor,TagDict& tagDict = tiffDict );
    bool valid();

    virtual void accept(class Visitor& visitor) { visit(visitor); }

private:
    bool bHasNext_;
};

class JpegImage : public Image
{
public:
    JpegImage(std::string path)
    : Image(path)
    { io_.seek(0); init() ; }
    JpegImage(Io& io,size_t start,size_t count)
    : Image(Io(io,start,count))
    { io_.seek(0); init() ; }
    virtual void accept(class Visitor& v);
    bool valid();

    int advanceToMarker()
    {   // Search for 0xff
        while ( !io_.eof() && io_.getb() != 0xff) {}
        // Search for next byte which isn't 0xff
        int c = -1;
        while ( !io_.eof() && (c=io_.getb()) == 0xff) {}
        return io_.eof() ? -1 : c;
    };
    
private:
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

    void init()
    {
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
        for (int i = 0; i < 256; i++)
            bHasLength[i] = (i >= sof0_ && i <= sof15_) || (i >= app0_ && i <= (app0_ | 0x0F)) ||
                            (i == dht_  || i == dqt_    || i == dri_   || i == com_  ||i == sos_);
    }
    // nmonic for markers
    std::string nm[256];
};

void CIFF::accept(Visitor& visitor)
{
    std::cout << ::indent(2) << stringFormat("CIFF Directory %s length = %d parent = %s",io_.path().c_str(),length_,parent_.path().c_str()) << std::endl;
    std::cout << ::indent(2) <<             "    tag | name                           |  count | offset "       << std::endl;
    io_.seek(2);
    DataBuf buf(10);
    for ( int i = 0 ; i < length_ ; i++ ) {
        io_.read(buf);
        uint16_t tag    = getShort(buf,0,image_.endian());
        size_t   count  = getLong (buf,2,image_.endian());
        size_t   offset = getLong (buf,6,image_.endian());
        std::cout << ::indent(2)<< stringFormat(" %6#x | %-30s | %6d | %d ",tag,tagName(tag,crwDict,28).c_str(),count,offset) << std::endl;
        
        if ( tag == 0x2008 ) {  // ThumbnailImage
            JpegImage jpeg(parent_,offset,count);
            jpeg.accept(visitor);
        } else if ( tag == 0x300a        /* ImageSpec      */ ) {
            // like dumpImageHeader();
            // CIFF imageSpec(something);
            // imageSpec.accept(visitor);
        }
    }
}

class CrwImage : public Image
{
public:
    CrwImage(std::string path) : Image(path) { start_ = 0 ;}
    CrwImage(Io& io) : Image(io) { start_ = 0 ; }
    bool valid() {
        IoSave  restore(io(),0);
        bool    result = false;
        DataBuf buf(2+4+8); //xxlongHEAPCCRD xx = II or MM
        if ( io().good() ) {
            io().read(buf);

            char I = 'I';
            char M = 'M';
            char c = buf.pData_[0];
            char C = buf.pData_[1];
            result = ::memcmp(buf.pData_+6,"HEAPCCDR",8) == 0
                    && c==C && (c == I || c == M)
            ;
            if ( result ) {
                endian_ = c == I ? keLittle : keBig ;
                start_  = getLong(buf,2,endian_);
                format_ = "CRW";
                // change our IO to be the CRW "Heap" sub-file
                io_ = Io(io_,start_,io_.size()-start_);
                // and get the start from the end of the heap
                io().seek(io().size()-4);
                DataBuf start(4);
                io().read(start);
                start_ = ::getLong(start,0,endian_);
            }
        }
        return result ;
    }

    void dumpImageHeader(size_t start,endian_e endian,uint16_t depth)
    {
        IoSave   restore(io(),start);

        uint32_t imageWidth         = io().getLong(endian);
        uint32_t imageHeight        = io().getLong(endian);
        int32_t  pixelAspectRatio   = (int32_t) io().getLong(endian);
        float    rotationAngle      = (float) io().getLong(endian);
        uint32_t componentBitDepth  = io().getLong(endian);
        uint32_t colorBitDepth      = io().getLong(endian);
        uint32_t colorBW            = io().getLong(endian);
        std::cout << ::indent(2) << stringFormat("width,height                    = %d,%d  ", imageWidth,imageHeight)          << std::endl;
        std::cout << ::indent(2) << stringFormat("pixelAspectRatio,rotationAngle  = %#x,%f" , pixelAspectRatio,rotationAngle)  << std::endl;
        std::cout << ::indent(2) << stringFormat("componentBitDepth,colorBitDepth = %d,%d"  , componentBitDepth,colorBitDepth) << std::endl;
        std::cout << ::indent(2) << stringFormat("colorBW                         = %d"     , colorBW)                         << std::endl;
    }
    virtual void accept(class Visitor& visitor)
    {
        IoSave save(io(),start_);
        CIFF ciff(*this,start_);
        ciff.accept(visitor);
#if 0
        byte      u[2];
        io().read(u,2);
        uint16_t dirLength = getShort(u,0,endian_);

        std::cout << ::indent(2) << stringFormat("CIFF Directory %s start_ = %d dirLength = %d",io().path().c_str(),start_,dirLength) << std::endl;
        std::cout << ::indent(2) <<             "    tag | name                           |  count | offset "       << std::endl;
        DataBuf buf(10); // It's a CIFF not an IFD!
        for ( int i = 0 ; i < dirLength ; i++ ) {
            io().read(buf);
            uint16_t tag    = getShort(buf,0,endian_);
            uint32_t count  = getLong (buf,2,endian_);
            uint32_t offset = getLong (buf,6,endian_);
            std::cout << ::indent(2)<< stringFormat(" %6#x | %-30s | %6d | %d ",tag,tagName(tag,crwDict,28).c_str(),count,offset) << std::endl;
            if ( tag & kStg_InRecordEntry ) {
                std::cout << "data" << std::endl ;
            } else if ( tag == 0x300a        /* ImageSpec      */ ) {
                dumpImageHeader(offset,endian(),depth_+3) ;
                // it's a CIFF
                DataBuf block(count);
                io().seek(offset);
                io().read(block);
                size_t    start  = getShort(block,count-4,endian_);
                uint16_t  length = getShort(block,start,endian_);
                std::cout << ::indent(2) << stringFormat("CIFF start, length = %d %d",start,length) << std::endl;
                std::cout << "    tag | name                           |  count | offset | value " << std::endl;
                for ( int i = 0 ; i < length ; i++ ) {
                    tag    = getShort(block,start+2+i*10+0,endian_);
                    count  = getLong (block,start+2+i*10+2,endian_);
                    offset = getLong (block,start+2+i*10+6,endian_);
                    std::string name = tagName(tag,crwDict,28);

                    if ( printTag(name)  ) { // ignore unknown tags
                        std::cout << ::indent(2) << stringFormat(" %6#x | %-30s | %6d | %-4d  ",tag,name.c_str(),count,offset);
                        if ( tag & kcAscii && tag != 0x2804 /*ImageDescription*/ ) {
                            std::cout << " | " << chop(block.toString(offset,kttAscii,count,endian_),60);
                        } else {
                            std::cout << " | " << chop(block.binaryToString(offset,count),60);
                        }
                        std::cout << std::endl;
#if 1
                        if ( tag == 0x300b /* ExifInformation */ ) {
                            // Io  exifStream(io(),offset,count);

                            //TiffImage tiff(exifStream);
                            //tiff.visitIFD(visitor,0,endian_,visitor.depth_,canonDict,false);
                            /*
                            CrwImage exif(exifStream);
                            exif.start_ = exifStream.size()-4;
                            exif.accept(visitor);
                            */
                        }
#endif
                    }
                }
            } else if ( tag == 0x2008 /* ThumbnailImage  */ ) {
                JpegImage jpeg(io(),offset,count) ;
                jpeg.accept(visitor);
            }
        }
#endif
    }
};

void IFD::visit(Visitor& visitor,const TagDict& tagDict/*=tiffDict*/)
{
    IoSave save(io_,start_);

    if ( !image_.depth_++ ) image_.visits().clear();
    visitor.visitBegin(image_);
    if ( image_.depth_ > 100 ) Error(kerCorruptedMetadata) ; // weird file

    // buffer
    DataBuf  dir(12);
    size_t   start=start_;
    while  ( start ) {
        // Read top of directory
        io_.read(dir.pData_, 2);
        uint16_t dirLength = getShort(dir,0,image_.endian());

        if ( dirLength > 500 ) Error(kerTiffDirectoryTooLarge,dirLength);
        visitor.visitDirBegin(image_,dirLength);

        // Run along the directory
        for ( int i = 0 ; i < dirLength ; i ++ ) {
            const size_t address = start + 2 + i*12 ;
            if ( visits().find(address) != visits().end()  ) { // never visit the same place twice!
                Error(kerCorruptedMetadata);
            }
            visits().insert(address);
            io_.seek(address);

            io_.read(dir);
            uint16_t tag    = getShort(dir,0,image_.endian());
            type_e   type   = getType (dir,2,image_.endian());
            uint32_t count  = getLong (dir,4,image_.endian());
            uint32_t offset = getLong (dir,8,image_.endian());

            if ( !typeValid(type) ) {
                Error(kerInvalidTypeValue);
            }

            visitor.visitTag(io_,image_,address,tagDict);  // Tell the visitor

            uint16_t pad     = isByteType(type)  ? 1 : 0;
            uint16_t size    = typeSize(type)    ;
            size_t   alloc   = size*count + pad+6;
            DataBuf  buf(alloc,io_.size());
            size_t   restore = io_.tell();
            io_.seek(offset);
            io_.read(buf);
            io_.seek(restore);

            // recursion anybody?
            if ( tagDict == tiffDict && tag == ktMake ) image_.setMaker(buf);
            if ( type    == kttIfd )    tag  = ktSubIFD;
            switch ( tag ) {
                case ktGps  : IFD(image_,offset,false).visit(visitor,gpsDict );break;
                case ktExif : IFD(image_,offset,false).visit(visitor,exifDict);break;
                case ktMakerNote :
                if ( image_.maker_ == kNikon ) {
                    // Nikon MakerNote is emabeded tiff `II*_.....` 10 bytes into the data!
                    size_t punt = buf.strequals("Nikon") ? 10 : 0 ;
                    Io     io(io_,offset+punt,count-punt);
                    TiffImage makerNote(io);
                    makerNote.visit(visitor,makerDict());
                } else {
                    bool   bNext = maker()  != kSony;                                        // Sony no trailing next
                    size_t punt  = maker()  == kSony && buf.strequals("SONY DSC ") ? 12 : 0; // Sony 12 byte punt
                    IFD makerNote(image_,offset+punt,bNext);
                    makerNote.visit(visitor,makerDict());
                }
                break;
                case ktSubIFD :
                    for ( size_t i = 0 ; i < count ; i++ ) {
                        uint32_t  off  = count == 1 ? offset : getLong(buf,i*4,image_.endian()) ;
                        IFD(image_,off).visit(visitor,tagDict );
                    }
                break;
                default: /* do nothing */ ; break;
            }
        } // for i < dirLength

        start = 0; // !stop
        if ( hasNext_ ) {
            io_.read(dir.pData_, 4);
            start = getLong(dir,0,image_.endian());
        }
        visitor.visitDirEnd(image_,start);
    } // while start != 0

    visitor.visitEnd(image_);
    image_.depth_--;
} // IFD::visit

bool TiffImage::valid()
{
    IoSave restore(io(),0);

    // read header
    DataBuf      header(20);
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

    return result;
} // TiffImage::valid

void TiffImage::visit(Visitor& visitor,TagDict& tagDict)
{
    if ( valid() ) {
        IFD ifd(*this,start_,bHasNext_);
        ifd.visit(visitor,tagDict);
    }
} // TiffImage::visit

bool JpegImage::valid()
{
    IoSave   restore(io(),0);
    bool     result = false;
    byte     h[2];
    io_.read(h,2);
    if ( h[0] == 0xff && h[1] == 0xd8 ) { // .JPEG
        start_  = 0;
        format_ = "JPEG";
        endian_ = keLittle;
        result  = true;
    } else if  ( h[0] == 0xff && h[1]==0x01 ) { // .EXV
        DataBuf buf(5);
        io_.read(buf);
        if ( buf.is("Exiv2") ) {
            start_ = 7;
            format_ = "EXV";
            endian_ = keLittle;
            result = true;
        }
    }
    return result;
}

#define REPORT_MARKER os << stringFormat("%8ld | 0xff%02x %-5s", io_.tell()-2,marker,nm[marker].c_str());

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
    IoSave save(io(),0);
    v.visitBegin((*this));

    bool bPrint = v.option() == kpsBasic || v.option() == kpsRecursive;

    // navigate the JPEG chunks
    std::ostringstream os; // string buffer for output to v.visitReport()

    // Container for the signature
    bool   bExtXMP    = false;
    size_t bufRead    = 0;
    size_t bufMinSize = 48;
    DataBuf buf(bufMinSize);

    // Read section marker
    int marker = advanceToMarker();
    if (marker < 0) {
        Error(kerNotAJpeg);
    }

    REPORT_MARKER;

    bool    done = false;
    while (!done) {
        size_t current = io_.tell();
        bool   bLF     = true; // tell v.visitReport() to append a LF

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
            if (v.option() == kpsXMP && signature.find("http://ns.adobe.com/x") == 0) {
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
                const size_t end = start + (size > bufMinSize-4 ? bufMinSize-4 : size);
                os << "| " << chop(buf.binaryToString(start, end),40);
            }
            if (bLF && bPrint) v.visitReport(os,bLF);

            // std::cout << "app0_+1 = " << app0_+1 << " compare " << signature << " = " << signature.find("Exif") == 2 << std::endl;
            bool bExif = v.option() == kpsRecursive && marker == (app0_ + 1) && signature.find("Exif") == 0;

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

        // Skip over the segment
        io_.seek(current+size);
        if (bLF && bPrint) v.visitReport(os,bLF);

        if (marker != sos_) {
            // Read the beginning of the next segment
            marker = advanceToMarker();
            REPORT_MARKER;
            if ( bPrint ) v.visitReport(os);
        }
        done |= marker == eoi_ || marker == sos_;
        bLF  |= done;
        if (done) {
            if ( bPrint) v.visitReport(os,bLF);
        }
    }

    v.visitEnd((*this));
}  // JpegImage::visitTiff

void init(); // prototype

int main(int argc,const char* argv[])
{
    init();

    int rc = 0;
    if ( argc == 2 || argc == 3 ) {
        const char* path = argv[argc-1];
        TiffImage tiff(path);
        JpegImage jpeg(path);
        CrwImage  crw (path);

        PSopt_e option = kpsBasic;
        if ( argc == 3 ) {
            std::string arg(argv[1]);
            option = arg.find("R") != std::string::npos ? kpsRecursive
                   : arg.find("X") != std::string::npos ? kpsXMP
                   : arg.find("S") != std::string::npos ? kpsBasic
                   : option
                   ;
        }
        ReportVisitor visitor(std::cout,option);
        if      ( tiff.valid() ) tiff.accept(visitor);
        else if ( jpeg.valid() ) jpeg.accept(visitor);
        else if (  crw.valid() )  crw.accept(visitor);
        else    { Error(kerUnknownFormat,path); }
    } else {
        std::cout << "usage: " << argv[0] << " [ {S | R | X} ] path" << std::endl;
        rc = 1;
    }
    return rc;
}

// simply data.  Nothing interesting.
void init()
{
    if ( tiffDict.size() ) return; // don't do this twice!

    tiffDict  [ktGroup ] = "Image";
    tiffDict  [ 0x8769 ] = "ExifTag";
    tiffDict  [ 0x014a ] = "SubIFD";
    tiffDict  [ 0x83bb ] = "IPTCNAA";
    tiffDict  [ 0x02bc ] = "XMLPacket";
    tiffDict  [ 0x8773 ] = "InterColorProfile";
    tiffDict  [ 0x00fe ] = "NewSubfileType";
    tiffDict  [ 0x0100 ] = "ImageWidth";
    tiffDict  [ 0x0101 ] = "ImageLength";
    tiffDict  [ 0x0102 ] = "BitsPerSample";
    tiffDict  [ 0x0103 ] = "Compression";
    tiffDict  [ 0x0106 ] = "PhotometricInterpretation";
    tiffDict  [ 0x010e ] = "ImageDescription";
    tiffDict  [ 0x010f ] = "Make";
    tiffDict  [ 0x0110 ] = "Model";
    tiffDict  [ 0x0111 ] = "StripOffsets";
    tiffDict  [ 0x0112 ] = "Orientation";
    tiffDict  [ 0x0115 ] = "SamplesPerPixel";
    tiffDict  [ 0x0116 ] = "RowsPerStrip";
    tiffDict  [ 0x0117 ] = "StringByteCounts";
    tiffDict  [ 0x011a ] = "XResolution";
    tiffDict  [ 0x011b ] = "YResolution";
    tiffDict  [ 0x011c ] = "PlanarConfiguration";
    tiffDict  [ 0x0128 ] = "ResolutionUnit";
    tiffDict  [ 0x0131 ] = "Software";
    tiffDict  [ 0x0132 ] = "DateTime";
    tiffDict  [ 0x0213 ] = "YCbCrPositioning";

    exifDict  [ktGroup ] = "Photo";
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

    nikonDict [ktGroup ] = "Nikon";
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

    canonDict [ktGroup ] = "Canon";
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

    gpsDict   [ktGroup ] = "GPSInfo";
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

    sonyDict  [ktGroup ] = "Sony";
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

    crwDict   [ktGroup ] = "CRW";
    crwDict   [ 0x0032 ] = "CanonColorInfo1";
    crwDict   [ 0x0805 ] = "CanonFileDescription";
    crwDict   [ 0x080a ] = "CanonRawMakeModel";
    crwDict   [ 0x080b ] = "CanonFirmwareVersion";
    crwDict   [ 0x080c ] = "ComponentVersion";
    crwDict   [ 0x080d ] = "ROMOperationMode";
    crwDict   [ 0x0810 ] = "OwnerName";
    crwDict   [ 0x0815 ] = "CanonImageType";
    crwDict   [ 0x0816 ] = "OriginalFileName";
    crwDict   [ 0x0817 ] = "ThumbnailFileName";
    crwDict   [ 0x100a ] = "TargetImageType";
    crwDict   [ 0x1010 ] = "ShutterReleaseMethod";
    crwDict   [ 0x1011 ] = "ShutterReleaseTiming";
    crwDict   [ 0x1016 ] = "ReleaseSetting";
    crwDict   [ 0x101c ] = "BaseISO";
    crwDict   [ 0x1028 ] = "CanonFlashInfo";
    crwDict   [ 0x1029 ] = "CanonFocalLength";
    crwDict   [ 0x102a ] = "CanonShotInfo";
    crwDict   [ 0x102c ] = "CanonColorInfo2";
    crwDict   [ 0x102d ] = "CanonCameraSettings";
    crwDict   [ 0x1030 ] = "WhiteSample";
    crwDict   [ 0x1031 ] = "SensorInfo";
    crwDict   [ 0x1033 ] = "CustomFunctions10D";
    crwDict   [ 0x1038 ] = "CanonAFInfo";
    crwDict   [ 0x1093 ] = "CanonFileInfo";
    crwDict   [ 0x10a9 ] = "ColorBalance";
    crwDict   [ 0x10b5 ] = "RawJpgInfo";
    crwDict   [ 0x10ae ] = "ColorTemperature";
    crwDict   [ 0x10b4 ] = "ColorSpace";
    crwDict   [ 0x1803 ] = "ImageFormat";
    crwDict   [ 0x1804 ] = "RecordID";
    crwDict   [ 0x1806 ] = "SelfTimerTime";
    crwDict   [ 0x1807 ] = "TargetDistanceSetting";
    crwDict   [ 0x180b ] = "SerialNumber";
    crwDict   [ 0x180e ] = "TimeStamp";
    crwDict   [ 0x1810 ] = "ImageInfo";
    crwDict   [ 0x1813 ] = "FlashInfo";
    crwDict   [ 0x1814 ] = "MeasuredEV";
    crwDict   [ 0x1817 ] = "FileNumber";
    crwDict   [ 0x1818 ] = "ExposureInfo";
    crwDict   [ 0x1834 ] = "CanonModelID";
    crwDict   [ 0x1835 ] = "DecoderTable";
    crwDict   [ 0x183b ] = "SerialNumberFormat";
    crwDict   [ 0x2005 ] = "RawData";
    crwDict   [ 0x2007 ] = "JpgFromRaw";
    crwDict   [ 0x2008 ] = "ThumbnailImage";
    crwDict   [ 0x2804 ] = "ImageDescription";
    crwDict   [ 0x2807 ] = "CameraObject";
    crwDict   [ 0x3002 ] = "ShootingRecord";
    crwDict   [ 0x3003 ] = "MeasuredInfo";
    crwDict   [ 0x3004 ] = "CameraSpecification";
    crwDict   [ 0x300a ] = "ImageProps";
    crwDict   [ 0x300b ] = "ExifInformation";

    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcVersion"         ,kttAscii , 0, 4));
    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcName"            ,kttAscii , 4,20));
    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcBase"            ,kttAscii ,24,20));
    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcAdjust"          ,kttUByte, 48, 1));
    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcQuickAdjust"     ,kttUByte, 49, 1));
    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcSharpness"       ,kttUByte, 50, 1));
    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcContrast"        ,kttUByte, 51, 1));
    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcBrightness"      ,kttUByte, 52, 1));
    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcSaturation"      ,kttUByte, 53, 1));
    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcHueAdjustment"   ,kttUByte, 54, 1));
    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcFilterEffect"    ,kttUByte, 55, 1));
    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcFilterEffect"    ,kttUByte, 56, 1));
    makerTags["Exif.Nikon.PictureControl"].push_back(Field("PcToningSaturation",kttUByte, 57, 1));
}

// That's all Folks!
////
