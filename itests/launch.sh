#INSERT PROCESS EXECUTABLE NAMES HERE IN THE ORDER THEY ARE LAUNCHED
PROCESSES=(ITcyan.exe ITmag.exe ITgreen.exe ITyellow.exe ITred.exe ITblue.exe)
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
    IDIR=../include
    ODIR=../obj
    CURDIR=$(pwd)

    #enforce the existence of object folder
    mkdir -p $ODIR

    #move to proper directory
    cd ..
    #clear the implementation
    make -f makefile.mcapi clean
    #build the implementation
    make -f makefile.mcapi CC=$CC DEFINE="ALLOW_THREAD_SAFETY"
    #build the cleaner
    make -f makefile.cleaner CC=$CC
    #move back to this directory
    cd $CURDIR

    #INSERT BUILD RULES FOR EXECUTABLES HERE IF SUCH IS NEEDED

    #build rules of individual executables
    $CC -o ITcyan.exe ITcyan.c $ODIR/*.o -I$IDIR -lrt -pthread
    $CC -o ITmag.exe ITmag.c $ODIR/*.o -I$IDIR -lrt -pthread
    $CC -o ITgreen.exe ITgreen.c $ODIR/*.o -I$IDIR -lrt
    $CC -o ITyellow.exe ITyellow.c $ODIR/*.o -I$IDIR -lrt
    $CC -o ITred.exe ITred.c $ODIR/*.o -I$IDIR -lrt -lm
    $CC -o ITblue.exe ITblue.c $ODIR/*.o -I$IDIR -lrt -lm
    #BUILD RULES END HERE
fi

#must be cached to avoid queer functionality
trap "exit" SIGTSTP

#now, application specific stuff ends here. launching the launcher proper
../launcher.sh ${PROCESSES[@]} &

pid=$!

wait $pid
