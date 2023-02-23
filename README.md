# Byte Buddy agent weaving advice into bootstrap classes (EnergyAgent)

Special thanks to [Alexander Kriegisch](https://github.com/kriegaex) for the shaded-jar implementation.
  * This project is simple demonstration of arbritary function hooking to any JVM process.
  * A special kernel-module [WIP] will be called from these entry/exit hooks to calculate various PMUs for data.

## How to build

```shell
mvn clean package
```
## How to run

### Console 1 - demo app

A simple demo app which runs in an endless loop to call functions based on choice: 1 | 2 | 3

### Console 2 - injector, dynamically attaching agent by input PID from scanned list.

```shell
# Run application as executable JAR
java -jar injector/target/injector-1.0-SNAPSHOT-jar-with-dependencies.jar
```
