# UDPMulticast
UDP Multicast library for C++, MATLAB and Python

Provides highspeed message sending and receiving through UDP Multicast
for Windows (using overlappedIO with completion ports). Its a C++ class,
MATLAB and Python wrappers are provided. Both 32bit and 64bit are
supported. Not all VS project settings are necessarily correct for all
configurations though


To use Tobii integration, setup somewhere out of this tree:
clone https://github.com/Microsoft/vcpkg
git clone https://github.com/Microsoft/vcpkg.git

cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install

for Tobii SDK, you need to manually put the right files in the right place of the vcpkg directory:
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


if you wish to compile the python lib, further run:
vcpkg install boost-python boost-python:x64-windows
(for python2 support, edit the file ports\boost-python\CONTROL in the vcpkg directory to include python 2 instead of python 3. Later, a boost-python2 package may become available solving this)