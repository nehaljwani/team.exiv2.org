#!/usr/bin/env python

from __future__ import division

import json
import os
import sys

global J,args,options,unexpected

##
# Help:
def reportHelp():
    global J,args,options
    print('syntax: %s [options]+' % (sys.argv[0]) )
    print 'options: %s' % options.keys()

    quit()

##
# read the json into an array of json objects
# see ./getdata.sh for an explanation of why the data is sliced into smaller files
def readData():
    global J,args,options
    for i in range(0,9):
        if not os.path.exists('data%d.json' % i):
            options['getdata'] = True

    if options['getdata']:
        os.system('./getdata.sh')

    J=[]
    for i in range(0,9):
        data=open('data%d.json' % i,'r').read();
        if len(data) > 100:
            J.append(json.loads( data ))

##
# Bugs
def reportBugs():
    global J,args,options
    bugs0='| Issue       | Done | Size | Left | Assigned | Description |'
    bugs1='| #%-4d       | %3d%% | %4d | %4d | %12s | %s | '

    if len(args):
        print( bugs0 )
        for j in J:
            issues=j['issues']
            for i in issues:
                try:
                    for bug in args:
                        if str(i['id']) == bug :
                            size = 0
                            try:
                                size = i['estimated_hours']
                            except:
                                pass
                            done = i['done_ratio'] * size / 100.0
                            left = size - done
                            assigned = i['assigned_to']['name'];
                            print(bugs1 % (i['id'],i['done_ratio'],size,left,assigned,i['subject']) )
                except:
                    pass

##
#
def printHeader(t):
    print(t)
    print('='*len(t))

##
# Features:
def reportFeatures():
    global J,args,options

    printHeader('Features')
    features0='| _Priority_ | _Issue_  | _Effort_ | _Status_ | _Done_ | _Description_ |'
    features1='|=. %3d | #%-4d  |>.  %4d |=. %3d%% |>. %-5.2f | %s |'
    features2='|\\2>. |>.  %4d |=. %3d%% |>. %-5.2f | %s |'
    features3='|\\2>. *Total:*  |>.   %3d |=. %3d%% |>. %-5.2f | |'
    if options['console']:
        features0='| Priority | Issue  | Effort | Status |  Done | Description |'
        features1='|      %3d | #%-4d  |   %4d |   %3d%% | %5.2f | %s |'
        features2='|          |        |   %4d |   %3d%% | %5.2f | %s |'
        features3='| Total:            |   %4d |   %3d%% | %5.2f | |'

    Left=0
    Size=0
    Minor=0
    print(features0)

    Features = [ { 'id': 1168, 'effort':5 }
               , { 'id': 1041, 'effort':5 }
               , { 'id': 1111, 'effort':3 }
               , { 'id': 1109, 'effort':3 }
               , { 'id': 1074 ,'effort':3 }
               , { 'id': 1034 ,'effort':3 }
               , { 'id': 1108, 'effort':2 }
               , { 'id': 1199 ,'effort':2 }
               , { 'id': 1236 ,'effort':2 }
               , { 'id': 1057 ,'effort':1 }
               , { 'id': 1177 ,'effort':1 }
               , { 'id': 1187 ,'effort':1 }
               , { 'id': 1193 ,'effort':1 }
               , { 'id': 1190 ,'effort':1 }
               , { 'id': 1243 ,'effort':1 }
               ]
    Done=0
    Effort=0
    priority=0
    for F in Features:
        for j in J:
            issues=j['issues']
            for i in issues:
                if i['id'] == F['id']:
                    try:
                        effort = F['effort']
                        status = i['done_ratio']
                        done   = status*effort
                        Done   = Done+done
                        Effort = Effort+effort
                        priority=priority+1
                        print(features1 % (priority,i['id'],effort,status,done/100,i['subject']) )
                    except:
                        pass

    effort=4
    Status=(Done*100 / Effort)/100
    print(features3 % (Effort, Status,Done/100) )
    print('')

##
# Progress
def reportProgress():
    printHeader('Progress')
    global J,args,options
    progress0='| v1.0 | Review/v0.27 | 0.26 | closed | open | resolved | left | progress | unassigned |'
    progress1='|>.%3d |>.  %3d/%2d |>.%3d |>.  %3d |>.%3d |>.    %3d |>.%3d |=.   %3.0f%% |>. %3d / %3d%% |'
    if options['console']:
        progress0='| v1.0 | Review/0.27 | 0.26 | closed | open | resolved | left | progress | unassigned |'
        progress1='|  %3d |     %3d/%3d |  %3d |    %3d |  %3d |      %3d |  %3d |    %3.0f%%  | %3d / %3d%% |'

    v0_26=0
    v0_27=0
    vReview=0
    v1_0=0
    open=0
    resolved=0
    closed=0
    unassigned=0

    for j in J:
        issues=j['issues']
        for i in issues:
            try:
                if    i['fixed_version']['name']=='1.0':
                    v1_0=v1_0+1
                elif  i['fixed_version']['name']=='Review':
                    vReview=vReview+1
                elif  i['fixed_version']['name']=='0.27':
                    v0_27=v0_27+1
                elif  i['fixed_version']['name']=='0.26':
                    v0_26=v0_26+1
                    if i['status']['name'] == 'Closed':
                        closed=closed+1
                    elif i['done_ratio'] >= 80:
                        resolved=resolved+1
                    else:
                        open=open+1
                    try:
                        if i['assigned_to']['name']=='':
                            assigned=1
                    except:
                        unassigned=unassigned+1
            except:
                pass

    progress= 100 - 100*open/v0_26
    print(progress0)
    print(progress1 % (v1_0,vReview,v0_27,v0_26,closed,open+resolved,resolved,open,progress,unassigned,unassigned*100/v0_26) )
    print('')

