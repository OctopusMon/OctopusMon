
c++ -O3 -Wall -shared -std=c++11 -fPIC $(python3 -m pybind11 --includes) ../C/dleft/Sketches_dleft.cpp -o ../C/dleft/Sketches$(python3-config --extension-suffix)
c++ -O3 -Wall -shared -std=c++11 -fPIC $(python3 -m pybind11 --includes) ../C/sumax/Sketches_sumax.cpp -o ../C/sumax/Sketches$(python3-config --extension-suffix)
c++ -O3 -Wall -shared -std=c++11 -fPIC $(python3 -m pybind11 --includes) ../C/marble/Sketches_marble.cpp -o ../C/marble/Sketches$(python3-config --extension-suffix)


