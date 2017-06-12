#!/usr/bin/python
from __future__ import print_function

r"""depends.py - find the ordered dependancies of a library/executable (deepest first)

To use this:
	depends.py [options]+ <pathtolibrary/pathtoexecutable/pathotoapp>

"""
__author__	= "Robin Mills <robin@clanmills.com>"

import string
import os
import sys
import optparse
import subprocess
from sets import Set

global version, options
version="2017-06-10"

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

##
# recursively update dependdict for libname (which can be @executable_path etc)
# execdir is the directory of @executable_path
# loaddir is the directory of @loader_path
# returns path to libname or '' (empty string)
def depend(execdir,loaddir,libname,dependdict):
	""" recursive descent of the libraries """

	global options

	##
	# discover libpath for libname
	epath='@executable_path/'
	lpath='@loader_path/'
	rpath='@rpath/'
	libpath=libname
	if libname[:len(epath)] == epath:
		libpath=os.path.join(execdir,libname[len(epath):])
	if libname[:len(lpath)] == lpath:
		libpath=os.path.join(loaddir,libname[len(lpath):])
	if libname[:len(rpath)] == rpath:
		libpath=os.path.join(loaddir,libname[len(rpath):])
	libpath=os.path.abspath(libpath)

	if os.path.isfile(libpath):
		if not dependdict.has_key(libpath):
			dependdict[libpath]=Set([])                 # push now to prevent infinite loop in recursion
			cmd = 'otool -L "%s"' % libpath         	# otool -L prints library dependancies:
														# libpath:
														# <tab>dependency (metadata) ...
			if options.verbose:
				print(cmd)
			for line in os.popen(cmd).readlines():      # run cmd
				if line[:1]=='\t':                      # parse <tab>dependency (metadata)
					dependency=line.split()[0]
					# print libpath,' => ',dependency
					# recurse to find dependancies of dependency
					dpath=depend(execdir,loaddir,dependency,dependdict)
					dependdict[libpath].add(dpath)      # update dependdict from recursion
	else:
		print("depend: execdir=%s, loaddir=%s, libname=%s" % (execdir,loaddir,libname))
		print("*** error NOT a FILE libname=%s libpath=%s ***" % (libname,libpath))
		libpath=''

	return libpath

def execute(cmd):
	global options
	if options.verbose:
		print('$ ' + cmd)
	if not options.dryrun:
		n=subprocess.call(cmd, shell=True)
		if n!=0:
			exit(n)

def deploy(execdir,loaddir,libname):
	""" modify the install_names in a library """

	global options

	##
	# discover libpath for libname
	libpath=os.path.abspath(libname)

	if os.path.isfile(libpath):
		cmd = 'otool -L "%s"' % libpath         	# otool -L prints library dependancies:
													# libpath:
													# <tab>dependency (metadata) ...
		for line in os.popen(cmd).readlines():      # run cmd
			if line[:1]=='\t':                      # parse <tab>dependency (metadata)
				dependency=line.split()[0]
				skip=False
				for i in ['@executable_path/', '@loader_path/', '/Library/', '/System/', '/usr/lib/']:
					skip = skip or dependency[:len(i)]==i;
				if not skip:
					stub=os.path.basename(dependency)
					rpath='@rpath/'
					if dependency[:len(rpath)] == rpath:
						stub=dependency[len(rpath):] # '@rpath/QtConcurrent.framework/Versions/5/QtConcurrent'
					cmd="install_name_tool -change '%s' '%s' '%s'" % (dependency,'@loader_path/../Frameworks/'+stub,libname)
					execute(cmd)
					# copy Qt Framework if necessary
					if stub.find('.framework') >=0:
						framework=stub[:stub.find('.framework')]
						Framework = '%s/%s.framework' % (loaddir,framework)
						framework = '%s/%s.framework' % (options.qt,framework)
						if not os.path.isdir(Framework):
							if os.path.isdir(framework):
								execute("ditto '%s' '%s'" % (framework,Framework) )
							else:
								sprint('*** error %s not available' % framework)
								exit(1)
					else:
						execute("ditto '%s' '%s'" % (dependency,loaddir+'/'+stub))

	else:
		print("deploy: execdir=%s, loaddir=%s, libname=%s" % (execdir,loaddir,libname))
		print("*** error NOT a FILE libname=%s libpath=%s ***" % (libname,libpath))

##
#
def tsort(dependdict): # returns (ordered) array of libraries
	filename='/tmp/tsortXX.txt'
	file=open(filename,'w')
	for key in dependdict.keys():
		depends=dependdict[key]
		for d in depends:
			if len(d) > 0:
				D=d.replace(' ','*')          # replace ' ' with '*'
				K=key.replace(' ','*')
				file.write( '%s %s\n' % (K,D) )
	file.close()
	cmd='tsort "%s" 2>/dev/null' % (filename) # tsort breaks on ' '
	lines=os.popen(cmd).readlines()
	lines.reverse()

	result=[]
	for line in lines:
		line=line[:-1].replace('*',' ')       # unreplace '*' with ' '
		result.append(line)
	return result;
#
##

##
#
def main():
	global options
	##
	# set up argument parser
	usage = "usage: %prog [options]+ target"
	parser = optparse.OptionParser(usage)

	parser.add_option('-V', '--version'		, action='store_true' , dest='version' ,help='report version'     )
	parser.add_option('-v', '--verbose'		, action='store_true' , dest='verbose' ,help='verbose output'	  )
	parser.add_option('-d', '--deploy'		, action='store_true' , dest='deploy'  ,help='update the bundle'  )
	parser.add_option('-D', '--depends'	    , action='store_true' , dest='depends' ,help='report dependancies')
	parser.add_option('-X', '--dryrun'	    , action='store_true' , dest='dryrun'  ,help='do not execute commands')
	parser.add_option('-q', '--qt'	        , action='store'      , dest='qt'      ,help='qt directory'       ,default = os.environ['HOME']+'/Qt/5.9/clang_64/lib')

	##
	# parse and test for errors
	(options, args) = parser.parse_args()

	if options.version:
		global version
		print('version: %s' % (version))
		exit(0)

	if len(args) != 1:
		parser.print_help()
		return

	if not options.depends and not options.deploy:
		options.depends=True
	options.verbose = options.verbose or options.dryrun

	##
	# dependdict key is a library path.  Value is a set of dependant library paths
	libname      = args[0]
	execname     = ''
	if os.path.isdir(libname):
		execname = os.path.basename(os.path.abspath(libname)).strip('.app');
		execname = os.path.join(libname,'Contents/MacOS/',execname)
		if os.path.isfile(execname):
			libname=execname
	execdir      = os.path.abspath(os.path.join(libname,'..'))
	loaddir      = execdir if execname == '' else os.path.abspath(os.path.join(execdir,'../Frameworks/'))
	if not os.path.isdir(loaddir):
		os.mkdir(loaddir)

	options.qt   = os.path.abspath(options.qt)

	if options.verbose:
		print('libname = %s execdir = %s, loaddir = %s' % (libname,execdir,loaddir) )

	##
	# --deploy:
	if options.deploy:
		deploy(execdir,loaddir,libname)
		print(options.qt)

	##
	# --depends: recursively build dependdict, tsort and report
	if options.depends:
		dependdict   = {}  # dependdict['/usr/lib/foo.dylib'] = Set([ '/usr/lib/a.dylib', ... ])
		depend(execdir,loaddir,libname,dependdict)
		results=tsort(dependdict)
		for result in results:
			print(result)
#
##

if __name__ == '__main__':
	main()

# That's all Folks!
##
