#include <bits/stdc++.h>
#include <thread>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include<filesystem>
#include <arpa/inet.h>

using namespace std;
namespace fs = std::filesystem;

#define BUFFER_SIZE 1024

int PORT;
unordered_map<string, string> Users;
unordered_map<string, int> sockets;
unordered_map<string, set<int>> groups;
vector<int> threads;
mutex client_mutex;
enum class Commands{
    MESSAGE = 0,
    BROADCAST = 1,
    CREATE_GROUP = 2,
    JOIN_GROUP = 3,
    MESSAGE_GROUP = 4,
    LEAVE_GROUP = 5
};
unordered_map<string, Commands> commandMap = {
    {"/msg", Commands::MESSAGE},
    {"/broadcast", Commands::BROADCAST},
    {"/create_group", Commands::CREATE_GROUP},
    {"/join_group", Commands::JOIN_GROUP},
    {"/group_msg", Commands::MESSAGE_GROUP},
    {"/leave_group", Commands::LEAVE_GROUP}
};


/*
    Random helpers for better code : start
*/

/**
 * @brief Splits a given string into a vector of substrings based on a delimiter.
 *
 * @param s The input string to be split.
 * @param key The delimiter character used to split the string.
 * @return vector<string> A vector containing the substrings after splitting.
 *
 * The function iterates over the input string `s` and extracts substrings 
 * separated by the delimiter `key`. It skips consecutive delimiters and 
 * ensures non-empty substrings are added to the result vector.
 */
vector<string> split(string &s, const char* key){
    vector<string> res;

    string temp = "";
    for(auto &it: s){
        if(it==*key){
            if(temp!="") res.push_back(temp);
            temp = "";
            continue;
        }
        temp += it;
    }
    if(temp!="") res.push_back(temp);
    return res;
}

/**
 * @brief Extracts the group name from command-line arguments and assigns it to `groupName`.
 *
 * @param argv A vector of strings representing command-line arguments.
 * @param groupName A reference to a string where the extracted group name will be stored.
 * @return int Returns 1 if the group name is successfully extracted, otherwise returns -1.
 *
 * The function checks if `argv` has at least two elements. If not, it returns -1.
 * Otherwise, it concatenates all elements from index 1 onwards (separated by spaces)
 * into a single string and assigns it to `groupName`, removing the trailing space.
 */
int getGroupname(vector<string> &argv, string &groupName){
    if(argv.size()<2) return -1;
    string temp = "";
    for(long unsigned int i=1;i<argv.size();i++){
        temp += (argv[i] + " ");
    }
    temp.pop_back();
    groupName = temp;
    return 1;
}

/**
 * @brief Reads a file containing user credentials and stores them in a global map.
 *
 * @param fileName The name of the file containing user credentials.
 * @return int Returns 1 if the file is successfully read, otherwise returns 2 if the file cannot be opened.
 *
 * The function attempts to open the specified file. If it fails, it prints an error message and returns 2.
 * It reads each line from the file, splits it using ':' as a delimiter, and stores the extracted key-value pairs
 * in the global `Users` map, where the first part (username) is the key and the second part (password) is the value.
 * Finally, it closes the file and returns 1.
 */
int getUsers(string &fileName){
    ifstream f(fileName);

    if (!f.is_open()) {
        cerr << "Error opening the file!";
        return 2;
    }
    string s;

    while (getline(f, s)){
        vector<string> creds = split(s, ":");
        Users[creds[0]] = creds[1];
    }

    f.close();
    return 1;
}

/*
    helpers : end
*/

/*
    helper functions for race condition handling : start
*/





/**
 * @brief Adds a new client to the sockets map in a thread-safe manner.
 *
 * @param client_fd A reference to the client's file descriptor.
 * @param username A reference to the client's username.
 *
 * The function locks `client_mutex` to ensure thread safety, then adds 
 * the client's file descriptor to the `sockets` map using the username as the key.
 */
