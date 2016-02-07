#!/usr/bin/env python

from __future__ import division

import json
import os
import sys

global J,args,options

##
# Help:
def reportHelp():
    print('syntax: %s [options] verb [arg]...' % (sys.argv[0]) )
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
    sofar=0.75

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

    Features = [ { 'id': 1041, 'effort':5 }
               , { 'id': 1111, 'effort':3 }
               , { 'id': 1109, 'effort':3 }
               , { 'id': 1074 ,'effort':2 }
               , { 'id': 1034 ,'effort':3 }
               , { 'id': 1121 ,'effort':3 }
               , { 'id': 1061 ,'effort':2 }
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
                        if i['done_ratio'] < 100:
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
    status=sofar*100
    done   = status*effort
    Done   = Done+done
    Effort = Effort+effort
    print(features2 % (effort,status,done/100,'User Support') )
    Status=(Done*100 / Effort)/100
    print(features3 % (Effort, Status,Done/100) )
    print('')

##
# Progress
def reportProgress():
    printHeader('Progress')
    global J,args,options
    progress0='| v1.0 | Review | 0.26 | closed | open | resolved | left | progress | unassigned |'
    progress1='|>.%3d |>.  %3d |>.%3d |>.  %3d |>.%3d |>.    %3d |>.%3d |>.   %3d%% |>. %3d / %3d%% |'
    if options['console']:
        progress0='| v1.0 | Review | 0.26 | closed | open | resolved | left | progress | unassigned |'
        progress1='|  %3d |    %3d |  %3d |    %3d |  %3d |      %3d |  %3d |    %3d%%  | %3d / %3d%% |'

    v0_26=0
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
    print(progress1 % (v1_0,vReview,v0_26,closed,open+resolved,resolved,open,progress,unassigned,unassigned*100/v0_26) )
    print('')

##
# Robin's to do list
def reportTodo():
    printHeader("Robin's todo list");
    global J,args,options

    todo0='| _Issue_     | _Done_ | _Size_ | _Left_ | _Description_ |'
    todo1='|\\2>. Other Minor Issues       |>.%4d ||'
    todo2='| #%-4d       |>. %3d%% |>. %3d |>. %3d | %s |'
    todo3='|\\2>. Left             |>.%4d |>.%4d ||'
    todo4='|\\3>. Unexpected 10%%           |>. %3d ||'
    todo5='|\\3>. Support 9 hr/week        |>. %3d ||'
    todo6='|\\3>. Review+1.0               |>. %3d ||'
    todo7='|\\3>. Total                    |>. %3d | %2d weeks |'
    if options['console']:
        todo0='| Issue       | Done | Size | Left | Description |'
        todo1='| Minor Issues              | %4d | |'
        todo2='| #%-4d       | %3d%% | %4d | %4d | %s |'
        todo3='| Left               | %4d | %4d | |'
        todo4='| Unexpected 10%%            | %4d | |'
        todo5='| Support 9 hr/week         | %4d | |'
        todo6='| Review+1.0                | %4d | |'
        todo7='| Total                     | %4d | %2d weeks |'

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
                    if left > 2.1:
                        Left = Left+left
                        Size = Size+size
                        print(todo2 % (i['id'],i['done_ratio'],size,left,i['subject']) )
                    else:
                        Minor = Minor + left
            except:
                pass

    Left=Left+Minor
    Unexpected=Left/10
    Weeks=(Left+20+Unexpected)/31
    Support=9*Weeks
    Review=20
    print(todo1 % (Minor) )
    print(todo3 % (Size,Left) )
    print(todo4 % (Unexpected))
    print(todo5 % (Support))
    print(todo6 % 20)
    Total=Left+Support+Unexpected+Review
    print(todo7 % (Total,(Total+30)/40))
    print('')

##
# parse command-line
options={}
options['console' ]=False
options['bugs'    ]=False
options['todo'    ]=False
options['features']=False
options['progress']=False
options['getdata' ]=False

args=sys.argv[1:]

if len(args)==0:
    reportHelp()

if len(args)>0:
    cmd=args[0]
    while cmd=='console' or cmd=='getdata':
        options[cmd]=True
        args=args[1:]
        cmd=args[0]

if len(args)>0:
    cmd=args[0]
    if cmd=='bugs':
        options['bugs']=True
        args=args[1:]
    elif cmd=='all':
        options['features']=True
        options['progress']=True
        options['todo'    ]=True
        args=args[1:]
    elif cmd=='features':
        options['features']=True
        args=args[1:]
    elif cmd=='progress':
        options['progress']=True
        args=args[1:]
    elif cmd=='todo':
        options['todo']=True
        args=args[1:]
    else:
        print('unknown argument')
        print(args[1:])
        exit(1)

readData()
if options['bugs']:
    reportBugs()

if options['features']:
    reportFeatures()

if options['progress']:
    reportProgress()

if options['todo']:
    reportTodo()

# That's all Folks!
##
