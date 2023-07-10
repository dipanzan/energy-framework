#!/bin/bash

CORE=$1
LOAD=$2
sudo perf stat -C $CORE -e power/energy-pkg/ \
    taskset -c $CORE \
    /home/dipanzan/PycharmProjects/pythonProject/venv/bin/python /home/dipanzan/PycharmProjects/pythonProject/main.py $CORE $LOAD

echo "sleeping zzz ... for 1 sec"
sleep 1

sudo perf stat -C $CORE -e power/energy-pkg/ \
    taskset -c $CORE \
    /usr/lib/jvm/java-1.17.0-openjdk-amd64/bin/java -javaagent:/snap/intellij-idea-ultimate/437/lib/idea_rt.jar=43145:/snap/intellij-idea-ultimate/437/bin -Dfile.encoding=UTF-8 -classpath /home/dipanzan/IdeaProjects/energy-framework/app/target/classes com.dipanzan.DemoApp $CORE $LOAD


echo "sleeping zzz ... for 1 sec"
sleep 1

sudo perf stat -C $CORE -e power/energy-pkg/ taskset -c $CORE /home/dipanzan/Desktop/hello $CORE $LOAD
