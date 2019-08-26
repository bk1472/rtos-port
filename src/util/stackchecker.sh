#!/bin/bash

set +x
target=ucos
file_1=stdin
file_2=apps/`cat apps/apps_name.mk | awk '{print $3}'`/$target
sefmt1="s/\\(.*\\): sp=0x\\([0-9a-fA-F]\\+\\), pc=0x\\([0-9a-fA-F]\\+\\).*/\\3/"
sefmt2="s^/^\\n^g"
ver=1

####################################################################################
# Function to execute sed script to extract address from input stream
####################################################################################
function extractAddrList()
{
	cat $file_1 | sed -e "$sefmt1" -e "$sefmt2"
}


####################################################################################
# Function to trace stack with original GNU addr2line
#      Relatively slow command.
####################################################################################
function traceViaAddr2Line_org()
{
	echo ============================================================================
	for addr in $addrlist
	do
		addr2line -f -e $file_2 $addr | awk "{ printf \"[pc=0x$addr %-24.24s\", \$1; getline; printf \"%s]\n\", \$1}"
	done	
	echo ============================================================================
}

####################################################################################
# Function to trace stack with modified GNU addr2line
####################################################################################
function traceViaAddr2Line()
{
	echo ============================================================================
	addr2line.lge -n 24 -e $file_2 $addrlist
	echo ============================================================================
}

while [ "$1" != "" ]
do
	case "$1" in
		-f)
		file_2=$2
		shift 2
		;;
		*)
		file_1=$1
		shift
		;;
	esac
done

if [ "$file_1" = "" ]; then
	echo "==== Tracing stack from \"stdin\" with $file_2"
else
	echo "==== Tracing stack from \"$file_1\" with $file_2"
	if [ ! -f $file_1 ]; then
		file_1=
	fi
fi

addrlist=`extractAddrList`

if [ $ver == 0 ]; then
	echo "<org GNU version>"
	traceViaAddr2Line_org
else
	echo "<modified GNU version>"
	traceViaAddr2Line
fi
