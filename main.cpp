#include <iostream>
#include <string>
#include <cstdio>
#include <cstring>
#include <vector>
#include <stack>
#include <algorithm>
#include <iomanip>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

// Clears console
#define clearTerminal() std::cout << "\033[H\033[2J\033[3J";

// Moves cursor position
#define gotoxy(x,y) printf("\033[%d;%dH", (x), (y))
#define MOVE_BACK cout<<"\033["<<1<<"D"
#define ERASE_LINE cout<<"\033[2K"

#define KEY_UP 72
#define KEY_DOWN 80
#define KEY_LEFT 75
#define KEY_RIGHT 77

#define FIRST 6

using namespace std;

// Stores the contents of the current directory
vector<string> contents;

// Stores the name of the current directory
string cwd;
// Stores the name of the current directory in command mode
string cmdCwd;
string home;

struct termios orig_termios;
struct termios raw;
struct winsize window;

// Points to first and last file of the window
unsigned int first = FIRST;
unsigned int last = 0;
unsigned int windowSize = 0;
unsigned int cursor = FIRST;
int commandLocation;

stack<string> st;
stack<string> rightStack;

// Stores the cwd name in cwdPath char array
void getDirPath(){
    // Returns NULL if error
    char cwdPath[PATH_MAX];

    if(getcwd(cwdPath, PATH_MAX) == NULL)
        perror("getcwd() error");
    cwd = cwdPath;
}

void getDirContents(string path, vector<string> &contents){
    // Clearing the contents vector
    contents.clear();

    // Opens the directory with cwdPath
    DIR *cwd = opendir(&path[0]);
    struct dirent *files;
    // Reading the contents of the directory
    while((files = readdir(cwd)) != NULL){
        string name = files->d_name;
        contents.push_back(name);
    }
    // Closing the directory
    closedir(cwd);
    // Storing the contents in sorted order
    sort(contents.begin(), contents.end());
}

void printMeta(string mode){
    gotoxy(0,0);
    ioctl(0, TIOCGWINSZ, &window);

    cout<<"\033[1;36mFeX - Linux File Explorer\033[0m"<<endl;
    cout<<"Developed by: ";
    cout<<"\033[1;32mYaswanth Haridasu\033[0m"<<endl;

    for(int i=0; i<window.ws_col; i++){
        cout<<"=";
    }
    cout<<"\033[1;33m** "<<mode<<" **\033[0m ";
    if(mode == "Command Mode"){
        cout<<"(Press Esc to enter Normal mode, q to exit)"<<endl;
    }
    else{
        cout<<"(Press : to enter Command mode, q to exit)"<<endl;
    }
    for(int i=0; i<window.ws_col; i++){
        cout<<"=";
    }
}

void printContents(int s, int e){
    clearTerminal();
    gotoxy(FIRST, 0);
    e = min((int)contents.size(), e);
    for(int i=s; i<e; i++){
        struct stat results;  
        string fileName = contents[i];
        stat(&fileName[0], &results);    
        // Printing file name
        int bold = 0;
        int color = 97;
        if(results.st_mode & S_IFDIR){
            color = 34; 
            bold = 1;
        }
        if(fileName.length() > 20){
            fileName = fileName.substr(0,20);
            fileName += "..";
        }
        // cout<< setw(26) << left ;
        printf("\033[%d;%dm%-24s\033[0m ", bold, color, &fileName[0]);    
        // cout<<"\033[1;32mYaswanth Haridasu\033[0m"<<endl;

        double size = results.st_size;
        int j = 0;
        const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
        char buf[PATH_MAX];
        while (size > 1024) {
            size /= 1024;
            j++;
        }
        sprintf(buf, " %.*f %s",j, size, units[j]);
        cout<< setw(10) <<right<<buf<<" ";
        string permissions;
        results.st_mode & S_IFDIR ? permissions += "d" : permissions += "-"; 
        results.st_mode & S_IRUSR ? permissions += "r" : permissions += "-";
        results.st_mode & S_IWUSR ? permissions += "w" : permissions += "-";
        results.st_mode & S_IXUSR ? permissions += "x" : permissions += "-";
        results.st_mode & S_IRGRP ? permissions += "r" : permissions += "-";
        results.st_mode & S_IWGRP ? permissions += "w" : permissions += "-";
        results.st_mode & S_IXGRP ? permissions += "x" : permissions += "-";
        results.st_mode & S_IROTH ? permissions += "r" : permissions += "-";
        results.st_mode & S_IWOTH ? permissions += "w" : permissions += "-";
        results.st_mode & S_IXOTH ? permissions += "x" : permissions += "-";

        printf("%-11s", &permissions[0]);
        struct passwd *pw = getpwuid(results.st_uid);
        struct group  *gr = getgrgid(results.st_gid);
        if(pw != 0){
            cout<<pw->pw_name<<" ";
        }
        if(gr != 0){
            cout<<gr->gr_name<<" ";
        }
        char time[50];
        // strftime(time, 50, "%Y-%m-%d %H:%M", localtime(&results.st_mtime));
        time_t modTime = results.st_mtime;
        struct tm localTime;
        localtime_r(&modTime, &localTime);
        strftime(time, sizeof(time), "%c", &localTime);
        cout<<setw(20)<< left<< time;
        if(s != e-1)
            cout << endl;
    }
    // cout<<cwd;
}

