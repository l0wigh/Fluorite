ISMUTE=$(pactl list sinks | grep Mute | cut -d ' ' -f 2 | tr -d \\n)

if [[ $ISMUTE == "yes" ]]; then
	echo -n "Mute"
else
pactl list sinks | grep '^[[:space:]]Volume:' | cut -d / -f 2 | sed 's/ //g' | tr -d \\n
fi
