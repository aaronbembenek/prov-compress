file=results/compression_results.out
rm $file
touch $file

for f in results/*.prov; do
    (echo $f) >> results/compression_results.out 2>&1
    cd ../compression && (time (./main.py ../benchmarks/$f) && xz -9 -k identifiers.txt) >> ../benchmarks/$file 2>&1
    cd ../benchmarks
    csize=$(ls -lrt ../compression/compressed_metadata.txt ../compression/identifiers.txt.xz | awk '{ total += $5 }; END { print total }')
    xz -9 $f 
    xzsize=$(ls -lrt $f.xz | awk '{ total += $5 }; END { print total }')
    xz -d $f.xz
    original=$(ls -lrt $f | awk '{ total += $5 }; END { print total }')
    (echo $csize $xzsize $original $(bc <<<"scale=2;$original/$csize") $(bc <<<"scale=2;$original/$xzsize")) >> $file 2>&1
done