void addNewClient(int &client_fd, string &username){
    lock_guard<mutex> lock(client_mutex);

    sockets[username] = client_fd;
}


/**
 * @brief Retrieves the username associated with a given client file descriptor.
 *
 * @param client_fd A reference to the client's file descriptor.
 * @param username A reference to a string where the retrieved username will be stored.
 * @return int Returns 1 if the username is found, otherwise returns -1.
 *
 * The function iterates over the `sockets` map to find the client file descriptor. 
 * If a match is found, the corresponding username is assigned to `username`, and the function returns 1. 
 * If no match is found, it returns -1.
 */
int getUsernameFromFD(int &client_fd, string &username){
    lock_guard<mutex> lock(client_mutex);

    for(auto it: sockets){
        if(it.second == client_fd) {
            username = it.first; 
            return 1;
        }
    }

    return -1;
}

/**
 * @brief Retrieves the client file descriptor associated with a given username.
 *
 * @param username A reference to the username.
 * @param client_fd A reference to an integer where the retrieved file descriptor will be stored.
 * @return int Returns 1 if the file descriptor is found, otherwise returns -1.
 *
 * The function checks if the username exists in the `sockets` map. If not, it returns -1. 
 * If the username is found, it retrieves the associated file descriptor and assigns it to `client_fd`, 
 * then returns 1.
 */
int getFDFromUsername(string &username, int &client_fd){
    lock_guard<mutex> lock(client_mutex);

    if(sockets.find(username)==sockets.end()) return -1;

    client_fd = sockets[username];

    return 1;
}


/**
 * @brief Checks if a command is valid and retrieves its corresponding `Commands` object.
 *
 * @param s A reference to the command string.
 * @param command A reference to a `Commands` object where the retrieved command will be stored.
 * @return int Returns 1 if the command is valid, otherwise returns -1.
 *
 * The function checks if the command string `s` exists in the `commandMap`. If not, it returns -1. 
 * If the command is found, it assigns the corresponding `Commands` object to the `command` parameter and returns 1.
 */
int checkCommandValidity(string &s, Commands &command){
    lock_guard<mutex> lock(client_mutex);

    if(commandMap.find(s) == commandMap.end()) return -1;

    command = commandMap[s];

    return 1;
}

/*
    helper functions: end
*/

/**
 * @brief Disconnects a client by closing the file descriptor and cleaning up associated data.
 *
 * @param client_fd The file descriptor of the client to be disconnected.
 *
 * The function closes the connection by calling `close()` on the client's file descriptor.
 * The username associated with the client is found and removed from the `sockets` map. 
 * The client is also removed from any groups they belong to. Finally, the entry in `sockets` for that username is erased.
 */
void disconnect(int client_fd){
    close(client_fd);

    lock_guard<mutex> lock(client_mutex);

    remove(threads.begin(), threads.end(), client_fd);

    string usernameToRemove = "";
    for(auto &it: sockets){
        if(it.second == client_fd){
            usernameToRemove = it.first;
        }
    }

    if(sockets.find(usernameToRemove)==sockets.end()) return;
    sockets.erase(usernameToRemove);

    //removes from all groups
    for(auto &it: groups){
        it.second.erase(client_fd);
    }
}

/**
 * @brief (Abstraction) Sends a message to a client through the given file descriptor.
 *
 * @param clientFd A reference to the client's file descriptor.
 * @param message A reference to the message string to be sent.
 *
 * The function sends the `message` to the client associated with the `clientFd` using the `send()` system call.
 */
void sendMessage(int &clientFd, string &message){
    send(clientFd, message.c_str(), message.size(), 0);
}


