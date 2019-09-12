<div id="TOC">
![Exiv2](exiv2.png)
# Image Metadata and Exiv2 Architecture

### TABLE OF CONTENTS

1. [Image File Formats](#1)<br>
[1 JPEG](#1-JPEG)<br>
[2 PNG](#1-PNG)<br>
[3 JP2](#1-JP2)<br>
[4 TIFF](#1-TIFF)<br>
[5 WebP](#1-WebP)<br>

2. [Tiff and Exif metadata](#2)
3. [MakerNotes](#3)
4. [Other metadata containers](#4)
5. [Lens Recognition](#5)
6. [Sample Applications](#6)
7. [I/O in Exiv2](#7)
8. [Exiv2 Architecture](#8)<br>
  [8.1 Tag Names in Exiv2](#8-1)<br>
  [8.2 Metadata Decoder](#8-2)<br>
  [8.3 Metadata Decoder](#8-3)<br>
  [8.4 Tiff Visitor](#8-4)<br>
9. [Test Suite and Build](#9)
10. [API/ABI](#10)
11. [Security](#11)
12. [Project Management, Release Engineering, User Support](#12)

A. [Appendix.  My home-made debugging tools   ](#a)

### Foreward

Before I start to discuss the subject of this book, I want to say "Thank You" to a few folks who have made this possbile.  First, my wife Alison, who has been my loyal support since the day we met in High School in 1967.  Secondly, I'd like to thank many people who have contributed to Exiv2 over the years.  In particular to Andreas Huggel the founder of the project and Luis and Dan who have worked tirelessly with me for more than 2 years.  And (in alphabet order): Abhinav, Alan, Ben, Gilles, Kevin, Nehal, Neils, Mahesh, Phil, Thomas, Tuan .... and others.

### About this book

This book is about Image Metadata and Exiv2 Architecture.

Image Metadata is the information stored in a digital image in addition to the image itself.  Data such as the camera model, date, time, location and camera setting are stored in the image file.  To my knowledge, no book has been written about this important technology.

Exiv2 Architecture is about the Exiv2 library and command-line application which implements cross-platform code in C++ to read, modify, insert and delete items of metadata.  I've been working on this code since 2008 and, as I approach my 70th birthday, would like to document my knowledge of the code in the hope that the code will be maintained and developed by other in the future.

### How did I get interested in this matter?

I first became interested in metadata because of a trail conversation with Dennis Connor in 2008.  Dennis and I ran frequently together in Silicon Valley and Dennis was a Software Development Manager in a company that made GPS systems for Precision Agriculture.  I had a Garmin Forerunner 201 Watch.  We realised that we could extract the GPS data from the watch in GPX format, then merge the position data into photos.  Today this is called "GeoTagging" and is supported by many applications.  However in 2008, we had never heard the term "GeoTagging".

![gpsexiftags](gpsexiftags.jpg)

I said "Oh, it can't be too difficult to do that!".  And here we are more than a decade later still working on the project.  The program geotag.py was completed in about 6 weeks.  Most of the effort went into porting both Exiv2 and pyexiv2 to Visual Studio and MacOSX.

The sample application samples/geotag.cpp provides a command line utility to geotag photos and I frequently use this on my own photographs.  Today, I have a Samsung Galaxy Watch which uploads runs to Strava.  I download the GPX from Strava.  The date/time information in the JPG is the key to search for the position data which is written as Exif GPS Tags into the image.

Back in 2008, I wanted to implement this in python as it was a good excuse to learn Python.  Having discovered exiv2 and the python wrapper pyexiv2, I set off with enthusiasm to build a cross-platform script to run on **Windows** _(XP, Visual Studio 2003)_, **Ubuntu Linux** _(Hardy Heron 2008.04 LTS)_ and **MacOSX** _(32 bit Tiger 10.4 on a big-endian PPC)_.  After I finished, I emailed Andreas.  He responded in less than an hour and invited me to join Team Exiv2.  Initialially, I provided support to build Exiv2 with Visual Studio.

Incidentally, later in 2008, Dennis offered me a contract to port his company's code to Visual Studio to be used on a Windows CE Embedded Controller.  1 million lines of C++ were ported from Linux in 6 weeks.  I worked with Dennis for 4 years on all manner of GPS related software development.

[https://clanmills.com/articles/gpsexiftags/](https://clanmills.com/articles/gpsexiftags/)

I have never been employed to work on Metadata.  Although I was a Senior Computer Scientist at Adobe for more than 10 years, I was never involved with XMP or Metadata.

### 2012 - 2017

By 2012, Andreas was loosing interest in Exiv2.  Like all folks, he has many matters which deserve his time.  A family, a business, biking and other pursuits.  From 2012 until 2017, I supported Exiv2 mostly alone.  I had lots of encouragement from Alan and other occasional contributors.  Neils did great work on lens recognition and compatibility with ExifTool.  Ben helped greatly with WebP support and managed the transition of the code from SVN to GitHub.  Phil (of ExifTool fame) has been very supportive and helpful.

I must also mention our adventures with Google Summer of Cocde and our students Abhinav, Tuan and Mahesh.  GSoC is a program at Google to sponsor students to contribute to open source projects. 1200 Students from around the world are given a bounty of $5000 to contribute 500 hours to a project over summer recess.  The projects are supervised by a mentor.  Exiv2 is considered to be part of the KDE family of projects.  Within KDE, there a sub-group of Graphics applications and Technology.  We advertised our projects, the students wrote proposals and some were accepted by Google on the Recommendation of the KDE/Graphics group.

In 2012, Abhinav joined us and contributed the Video read code and was mentored by Andreas.  In 2013, Tuan joined us and contributed the WebReady code and was mentored by me.  Mahesh also joined us and contribute the Video write code and was mentored by Abhinav.

I personally found working with the students to be enjoyable and interesting.  I retired from work in 2014 and returned to England after 15 years in Silicon Valley.  In 2016, Alison and I had a trip round the world and spent a day with Mahesh in Bangalore and with Tuan in Singapore.  We subsequently went to Vietnam to attend Tuan's wedding in 2017.

### Where are we now?

After v0.27 was released in 2017, Luis and Dan started making contributions.  They have made many important contributions to in the areas of security, test and build.

I started working on Exiv2 to implement GeoTagging.  As the years have passed, I've explored most of the code.  And I've added new capability such as support for ICC profiles, metadata-piping and file-debugging.  I've done lots of work on the build and test apparatus.  I've talked to users all over the world and closed several hundred issues and feature requests.  After I retired, Alison and I had a trip around the world.  We were invited to stay with Andreas and his family.  We met users in India, Singapore, Armenia, the USA and the UK.  And I've attended 3 Open-Source Conferences. It's been an adventure and mostly rewarding.  It's remarkable how seldom users express appreciation.

### Current Development Priorities

In July 2017 we received our first security CVE.  Not a pleasant experience.  The security folks started hitting us with fuzzed files. These are files which violate format specifications and can cause the code to crash. We responded with v0.27 which will have quarterly "dot" releases to provide security fixes.  Managing frequent releases and user correspondence consumes lots of my time.

In parallel to "the dots", major work is being carried to prepare Exiv2 for the future. Dan and Luis are working on v0.28 which will be released in 2020. This is a considerable reworking of the code into C++11.

I'm delighted by the work done by Dan, Luis and Kevin to deal with the assault of the security people. I believe we are responding effectively to security issues. None-the-less, they have dominated the development of Exiv2 for the last couple of years and many ideas could not be persued because security has consumed our engineering resources

### Purpose and Scope of this book

This book is my legacy to Exiv2.  I hope Exiv2 will continue to exist long into the future and this book is being written to document my discoveries about metadata and Exiv2 architecture.  However, I want to avoid a "cut'n'paste" of information already in the project documentation.  This book is an effort to collect my knowledge of this code into a single accessible location.  Many topics in this book are discussed in more detail in the issues stored in Redmine and GitHub.  Of course, finding those discussions isn't easy.  I hope this book helps future maintainers to understand Exiv2, solve issues and develop the code for years to come.

[TOC](#TOC)

<div id="1">
# 1 Image File Formats

The following summaries of the file formats are provided to help reader understand both this book and the Exiv2 code.  The specifications should be consulted for more details.

<div id="1-JPEG">
### 1.1 JPEG
![jpeg](jpeg.png)

![Exif22Jpg.png](Exif22Jpg.png)

[TOC](#TOC)


<div id="1-PNG">
### 1.2 PNG
![png](png.png)

[TOC](#TOC)

<div id="1-JP2">
### 1.3 JPEG 2000
![jp2](jp2.png)

[TOC](#TOC)

<div id="1-TIFF">
### 1.4 TIFF
![Tiff](Tiff.png)

[TOC](#TOC)

<div id="1-WebP">
### 1.5 WEBP
![webp](webp.png)

[TOC](#TOC)

<div id="2">
# 2 Tiff and Exif metadata

![Exif22Tiff.png](Exif22Tiff.png)

Before we get into the Exiv2 code, let's look at the simpler python JPG/TIFF Exif library: [https://github.com/Moustikitos/tyf](https://github.com/Moustikitos/tyf)

This is a library and I've constructed a simple wrapper to reveal the Exif metadata.

```python
#!/usr/bin/env python3

import Tyf
import os
import sys

##
#
def dumpTags(ifd):
	for tag in ifd:
		V=tag[1]
		v=str(V)
		if type(V)==type(''):
			v='"'+v+'"'
		if len(v) > 50:
		   v = v[0:48] + '...'
		t=str(type(V))
		t=t[8:len(t)-2]
		if t == 'bytes':
			t=str(len(V)) + ' ' + t
		elif t == 'str':
			t=str(len(V))
		t='('+t+')'

		print('%s -> %s %s' % ( tag[0], v , t)	)

##
#
def main(argv):
	"""main - main program of course"""

	image = Tyf.open(argv[1])
	# help(jpg)

	if str(type(image)) == "<class 'Tyf.TiffFile'>":
		dumpTags(image[0])
	elif str(type(image)) == "<class 'Tyf.JpegFile'>":
		dumpTags(image.ifd0)
	else:
		print("unknown image type " + str(type(image)))

if __name__ == '__main__':
	main(sys.argv)

# That's all Folks
##
```

You can obtain Stonehenge.jpg from https://clanmills.com/Stonehenge.jpg
The tiff version is available in test/data/Stonehenge.tif

```
$ ~/bin/mdump.py  ~/Stonehenge.jpg                             $ ~/bin/mdump.py  ~/Stonehenge.jpg
Make -> "NIKON CORPORATION" (17)                               ImageLength -> 1 (int)
Model -> "NIKON D5300" (11)                                    BitsPerSample -> (8, 8, 8, 8) (tuple)
Orientation -> 1 (int)                                         Compression -> 1 (int)
XResolution -> 300.0 (float)                                   PhotometricInterpretation -> 2 (int)
YResolution -> 300.0 (float)                                   FillOrder -> 1 (int)
ResolutionUnit -> 2 (int)                                      ImageDescription -> "Classic View" (12)
Software -> "Ver.1.00 " (9)                                    Make -> "NIKON CORPORATION" (17)
DateTime -> 2015-07-16 20:25:28 (datetime.datetime)            Model -> "NIKON D5300" (11)
YCbCrPositioning -> 1 (int)                                    StripOffsets -> 901 (int)
Exif IFD -> 222 (int)                                          Orientation -> 1 (int)
GPS IFD -> 4060 (int)                                          SamplesPerPixel -> 4 (int)
ExposureTime -> 0.0025 (float)                                 RowsPerStrip -> 1 (int)
FNumber -> 10.0 (float)                                        StripByteCounts -> 4 (int)
ExposureProgram -> 0 (int)                                     XResolution -> 100.0 (float)
ISOSpeedRatings -> 200 (int)                                   YResolution -> 100.0 (float)
ExifVersion -> b'0230' (4 bytes)                               PlanarConfiguration -> 1 (int)
DateTimeOriginal -> 2015-07-16 15:38:54 (datetime.datetime)    ResolutionUnit -> 2 (int)
DateTimeDigitized -> 2015-07-16 15:38:54 (datetime.datetime)   Software -> "Ver.1.00 " (9)
ComponentsConfiguration -> b'\x01\x02\x03\x00' (4 bytes)       DateTime -> 2015-07-16 20:25:28 (datetime.datetime)
CompressedBitsPerPixel -> 2.0 (float)                          ExtraSamples -> 1 (int)
ExposureBiasValue -> (0, 6) (tuple)                            SampleFormat -> (1, 1, 1, 1) (tuple)
MaxApertureValue -> 4.3 (float)                                XMP -> (60, 120, 58, 120, 109, 112, 109, 101, 116, 97, ... (tuple)
MeteringMode -> 5 (int)                                        IPTC -> b'\x1c\x01Z\x00\x03\x1b%G\x1c\x02\x00\x00\x02\x0... (80 bytes)
LightSource -> 0 (int)                                         Exif IFD -> 8 (int)
Flash -> 16 (int)                                              ICC Profile -> b'\x00\x00\x0cHLino\x02\x10\x00\x00mntrRGB XYZ \... (3144 bytes)
FocalLength -> 44.0 (float)                                    GPS IFD -> 655 (int)
MakerNote -> b'Nikon\x00\x02\x11\x00\x00II*\x... (3152 bytes)  ExposureTime -> 0.0025 (float)
UserComment -> "                                    " (36)     FNumber -> 10.0 (float)
SubsecTime -> "00" (2)                                         ExposureProgram -> 0 (int)
SubsecTimeOriginal -> "00" (2)                                 ISOSpeedRatings -> 200 (int)
SubsecTimeDigitized -> "00" (2)                                ExifVersion -> b'0230' (4 bytes)
FlashpixVersion -> b'0100' (4 bytes)                           DateTimeOriginal -> 2015-07-16 15:38:54 (datetime.datetime)
ColorSpace -> 1 (int)                                          DateTimeDigitized -> 2015-07-16 15:38:54 (datetime.datetime)
PixelXDimension -> 6000 (int)                                  ComponentsConfiguration -> b'\x01\x02\x03\x00' (4 bytes)
PixelYDimension -> 4000 (int)                                  CompressedBitsPerPixel -> 2.0 (float)
Interoperability IFD -> 4306 (int)                             ExposureBiasValue -> (0, 1) (tuple)
SensingMethod -> 2 (int)                                       MaxApertureValue -> 4.3 (float)
FileSource -> b'\x03' (1 bytes)                                MeteringMode -> 5 (int)
SceneType -> b'\x01' (1 bytes)                                 LightSource -> 0 (int)
CFAPattern -> b'\x02\x00\x02\x00\x00\x01\x01\x02' (8 bytes)    Flash -> 16 (int)
CustomRendered -> 0 (int)                                      FocalLength -> 44.0 (float)
ExposureMode -> 0 (int)                                        UserComment -> "                                    " (36)
WhiteBalance -> 0 (int)                                        SubsecTime -> "00" (2)
DigitalZoomRatio -> 1.0 (float)                                SubsecTimeOriginal -> "00" (2)
FocalLengthIn35mmFilm -> 66 (int)                              SubsecTimeDigitized -> "00" (2)
SceneCaptureType -> 0 (int)                                    FlashpixVersion -> b'0100' (4 bytes)
GainControl -> 0 (int)                                         ColorSpace -> 1 (int)
Contrast -> 0 (int)                                            PixelXDimension -> 1 (int)
Saturation -> 0 (int)                                          PixelYDimension -> 1 (int)
Sharpness -> 0 (int)                                           SensingMethod -> 2 (int)
SubjectDistanceRange -> 0 (int)                                FileSource -> b'\x03' (1 bytes)
ImageUniqueID -> "090caaf2c085f3e102513b24750041aa" (32)       SceneType -> b'\x01' (1 bytes)
InteropIndex -> "R98" (3)                                      CustomRendered -> 0 (int)
InteropVersion -> b'0100' (4 bytes)                            ExposureMode -> 0 (int)
GPSLatitudeRef -> 1 (int)                                      WhiteBalance -> 0 (int)
GPSLatitude -> 51.17828166666666 (float)                       DigitalZoomRatio -> 1.0 (float)
GPSLongitudeRef -> -1 (int)                                    FocalLengthIn35mmFilm -> 66 (int)
GPSLongitude -> 1.8266399999999998 (float)                     SceneCaptureType -> 0 (int)
GPSAltitudeRef -> -1 (int)                                     GainControl -> 0 (int)
GPSAltitude -> 97.0 (float)                                    Contrast -> 0 (int)
GPSTimeStamp -> 14:38:55 (datetime.time)                       Saturation -> 0 (int)
GPSSatellites -> "09" (2)                                      Sharpness -> 0 (int)
GPSMapDatum -> "WGS-84          " (16)                         SubjectDistanceRange -> 0 (int)
GPSDateStamp -> 2015-07-16 00:00:00 (datetime.datetime)        ImageUniqueID -> "090caaf2c085f3e102513b24750041aa" (32)
                                                               GPSLatitudeRef -> 1 (int)
                                                               GPSLatitude -> 51.17828055555555 (float)
                                                               GPSLongitudeRef -> -1 (int)
                                                               GPSLongitude -> 1.826638888888889 (float)
                                                               GPSAltitudeRef -> -1 (int)
                                                               GPSAltitude -> 97.0 (float)
                                                               GPSTimeStamp -> 14:38:55 (datetime.time)
                                                               GPSSatellites -> "09" (2)
                                                               GPSMapDatum -> "WGS-84          " (16)
                                                               GPSDateStamp -> 2015-07-16 00:00:00 (datetime.datetime)
```
Data's similar.  The order is different.  Good news is that the commands `$ exiv2 -pe ~/Stonehenge.jpg` and `$ exiv2 -pe ~/Stonehenge.tif`produce similar data in the same order.  We'd hope so and both codes read the same data.

[TOC](#TOC)

<div id="3">
# 3 MakerNotes

<div id="4">
# 4 Other metadata containers

Exif is the most important of the metadata containers.  However the other exist and are supported by Exiv2:


| Type | Definition | Comment |
|:---  |:----       |:------  |
| EXIF | EXchangeable Image Format | Japanese Electronic Industry Development Association Standard |
| IPTC | International Press Telecommunications Council  | Press Industry Standard |
| Xmp  | Extensible Metadata Platform | Adobe Standard |
| ICC  | International Color Consortium | Industry Consortium for Color Handling Standards |
| ImageMagick/PNG | Portable Network Graphics | Not implemented in Exiv2 |

[TOC](#TOC)

<div id="5">
# 5 Lens Recognition
[TOC](#TOC)

<div id="6">
# 6 Sample Applications
[TOC](#TOC)

<div id="7">
# 7 I/O in Exiv2
[TOC](#TOC)

<div id="8">
# 8 Exiv2 Architecture
[TOC](#TOC)

<div id="8-1">
### 8.1 Tag Names in Exiv2

The following test program is very useful for understanding tags:

```
$ taglist --help
Usage: taglist [--help]
           [--group name|
            Groups|Exif|Canon|CanonCs|CanonSi|CanonCf|Fujifilm|Minolta|Nikon1|Nikon2|Nikon3|Olympus|
            Panasonic|Pentax|Sigma|Sony|Iptc|
            dc|xmp|xmpRights|xmpMM|xmpBJ|xmpTPg|xmpDM|pdf|photoshop|crs|tiff|exif|aux|iptc|all|ALL
           ]
Print Exif tags, MakerNote tags, or Iptc datasets
```

Let me explain what the tag names mean.

Tag: Family.Group.TagName<br>
Family: Exif | Iptc | Xmp<br>
Group : There are 106 groups:

```
$ taglist Groups | wc
     106     106    1016
$ taglist Groups | grep Minolta
Minolta
MinoltaCs5D
MinoltaCs7D
MinoltaCsOld
MinoltaCsNew
SonyMinolta
$
```

TagName: Can be almost anything and depends on the Group.

```
$ taglist MinoltaCsNew
ExposureMode,	1,	0x0001,	MinoltaCsNew,	Exif.MinoltaCsNew.ExposureMode,	Long,	Exposure mode
FlashMode,	2,	0x0002,	MinoltaCsNew,	Exif.MinoltaCsNew.FlashMode,	Long,	Flash mode
...
FlashMetering,	63,	0x003f,	MinoltaCsNew,	Exif.MinoltaCsNew.FlashMetering,	Long,	Flash metering
$
```

There isn't a tag Exif.MinoltaCsNew.ISOSpeed.  There is a Exif.MinoltaCSNew.ISO

```
$ taglist all | grep ISOSpeed$         $ taglist all | grep \\.ISO$
Photo.ISOSpeed                         Casio.ISO
PanasonicRaw.ISOSpeed                  Casio2.ISO
CanonCs.ISOSpeed                       MinoltaCsOld.ISO
CanonSi.ISOSpeed                       MinoltaCsNew.ISO
Casio2.ISOSpeed                        NikonIi.ISO
MinoltaCs5D.ISOSpeed                   NikonSiD300a.ISO
MinoltaCs7D.ISOSpeed                   NikonSiD300b.ISO
Nikon1.ISOSpeed                        NikonSi02xx.ISO
Nikon2.ISOSpeed                        NikonSi01xx.ISO
Nikon3.ISOSpeed                        PentaxDng.ISO
Olympus.ISOSpeed                       Pentax.ISO
Olympus2.ISOSpeed                      Samsung2.ISO
Sony1MltCs7D.ISOSpeed                  Sony1MltCsOld.ISO
                                       Sony1MltCsNew.ISO

```

You can use the program exifvalue to look for a tag in a file.  If the tag doesn't exist in the file, it will report "value not set":

```
$ exifvalue ~/Stonehenge.jpg Exif.MinoltaCsNew.ISO
Caught Exiv2 exception 'Value not set'
$
```

If the tag is not known, it will report 'Invalid tag':

```
$ exifvalue ~/Stonehenge.jpg Exif.MinoltaCsNew.ISOSpeed
Caught Exiv2 exception 'Invalid tag name or ifdId `ISOSpeed', ifdId 37'
$
```

Is there a way to report every tag known to exiv2?  Yes.  There are 5430 known tags:

```
$ for group in $(taglist Groups); do for tag in $(taglist $group | cut -d, -f 1) ; do echo $group.$tag ; done; done
Image.ProcessingSoftware
Image.NewSubfileType
Image.SubfileType
Image.ImageWidth
...
$ for group in $(taglist Groups); do for tag in $(taglist $group | cut -d, -f 1) ; do echo $group.$tag ; done; done | wc
    5430    5430  130555
$
```

Now, let me explain why there are 106 groups.  There are about 10 camera manufacturers (Canon, Minolta, Nikon etc) and they use the tag Exif.Photo.MakerNote to store data in a myriad of different (and proprietary standards).

```
$ exifvalue ~/Stonehenge.jpg Exif.Photo.MakerNote
78 105 107 111 110 0 2 ...
```

Exiv2 has code to read/modify/write makernotes.  All achieved by reverse engineering.  References on the web site.

The MakerNote usually isn't a simple structure.  The manufacturer usually has "sub-records" for Camera Settings (Cs), AutoFocus (Af) and so on.  Additionally, t ehe format of the sub-records can evolve and change with time.  For example (as above)

```
$ taglist Groups | grep Minolta
Minolta
MinoltaCs5D
MinoltaCs7D
MinoltaCsOld
MinoltaCsNew
SonyMinolta
$
```

So, Minolta have 6 "sub-records".  Other manufacturers have more.  Let's say 10 manufacturers have an average of 10 "sub-records".  That's 100 groups.

Now to address your concern about `Exif.MinoltaCsNew.ISOSpeed`.  It will throw an exception in Exiv2 v0.27.2.  Was it defined in an earlier version of Exiv2 such as 0.21?  I don't know.

Your application code has to use exception handlers to catch these matters and determine what to do.  Without getting involved with your application code, I can't comment on your best approach to dealing with this.  There is a macro EXIV2_TEST_VERSION which enables you to have version specific code in your application.

[TOC](#TOC)

<div id="8-2">
### 8.2 Metadata Decoder

Please read: [#988](https://github.com/Exiv2/exiv2/pull/988)

This PR uses a decoder listed in TiffMappingInfo to decode Exif.Canon.AFInfo. The decoding function "manufactures" Exif tags such as Exif.Canon.AFNumPoints from the data in Exif.Canon.AFInfo. These tags must never be written to file and are removed from the metadata in exif.cpp/ExifParser::encode().

Three of the tags created (AFPointsInFocus,AFPointsSelected, AFPrimaryPoint) are bitmasks. As the camera can have up to 64 focus points, the tags are a 64 bit mask to say which points are active. The function printBitmask() reports data such as 1,2,3 or (none).

This decoding function decodeCanonAFInfo() added to TiffMappingInfo manufactures the new tags. Normally, tags are processed by the binary tag decoder and that approach was taken in branch fix981_canonAf. However, the binary tag decoder cannot deal with AFInfo because the size of some metadata arrays cannot be determined at compile time.

We should support decoding AFInfo in 0.28, however we should NOT auto-port this PR. We can avoid having to explicitly delete tags from the metadata before writing by adding a "read-only" flag to TagInfo. This would break the Exiv2 v0.27 API and has been avoided. There is an array in decodeCanonAFInfo() which lists the "manufactured" tags such as Exif.Canon.AFNumPoints. In the Exiv2 v0.28 architecture, a way might be designed to generate that data at run-time.

[TOC](#TOC)

<div id="8-3">
### 8.3 Metadata Binary Tag Decoder

Please read: [#900](https://github.com/Exiv2/exiv2/pull/900)

There is a long discussion in [#646](https://github.com/Exiv2/exiv2/pull/646) about this issue and my investigation into the how the makernotes are decoded.

### History

The tag for Nikon's AutoFocus data is 0x00b7

Nikon encode their version of tag in the first 4 bytes.  There was a 40 byte version of AutoFocus which decodes as Exif.NikonAf2.XXX.  This new version (1.01) is 84 bytes in length and decoded as Exif.NikonAf22.XXX.

The two versions (NikonAF2 and NikonAF22) are now encoded as a set with the selector in tiffimage_int.cpp

```
    extern const ArraySet nikonAf2Set[] = {
        { nikonAf21Cfg, nikonAf21Def, EXV_COUNTOF(nikonAf21Def) },
        { nikonAf22Cfg, nikonAf22Def, EXV_COUNTOF(nikonAf22Def) },
    };
```

The binary layout of the record is defined in tiff image_int.cpp.  For example, AF22 is:

```
    extern const ArrayCfg nikonAf22Cfg = {
        nikonAf22Id,      // Group for the elements
        littleEndian,     // Byte order
        ttUndefined,      // Type for array entry
        notEncrypted,     // Not encrypted
        false,            // No size element
        true,             // Write all tags
        true,            // Concatenate gaps
        { 0, ttUnsignedByte,  1 }
    };
    //! Nikon Auto Focus 22 binary array - definition
    extern const ArrayDef nikonAf22Def[] = {
        {  0, ttUndefined,     4 }, // Version
        {  4, ttUnsignedByte,  1 }, // ContrastDetectAF
        {  5, ttUnsignedByte,  1 }, // AFAreaMode
        {  6, ttUnsignedByte,  1 }, // PhaseDetectAF
        {  7, ttUnsignedByte,  1 }, // PrimaryAFPoint
        {  8, ttUnsignedByte,  7 }, // AFPointsUsed
        { 70, ttUnsignedShort, 1 }, // AFImageWidth
        { 72, ttUnsignedShort, 1 }, // AFImageHeight
        { 74, ttUnsignedShort, 1 }, // AFAreaXPosition
        { 76, ttUnsignedShort, 1 }, // AFAreaYPosition
        { 78, ttUnsignedShort, 1 }, // AFAreaWidth
        { 80, ttUnsignedShort, 1 }, // AFAreaHeight
    };
```

The two versions of the data are encoded in tiffimage_int.cpp

```
        { Tag::root, nikonAf21Id,      nikon3Id,         0x00b7    },
        { Tag::root, nikonAf22Id,      nikon3Id,         0x00b7    },
```

### Binary Selector

The code to determine which version is decoded is in tiffimage_int.cpp

```
       {    0x00b7, nikon3Id,         EXV_COMPLEX_BINARY_ARRAY(nikonAf2Set, nikonAf2Selector) },
```

When the tiffvisitor encounters 0x00b7, he calls nikonAf2Selector() to return the index of the binary array to be used.  By default it returns 0 (the existing `nikonAf21Id`).  If the tag length is 84, he returns 1 for `nikonAf21Id`

```
    int nikonAf2Selector(uint16_t tag, const byte* /*pData*/, uint32_t size, TiffComponent* const /*pRoot*/)
    {
        int result = tag == 0x00b7 ? 0 : -1 ;
        if (result > -1 && size == 84 ) {
            result = 1;
        }
        return result;
    }
```

### The decoder

```
EXV_CALL_MEMBER_FN(*this, decoderFct)(object);
```

This function understands how to decode byte-by-byte from `const ArrayDef` into the Exiv2 tag/values such as Exif.NikonAF22.AFAreaYPosition which it stores in the ExifData vector.

This is ingenious magic.  I'll revisit/edit this explanation in the next few days when I have more time to explain this with more clarity.


[TOC](#TOC)

<div id="8-4">
### 8.4 Tiff Visitor

```
#include <iostream>
#include <string>
#include <vector>

// using namespace std;

// 1.  declare Element types
class Element;
class This;
class That;
class TheOther;
typedef std::vector<Element*> Elements;
typedef Elements::iterator    Elements_it;

// 2. Create a "visitor" base class with a visit() method for every element type
class Visitor
{
public:
    virtual void visit(This *e) = 0;
    virtual void visit(That *e) = 0;
    virtual void visit(TheOther *e) = 0;
};

// 3. Add an accept(Visitor) method to the "element" hierarchy
class Element
{
public:
    virtual void accept(class Visitor &v) = 0;
};

class This: public Element
{
public:
    void accept(Visitor &v);
    std::string name() { return "This"; } ;
};

class That: public Element
{
public:
    void accept(Visitor &v);
    std::string name() { return "That"; };
};

class TheOther: public Element
{
public:
    void accept(Visitor &v);
    std::string name() { return "TheOther"; };
};


// 5. Define the Element type accept() method
void This::accept(Visitor &v)
{
    v.visit(this);
}

void That::accept(Visitor &v)
{
    v.visit(this);
}

void TheOther::accept(Visitor &v)
{
    v.visit(this);
}

// 6. Create a "visitor" derived class for each "operation" to do on "elements"
class UpVisitor: public Visitor
{
    void visit(This *e)
    {
        std::cout << "UpVisitor: " << e->name() << std::endl;
    }

    void visit(That *e)
    {
        std::cout << "UpVisitor: " <<  e->name() << std::endl;
    }
    void visit(TheOther *e)
    {
        std::cout << "UpVisitor: " << e->name() << std::endl;
    }
};

class DownVisitor: public Visitor
{
    void visit(This *e)
    {
        std::cout << "DownVisitor: " << e->name() << std::endl;
    }

    void visit(That *e)
    {
        std::cout << "DownVisitor: " <<  e->name() << std::endl;
    }

    void visit(TheOther *e)
    {
        std::cout << "DownVisitor: " << e->name() << std::endl;
    }
};


int main() {
    // create some elemenents
    Elements elements;
    elements.push_back(new This()    );
    elements.push_back(new That()    );
    elements.push_back(new TheOther());
    elements.push_back(new That()    );

    // traverse objects and visit them
    UpVisitor   upVisitor;
    for ( Elements_it it = elements.begin() ; it != elements.end() ; it++ ) {
        (*it)->accept(upVisitor);
    }

    DownVisitor downVisitor;
    for ( Elements_it it = elements.begin() ; it != elements.end() ; it++ ) {
        (*it)->accept(downVisitor);
    }

    return 0 ;
}

```

[TOC](#TOC)

<div id="9">
# 9 Test Suite and Build
[TOC](#TOC)

<div id="10">
# 10 API/ABI
[TOC](#TOC)

<div id="11">
# 11 Security
[TOC](#TOC)

<div id="12">
# 12 Project Management, Release Engineering, User Support

### 12.1) C++ Code

Exiv2 is written in C++.  Prior to v0.28, the code is written for C++ 1998 and makes considerable use of STL containers such as vector, map, set, string and many others.  The code started life as a 32-bit library on Unix and today builds well for 32 and 64 bit systems running Linux, Unix, MacOS-X and Windows (Cygwin, MinGW, and Visual Studio).  The Exiv2 project has never supported Mobile Platforms or Embedded Systems, however it should be possible to build for other platforms with a modest effort.

The code has taken a great deal of inspiration from the book [Design Patters: Elements of Reusable Object=Oriented Software](https://www.oreilly.com/library/view/design-patterns-elements/0201633612/).

Starting with Exiv2 v0.28, the code requires a C++11 Compiler.  Exiv2 v0.28 is a major refactoring of the code and provides a new API.  The project maintains a series of v0.27 "dot" releases for security updates.  These releases are intended to ease the transition of existing application to adopting the v0.28 environment.

### 12.2) Build

The build code in Exiv2 is implemented using CMake: cross platform make.  This system enables the code to be built on many different platforms in a consistant manner.  CMake recursively reads the files CMakeLists.txt in the source tree and generates build environments for different build systems.  For Exiv2, we actively support using CMake to build on Unix type plaforms (Linux, MacOSX, Cygwin and MinGW), and several editions of Visual Studio.  CMake can generate project files for Xcode and other popular IDEs.

Exiv2 has dependencies on other libraries.

### 12.3) Security

### 12.4) Documentation

### 12.5) Testing

### 12.6) Sample programs

### 12.7) User Support

### 12.8) Bug Tracking

### 12.9) Release Engineering

### 12.10) Platform Support

### 12.11) Localisation

### 12.12) Build Server

### 12.13) Source Code Management

### 12.14) Project Web Site

### 12.15) Project Servers (apache, SVN, GitHub, Redmine)

### 12.16) API Management

### 12.17) Recruiting Contributors

### 12.18) Project Management and Scheduling

### 12.19) Enhancement Requests

### 12.20) Tools

Every year brings new/different tools (cmake, git, MarkDown, C++11)

### 12.21) Licensing

### 12.22) Back-porting fixes to earlier releases

### 12.23) Other projects demanding support and changes

[TOC](#TOC)

<div id="a">
# Appendix: Home made debugging tools.

#### args.cpp
```
#include <stdio.h>
int main(int argc, char* argv[])
{
    int i = 1 ;
    while ( i < argc ) {
        printf("%-2d: %s\n",i,argv[i]) ;
        i++;
    }
    return 0 ;
}
```

#### dmpf.cpp

```
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

bool printable(unsigned char c) { return c >= 32 && c < 127 && c != '.' ; }

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
		char buff[16]   ;
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
		        l += sprintf(line+l,"%c", printable(c) ? c : '.' ) ;
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
```


Robin Mills

Revised: 2019-09-04
