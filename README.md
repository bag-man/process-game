# Can you quit me?

The idea of this game is to try to make an application that is impossible to quit without an exit function being ran. The only way to win is to either play the game until the time limit is up, or to find a way to kill the process without the game knowing. If you see "Game over.", you lost!

I've been able to beat it with:

     sleep 3; kill -9 $(pgrep quitme)
   
Would love idea's on how to prevent that. 

Build with `make` and run with `./quitme`. 