/**
 * @brief Receives a message from a client and stores it in the `message` string.
 *
 * @param client_fd A reference to the client's file descriptor.
 * @param message A reference to the string where the received message will be stored.
 * @return int Returns 1 if the message is successfully received, otherwise returns -1 if the client disconnects or an error occurs.
 *
 * The function attempts to receive a message from the client using the `recv()` system call. 
 * If no data is received (indicating the client disconnected), it calls the `disconnect()` function and returns -1.
 * If an error occurs during reception, it prints an error message and disconnects the client, returning -1.
 * If the message is successfully received, it stores the message in `message` and returns 1.
 */
int recvMessage(int &client_fd, string &message) {
    char buff[BUFFER_SIZE] = {0};
    int bytesReceived = recv(client_fd, buff, sizeof(buff) - 1, 0);

    //check for abrupt disconnection of the client
    if (bytesReceived == 0) {
        disconnect(client_fd);
        cout<<"client disconnected"<<endl;
        return -1;
    } else if (bytesReceived < 0) { //If the recv function gives any error
        perror("recv failed");
        disconnect(client_fd);
        return -1;
    }

    buff[bytesReceived] = '\0';
    message.assign(buff, bytesReceived);
    cout<<message<<endl;
    return 1;
}

/**
 * @brief Validates whether the given argument represents a valid integer port number.
 *
 * @param arg A pointer to a string representing the port number to be validated.
 * @return bool Returns true if the port number is valid (contains only digits), otherwise returns false.
 *
 * The function checks if every character in the string `arg` is a digit. If any character is not a digit,
 * the function returns false. If all characters are digits, it returns true, indicating a valid port number.
 */
bool validatePort(char *arg){
    int n = strlen(arg);

    for(int i=0;i<n;i++){
        if(!(arg[i]<='9' && arg[i]>='0')) return false;
    }
    return true;
}


/**
 * @brief Authenticates a client by verifying their username and password.
 *
 * @param client_fd The file descriptor of the client attempting to authenticate.
 * @param username A reference to a string where the authenticated username will be stored.
 * @return int Returns 1 if authentication is successful, otherwise returns -1.
 *
 * The function sends the prompts to the client to enter a username and password. It then receives the inputs and 
 * checks whether the username exists in the `Users` map and if the password matches. If authentication fails, 
 * it sends an error message and returns -1. Additionally, it checks if the username is already in use, and if so, 
 * returns -1. If authentication succeeds, it returns 1.
 */
int Authenticate(int client_fd, string &username){
    string password;

    string authPrompts = "Enter username: ";
    sendMessage(client_fd, authPrompts );
    if(recvMessage(client_fd,username)<0){
        perror("Error recieving the username");
        return -1;
    }

    authPrompts = "Enter password: ";
    sendMessage(client_fd, authPrompts);
    if(recvMessage(client_fd,password)<0){
        perror("Error: recieving the password");
        return -1;
    }

    //Just a check to remove the spaces in the back
    username = split(username, " ")[0];
    password = split(password, " ")[0];
    if(Users.find(username)==Users.end() || Users[username]!= password){
        authPrompts = "Authentication failed. \n";
        sendMessage(client_fd, authPrompts);
        return -1;
    }
    if(sockets.find(username)!=sockets.end()) return -1;

    return 1;
}

/*
    Command execution functions: start
*/


/**
 * @brief Sends a private message from one client to another.
 *
 * @param sender_fd A reference to the sender's file descriptor.
 * @param argv A reference to a vector of strings containing the command and message components.
 * @return int Returns 1 if the message is successfully sent, otherwise returns -1.
 *
 * The function retrieves the recipient's file descriptor using their username. It constructs the message by
 * concatenating the components of `argv` starting from index 2. It checks that the sender and recipient are not the same,
 * and that both the sender and recipient are valid. The message is prefixed with the sender's username and then sent to the recipient.
 */
