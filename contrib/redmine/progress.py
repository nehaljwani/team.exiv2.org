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

Left=0
Size=0
print( '| Issue  |Left |Size | Description |')

##
# Robin's to do list
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
				print('| #%-4d  | %3d | %3d | %s |' % (i['id'],left,i['estimated_hours'],i['subject']) )

		except:
			pass

print('| Left   | %4d|%4d | %3d |' % (Left, Size, Left/40) )
print('')
print('| Unexpected 10%%     | %3d |'  % (Left/10))
print('| Support 12 hr/week | %3d |'  % (12*Left/40))
print('| Review+1.0         | %3d |'  % 20)
Total=Left+Left/40+12*Left/40+20
print('| Total = %d   = %d weeks |' % (Total,Total/40))
print('')

##
# Robin's to do list
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
print('|  %3d |    %3d |  %3d |    %3d |  %3d |      %3d |  %3d |     %3d%% | %3d = %3d%% |' % (v1_0,vReview,v0_26,closed,open+resolved,resolved,open,progress,unassigned,unassigned*100/v0_26) )

# That's all Folks!
##
