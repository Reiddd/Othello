# Othello
an Othello game in shell

# Usage
```
> g++ -O -Wall -std=c++17 -o othello othello.cpp -lcurses
> ./othello
```

w a s d to control the curse.
press space to place the piece.


at first, choose a mode
```                                                                                                           
                        Please choose a mode                                                                  
                              1. Human VS Human                                                             

                              2. Human VS PC                                                                                                                                              
                              Press 1 and 2 to choose or q to quit                                          
```


it will look like this in your shell (the reverse effect can't be shown in the introduction, try it by yourself)
```
+-----+-----+-----+-----+-----+-----+-----+-----+     now:   BLACK  
|     |     |     |     |     |     |     |     |  
|     |     |     |     |     |     |     |     |     BLACK: 7                                 
+-----+-----+-----+-----+-----+-----+-----+-----+                                                         
|     |     | + + |     |     |     |     |     |     WHITE: 7                                              
|     |     | + + |     |     |     |     |     |                                                           
+-----+-----+-----+-----+-----+-----+-----+-----+                                                           
|     |     |     | + + | + + | + + |     |     |                                                           
|     |     |     | + + | + + | + + |     |     |                                                           
+-----+-----+-----+-----+-----+-----+-----+-----+                                                           
|     |     |     | + + | + + | + + | + + | + + |                                                           
|     |     |     | + + | + + | + + | + + | + + |                                                           
+-----+-----+-----+-----+-----+-----+-----+-----+                                                           
|     |     |     | + + | + + | + + |     |     |                                                           
|     |     |     | + + | + + | + + |     |     |                                                           
+-----+-----+-----+-----+-----+-----+-----+-----+                                                           
|     |     | + + | + + |     |     |     |     |                                                           
|     |     | + + | + + |     |     |     |     |                                                           
+-----+-----+-----+-----+-----+-----+-----+-----+                                                           
|     |     |     |     |     |     |     |     |                                                           
|     |     |     |     |     |     |     |     |                                                           
+-----+-----+-----+-----+-----+-----+-----+-----+                                                           
|     |     |     |     |     |     |     |     |                                                           
|     |     |     |     |     |     |     |     |                                                           
+-----+-----+-----+-----+-----+-----+-----+-----+                                                           
```