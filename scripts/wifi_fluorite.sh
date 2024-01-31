# This script uses nmcli and search for the work "oui"
# Change it based on your own locale with "yes" or something else

#!/bin/bash

WIFI=$(nmcli -t -f active,ssid dev wifi | grep 'oui' | cut -d ':' -f 2 | tr -d \\n)

if [[ $WIFI == "" ]]; then
	echo -n "󰖪  Not Connected"
else
	echo -n "󰖩  "$WIFI
fi
