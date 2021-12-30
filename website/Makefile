TABLES = Exif \
         Canon \
         CanonCs \
         CanonSi \
         CanonCf \
         CanonPi \
         CanonFi \
         CanonPa \
         Casio \
         Casio2 \
         Fujifilm \
         Minolta \
         MinoltaCsNew \
         MinoltaCs5D \
         MinoltaCs7D \
         Nikon1 \
         Nikon2 \
         Nikon3 \
         NikonVr \
         NikonPc \
         NikonWt \
         NikonIi \
         NikonAf \
         NikonAf2 \
         NikonAFT \
         NikonFi \
         NikonMe \
         NikonFl1 \
         NikonFl2 \
         NikonFl3 \
         NikonSiD80 \
         NikonSiD40 \
         NikonSiD300a \
         NikonSiD300b \
         NikonSi01xx \
         NikonCb1 \
         NikonCb2 \
         NikonCb2a \
         NikonCb2b \
         NikonCb3 \
         NikonCb4 \
         NikonLd1 \
         NikonLd2 \
         NikonLd3 \
         Olympus \
         OlympusCs \
         OlympusEq \
         OlympusRd \
         OlympusRd2 \
         OlympusIp \
         OlympusFi \
         OlympusFe1 \
         OlympusRi \
	     Panasonic \
         PanasonicRaw \
         Pentax \
         Samsung2 \
         Sigma \
         Sony1 \
         SonyMinolta \
         Sony1Cs \
         Sony1Cs2 \
         Sony1MltCs7D \
         Sony1MltCsOld \
         Sony1MltCsA100

SCHEMA = dc \
         xmp \
         xmpRights \
         xmpMM \
         xmpBJ \
         xmpTPg \
         xmpDM \
         pdf \
         photoshop \
         crs \
         crss \
         tiff \
         exif \
         exifEX \
         aux \
         iptc \
         iptcExt \
         plus \
         mwg-rs \
         mwg-kw \
         dwc \
         dcterms \
         digiKam \
         kipi \
         GPano \
         lr \
         MP \
         MPRI \
         MPReg \
         acdsee \
         mediapro \
         expressionmedia \
         MicrosoftPhoto

all: check-environment html doc

.PHONY: html doc news clean update check-environment \
        update-intro update-sample update-conversion update-changelog update-tags update-makernotes update-manpage \

check-environment:
ifndef EXIV2HOME
  $(error EXIV2HOME is undefined.  It should be something like /home/rmills/github/exiv2/exiv2)
endif
ifndef EXIV2WEB
  $(error EXIV2WEB is undefined.  It should be something like /home/rmills/exiv2/team/website)
endif
ifeq ("$(wildcard $(EXIV2HOME)/doc/templates/Exif.xml)","")
  $(error $(EXIV2HOME)/doc/templates/Exif.xml does not exist)
endif
ifeq ("$(wildcard $(EXIV2HOME)/build/bin/exiv2)","")
  $(error $(EXIV2HOME)/build/bin/exiv2 does not exist)
endif
ifeq ("$(wildcard $(EXIV2WEB)/bin/rssdate)","")
  $(error $(EXIV2WEB)/bin/rssdate does not exist)
endif

