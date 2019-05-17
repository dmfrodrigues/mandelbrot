CC     ="C:\Program Files (x86)\CodeBlocks\MinGW\bin\mingw32-g++.exe"
IDIR   =-I"D:\_ProgrammingLibraries\wxWidgets-3.0.4\include" -I"D:\_ProgrammingLibraries\wxWidgets-3.0.4" -I"D:\_ProgrammingLibraries\wxWidgets-3.0.4\lib\gcc_dll_SHARED_RELEASE_MONOLITHIC_UNICODE\mswu"
CFLAGS =-std=c++11 -ffast-math -pipe -mthreads -D__GNUWIN32__ -D__WXMSW__ -DWXUSINGDLL -DwxUSE_UNICODE -O3 $(IDIR) -c

ODIR   =./build/obj
BDIR   =./build/bin

#LFLAGS =-L"D:\_ProgrammingLibraries\wxWidgets-3.0.4\lib\gcc_dll_SHARED_RELEASE_MONOLITHIC_UNICODE"
LFLAGS =-L"D:\_ProgrammingLibraries\wxWidgets-3.0.4\lib\gcc_dll_SHARED_RELEASE_MONOLITHIC_UNICODE"

all: $(BDIR)\main.exe

$(BDIR)\main.exe: $(ODIR)\Mandelbrot.o $(ODIR)\FractalFrame.o $(ODIR)\FractalPanel.o $(ODIR)\InfoPanel.o $(ODIR)\main.o
	$(CC) $(LFLAGS) -o $(BDIR)\main.exe $(ODIR)\Mandelbrot.o $(ODIR)\FractalFrame.o $(ODIR)\FractalPanel.o $(ODIR)\InfoPanel.o $(ODIR)\main.o -s -mthreads -lwxmsw30u -mwindows
$(ODIR)\Mandelbrot.o: Mandelbrot.cpp
	$(CC) $(CFLAGS) Mandelbrot.cpp   -o $(ODIR)\Mandelbrot.o
$(ODIR)\FractalFrame.o: FractalFrame.cpp
	$(CC) $(CFLAGS) FractalFrame.cpp -o $(ODIR)\FractalFrame.o
$(ODIR)\FractalPanel.o: FractalPanel.cpp
	$(CC) $(CFLAGS) FractalPanel.cpp -o $(ODIR)\FractalPanel.o
$(ODIR)\InfoPanel.o: InfoPanel.cpp
	$(CC) $(CFLAGS) InfoPanel.cpp    -o $(ODIR)\InfoPanel.o
$(ODIR)\main.o: main.cpp
	$(CC) $(CFLAGS) main.cpp         -o $(ODIR)\main.o
