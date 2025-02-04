# 1. Assignment Features:

## Implemented Features:

    - Client Authentication: Users are required to authenticate with a username and password before accessing the chat server.
    
    - Direct Messaging: Clients can send messages to specific users.

    - Broadcast Messaging: Clients can send a message to all active users except themselves.

    - Group Chat Functionality:

        - Create groups.

        - Join existing groups.

        - Leave groups.

        - Send messages within a group.

    Command Handling: Users can send various commands to interact with the chat system.

    Threaded Client Handling: Each client connection runs on a separate thread.

    Proper Synchronization: All shared resources are protected using mutex locks to avoid race conditions.

    Graceful Disconnection Handling: Clients who disconnect are removed from active lists, and groups update accordingly.

## Non-Implemented Features:

    Features like persistence storage which will store data even after disconnection of server/client could have been implemented

# 2. Design Decisions:

    Each client connection is handled on a separate thread to allow concurrent interactions. This is done rather than generating a new process altogether.

    Storage:
        1. A "vector<int> threads", for the client file descriptors
        2. A "map<string, int> sockets" which maps username with the client file descriptors
        3. A "map<int, set<int>> groups" which maps group names with all the file descriptors of the clients joined in that group
        4. An enum Commands is made for better access of the different commands, a "map<string, Commands> commandMap" is made for mapping each command string with its enum
        5. A "map<string, string> Users" containing user usernames and password.
    
    Rather than using the code given by Sir to parse the message and commands, we went by:
        1. Making a custom split function which will split a string into an array of delimiter(here space -> " ") seprated strings
        2. The first part of this string will always be the command. Now, to check is the command is valid, we only need to search for the 
            map commandMap to see if the command is valid.
        3. Rest all is sent to the individual functions that handle the specific commands. These functions now check for the others errors
            in the rest of the array and concatenates the required message again.
    
    Most of the functional routing based on the commands is done by a single function handleCommandRouting which just uses a switch statement based on 
    the various commands.

# 3. Some Design Rationale: 

Shared storage elements like sockets map, groups map and the commands map which every thread is using needed to be protected so 

problems like race conditions, possible deadlocks would not arrive. So functionalities like std::mutex and various checks needed 

to be performed in each function which used these shared resources.

## Group Membership Handling:

    Only Active Clients in Groups: When a client disconnects, they are removed from any groups they were part of.

## 4. Implementation:

## High-Level Design:

###    Important Functions:

        handle_client(int client_fd): Handles authentication and continuous message processing.

        Authenticate(int client_fd, string &username): Authenticates users before allowing them to chat.

        sendMessage(int &clientFd, string &message): Sends messages over the socket.

        recvMessage(int &client_fd, string &message): Receives messages from the client and handles disconnection if needed.

        broadcast(int neglectClient, vector<string> &argv): Broadcasts a message to all clients except the sender.

        sendIndividualMessage(int &sender_fd, vector<string> &argv): Sends a private message between two users.

        createGroup(int &client_fd, vector<string> &argv): Creates a new chat group.

        joinGroup(int &client_fd, vector<string> &argv): Adds a client to an existing group.

        leaveGroup(int &client_fd, vector<string> &argv): Removes a client from a group.

        groupMessage(int &sender_fd, vector<string> &argv): Sends a message to all members of a group.
    
    All these functions and more are explained in detail in the code.

###     Code Flow:

        1. A client connects to the server from the main function which accept all the incoming connections and creates a new thread for them.

        2. handle_client authenticates the client and procedes to listen to all the messages sent by the client.

        3. handle_client sends the incoming message to handleCommandRouting().

        4. Commands are processed through handleCommandRouting which routes to different functions based on the commands.

        5. Here, messages are based on the above implemented features like sendIndividualMessage or createGroup or groupMessage etc.

        6. When a client disconnects, disconnect is called to remove them from active structures.

        Please refer to the "code flow" image attached.


# 5. Testing Strategy:

## Correctness Testing:

    Sent and received messages between multiple clients.

    Verified group chat functionality.

    Checked handling of invalid commands.

## Stress Testing:

    Connected multiple clients simultaneously to observe performance.

    Sent high-frequency messages to test synchronization.

## Edge Case Testing:

    Attempted to send messages with empty content.

    Tried to join non-existent groups.

    Tested rapid client disconnections.

# 6. Server Restrictions:

    Max Clients: Limited by system resources and threading constraints.

    Max Groups: Limited by memory, but practically large.

    Max Members per Group: No explicit limit but dependent on server capacity.

    Max Message Size: Defined by BUFFER_SIZE (e.g., 1024 bytes per message).

# 7. Challenges Faced:

## Thread Safety Issues:

    Initial version had race conditions due to concurrent access to shared data structures.

    Solution: Introduced mutex locks around critical sections.

## Handling Disconnections Gracefully:

    Early versions crashed when clients disconnected unexpectedly.

    Solution: Proper error handling in recvMessage and disconnect functions.

## Efficient String Manipulation and Error Handling:

    Efficient string manipulation and its error handling, ensuring minimal processing overhead while sending direct messages, group messages, and broadcasts.

# 8. Individual Contributors:

    Aditya Gudadhe (210397) : Server Architecture, TCP server logic, multithreading and race condition handling, testing.

    Aryan Bharadwaj (210200) : Authentication, major Error Handling logic, command functions, function routing based on commands, documentation.

# 9. Sources:
    1. [TCP server logic](https://medium.com/@tharunappu2004/creating-a-simple-tcp-server-in-c-using-winsock-b75dde86dd39)
    
    2. [MultiThreading](https://www.geeksforgeeks.org/multithreading-in-cpp/)

    3. [C++ Reference](https://en.cppreference.com/w/)

# 10. Declaration:

    We hereby declare that this project was completed independently and without any unauthorized collaboration or plagiarism.

# 11. Feedback:

    The assignment was well-structured, allowing for learning about multi-threaded network programming.

    More clarification on expected outputs on specific commands and errors, contraints (e.g., maximum clients, message size) would be helpful.