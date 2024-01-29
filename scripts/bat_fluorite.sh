#!/bin/bash

CURRENT=$(cat /sys/class/power_supply/BAT0/charge_now)
MAX=$(cat /sys/class/power_supply/BAT0/charge_full)
PRE=`expr $CURRENT \* 100`

TOTAL=`expr $PRE / $MAX`

if [[ $TOTAL -gt "90" ]]; then
	ICON=" "
elif [[ $TOTAL -gt "50" ]]; then
	ICON=" "
elif [[ $TOTAL -gt "40" ]]; then
	ICON=" "
elif [[ $TOTAL -gt "25" ]]; then
	ICON=" "
else
	ICON=" "
fi

if [[ $(cat /sys/class/power_supply/BAT0/status) == "Discharging" ]]; then
	echo $ICON" "$TOTAL% | tr -d \\n
else
	echo $ICON" "+$TOTAL% | tr -d \\n
fi
