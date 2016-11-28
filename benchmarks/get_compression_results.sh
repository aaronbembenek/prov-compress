for f in results/*; do
    cd ../compression && time (./main.py ../benchmarks/$f)
    cd ../benchmarks
    csize=$(ls -lrt ../compression/compressed_metadata.txt ../compression/identifiers.txt | awk '{ total += $5 }; END { print total }')
    xz -9 $f 
    xzsize=$(ls -lrt $f.xz | awk '{ total += $5 }; END { print total }')
    xz -d $f.xz
    original=$(ls -lrt $f | awk '{ total += $5 }; END { print total }')
    echo $csize $xzsize $original $(bc <<<"scale=2;$original/$csize") $(bc <<<"scale=2;$original/$xzsize") 
done
