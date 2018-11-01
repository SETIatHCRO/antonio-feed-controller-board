all:
	convert antonio-feed-control-board-outline-and-connectors-annotated.png -scale 100% -background gray94 -compose copy -gravity center -extent 1100x850 eps2:antonio-feed-control-board-outline-and-connectors.eps
	convert pic32mx795f512l-antonio-feed-pinout.png -scale 100% -background gray94 -compose copy -gravity center -extent 1100x850 eps2:pic32mx795f512l-antonio-feed-pinout.eps
	gs -sDEVICE=pdfwrite -sPAPERSIZE=letter -sOutputFile="2013-10-08-antonio-feed-board-schematic-v10.pdf" -c "<< /PageSize [792 612]  >> setpagedevice" -dAutoRotatePages=/None -dEPSFitPage -dPDFSETTINGS=/prepress -dNOPAUSE -dBATCH  \
antonio-feed-control-board-outline-and-connectors.eps \
pic32mx795f512l-antonio-feed-pinout.eps \
microcontroller/microcontroller-support.eps \
power/power-supply.eps \
relay/antonio-feed-relay-circuit.eps \
cryo/cryo.eps \
lakeshore/lakeshore.eps \
vacuum/vacuum.eps \
guage/guage.eps \
fan/fan-control.eps \
tc7x/temp-sensor.eps \
accel/accel.eps \
flash/flash.eps \
pdfmarks