int sendIndividualMessage(int &sender_fd, vector<string> &argv){
    if(argv.size()<3) return -1;

    string recvUsername = argv[1], message = "";
    int recv_fd;
    if(getFDFromUsername(recvUsername, recv_fd)<0) return -1;

    for(long unsigned int i=2;i<argv.size();i++){
        message += (argv[i] + " ");
    }

    string senderUsername;
    if(getUsernameFromFD(sender_fd, senderUsername)<0) return -1;
    if(sender_fd == recv_fd) return -1;

    message = "[" + senderUsername + "]: " + message;

    sendMessage(recv_fd, message);

    return 1;
}
/*
*  Functional Overloading for the broadcast function, first is the
*  main function that is called when command is called,
*  the second is for commond messages such as 'x joined the chat'
*  There is no requirement for the two to be different but we just 
*  felt like it does the same thing so we named them the same.
*/

/**
 * @brief Broadcasts a message to all clients except the sender.
 *
 * @param neglectClient The file descriptor of the client whose message should be excluded from the broadcast.
 * @param argv A reference to a vector of strings containing the broadcast message components.
 * @return int Returns 1 if the message is successfully broadcasted, otherwise returns -1.
 *
 * The function constructs the broadcast message by concatenating the components of `argv` starting from index 1.
 * It retrieves the username associated with the sender's file descriptor and sends the message to all clients in the `sockets` map, 
 * excluding the sender. The message is prefixed with the sender's username.
 */
int broadcast(int neglectClient, vector<string> &argv){
    if(argv.size()<2) return -1;
    lock_guard<mutex> lock(client_mutex);

    string message = "";
    for(long unsigned int i=1;i<argv.size();i++){
        message += (argv[i] + " ");
    }

    string username = "";
    for(auto &it: sockets){
        if(it.second == neglectClient){
            username = it.first;
        }
    }

    for(auto &it : sockets){
        if(it.second == neglectClient) continue;

        int client_fd = it.second;

        string s = "[Broadcast from " + username + "]: " + message;

        sendMessage(client_fd, s);
    }

    return 1;
}


/**
 * @brief Broadcasts a message to all clients except the sender.
 *
 * @param message A reference to the message string to be broadcasted.
 * @param neglectClient A reference to the file descriptor of the client whose message should be excluded from the broadcast.
 * @return int Returns 1 if the message is successfully broadcasted, otherwise returns -1.
 *
 * The function retrieves the username associated with the sender's file descriptor and sends the provided 
 * `message` to all clients in the `sockets` map, excluding the sender. The message is prefixed with the sender's username.
 */
int broadcast(string &message, int &neglectClient){
    lock_guard<mutex> lock(client_mutex);

    string username = "";
    for(auto &it: sockets){
        if(it.second == neglectClient){
            username = it.first;
        }
    }

    for(auto it : sockets){
        if(it.second == neglectClient) continue;

        int client_fd = it.second;

        string s = username + " " + message;

        sendMessage(client_fd, s);
    }

    return 1;
}


/**
 * @brief Creates a new group.
 *
 * @param client_fd A reference to the file descriptor of the client creating the group.
 * @param argv A reference to a vector of strings containing the group name.
 * @return int Returns 1 if the group is successfully created, otherwise returns -1.
 *
 * The function extracts the group name from `argv` and checks if it already exists in the `groups` map.
 * If the group does not exist, it creates a new entry with the creator's file descriptor as the first member.
 * A confirmation message is then sent to the creator.
 */
int createGroup(int &client_fd, vector<string> &argv){
    lock_guard<mutex> lock(client_mutex);

    string groupName; 
    if(getGroupname(argv, groupName)<0) return -1;
    if(groups.find(groupName)!=groups.end()) return -1;

    set<int> st;
    st.insert(client_fd);
    
    groups[groupName] = st;

    string finalMessage = "Group " + groupName + " Created.";
    sendMessage(client_fd, finalMessage);

    return 1;
}


