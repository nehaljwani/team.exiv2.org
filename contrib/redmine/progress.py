#!/usr/bin/env python

import json
import os

##
# read the json into an array of json objects
# see ./getdata.sh for an explanation of why the data is sliced into smaller files
J=[]
for i in range(0,9):
	data=open('data%d.json' % i,'r').read();
	if len(data) > 100:
		J.append(json.loads( data ))

##
#
def printHeader(t):
	print(t)
	print('='*len(t))

##
# Features:
printHeader('Features')
Left=0
Size=0
print( '| _Priority_ | _Issue_  | _Effort_ | _Status_ | _Done_ | _Description_ |')

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
						print('|=. %3d | #%-4d  |>.  %4d |=. %3d%% |>. %-5.2f | %s |' % (priority,i['id'],effort,status,done/100,i['subject']) )
			  	except:
					pass

effort=4
status=75
done   = status*effort
Done   = Done+done
Effort = Effort+effort
print('|\\2>. |>.  %4d |=. %3d%% |>. %-5.2f | %s |' % (effort,status,done/100,'User Support') )
Status=(Done*100 / Effort)/100
print('|\\2>. *Total:*  |>.   %3d |=. %3d%% |>. %-5.2f | |' % (Effort, Status,Done/100) )
print('')

##
# Progress
printHeader('Progress')
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
print('| v1.0 | Review | 0.26 | closed | open | resolved | left | progress | unassigned |')
print('|>.%3d |>.  %3d |>.%3d |>.  %3d |>.%3d |>.    %3d |>.%3d |>.   %3d%% |>. %3d / %3d%% |' % (v1_0,vReview,v0_26,closed,open+resolved,resolved,open,progress,unassigned,unassigned*100/v0_26) )
print('')

##
# Robin's to do list
printHeader("Robin's todo list");
print( '| _Issue_       | _Done_ | _Size_ | _Left_ | _Description_ |')
for j in J:
	issues=j['issues']
	for i in issues:
		try:
			if   i['assigned_to']['name']=='Robin Mills':
			 if  i['fixed_version']['name']=='0.26':
			  if i['done_ratio'] < 100:
				size= i['estimated_hours']
				done= i['done_ratio'] * size / 100.0
				left= size - done
				Left = Left+left
				Size = Size+size
				print('| #%-4d       |>. %3d%% |>. %3d |>. %3d | %s |' % (i['id'],i['done_ratio'],size,left,i['subject']) )
		except:
			pass

Unexpected=Left/10
Weeks=(Left+20+Unexpected)/31
Support=9*Weeks
Review=20
print('|\\2>. Left               |>.%4d |>.%4d ||' % (Size,Left) )
print('|\\3>. Unexpected 10%%           |>. %3d ||'  % (Unexpected))
print('|\\3>. Support 9 hr/week        |>. %3d ||'  % (Support))
print('|\\3>. Review+1.0               |>. %3d ||'  % 20)
Total=Left+Support+Unexpected+Review
print('|\\3>. Total                    |>. %3d | %2d weeks |' % (Total,(Total+30)/40))
print('')



# That's all Folks!
##
