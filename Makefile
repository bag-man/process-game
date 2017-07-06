main: 
	gcc quitme.c -o quitme -std=c99 -lpthread -lX11 -Wall

clean:
	rm quitme game.over
