#!/bin/sh
for i in `ls retroshare_0.6.3-1.*.changes` ; do
	dput ppa:retroshare/unstable $i
done
