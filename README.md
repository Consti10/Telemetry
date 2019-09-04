# Telemetry

## TelemetryCore 
contains the core library, written in cpp/c. TelemetryReceiver.cpp is the main object.
It parses telemetry coming from different uav platforms ( MAVLINK,LTM,FRSKY ) into one uniform data model and provides an interface
for optaining Telemetry values as human-readable strings (e.g. for display on an On-Screen-Display) without being platform-dependent.
Its functions are also exposed to JAVA code using the ndk. 


### TelemetryExample
contains just a MainActivity.java class that allows modification of settings and lists debugging values on TextViews.
These debugging values also include all received data as prefix - value - metric triples. Note that some prefixes have to be interpreted
as Icons instead of utf-8 strings, and therefore are not displayed properly in the example.

<img src="https://github.com/Consti10/Telemetry/blob/master/screenshots/screenshot1.jpg" alt="Example" width="240">

#### Usage:
TelemetryExample depends on TelemetryCore, other projects should only include TelemetryCore not TelemetryExample.

**Setup Dependencies**\
There are 2 ways to use TelemetryCore in your Project \
1. Declaring Dependency via Jitpack: [jitpack.io](https://jitpack.io)
:+1: Easy
:-1: cannot browse native libraries
Gradle example:
```gradle
    allprojects {
        repositories {
            jcenter()
            maven { url "https://jitpack.io" }
        }
   }
   dependencies {
        implementation 'com.github.Consti10:Telemetry:v1.0'
   }
```
2. Forking the repo and including sources manually:
:+1: browse native libraries
:+1: modify code
* To your top level settings.gradle file, add
```
include ':VideoCore'
project(':VideoCore').projectDir=new File('..\\Telemetry\\TelemetryCore')
```
and modify the path according to your download file
* To your app level gradle file add
```
implementation project(':TelemetryCore')
```
See [FPV-VR](https://github.com/Consti10/FPV_VR_2018) as an example how to add dependencies.
