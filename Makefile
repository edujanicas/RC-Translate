all : tcs trs user

tcs : tcsDaemon.c
	gcc -std=gnu11 tcsDaemon.c -o TCS

trs : trsDaemon.c
	gcc -std=gnu11 trsDaemon.c -o TRSfolder/TRS

user : user.c
	gcc -std=gnu11 user.c -o user

clean :
	rm TCS TRSfolder/TRS user languages.txt
