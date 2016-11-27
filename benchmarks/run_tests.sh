for f in *.py; do
    sudo rm -rf /tmp/audit.log
    sudo camflow-prov --track-file $f true 
    sudo camflow-prov --track-file $f propagate
    sudo camflow-prov --file $f
    python $f
    sudo cp /tmp/audit.log results/$f.prov
    sudo camflow-prov --track-file $f false
done
