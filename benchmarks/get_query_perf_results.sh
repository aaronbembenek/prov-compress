cfile=compression_perf.data
dfile=dummy_perf.data
declare -a queries=( 0 1 2 3 4 5 )

rm results/$cfile
rm results/$dfile
touch results/$cfile
touch results/$dfile

export BESAFE=0

# dummy graph
cd ../querier
make clean && export COMPRESSED=0 && make query
cd ../benchmarks

for f in results/*.prov; do
    echo File $f
    cd ../querier 
    (echo File $f) >> ../benchmarks/results/$dfile
    for q in ${queries[@]}; do
        ./query --query=$q --auditfile=../benchmarks/$f >> ../benchmarks/results/$dfile
    done
    cd ../benchmarks
done

# compressed
cd ../querier
make clean && export COMPRESSED=1 && make query

# perf compressed graph 
for f in ../benchmarks/results/*.prov; do
    echo File $f
    cd ../compression && ./main.py ../benchmarks/$f 
    cd ../querier 
    (echo File $f) >> ../benchmarks/results/$cfile
    for q in ${queries[@]}; do
        ./query --query=$q >> ../benchmarks/results/$cfile
    done
    cd ../benchmarks
done
