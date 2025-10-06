## New Project Log
This is just a comprehensive list of how I set up a useful VibeKoder project for PlanStanLite.

- Create new `VK/` subfolder. 
- Open VibeKoder and make new project in said folder.
- manually inspect Folders, root is automatically set which is good.
- added `qml` to source filetypes
- interestingly, I have set `src` to the source folder. Yet, I realize that that means the project's main `CMakeLists.txt` fill will be excluded. I wonder if adding `../CMakeLists.txt` to the source file definition will work? Not too important for VibeKoder because only `main.cpp` is specified to be compiled, yet still.

- and once I hit OK, the program crashed. No log because I launched it from the plasma taskbar. Ugh.



- launching the program from build directory does not cause a crash, and creates the file fine.
- no API key
- actually, also i fucked up and didn't notice that VibeKoder, the launch directory for the app, was set as the path in the config file.


- Okay, so the newly created config has full paths in each path entry, where it should be local except the root definition in order to be parsed.

- When I manually copied in the template files, I forgot to rename them from `.VKTemplate` to `.md` and was confused why they wern't populated.

- The initial config gave no file types specified for source files, yet somewhere apparently, hardcoded, is `.ui` files and `CMakeLists.txt` according to this output:

```
[runSrcAmalgamate] Scanning source files in: "/home/clinton/Sync/PlanStanLite/src"
[scanSourceFiles] Using patterns: ("", "CMakeLists.txt", "*.ui")
[scanSourceFiles] Total files found: 1
[runSrcAmalgamate] Found 1 source files.
[writeAmalgamatedSource] Creating src cache folder: "/home/clinton/Sync/PlanStanLite/VK/sessions/002/src"
[writeAmalgamatedSource] Absolute output file path: "/home/clinton/Sync/PlanStanLite/VK/sessions/002/src/src.txt"
[writeAmalgamatedSource] Writing amalgamated source to: "/home/clinton/Sync/PlanStanLite/VK/sessions/002/src/src.txt"
[writeAmalgamatedSource] Finished writing amalgamated source.
```
