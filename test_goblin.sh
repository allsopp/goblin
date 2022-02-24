#!/bin/sh

if [ "$#" -ne 2 ]; then
	echo "usage: <binary> <size>" >&2
	exit 1
fi

f=$(printf "%03d.png" $2)
[ -f $f ] || gm convert -size $2x$2 xc: +noise Random PNG8:$f || exit 1
(gm convert $f PNG24:- | gm convert - $f.tga) || exit 1
("$1" $f > $f.goblin.tga) || exit 1
(cmp $f.goblin.tga $f.tga >&2) || exit 1
