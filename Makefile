run:smake.o:compile a.out
	c++ smake.o -g -Wall -Wextra -fsanitize=address

smake.o:smake.cpp:compile smake.o
	c++ smake.cpp -c -o smake.o -g -Wall -Wextra -fsanitize=address

clean:
	rm -f smake.o a.out
