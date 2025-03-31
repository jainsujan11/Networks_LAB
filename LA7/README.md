# Build and Run Instructions 
1) First compile the two files using the following command
    a) gcc cldp_server.c -o server
	b) gcc cldp_client.c -o client
2) Now open two terminals and run the executables 
    T1: ./client
    T2: ./server

# Assumptions 
1) Flow of server code: 
    a) Server is sending Hello 
    b) Server waits for Query
    c) Server sends the response
    d) Server sleeps for 10 sec and goes to (a)

2) Flow of Client code: 
    a) Waits for active nodes Hello 
    b) Asks for MetaData it needs by default is asks for everything you can change it by changing the reserve value
    c) Prints the response and goes to (a)