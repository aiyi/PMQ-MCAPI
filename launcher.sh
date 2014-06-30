#parameters are the names of the processes to be launched
PROCESSES=("$@")
echo ${PROCESSES[@]}
#Killing old processes before starting new ones

commit_hits () {
    #filename for old processes to be killed
    hitlist=hitlist.txt

    #skip if does not exist
    if [ -f $hitlist ]; then
        echo ""
        echo "KILLING PROCESSES"

        #And while there are lines to be red...
        while read line
        do
            #Kill the process indicated by the line
            killall $line &> /dev/null
        done < $hitlist
    fi
}

commit_hits

#And finally remove the old hitlist before new one is generated
rm $hitlist &> /dev/null

#run the cleaner before processes: it is mandatory
./cleaner

echo ""
echo "LAUNCHING NEW PROCESSES"
echo ""

#launch the proceses in parallel
j=0
for name in "${PROCESSES[@]}"
do
    #launch the process
    ./$name &

    #read its pid
    mypid=$!
    #mark it to the hitlist so that it will be killed before next launch
    echo "$name" >> $hitlist
    #add to the list of launched processes
    pids=$pids"$mypid "
    #increase the index
    j=`expr $j + 1`
done

#initialize the trap to commit hits when needed
trap "commit_hits" SIGTSTP

echo ""
echo "PROCESSES LAUNCHED"
echo ""

#wait for the processes until yhtey are finished or terminated
wait $pids

echo ""
echo "DONE"
echo ""

#rerun the cleaner in case the resources need to be freed
./cleaner
