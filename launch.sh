#INSERT PROCESS EXECUTABLE NAMES HERE IN THE ORDER THEY ARE LAUNCHED
PROCESSES=(esender.exe ereceiver.exe)
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

    echo ""
    echo "BUILDING PROCESSES"
    echo ""

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

#must be cached to avoid queer functionality
trap "exit" SIGTSTP

#now, application specific stuff ends here. launching the launcher proper
./launcher.sh ${PROCESSES[@]} &

pid=$!

wait $pid
