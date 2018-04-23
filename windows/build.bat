cd windows
set PATH=%PATH%;C:\Qt\qt-5.5.0-x64-mingw510r0-seh-rev0\mingw64\bin;C:\Qt\qt-5.5.0-x64-mingw510r0-seh-rev0\qt-5.5.0-x64-mingw510r0-seh-rev0\bin
cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:\Qt\qt-5.5.0-x64-mingw510r0-seh-rev0 -DCMAKE_BUILD_TYPE=Release ..
mingw32-make
