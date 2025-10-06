## Loading from Templates and Reading Source

Until now, the `void MainWindow::onCreateSessionFromTemplate()` has naively copied templates into sessions to create new sessions.

We need to make this much smarter.

I am currently auto-generated a file with the full source code of the project from a bash script, and copying it manually into the session cache. Here are the two scripts:

### amalgamateSource.sh
```bash
#!/bin/bash

# Ensure exactly one argument (folder path) is provided
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <folder>"
    exit 1
fi

# Assign folder path and remove any trailing slash
FOLDER="${1%/}"

# Ensure the folder exists
if [ ! -d "$FOLDER" ]; then
    echo "Argument must be a valid directory."
    exit 1
fi

# Print the directory tree.
# The -P flag uses a pattern (here a regex-like alternation using |) to only list:
#   *.h, *.cpp, and CMakeLists.txt files.
# The -I flag excludes any files or directories matching 'build'.
# Note: A trailing slash in a pattern (e.g. 'build/') would match directories.
echo "Directory Tree:"
tree --prune -P '*.h|*.cpp|*.ui|CMakeLists.txt' -I 'build' "$FOLDER"
echo

# Function to print files in Markdown format (recursively, excluding the build folder)
print_files() {
    local pattern="$1"
    # -path "$FOLDER/build" -prune tells find to ignore the build folder
    find "$FOLDER" -path "$FOLDER/build" -prune -o -type f -name "$pattern" -print | sort | while read -r file; do
        # Calculate relative path: remove the base folder plus trailing slash
        rel="${file#$FOLDER/}"
        echo "### \`${rel}\`"
        echo '```cpp'
        cat "$file"
        echo '```'
        echo
    done
}

# Print files in the desired order.
print_files "CMakeLists.txt"
print_files "*.h"
print_files "*.cpp"
print_files "*.ui"

```
### updateSrc.sh
```bash
#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <session number>"
    exit 1
fi

# Assign folder path and remove any trailing slash
FOLDER="${1}"

rm "/mnt/oldhome/clinton/Sync/VibeKoder/VK/sessions/$FOLDER/docs/src.txt"
./amalgamateSource.sh . >> "/mnt/oldhome/clinton/Sync/VibeKoder/VK/sessions/$FOLDER/docs/src.txt"

```

As you can see, I simply call `updateSrc.sh 001` to give you the latest source code in the opening User prompt-slice.

I need, instead, to have a way to insert this source dynamically. Maybe one-day this will all be configurable in the GUI, but for now I'd be happy with just having a function to generate the monolithic `.src` file, rather than do it myself from the bash script.

Let's have the command pipe formatted thus: `<!-- command: src_amalgamate -->`. We'll make `src_amalgamate` hard-coded for now, but we'll eventually include definitions in the project file. 

We should re-write our include and cached parser to handle sub-directories, so that the references to docs are always preceded by the folder: `docs/Vision.md`, `src/src.txt`, etc. For now, let's just get the `src.txt` file into the `src/` directory, planning ahead for something more modular in the future.

When I create a template, I want to have our first command pipe: a single line which is replaced by an generic, passive include line like `<!-- cached: src/src.txt -->`, pointing to the to the cached and generated `src/src.txt` file. This line will work exactly the same way as it does for documents, just passively load them from the session cash.

But whenever I replace the `<!-- cached: src/src.txt -->` line with the command pipe, the `src/src.txt`file in the session folder will be rebuilt from the latest source before putting the `<!-- cached: src/src.txt -->` line back again. This is the only time the command pipe will be run from the session: when manually triggered by the line re-write. 

Since command pipes have to be called from both the GUI and from Session, lets put their logic into a dedicated class. 

We already have settings in our project file which correspond to paths for the source, and source file file types. We need to hardcode an exclusion to folders called `build/` and otherwise find source files and do everything that my `amalgamateSource.sh` script does. 

I'm happy for synchronous execution of command pipes for now. It is planned, much later, to create a more dynamical source inclusion tool, but for now this works fine. We should include `CMakeLists.txt` and `.ui` files. I want to keep the existing `cacheIncludesInContent()` logic for normal includes, but enhanced to handle folder prefixes properly. 
