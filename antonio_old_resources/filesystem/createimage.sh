CMND="dd if=/dev/zero of=antoniofsimage bs=512 count=1024"
echo $CMND
eval $CMND
CMND="mkfs.vfat -S 512 -I antoniofsimage"
echo $CMND
eval $CMND
CMND="mount -o loop -o rw antoniofsimage mnt"
echo $CMND
eval $CMND
CMND="cp ../mplab/antonio_feed_pic32_control_software.X/dist/XC32_PIC32MX795F512L/production/antonio_feed_pic32_control_software.X.production.hex mnt"
echo $CMND
eval $CMND
CMND="date --iso-8601=seconds > support-files/fsdatetm.txt"
echo $CMND
eval $CMND
CMND="mkdir mnt/HELP"
echo $CMND
eval $CMND
CMND="cp help-files/* mnt/HELP"
echo $CMND
eval $CMND
CMND="cp support-files/* mnt"
echo $CMND
eval $CMND
CMND="sync"
echo $CMND
eval $CMND
CMND="sleep 1"
echo $CMND
eval $CMND
CMND="df mnt"
echo $CMND
eval $CMND
CMND="sync"
echo $CMND
eval $CMND
CMND="sleep 1"
echo $CMND
eval $CMND
CMND="umount mnt"
echo $CMND
eval $CMND
