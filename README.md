# UDPMulticast
UDP Multicast library for C++, MATLAB and Python

Provides highspeed message sending and receiving through UDP Multicast
for Windows (using overlappedIO with completion ports). Its a C++ class,
MATLAB and Python wrappers are provided. Both 32bit and 64bit are
supported. Not all VS project settings are necessarily correct for all
configurations though

When using this tool, please cite `Niehorster, D.C., Cornelissen, T., Holmqvist, K. & Hooge, I.T.C (2019). Searching with and against each other: Spatiotemporal coordination of visual search behavior in collaborative and competitive settings. Attention, Perception, & Psychophysics 81(3), 666–683. doi: 10.3758/s13414-018-01640-0`


To use Tobii integration, setup somewhere out of this tree:
clone https://github.com/Microsoft/vcpkg
git clone https://github.com/Microsoft/vcpkg.git

cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install

for Tobii SDK, you need to manually put the right files in the right place of the vcpkg directory:
```
Tobii_C_SDK\64\lib\tobii_research.dll -> vcpkg\installed\x64-windows\bin
Tobii_C_SDK\64\lib\tobii_research.dll -> vcpkg\installed\x64-windows\debug\bin
Tobii_C_SDK\64\lib\tobii_research.lib -> vcpkg\installed\x64-windows\lib
Tobii_C_SDK\64\lib\tobii_research.lib -> vcpkg\installed\x64-windows\debug\lib
Tobii_C_SDK\64\include\*              -> vcpkg\installed\x64-windows\include

Tobii_C_SDK\32\lib\tobii_research.dll -> vcpkg\installed\x86-windows\bin
Tobii_C_SDK\32\lib\tobii_research.dll -> vcpkg\installed\x86-windows\debug\bin
Tobii_C_SDK\32\lib\tobii_research.lib -> vcpkg\installed\x86-windows\lib
Tobii_C_SDK\32\lib\tobii_research.lib -> vcpkg\installed\x86-windows\debug\lib
Tobii_C_SDK\32\include\*              -> vcpkg\installed\x86-windows\include
```

if you wish to compile the python lib, further run:
vcpkg install boost-python boost-python:x64-windows
(for python2 support, edit the file ports\boost-python\CONTROL in the vcpkg directory to include python 2 instead of python 3. Later, a boost-python2 package may become available solving this)

For the SMI SDK, there is an error in their import lib for the 64 version. This repo comes with a fixed version, in the folder SMI64bitImportLib
