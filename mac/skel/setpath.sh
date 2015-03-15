#!/bin/bash

# Step 1 -- locate the directory containing this script

progname="`basename $0`"
if [ "$progname" != "$0" ]
then
    # programname contains directory components
    progdir="`dirname $0`"
else
    # must search along path
    tmpvar="$PATH"
    while [ -n "$tmpvar" ]
    do
      IFS=':' read progdir tmpvar <<EOF
$tmpvar
EOF
      test -x "$progdir/$progname" && break
    done
fi
progdir=`cd $progdir ; pwd`
while [ -L "$progdir/$progname" ]
do
    tmpvar=`ls -ld $progdir/$progname`
    tmpvar=`expr "$tmpvar" : '.*-> *\(.*\)'`
    progname=`basename $tmpvar` 
    tmpvar=`dirname $tmpvar` 
    progdir=`cd $progdir ; cd $tmpvar ; pwd` 
done
if [ ! -x "$progdir/$progname" ]
then
   echo 1>&2 "$progname: command not found"
   exit 10
fi

# Step 2 -- check path and manpath

contains() {
  tmpvar=`eval echo '$'$1`
  while [ -n "$tmpvar" ]
  do
    IFS=':' read dir tmpvar <<EOF
$tmpvar
EOF
    [ "$dir" = "$2" ] && exit 0
  done
  false
}


if ! contains PATH $progdir/bin
then
  echo -n 'PATH="'
  test -n "$PATH" && echo -n '$'PATH:
  echo $progdir/bin'"'
  echo export PATH
fi

if ! contains MANPATH $progdir/share/man
then
  echo MANPATH='"$'MANPATH:$progdir/share/man'"'
  echo export MANPATH
fi



