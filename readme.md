## FeX - Linux File Explorer

- FeX works in two modes:
   1. Normal Mode (Default mode)    
   2. Command Mode
#### Normal Mode:  
- Used to explore the current directory and navigate through the filesystem.
  1. Display the list of directories and files in the current folder.
  2. User can navigate up and down in the list using the corresponding **up** and **down** arrow keys.
  3. Handles vertical overflow using keys k & l.
  4. Traversal:
     1. **Left arrow** - Go back to the previously visited directory.
     2. **Right arrow** - Go to the next directory.
     3. **Backspace** - Go up one level.
     4. **h** - Go to the home directory.
     5. **ENTER** - If file opens in the vi editor. If directory navigate into that directory.

- Enters into the command mode whenever **:** (colon) key is pressed.


#### Command Mode:
- All the commands entered are displayed at the bottom.
- Supports following commands:
  1. **Create file** - *create_file <file_name> <destination_path>*
  2. **Create directory** - *create_dir <dir_name> <destination_path>*
  3. **Delete File** - *delete_file <file_path>*
  4. **Delete Directory** - *delete_dir <dir_path>*
  5. **Goto** - *goto <dir_path>*
  6. **Search** - *search <file_name/dir_name>*
  7. **Copy** - *copy <source_file(s)> <destination_directory>*
  8. **Move** - *move <source_file(s)> <destination_directory>*
  9. **Rename** - *rename <old_filename> <new_filename>*

- Commands works for both relative and absolute paths.
- Enters into the normal mode whenever **ESC** key is pressed.
- On pressing **q** key, FeX closes.
