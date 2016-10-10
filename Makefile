all : tcs trs user

tcs : tcsDaemon.c
	gcc tcsDaemon.c -o TCS

trs : trsDaemon.c
	gcc trsDaemon.c -o TRS

user : user.c
	gcc user.c -o user

clean :
	rm TCS TRS user
