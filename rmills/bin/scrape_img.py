#!/usr/bin/env python3
import sys
from   bs4 import BeautifulSoup as BSHTML
import ssl
import urllib.request

# parse command-line
if len(sys.argv) != 2:
    print('Number of arguments:', len(sys.argv), 'arguments.')
    print('Argument List:', str(sys.argv))
    print('usage: ' + sys.argv[0] + ' url')
    sys.exit(1)
url = sys.argv[1]

# Read the page from the url
ctxt = ssl._create_unverified_context()
page = urllib.request.urlopen(url, context=ctxt)
soup = BSHTML(page,features='lxml')

# find and report every image
images = soup.findAll('img')
for image in images:
	url=image['src']
	if len(url) > 120:  # ignore short urls, they are icons
		url = url.split('=')[0]+'=w1920-h1080';
		print('<object data="'+url+'"></object>')
