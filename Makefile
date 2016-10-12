all : tcs trs user

tcs : tcsDaemon.c
	gcc -std=gnu11 -Wall -pedantic tcsDaemon.c -o TCS

trs : trsDaemon.c
	gcc -std=gnu11 -Wall -pedantic trsDaemon.c -o TRS

user : user.c
	gcc -std=gnu11 -Wall -pedantic user.c -o user

clean :
	rm TCS TRS user languages.txt
