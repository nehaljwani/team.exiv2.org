#!/usr/bin/env python

import json
import os

##
# read in the json
J=[]
for i in range(0,9):
	if len(open('data%d.json' % i,'r').read()) > 100:
		J.append(json.loads( open('data%d.json' % i,'r').read()   ))

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
				done= i['done_ratio'] * i['estimated_hours'] / 100.0
				left= i['estimated_hours'] - done
				Left = Left+left
				Size = Size+i['estimated_hours']
				print('| #%-4d  | %3d | %3d | %s |' % (i['id'],left,i['estimated_hours'],i['subject']) )

		except:
			pass

print('| Left   | %4d|%4d | %3d |' % (Left, Size, Left/40) )
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
		except:
			pass

progress= 100 - 100*open/v0_26
print('| v1.0| Review |0.26 | closed |open | resolved |left |progress |')
print('| %3d |    %3d | %3d |    %3d | %3d |      %3d | %3d |    %3d%% |' % (v1_0,vReview,v0_26,closed,open+resolved,resolved,open,progress) )

# That's all Folks!
##
