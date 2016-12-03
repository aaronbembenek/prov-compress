cfile=compressed_perf.data
dfile=dummy_perf.data
queries=( 0,1,2,3,4,5,6 )

rm $cfile
rm $dfile
touch $cfile
touch $dfile

cd ../querier
make clean && export COMPRESSED=1 && make query
cd ../benchmarks

# perf compressed graph 
for f in results/*.prov; do
    echo $f >> $cfile 2>&1
    cd ../compression && ./main.py ../benchmarks/$f
    cd ../querier 
    for q in ${queries[@]}; do
        ./query --query=$q >> ../benchmarks/results/$cfile 2>&1
    done
done

# repeat for dummy graph
cd ../querier
make clean && export COMPRESSED=0 && make query
cd ../benchmarks

for f in results/*.prov; do
    echo $f >> $dfile 2>&1
    cd ../compression && ./main.py ../benchmarks/$f
    cd ../querier 
    for q in ${queries[@]}; do
        ./query --query=$q >> ../benchmarks/results/$dfile 2>&1
    done
done
