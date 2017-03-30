#!/bin/bash

header() {
	cat <<-EOH
<!DOCTYPE html>
<html >

<head>
	<meta charset="UTF-8">
	<link rel="shortcut icon" type="image/x-icon" href="https://production-assets.codepen.io/assets/favicon/favicon-8ea04875e70c4b0bb41da869e81236e54394d63638a1ef12fa558a4a835f1164.ico" />
	<link rel="mask-icon" type="" href="https://production-assets.codepen.io/assets/favicon/logo-pin-f2d2b6d2c61838f7e76325261b7195c27224080bc099486ddd6dccb469b8e8e6.svg" color="#111" />
	<title>Gallery</title>
	<link rel='stylesheet prefetch' href='/js/PhotoSwipe/dist/photoswipe.css'>
	<link rel='stylesheet prefetch' href='/js/PhotoSwipe/dist/default-skin/default-skin.css'>

	<style>
		.my-gallery {
		  width: 100%;
		  float: left;
		}
		.my-gallery img {
		  width: 100%;
		  height: auto;
		}
		.my-gallery figure {
		  display: block;
		  float: left;
		  margin: 0 5px 5px 0;
		  width: 150px;
		}
		.my-gallery figcaption {
		  display: none;
		}
	</style>

	<script>
		window.console = window.console || function(t) {};
	</script>

	<script>
		if (document.location.search.match(/type=embed/gi)) {
			window.parent.postMessage("resize", "*");
		}
	</script>
</head>
EOH
}

footer() {
	cat <<-EOF
<div class="pswp" tabindex="-1" role="dialog" aria-hidden="true">

    <!-- Background of PhotoSwipe.
         It is a separate element, as animating opacity is faster than rgba().
    -->
    <div class="pswp__bg"></div>

    <!-- Slides wrapper with overflow:hidden. -->
    <div class="pswp__scroll-wrap">

        <!-- Container that holds slides. PhotoSwipe keeps only 3 slides in DOM to save memory. -->
        <!-- do not modify these 3 pswp__item elements, data is added later on. -->
        <div class="pswp__container">
            <div class="pswp__item"></div>
            <div class="pswp__item"></div>
            <div class="pswp__item"></div>
        </div>

        <!-- Default (PhotoSwipeUI_Default) interface on top of sliding area. Can be changed. -->
        <div class="pswp__ui pswp__ui--hidden">

            <div class="pswp__top-bar">

                <!--  Controls are self-explanatory. Order can be changed. -->

                <div class="pswp__counter"></div>
                <button class="pswp__button pswp__button--close" title="Close (Esc)"></button>
                <button class="pswp__button pswp__button--share" title="Share"></button>
                <button class="pswp__button pswp__button--fs" title="Toggle fullscreen"></button>
                <button class="pswp__button pswp__button--zoom" title="Zoom in/out"></button>

                <!-- Preloader demo http://codepen.io/dimsemenov/pen/yyBWoR -->
                <!-- element will get class pswp__preloader__active when preloader is running -->
                <div class="pswp__preloader">
                    <div class="pswp__preloader__icn">
                      <div class="pswp__preloader__cut">
                        <div class="pswp__preloader__donut"></div>
                      </div>
                    </div>
                </div>
            </div>

            <div class="pswp__share-modal pswp__share-modal--hidden pswp__single-tap">
                <div class="pswp__share-tooltip"></div>
            </div>

            <button class="pswp__button pswp__button--arrow--left" title="Previous (arrow left)"></button>
            <button class="pswp__button pswp__button--arrow--right" title="Next (arrow right)"></button>

            <div class="pswp__caption">
                <div class="pswp__caption__center"></div>
            </div>
          </div>
        </div>
</div>

<script src='/js/PhotoSwipe/dist/photoswipe.js'></script>
<script src='/js/PhotoSwipe/dist/photoswipe-ui-default.js'></script>
<script src='/js/PhotoSwipe/dist/photoswipe-init.js'></script>
<script>initPhotoSwipeFromDOM('.my-gallery');</script>
EOF
}

(
	header
	echo '<body translate="no" style="background-image: url('/tartan.gif');margin-left:150px;">'
	echo '<div align="right"><a href="."><img src="/js/PhotoSwipe/dist/default-skin/close.png"></a></div>'
	echo '<div class="my-gallery" itemscope itemtype="http://schema.org/ImageGallery">'
	# exiv2 -pa --grep DateTimeOriginal *.jpg *.JPG | sort -k 5

	pi=Plates
	if [ ! -e $pi ]; then pi=Images ; fi

	for I in $(find $pi -iname "*.jpg" -maxdepth 1); do
		i=$(basename "$I")
		S=$(pathname "$I")
		s=$(echo $S | sed -e 's/.*clanmills//g' | sed -e "s/$pi/Thumbs/")
		S=$(echo $S | sed -e 's/.*clanmills//g')
		w=$(sips -g pixelWidth  "$I" | tail -1 | cut -d: -f 2- | sed -e 's/ //g')
		h=$(sips -g pixelHeight "$I" | tail -1 | cut -d: -f 2- | sed -e 's/ //g')
		echo '<figure itemprop="associatedMedia" itemscope itemtype="http://schema.org/ImageObject">'
		echo '  <a href="'$S'" itemprop="contentUrl" data-size="'${w}x${h}'">'
		echo '    <figcaption><h3>'$i'</h3><p>'$i'</p></figcaption>'
		echo '    <image src="'$s'"/>'
		echo '  </a>'
		echo '  <figcaption itemprop="caption description">'$i'</figcaption>'
		echo '</figure>'
	done
	echo '</div>'
	footer
	echo '</body>'
    echo '</html>'

) > gallery.html


# That's all Folks!
##





