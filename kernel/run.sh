#!/bin/bash

CPU_LIST=$1
NUM_THREADS=$2

taskset -c $CPU_LIST /usr/lib/jvm/java-1.17.0-openjdk-amd64/bin/java \
	-javaagent:/snap/intellij-idea-ultimate/430/lib/idea_rt.jar=46141:/snap/intellij-idea-ultimate/430/bin \
	-Dfile.encoding=UTF-8 \
	-classpath /home/dipanzan/IdeaProjects/energy-framework/app/target/classes com.dipanzan.DemoApp $NUM_THREADS