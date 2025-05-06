# - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Name : Tammy Young
# SID : 1706229
# CCID : tqyoung
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

# Technologies
Language: C

# Design

## Part 1
One message sent between the client and the server takes the following format: `MARK <MOVE:%d>`. When the client receives this message, it reads it into a buffer, extracts the components, command and move, then makes the move on the client's copy of the board. Similarly, when the server receives this message, it reads it into a buffer, extracts the components, command and move, then makes the move on the server's copy of the board. Another message is `TIMEOUT -1`. This message is sent from the client to the server when the client is inactive for 30 seconds. After the client sends it, it closes, and after the server receives it, it closes and exits as well.

The server informs the clients who established a connection that the game can begin by showing the client, after it connects, a message that says it's their turn. I chose this method because that was in the sample output for this assignment.

For communicating initial game information, I show the client a message when they connect to the server that it's their turn, and the game board is assumed to be empty. I made this design choice because I followed the sample output for this assignment.

The client indicates the position they want to mark using the command `MARK <MOVE>`. I made this design choice because I followed the assignment description.

In my implementation, the server does not communicate the current state of the game board to the client. I made this design choice because I didn't see a reason to send the game board to the client when both the server and the client kept separate copies of the board, and it would be simpler to just send the move that was played and have that move played on both boards.

When someone makes an invalid move or tries to make a move on a marked position, an error message is shown that says `Must mark an unoccupied box on the board (1-9)`.

When a win/draw/loss happens, a message that says `You <won,lost,drew>` is shown. If the client disconnects, a message on the server is shown that says `Client disconnected or error receiving data`, and the server stops running. When the client is inactive for 30 seconds, the client is shown `Time limit exceeded! You lose.` and the server is shown `Client took too long to play. You won`, and both disconnect and exit.

## Part 2
The client sends the server `JOIN <room>` so the client can join a room. This design choice was made for me by the assignment spec. When the game starts, there is `WAIT` and `TURN` which tell the client whose turn it is. This is also used to confirm that the request to join a room was accepted and that another client has joined the room and they're ready to play. If they have `TURN` they can play a move. They need to acknoledge by sending `ACK` back to the server to say that they are ready to play. They send `MARK <move>` back to the server and the server send it to the opponent client as well. At the end of the game, the clients will get `WIN`, `LOST`, or `DRAW` to indicate how they did in the game.

# Citations
source: https://www.geeksforgeeks.org/posix-threads-in-os/
date: feb 10, 2025

source: ChatGPT
prompt: "give me an example for connecting a server to a client in C"
date: feb 9, 2025

source: ChatGPT
prompt: "how to shut down client processes when a server is shut down in C?"
date: feb 10, 2025

source: https://www.geeksforgeeks.org/socket-programming-cc/
date: feb 9, 2025
