# This is the best balls game ever created ! Open-source project !
![image](https://github.com/user-attachments/assets/c9b5ef8c-aeaf-4232-a385-c2ef44451701)

The rules are simple: destroy the balls of the other player.

//
![image](https://github.com/user-attachments/assets/1a840a84-e909-4db3-9808-70a027c2fafc)
//
![image](https://github.com/user-attachments/assets/c099f2bc-8900-48e1-b03b-764cf8369ea5)



## Build & run

Requires SDL2 and SDL2_ttf (e.g. `sudo apt install libsdl2-dev libsdl2-ttf-dev`).

```
make
./clash_of_balls   # run from the project root (loads ./img and Jersey_Sharp.ttf)
```

You play the black balls: pick a card, then click in the arena to launch a ball
(right click cancels, cards come back after 5 s). The AI launches white balls.
The first side with no ball left loses. `Esc` quits.