void disableRawMode(){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void splitString(string &command, string &commandType, vector<string> &files){
    // Removing trailing spaces in the command
    while(command[command.length()-1] == ' '){
        command.pop_back();
    }
    // Removing leading whitespaces in the command
    while(command[0] == ' '){
        command = command.substr(1);
    } 

    bool foundCommand = false;
    string fileName;
    bool quotes = false;
    for(int i=0; i<command.length(); i++){
        if(!foundCommand){
            if(command[i] == ' '){
                foundCommand = true;
                continue;
            }
            commandType += command[i];
        }
        else{
            if(command[i] == ' ' && !quotes){
                if(fileName.length() != 0)
                    files.push_back(fileName);

                fileName = "";
            }
            else if(command[i] == '\''){
                quotes = !quotes;    
            }
            else
                fileName += command[i];
        }
    }
    // Pushing the last file present in the fileName
    files.push_back(fileName);
}

void printMessage(string msg){
    gotoxy(commandLocation-3, 0);
    for(int i=0; i<window.ws_col; i++)
        cout<<"=";
    gotoxy(commandLocation-2, 0);
    ERASE_LINE;
    cout<<msg<<endl;
    for(int i=0; i<window.ws_col; i++)
        cout<<"=";
    gotoxy(commandLocation, 0);
}

// TODO: Check for isFile
bool isDirectory(string path){
    DIR *dir = opendir(&path[0]);
    /* Directory exists. */
    if (dir) {
        closedir(dir);
        return true;
    }
    /* Directory does not exist. */
    else if (ENOENT == errno) {
        // printMessage("ERROR: Directory doesnot exist");
        return false;
    }
    /* opendir() failed for some other reason. */
    else {
        // printMessage("ERROR: Not a Directory");
        return false;
    }
}

bool search(string searchFile, string destination){
    chdir(&destination[0]);

    DIR *dstDir = opendir(&destination[0]);
    struct dirent *files;
    // Reading the contents of the directory
    while((files = readdir(dstDir)) != NULL){
        string name = files->d_name;
        if(name == searchFile){
            // cout<<"YUP"<<endl;
            closedir(dstDir);
            chdir(&cwd[0]);
            return true;
        }
    }
    closedir(dstDir);
    // Going back to the current directory
    chdir(&cwd[0]);
    return false;
}

string absolutePath(string path){
    string absPath;
    if(path.length() == 1 && path[0] == '.'){
        return cmdCwd;
    }
    if(path[0] == '~' && path[1] == '/'){
        path = path.substr(1);
        absPath = home + path;
    }
    else{
        // ERASE_LINE;
        absPath = cmdCwd + '/' + path;
    }
    return absPath;
}

void createDir(string source, string destination){
    if(isDirectory(destination)){
        // Changing from current to the destination directory
        chdir(&destination[0]);
        if (mkdir(&source[0], 0777) == -1){
            printMessage("ERROR: Failed to create directory");
        }
        // Going back to the current directory
        chdir(&cmdCwd[0]);
    }
}

void createFile(string source, string destination){
    if(isDirectory(destination)){
        chdir(&destination[0]);
        if(open(&source[0], O_CREAT | O_RDONLY, 0) != -1){
            // Giving all permissions to user and read, execute permissions for group and other users
            chmod(&source[0], S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
        }
        else{
            printMessage("ERROR: Failed to create file");
        }
        // Going back to the current directory
        chdir(&cmdCwd[0]);
    }
}

void goToPath(string path){
    if(isDirectory(path)){
        cmdCwd = path;
        chdir(&cmdCwd[0]);
    }
}

void deleteFile(string path){
    // Extracting the file name from it
    string fileName;
    while(path.back() != '/'){
        fileName += path.back();
        path.pop_back();
    }
    path.pop_back();
    reverse(fileName.begin(), fileName.end());
    // Checking if the file exists or not
    if(search(fileName, path)){
        path += '/' + fileName;
        // Checking if it is file or not
        if(!isDirectory(path)){
            if(remove(&path[0]) != 0){
                printMessage("ERROR: File deletion failed");
            }
            else{
                printMessage("RESULT: File Deleted Successfully");
            }
        }
        else{
            printMessage("Result: Not a file");
        }
    }
    else{
        printMessage("ERROR: File doesnot exist");
    }
}

void deleteFolder(vector<string> files){
    for(int i=0; i<files.size(); i++){
        if(files[i] == "." || files[i] == "..")
            continue;
        string path = files[i];
        // Directory delete
        if(isDirectory(path)){
            // Get all the contents present in the files
            vector<string> dirContents;
            getDirContents(path, dirContents);
            // Making all the names of the retireved folders absolute path
            for(int j=0; j<dirContents.size(); j++){
                if(dirContents[j] == "." || dirContents[j] == "..")
                    continue;
                dirContents[j] = path + "/" + dirContents[j];
            }
            deleteFolder(dirContents);
            rmdir(&path[0]);
        }
        // File Delete
        else{
            if(remove(&path[0]) != 0){
                printMessage("ERROR: File deletion failed");
            }
        }
    }
}

void copyFile(string source, string destination){
    char buff[BUFSIZ];
    FILE* src = fopen(&source[0], "r");
    FILE* dest = fopen(&destination[0], "w");
    size_t size;
    // Copying the contents
    while((size = fread(buff, 1, BUFSIZ, src)) > 0) {
        fwrite(buff, 1, size, dest);
    }
    // Copying the permissions
    struct stat st;
    stat(&source[0], &st);
    chmod(&destination[0], st.st_mode);
    fclose(src);
    fclose(dest);
}

void copyContents(vector<string> files, string destination){
    for(int i=0; i<files.size(); i++){
        if(files[i] == "." || files[i] == "..")
            continue;
        string path = files[i];
        // Directory copy
        if(isDirectory(path)){
            // Extracting folder name
            string folderName;
            int e = path.size()-1;
            while(path[e] != '/'){
                folderName += path[e];
                e--;
            }
            reverse(folderName.begin(), folderName.end());
            // Creating the folder at destination
            createDir(folderName, destination);
            // Moving inside that folder
            string newDest = destination + "/" + folderName;
            // Copying the permissions
            struct stat st;
            stat(&path[0], &st);
            chmod(&newDest[0], st.st_mode);

            // Get all the contents present in the files
            vector<string> dirContents;
            getDirContents(path, dirContents);
            // Making all the names of the retireved folders absolute path
            for(int j=0; j<dirContents.size(); j++){
                if(dirContents[j] == "." || dirContents[j] == "..")
                    continue;
                dirContents[j] = path + "/" + dirContents[j];
            }
            copyContents(dirContents, newDest);
        }
        // File copy
        else{
            // Extracting filename
            string fileName;
            int e = path.size()-1;
            while(path[e] != '/'){
                fileName += path[e];
                e--;
            }
            reverse(fileName.begin(), fileName.end());
            copyFile(path, destination + "/" + fileName);
        }
    }
}

// Excutes the command present in the commandType on the files
void executeCommand(string &commandType, vector<string> &files){
    printMessage(" ");
    if(commandType == "create_file"){
        if(files.size() != 2){
            printMessage("ERROR: Two parameters required: File name and destination");
        }
        else{
            // Expanding to absolute path
            string destination = absolutePath(files[1]);
            createFile(files[0], files[1]);
            printMessage("RESULT: Created file successfully");
        }
    }
    else if(commandType == "create_dir"){
        if(files.size() != 2){
            printMessage("ERROR: Two parameters required: Directory name and destination");
        }
        else{
            // Expanding to absolute path
            string destination = absolutePath(files[1]);
            createDir(files[0], files[1]);
            printMessage("RESULT: Created directory successfully");
        }
    }
    else if(commandType == "search"){
        if(files.size() != 1){
            printMessage("ERROR: One file/folder name required to search in current directory");
        }
        else{
            string path = absolutePath(files[0]);
            if(search(path, cmdCwd)){
                printMessage("RESULT: True");
            }
            else{
                printMessage("RESULT: False");
            }
        }
    }
    else if(commandType == "goto"){
        if(files.size() != 1){
            printMessage("ERROR: One paramater required");
        }
        else{
            // Expanding if absolute path
            string path = absolutePath(files[0]);
            goToPath(path);
            printMessage("SUCCESS: Current directory updated to " + files[0]);
        }
    }
    else if(commandType == "pwd"){
        printMessage("RESULT: " + cmdCwd);
    }
    else if(commandType == "rename"){
        if(files.size() != 2){
            printMessage("ERROR: Two parameters required");
        }
        else{
            string oldName = absolutePath(files[0]);
            string newName = absolutePath(files[1]);
            if(rename(&oldName[0], &newName[0]) != 0){
                printMessage("ERROR: Rename failed");
            }
            else{
                printMessage("RESULT: Renamed file successfully");
            }
        }
    }
    else if(commandType == "delete_file"){
        if(files.size() != 1){
            printMessage("ERROR: Only File Path required");
        }
        else{
            // Finding the absolute path of that file
            string path = absolutePath(files[0]);
            deleteFile(path);
        }
    }
    else if(commandType == "delete_dir"){
        if(files.size() != 1){
            printMessage("ERROR: Only Directory Path required");
        }
        else{
            string path = absolutePath(files[0]);
            if(!isDirectory(path)){
                printMessage("ERROR: Not a Directory");
            }
            else{
                vector<string> dirContents;
                getDirContents(path, dirContents);
                // Only two directories '.' and '..'
                if(dirContents.size() == 2){
                    rmdir(&path[0]);
                }
                else{
                    for(int i=0; i<dirContents.size(); i++){
                        if(dirContents[i] == "." || dirContents[i] == "..")
                            continue;
                        dirContents[i] = path + "/" + dirContents[i];
                    }
                    deleteFolder(dirContents);
                    rmdir(&path[0]);
                }
                printMessage("RESULT: Deleted Directory successfully");
            }
        }
    }
    else if(commandType == "copy"){
        if(files.size() < 2){
            printMessage("ERROR: Atleast two parameters required: source and destination");
        }
        else{
            // Extracting destination from the files in the command
            string destination = files.back();
            files.pop_back();
            // Absolute Path of the directory location
            destination = absolutePath(destination);
            // Destination is not a directory 
            if(!isDirectory(destination)){
                printMessage("ERROR: Destination directory doesnot exist");
            }
            else{
                for(int i=0; i<files.size(); i++){
                    files[i] = absolutePath(files[i]);
                }
                copyContents(files, destination);
                printMessage("RESULT: Copied files successfully");
            }
        }
    }
    else if(commandType == "move"){
        if(files.size() < 2){
            printMessage("ERROR: Atleast two parameters required: source and destination");
        }
        else{
            // Extracting destination from the files in the command
            string destination = files.back();
            files.pop_back();
            // Absolute Path of the directory location
            destination = absolutePath(destination);
            if(!isDirectory(destination)){
                printMessage("ERROR: Destination directory doesnot exist");
            }
            else{
                for(int i=0; i<files.size(); i++){
                    files[i] = absolutePath(files[i]);
                }
                copyContents(files, destination);
                deleteFolder(files);
                printMessage("RESULT: Movied files successfully");
            }
        }
    }
    else{
        printMessage("ERROR: Command doesn't exist");
    }
}

void commandMode(){

    cmdCwd = cwd;

    ioctl(0, TIOCGWINSZ, &window);
    windowSize = window.ws_row;

    printMeta("Command Mode");

    commandLocation = windowSize;
    gotoxy(commandLocation, 0);
    
    char key[3];
    memset(key, 0, sizeof(key));

    // Stores the entered command
    string command;

    while(true){
        fflush(0);
        memset(key, 0, sizeof(key));

        // Read nothing
        if(read(STDIN_FILENO, key, 3) == 0)
            continue;
        // cout<<"key[0]: "<<(int)key[0]<<" key[1]: "<<(int)key[1]<<" key[2]: "<<(int)key[2]<<endl;

        // ARROW Keys
        if(key[0] == 27 && key[1] == 91 && (key[2] == 65 || key[2] == 66 || key[2] == 67 || key[2] == 68)){
            continue;
        }
        // ESC key
        if(key[0] == 27)
            break;
        // BACKSPACE key
        else if(key[0] == 127){
            if(command.length() > 0){
                command.pop_back();
                MOVE_BACK;
                cout<<" ";
                MOVE_BACK;
            }
        }
        // ENTER key
        else if(key[0] == 10){
            // Stores the type of command
            string commandType;
            // Stores paths of all the files/folders entered in the command
            vector<string> files;
            // Extracts the type of the command and files/directories separately from the given command; 
            splitString(command, commandType, files);

            executeCommand(commandType, files);

            ERASE_LINE;
            gotoxy(commandLocation, 0);
            command.clear();
        }
        else if(key[0] == 'q' && command.length() == 0){
            clearTerminal();
            disableRawMode();
            exit(0);
        }
        else{
            cout<<key[0];
            command.push_back(key[0]);
        }
    }
}

void normalMode() {
    // Storing the original attrributes in orig_termios struct
    tcgetattr(STDIN_FILENO, &orig_termios);

    raw = orig_termios;

    // Turn off Echoing
    raw.c_lflag &= ~(ECHO);
    // Turn off canonical mode: Reads byte by byte
    raw.c_lflag &= ~(ICANON);
    // Turn off ctrl-c(terminate) and ctrl-z(suspend) signals:  
    raw.c_lflag &= ~(ISIG);
    // Turn off ctrl-v signal: Waits for you to type another character
    raw.c_lflag &= ~(IEXTEN);
    // Disable ctrl-s(stops data from being transmitted to the terminal until ctrl-q is pressed)
    raw.c_iflag &= ~(IXON);

    // Setting atributes
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    // Retrieving window size
    ioctl(0, TIOCGWINSZ, &window);
    windowSize = window.ws_row;
    last = windowSize-3;

    // Stores the dir path name in variable cwdPath
    getDirPath();
    // Retrieve current directory contents
    chdir(&cwd[0]);
    getDirContents(cwd, contents);
    printContents(first-FIRST,last-FIRST);

    printMeta("Normal Mode");

    gotoxy(FIRST, 0);
    cursor = FIRST;

    unsigned int tempLast = 0;
    int x = FIRST,y=0;

    char key[3];
    memset(key, 0, sizeof(key));

    while(true){
        // Retrieving window size
        ioctl(0, TIOCGWINSZ, &window);
        windowSize = window.ws_row;

        memset(key, 0, sizeof(key));
        fflush(0);
        // Read nothing
        if(read(STDIN_FILENO, key, 3) == 0){
            continue;
        }
        else if(key[0] == 'q'){
            disableRawMode();
            clearTerminal();
            break;
        }
        // UP ARROW
        else if(key[2] == 'A'){
            if(cursor-FIRST > first-FIRST){
                cursor--;
                x = cursor-first+FIRST, y = 0;
                gotoxy(x, y);
            }
            continue;
        }
        // DOWN ARROW
        else if(key[2] == 'B'){
            if(cursor < tempLast+last-1 && cursor-FIRST < contents.size()-1){
                cursor++;
                x = cursor-first+FIRST, y = 0;
                gotoxy(x, y);
            }
            continue;
        }
        // LEFT ARROW: Going back one level
        else if(key[2] == 'D'){
                        // No Previously visited directory
            if(st.empty())
                continue;

            // There exits previous directory
            rightStack.push(cwd);
            cwd = st.top();
            st.pop();
            first = FIRST;
            tempLast = 0;
            cursor = FIRST;
            x = FIRST, y = 0;
        }
        // RIGHT ARROW: Goind forward one level
        else if(key[2] == 'C'){
            // Cannot move forward
            if(rightStack.empty())
                continue;
            // There exists forward directory
            st.push(cwd);
            cwd = rightStack.top();
            rightStack.pop();
            first = FIRST;
            tempLast = 0;
            cursor = FIRST;
            x = FIRST, y = 0;
        }
        // BACKSPACE 
        else if(key[0] == 127){
            // Pushing current directory
            st.push(cwd);
            while(cwd[cwd.length()-1] != '/'){
                cwd.pop_back();
            }
            // Removing '/' if it is not the root directory
            if(cwd.length() != 1)
                cwd.pop_back();

            first = FIRST;
            tempLast = 0;
            cursor = FIRST;
            x = FIRST, y = 0;
        }
        // HOME directory
        else if(key[0] == 'H' || key[0] == 'h'){
            st.push(cwd);
            cwd = home;
            first = FIRST;
            tempLast = 0;
            cursor = FIRST;
            x = FIRST, y = 0;
        }
        else if(key[0] == 'K' || key[0] =='k'){
            if(cursor > FIRST && cursor == first){
                first--;
                tempLast--;
                cursor--;
                x = FIRST, y = 0;
            }
            gotoxy(x,y);
        }
        else if(key[0] == 'L' || key[0] == 'l'){
            if(cursor<contents.size()-1 && cursor == tempLast+last-1){
                first++;
                tempLast++;
                cursor++;
                x = tempLast+last-first+FIRST-1, y = 0;
            }
        }
        // ENTER key
        else if(key[0] == 10){
            cout<<cursor<<" ";
            // Cursor pointed to current directory
            if(cursor == FIRST){
                continue;
            }
            // Cursor pointed to parent directory
            else if(contents[cursor-FIRST] == ".."){
                // Pushing current directory
                st.push(cwd);
                while(cwd[cwd.length()-1] != '/'){
                    cwd.pop_back();
                }
                // Removing '/' if it is not the root directory
                if(cwd.length() != 1)
                    cwd.pop_back();
            }
            // Cursor pointed to file/directory
            else{
                struct stat results;  
                string fileName = contents[cursor-FIRST];
                stat(&fileName[0], &results);    

                // Cursor pointed to directory 
                if(results.st_mode & S_IFDIR){
                    st.push(cwd);
                    if(cwd.length() != 1){
                        cwd += "/";
                    }
                    cwd += contents[cursor-FIRST];
                }
                // TODO: Cursor pointed to file
                else{
                    int pid = fork();
                    // New child process should open the file
                    if(pid == 0){
                        char *path = &fileName[0];
                        execl("/usr/bin/vi", "vi", path, NULL);
                        exit(1);
                    }
                    else{
                        wait(NULL);
                    }
                }
            }
            first = FIRST;
            tempLast = 0;
            cursor = FIRST;
            x = FIRST, y = 0;
        }
        // Command mode
        else if(key[0] == ':'){
            clearTerminal();
            // Entering Command mode
            commandMode();
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
            // Starting from the directory from which command mode was started
            first = FIRST;
            tempLast = 0;
            cursor = FIRST;
            x = FIRST, y = 0;
        }
        else{
            continue;
        }
        chdir(&cwd[0]);
        getDirContents(cwd, contents);
        printContents(first-FIRST, tempLast+last-FIRST);
        printMeta("Normal Mode");
        gotoxy(x, y);
    }
}

int main(){
    char *homeDir;
    if((homeDir = getenv("HOME")) != NULL)
        home = homeDir;
    else
        cout<<"Cannot Retrive Home directory"<<endl;

    normalMode();
    disableRawMode();
    return 0;
}
