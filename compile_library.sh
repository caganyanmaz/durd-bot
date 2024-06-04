filename="main.cpp"
g++ -c -fPIC $filename -o a.o
g++ -shared -Wl,-soname,liba.so -o liba.so a.o