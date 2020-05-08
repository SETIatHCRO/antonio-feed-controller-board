function die
{
	local message=$1
	[ -z "$message" ] && message="Died"
	echo "$message at ${BASH_SOURCE[1]}:${FUNCNAME[1]} line ${BASH_LINENO[0]}." >&2
	exit 1
}

CMND="mkdir mnt"
echo $CMND
eval $CMND
CMND="dd if=/dev/zero of=antoniofsimage bs=512 count=1024 || die"
echo $CMND
eval $CMND
CMND="mkfs.vfat -S 512 -I antoniofsimage || die"
echo $CMND
eval $CMND
CMND="sudo mount -o loop -o rw -o umask=0000 antoniofsimage mnt || die"
echo $CMND
eval $CMND
CMND="cp ../antonio-feed-control-v2.X/dist/default/production/antonio-feed-control-v2.X.production.hex mnt  || die"
#CMND="cp ../mplab/antonio_feed_pic32_control_software.X/dist/XC32_PIC32MX795F512L/production/antonio_feed_pic32_control_software.X.production.hex mnt"
echo $CMND
eval $CMND
CMND="date --iso-8601=seconds > support-files/fsdatetm.txt || die"
echo $CMND
eval $CMND
CMND="mkdir mnt/HELP || die"
echo $CMND
eval $CMND
CMND="cp help-files/* mnt/HELP || die"
echo $CMND
eval $CMND
CMND="cp support-files/* mnt || die"
echo $CMND
eval $CMND
CMND="sync || die"
echo $CMND
eval $CMND
CMND="sleep 1 || die"
echo $CMND
eval $CMND
CMND="df mnt || die"
echo $CMND
eval $CMND
CMND="sync || die"
echo $CMND
eval $CMND
CMND="sleep 1 || die"
echo $CMND
eval $CMND
CMND="sudo umount mnt || die"
echo $CMND
eval $CMND
