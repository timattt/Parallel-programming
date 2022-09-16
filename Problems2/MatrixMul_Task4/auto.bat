echo "Start auto build and test"
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make.exe
cd ..
python Analyzer.py
rmdir /s /q build