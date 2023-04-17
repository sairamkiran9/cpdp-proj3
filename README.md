### Project 3: A Simple Chat Application

#### Please enable devtoolset 11 in linprog to make the code working
- command to enable devtoolset 11 `scl enable devtoolset-11 bash`

#### Makefile instructions
- `make` will create two binaries.
    - chat_server.x on the current directory.
    - chat_client.x inside bin folder of the current directory.

- `make clean` will remove chat_server.x and chat_client.x binaries.

#### Steps to execute
- <b>Server:</b> 
    - <b>Usage:</b> `./chat_server.x configration_file`
    - <b>Configuration file format:</b> \
        ```
        port: [port number]
        ```

- <b>Client:</b>
    - <b> Usage: </b> `./chat_client.x configuration_file`
    - <b> Configuration file format:</b> \
        ```
        servhost: [server ip]
        servport: [server port]
        ```

#### Method functionality details:
- <b>Server:</b>
    - The server uses pthread and select methods. 
    - After creating a socket and a connnection is establised to listen and accept new clients. The server model intitalizes a new thread. 
    - This new thread use select method to check the status of the file descriptor and than checks for new client connections followed by receiving of messages if any from each client.
    - The received message is parsed and than redirected as per the instructions.

- <b>Client:</b>
    - The client uses pthread method for reading messages from the server.
    - The client reads input messages from the user and redirects to the server. This is independent of the thread functionality (reading messages from the server).