#!/bin/bash

cat sunrise.data | while read line ; do
	usr=clanmills
	if [ "${line:0:1}" == "[" ]; then
		# [SJC]   37.35 -121.92 San Jose,Usa
		lat=$(echo $line| sed -E 's/ *//'| cut '-d ' -f 2)
		lng=$(echo $line| sed -E 's/ *//'| cut '-d ' -f 3)
		# echo  "curl   http://api.geonames.org/timezoneJSON?lat=$lat&lng=$lng&username=$usr"
		json=$(curl  "http://api.geonames.org/timezoneJSON?lat=$lat&lng=$lng&username=$usr" 2>/dev/null)
		echo $line $json
		sleep 2
	else
		echo $line
	fi
done

# That's all Folks!
##