##
# Robin's to do list
def reportTodo():
    printHeader("Robin's todo list");
    global J,args,options,unexpected

    todo0='| _Issue_     | _Done_ | _Size_ | _Left_ | _Description_ |'
    todo1='|\\2>. Other Minor Issues       | |>.%4d ||'
    todo2='|>. #%-4d       |>. %3d%% |>. %3d |>. %3d | %s |'
    todo3='|\\2>. Left             |>.%4d |>.%4d ||'
    todo4='|\\3>. Unexpected %d%%           |>. %3d ||'
    todo5='|\\3>. Review+1.0               |>. %3d ||'
    todo6='|\\3>. Total                    |>. %3d | %2d weeks |'
    if options['console']:
        todo0='| Issue       | Done | Size | Left | Description |'
        todo1='| Minor Issues              | %4d | ||'
        todo2='| #%-4d       | %3d%% | %4d | %4d | %s |'
        todo3='| Left               | %4d | %4d | |'
        todo4='| Unexpected %d%%            | %4d | |'
        todo5='| Review+1.0                | %4d | |'
        todo6='| Total                     | %4d | %2d weeks |'

    print( todo0 )
    Left=0
    Size=0
    Minor=0

    for j in J:
        issues=j['issues']
        for i in issues:
            try:
                if   i['assigned_to']['name']=='Robin Mills':
                 if  i['fixed_version']['name']=='0.26':
                    size = i['estimated_hours']
                    done = i['done_ratio'] * size / 100.0
                    left = size - done
                    if left > 0:
                        Left = Left+left
                        Size = Size+size
                        print(todo2 % (i['id'],i['done_ratio'],size,left,i['subject']) )
                    else:
                        Minor = Minor + left
            except:
                pass

    Left=Left+Minor
    Unexpected=unexpected * Left / 100;
    if Minor > 0:
    	print(todo1 % (Minor) )
    if Left > 0:
    	print(todo3 % (Size,Left) )
    if Unexpected > 0.0:
    	print(todo4 % (unexpected,Unexpected))
    Total=Left+Unexpected
    print(todo6 % (Total,(Total+30)/40))
    print('')

##
# Robin's to do list
def reportEngineers():
    printHeader("Engineers");
    global J,args,options,unexpected

    engineers0='| _Engineer_                  |>. _Issues_|>. _Hours_|'
    engineers1='| %-26s  |>. %6d  |>. %5d  |'
    engineers2='| *%s*                     |>.  *%d*  |>. *%d* |'
    if options['console']:
        engineers0='| Engineer                   | Issues | Hours |'
        engineers1='| %-26s | %6d | %5d |'
        engineers2='| %-26s | %6d | %5d |'

    total='Total'
    engineers={total : 0 }
    effort   ={total : 0 }

    for j in J:
        issues=j['issues']
        for i in issues:
            try:
                engineer=i['assigned_to']['name']
                if not engineers.has_key(engineer):
                    engineers[engineer]=0
                    effort   [engineer]=0
                if i['fixed_version']['name'] == '0.26':
                    effort[engineer]=effort[engineer]+1
                    done=0
                    try:
                    	size = i['estimated_hours']
                    	done = i['done_ratio'] * size / 100.0
                    except:
                        pass
                    engineers[engineer]=engineers[engineer]+done
                    engineers[total]=engineers[total]+done
                    effort   [total]=effort[total]+1
            except:
                pass

    print engineers0
    guys=engineers.keys()
    guys.sort()
    for engineer in guys:
        if not engineer == total:
            if not engineers[engineer]==0 and not effort[engineer]==0:
                print engineers1 % (engineer,effort[engineer],engineers[engineer])
    print engineers2 % (total,effort[total],engineers[total])

##
# parse command-line
options={}
options['console'   ]=False
options['bugs'      ]=False
options['todo'      ]=False
options['features'  ]=False
options['engineers' ]=False
options['progress'  ]=False
options['bugs'      ]=False
options['getdata'   ]=False
options['getdata.sh']=False
options['all'       ]=False

actions= ['todo','features','progress','bugs','engineers']

args=sys.argv[1:]

if len(args)==0:
    reportHelp()

if len(args)>0:
    cmd=args[0]
    while cmd in options.keys():
        options[cmd]=True
        args=args[1:]
        cmd=args[0] if len(args)>0 else ''

if len(args)>0 and not options['bugs']:
    print('unknown argument %s' % args[0])
    exit(1)

options['getdata'] = options['getdata'] or options['getdata.sh']
if not options['all']:
    bAll=True
    for action in actions:
        if options[action]:
            bAll=False
    options['all']=bAll

if options['all']:
    options['features' ]=True
    options['progress' ]=True
    options['todo'     ]=True
    options['engineers']=True

unexpected=0
readData()
if options['bugs']:
    reportBugs()

if options['features']:
    reportFeatures()

if options['progress']:
    reportProgress()

if options['todo']:
    reportTodo()

if options['engineers']:
    reportEngineers()

# That's all Folks!
##
