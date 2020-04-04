cd cpp/build
for j in {3..3} ; do
    for i in {1..16} ; do
        echo traces $j Test case \#$i
        ./src/sim_cpp ../../configs/sim_config.json ../../traces/traces$j/trace-$i.json
    done
done
