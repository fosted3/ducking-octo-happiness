#!/bin/sh
if [[ -z "$3" ]]; then
	echo "Usage: $0 outfile scale fps" 
else
	ffmpeg -r $3 -i img/edit/%04dE.png -q:v 2 -pix_fmt yuv444p -vf scale=iw/$2:-1 $1
fi