html: news
	@mkdir -p         html/releases
	@cp ../releases/* html/releases
	@bin/download.sh
	@bin/archive.sh
	@bin/gen.py master/*.in
	@echo "Moving files to html/ ..."
	-@mv -f master/*.html html/
	-@mv -f master/*.jpg  html/

news: var/__whatsnew-latest__ var/__whatsnew-table__ html/rss.xml

doc: html
	@echo "Generating API documentation ... "
        # Preprocessing
	@bin/gen.py doxygen/*.in
	@sed -e 's,="dist/,="../dist/,' \
             -e 's,="include/,="../include/,' \
	    doxygen/exiv2-header.html > ttt
	@mv -f ttt doxygen/exiv2-header.html
	@sed -e 's,="dist/,="../dist/,' \
             -e 's,="include/,="../include/,' \
	    doxygen/exiv2-footer.html > ttt
	@mv -f ttt doxygen/exiv2-footer.html
        # API doc generation
        # diff -u oldfile newfile > a.patch
	# @cp $(EXIV2HOME)/cmake/Doxyfile.in doxygen/Doxyfile
	# @patch  doxygen/Doxyfile doxygen/Doxyfile.patch
	doxygen doxygen/Doxyfile.v27
        # Postprocessing
	-@cp -f doxygen/doxy-boot.js html/doc/
        # Cleanup
	-@rm -f html/doc/namespacemembers*.html
	-@rm -f html/doc/functions*.html

var/__whatsnew-table__: master/news.xml
	@echo "Generating news table ..."
	@xsltproc master/whatsnew-table.xsl $< > $@

var/__whatsnew-latest__: master/news.xml
	@echo "Generating latest news ..."
	@xsltproc master/whatsnew-latest.xsl $< > $@

html/rss.xml: master/news.xml
	@echo "Generating RSS feed ..."
	@xsltproc master/whatsnew-rss.xsl $< \
	| awk '/pubDate/ { gsub("<[/]?pubDate>", ""); printf "      <pubDate>"; system("bin/rssdate " $$0); printf "</pubDate>\n"; next } /__lastBuildDate__/ { printf "    <lastBuildDate>"; system("date -R"); printf "    </lastBuildDate>\n"; next; } {print}' > $@


# Run bin/release_info.sh to update release related variables (incl __version__)
update: update-manpage update-intro update-sample update-conversion update-changelog update-tags update-makernotes

update-manpage:
	@/usr/bin/env "MANPATH=$${EXIV2HOME}/man:$$MANPATH" man -t exiv2 | ps2pdf - html/exiv2-manpage.pdf
	@man2html              $${EXIV2HOME}/man/man1/exiv2.1  | \
	sed -e s'_http://localhost/cgi-bin/man/man2html_/_g'     \
	    -e s'_Manpage of EXIV2</TITLE>_<HTML>Manpage of EXIV2</TITLE><LINK href="manpage.css" REL="stylesheet" type="text/css">_' \
	> var/__manpage__

update-intro:
# commented for now, until the doc/ intro is updated accordingly
#	@cp -f ../doc/templates/__intro__ doxygen/draft-intro.html
#	@sed -e 's,="makernote.html,="../makernote.html,' \
#	    doxygen/draft-intro.html > doxygen/intro.html
#	@rm -f doxygen/draft-intro.html

update-sample:
	@./bin/sample.sh

update-conversion:
	@./bin/conversion.sh

update-changelog:
	@echo -------------------------------------------------------------
	@echo '***' manually update Changelogs, releasenotes and whatsnew '***'
	@ls -l $(EXIV2WEB)/var/__ChangeLog__ $(EXIV2WEB)/var/__whatsnew-table__
	@ls -l $(EXIV2HOME)/doc/ChangeLog $(EXIV2HOME)/releasenotes/releasenotes.txt
	@echo '***' changelogs can be updated using team/contrib/redmine/progress.py '***'
	@echo ------------------------------------------------------------------------

update-makernotes:
	@cp -f ${EXIV2HOME}/doc/templates/__makernotes__ var/__makernotes__

$(TABLES):
	@cp -f ${EXIV2HOME}/doc/templates/$@.xml var/
	@xsltproc var/exif.xsl var/$@.xml > var/__$@__

Iptc:
	@cp -f ${EXIV2HOME}/doc/templates/$@.xml var/
	@xsltproc var/iptc.xsl var/$@.xml > var/__$@__

$(SCHEMA):
	@cp -f ${EXIV2HOME}/doc/templates/xmp_$@.xml var/
	@xsltproc var/xmp.xsl var/xmp_$@.xml > var/__xmp_$@__

update-tags: $(TABLES) Iptc $(SCHEMA)

clean:
	@rm -vf html/*.html
	@rm -vf html/rss.xml
	@rm -rf html/doc
	@rm -vf var/__whatsnew-latest__
	@rm -vf var/__whatsnew-table__
	@rm -vf doxygen/mainpage.hpp
	@rm -vf doxygen/exiv2-header.html
	@rm -vf doxygen/exiv2-footer.html

get-latest:
	@./bin/getLatest.sh

publish:
	ditto  html ~/Jenkins/website/html