/**
 * @brief Adds a client to an existing group.
 *
 * @param client_fd A reference to the file descriptor of the client joining the group.
 * @param argv A reference to a vector of strings containing the group name.
 * @return int Returns 1 if the client successfully joins the group, otherwise returns -1.
 *
 * The function extracts the group name from `argv` and checks if the group exists.
 * If the group exists and the client is not already a member, the client's file descriptor is added to the group.
 * A confirmation message is then sent to the client.
 */
int joinGroup(int &client_fd, vector<string> &argv){
    lock_guard<mutex> lock(client_mutex);

    string groupName; 
    if(getGroupname(argv, groupName)<0) return -1;

    if(groups.find(groupName)==groups.end()) return -1;
    if(groups[groupName].find(client_fd)!=groups[groupName].end()) return -1;

    groups[groupName].insert(client_fd);

    string finalMessage = "You joined the group " + groupName + ".";
    sendMessage(client_fd, finalMessage);

    return 1;
}

/**
 * @brief Removes a client from a specified group.
 *
 * @param client_fd A reference to the file descriptor of the client leaving the group.
 * @param argv A reference to a vector of strings containing the group name.
 * @return int Returns 1 if the client successfully leaves the group, otherwise returns -1.
 *
 * The function extracts the group name from `argv` and checks if the group exists.
 * If the group exists and the client is a member, the client's file descriptor is removed from the group.
 * A confirmation message is then sent to the client.
 */
int leaveGroup(int &client_fd, vector<string> &argv){
    lock_guard<mutex> lock(client_mutex);

    string groupName; 
    if(getGroupname(argv, groupName)<0) return -1;
    if(groups.find(groupName)==groups.end()) return -1;
    if(groups[groupName].find(client_fd)==groups[groupName].end()) return -1;

    groups[groupName].erase(client_fd);

    string finalMessage = "You left the group " + groupName + ".";
    sendMessage(client_fd, finalMessage);

    return 1;
}


/**
 * @brief Sends a message to all members of a specified group except the sender.
 *
 * @param sender_fd A reference to the file descriptor of the client sending the message.
 * @param argv A reference to a vector of strings containing the group name and the message.
 * @return int Returns 1 if the message is successfully sent, otherwise returns -1.
 *
 * The function extracts the group name from `argv` and checks if the group exists.
 * If the sender is a member of the group, the message is prefixed with the group name and broadcasted to all group members except the sender.
 */
int groupMessage(int &sender_fd, vector<string> &argv){
    if(argv.size()<3) return -1;

    lock_guard<mutex> lock(client_mutex);

    string groupName = argv[1];
    if(groups.find(groupName)==groups.end()) return -1;
    if(groups[groupName].find(sender_fd)==groups[groupName].end()) return -1;

    string message = "";
    for(long unsigned int i=2;i<argv.size();i++){
        message += (argv[i] + " ");
    }
    message = "[Group " + groupName + "]: " + message;


    for(auto recv_fd: groups[groupName]){
        if(recv_fd == sender_fd) continue;
        sendMessage(recv_fd, message);
    }

    return 1; 
}

/*
    Command execution functions: end
*/


/**
 * @brief Handles errors by sending an error message to the client if a command fails.
 *
 * @param client_fd A reference to the file descriptor of the client receiving the error message.
 * @param returnValue The return value of the executed command.
 * @param errMessage A constant character pointer to the error message to be sent.
 *
 * If `returnValue` is negative, the function converts `errMessage` to a string and sends it to the client.
 * Otherwise, it does nothing.
 */
void handleCommandFunctions(int &client_fd, int returnValue, const char* errMessage){
    if(returnValue>=0) return;
    string err = errMessage;
    sendMessage(client_fd, err);
    return;
}

/**
 * @brief Routes incoming commands from the client to the appropriate handler function.
 *
 * @param client_fd A reference to the file descriptor of the client sending the command.
 * @param incoming A reference to the string containing the client's command.
 * @return int Returns 1 if the command is successfully processed, otherwise returns -1.
 *
 * The function parses the incoming command string, checks if it's valid, and calls the corresponding handler function.
 * If the command is invalid, an error message is sent back to the client.
 * The function handles different types of commands such as direct messages, broadcasting, group creation, group joining, 
 * leaving a group, and sending group messages.
 */
