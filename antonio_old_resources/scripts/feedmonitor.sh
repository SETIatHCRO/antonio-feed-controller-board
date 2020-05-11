HDRCMD='           TC    gd     gv    p398  p310  p316  p326  p330  p342  p346   a0    a1    a2    a3    a5    a6       E (3) E (1) E (2)  '
HDRONE='        cld hd  LNA    Vac   Turbo Turbo Turbo elec.  botm  brng motor board inlet  pax  exhst rjctn housn  Fan  crnt  max   min   '
HDRTWO='  Time    (K)   (K)    mbar   RPM   amps watts  (C)   (C)   (C)   (C)   (C)   (C)   (C)   (C)   (C)   (C)   RPM watts watts watts  '
HDRCNT=0
LN=0
SLEEPSECS=180  # default every 3 minutes
HOST=""
PORT=0

while getopts i:h:p: OPT
do
  case $OPT in
    i) SLEEPSECS=$OPTARG
       ;;
    h) HOST=$OPTARG
       ;;
    p) PORT=$OPTARG
       ;;
   \?) echo "Usage: bash feedmonitor.sh -i <interval seconds> -h <host> -p <port>" 1>&2
       exit 1
       ;;
  esac
done

while :
do
    if [[ $(($LN % 10)) -eq 0 ]] ; then
        if [[ $(($HDRCNT & 1)) -eq 0 ]] ; then
            echo "$HDRONE"
        else
            echo "$HDRCMD"
        fi
        echo "$HDRTWO"
        HDRCNT=$(($HDRCNT + 1))
    fi
    CMND="python ./feedmonitor.py"
    if [[ -n $HOST ]] ; then
        CMND="$CMND -h $HOST"
    fi
    if [[ $PORT -ne 0 ]] ; then
        CMND="$CMND -p $PORT"
    fi
#    echo $CMND
    eval $CMND
    LN=$(($LN + 1))
    sleep $SLEEPSECS
done
