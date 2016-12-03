cfile=compression_perf.data
dfile=dummy_perf.data
declare -a queries=( 0 1 2 3 4 5 6 )

rm results/$cfile
rm results/$dfile
touch results/$cfile
touch results/$dfile

cd ../querier
make clean && export COMPRESSED=1 && make query
cd ../benchmarks

# perf compressed graph 
for f in results/*.prov; do
    (echo File $f) >> $cfile
    cd ../compression && ./main.py ../benchmarks/$f
    cd ../querier 
    for q in ${queries[@]}; do
        ./query --query=$q >> ../benchmarks/results/$cfile
    done
done

# repeat for dummy graph
cd ../querier
make clean && export COMPRESSED=0 && make query
cd ../benchmarks

for f in results/*.prov; do
    echo $f >> $dfile
    cd ../compression && ./main.py ../benchmarks/$f
    cd ../querier 
    for q in ${queries[@]}; do
        ./query --query=$q --auditfile=$f >> ../benchmarks/results/$dfile
    done
done
