#INSERT PROCESS EXECUTABLE NAMES HERE IN THE ORDER THEY ARE LAUNCHED
PROCESSES=(esender.exe ereceiver.exe)
#INSERT THEIR RESPECTIVE "niceness" IN THE SAME ORDER
NICENESS=(11 10)
#SET TO ONE TO USE NICENESS: NOT SUPPORTED ON EVERY PLATFORM!!!
USENICE=0
#SET TO ONE BUILD
BUILDEXES=1

#build only, if set so
if [ $BUILDEXES -eq 1 ]; then
    #compiler used: select GCC if nothing else is provided
    if [ "$1" == "" ]; then
        CC=gcc
    else
        CC=$1
    fi

    #folders for includes and objects
    IDIR=include
    ODIR=obj

    #enforce the existence of object folder
    mkdir -p $ODIR

    #clear the implementation
    make -f makefile.mcapi clean
    #build the implementation
    make -f makefile.mcapi CC=$CC
    #build the cleaner
    make -f makefile.cleaner CC=$CC

    #INSERT BUILD RULES FOR EXECUTABLES HERE IF SUCH IS NEEDED

    #build rules of individual executables
    $CC -o esender.exe example/example_sender.c $ODIR/*.o -I$IDIR -lrt
    $CC -o ereceiver.exe example/example_receiver.c $ODIR/*.o -I$IDIR -lrt
    #BUILD RULES END HERE
fi

#Killing old processes before starting new ones

#filename for old processes to be killed
hitlist=hitlist.txt

#skip if does not exist
if [ -f $hitlist ]; then
    echo ""
    echo "KILLING OLD PROCESSES"

    #And while there are lines to be red...
    while read line
    do
        #Kill the process indicated by the line
        killall $line &> /dev/null
    done < $hitlist
fi

#And finally remove the old hitlist before new one is generated
rm $hitlist

#run the cleaner before processes: it is mandatory
./cleaner

echo ""
echo "LAUNCHING NEW PROCESSES"
echo ""

#launch the proceses in parallel
j=0
for name in "${PROCESSES[@]}"
do
    if [ $USENICE -eq 1 ]; then
        #read niceness from corresponding index
        n=${NICENESS[$j]}
        #launch as parallel process with the given niceness
        nice -n $n ./$name &
    else
        #launch without niceness
        ./$name &
    fi

    #read its pid
    mypid=$!
    #mark it to the hitlist so that it will be killed before next launch
    echo "$name" >> $hitlist
    #add to the list of launched processes
    pids=$pids"$mypid "
    #increase the index
    j=`expr $j + 1`
done

echo ""
echo "PROCESSES LAUNCHED"
echo ""

#wait for the processes until yhtey are finished or terminated
wait $pids

echo ""
echo "DONE"
echo ""
