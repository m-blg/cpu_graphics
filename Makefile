# -*- MakeFile -*-

Test1: Test1.cc
	g++ -g Test1.cc -o Test1 -lX11

drawing_test: drawing_test.cc
	g++ -g drawing_test.cc -o drawing_test -lX11