int handleCommandRouting(int &client_fd, string &incoming){
    if(incoming.size()<1) return -1;
    vector<string> parsedString = split(incoming, " ");
    Commands command;
    if(parsedString.size()<1 || checkCommandValidity(parsedString[0],command)<0){
        string err = "Error: Invalid: Check and type the valid command";
        sendMessage(client_fd, err);
        return -1;
    }

    switch(command){
        case Commands::MESSAGE:
            handleCommandFunctions(
                client_fd,
                sendIndividualMessage(client_fd, parsedString),
                "Error: Check reciever name or message and try again"
            );
            break;
        case Commands::BROADCAST:
            handleCommandFunctions(
                client_fd,
                broadcast(client_fd, parsedString),
                "Error: Check message and try again"
            );
            break;
        case Commands::CREATE_GROUP:
            handleCommandFunctions(
                client_fd,
                createGroup(client_fd, parsedString),
                "Error: Check if group already exists and try again"
            );
            break;
        case Commands::JOIN_GROUP:
            handleCommandFunctions(
                client_fd,
                joinGroup(client_fd, parsedString),
                "Error: Check if group name already exist and try again"
            );
            break;
        case Commands::LEAVE_GROUP:
            handleCommandFunctions(
                client_fd,
                leaveGroup(client_fd, parsedString),
                "Error: Check if group name exists and try again"
            );
            break;
        case Commands::MESSAGE_GROUP:
            handleCommandFunctions(
                client_fd,
                groupMessage(client_fd, parsedString),
                "Error: Check group name or message and try again"
            );
            break;
        default:
            break;
    }

    return 1;
}


/**
 * @brief Handles communication with a connected client.
 *
 * @param client_fd The file descriptor for the client connection.
 *
 * The function first authenticates the client, adds them to the active client list, and sends a welcome message.
 * It then continuously listens for incoming messages and processes them by routing commands or broadcasting messages.
 * If the client disconnects or encounters an error, the function ensures proper cleanup.
 */
void handle_client(int client_fd) {
    string username = "";
    if(Authenticate(client_fd, username) == -1){
        disconnect(client_fd);
        return;
    }

    addNewClient(client_fd, username);

    string message = "Welcome to the chat server !";
    sendMessage(client_fd, message);
    message = "has joined the chat.";
    broadcast(message, client_fd);

    string incoming;
    while(true){
        if(recvMessage(client_fd, incoming)<0) {
            disconnect(client_fd);
            return;
        }
        
        if(handleCommandRouting(client_fd, incoming)<0) continue; 
    }
}

int main(int argc, char *argv[]) {

    if(argc==1 || !validatePort(argv[1])){
        cout<<"Error: PORT number not mentioned or Invalid Port";
        return 2;
    }

    PORT = atoi(argv[1]);

    string usersFilePath = "users.txt";
    if(getUsers(usersFilePath)==2){
        perror("Cannot convert users");
        return 2;
    }

    int server_fd;
    struct sockaddr_in address;
    

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    

    if (listen(server_fd, 15) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    std::cout << "Server is listening on port " << PORT << "...\n";
    
    
    
    while (true) {
        int client_fd;
        sockaddr_in clientAddr;
        socklen_t client_addr_len = sizeof(clientAddr);

        if ((client_fd = accept(server_fd, (struct sockaddr*)&clientAddr, &client_addr_len)) < 0) {
            perror("Accept failed");
            continue;
        }
        {
            lock_guard<mutex> lock(client_mutex);
            threads.push_back(client_fd);
        }

        thread client_thread(handle_client, client_fd);
        client_thread.detach();
    }
    
    
    close(server_fd);
    return 0;
}
