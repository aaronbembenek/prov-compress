file=results/metadata_queries.perf
rm $file
touch $file

for f in results/*.prov; do
    cd ../compression && ./main.py ../benchmarks/$f
    cd ../querier 
    (echo $f) >> ../benchmarks/$file 2>&1
    make && (time ./query -c &> /dev/null) >> ../benchmarks/$file 2>&1
done
