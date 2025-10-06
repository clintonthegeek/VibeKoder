# Todo

Well, I accidently deleted the primary documenntation, and now it only exists in the playground. Sigh.

Luckily when this program is working, I will be using it and get to ditch the playground completely! So.

## Soonish

- bug: open two tabs. close the first one, the second one closes as well.

- make sure generator class updates all views, syncs between open tab and project tab instantly. 

- create a template editor using pebk's markdown widget. put the source code selector in there, make this the opportunity to gut or replace command pipes altogether. just take them out. have includes. have a source managing class, future-expandable to include Doxygen etc.

- Add "maximize" buttons floating in top-right corner of `m_sliceViewer` and `m_userAppendPrompt`.

- change tab titles to number. maybe generate one word summaries??

- refactor `SessionTabWidget` to change depending on whether a user or an assistant slice is selected. One big, editable QEditBox for user slices, which allow for delete-afters and re-writes, and forks to new sesions when it's not the last slice. And for assistant slices, the familiar split which offers the non-editable reader and the response box below. Saving the response box without sending makes a new user slice, which is also the last slice, opening up the one big user slice editor mentioned above. Whenever a session which is opened or refreshed ends in a user slice, it will display this user view. 

- fix app config loading and saving by emphasizing readability, modularity, structure, comments, debug. no efficient tricks. 

- Empty template reveals that, when there is only the system prompt slice, it still shows an empty m_sliceViewer; lets hide that.

- PROJECT TAB REWRITE
    - a doc list which allows you to copy an include line for pasting into a template or prompt slice. 

- finish session 68 step 5, UI for app-wide config

- get temp sessions to use API key from app settings.

- change src.txt to src.md for highlighting in kate

- filter out commandpipe, include, and cached lines from execution in a) anything but a user role slice, and b) any code block

- we have relative paths being converted to full paths in random spots all over the code. just change the m_project default to full paths, and undo all these stupid conversions everywhere.

- test if project with source directory set works for PlanStanLite, using `../CMakeLists.txt` explicitly in project file. 

- some file caused an error about too many requests from open AI, suggesting that there the parser can accidently spam the server with a lot of shit. don't know what it is, but we need to catch this. 

- don't let failed command pipe block; give cancel and ignore options. 

- Optional improvements for draggable tab bar
    - **Set the window title of new windows** to something meaningful (e.g., the tab name or session name).
    - **Forward signals** from detached windows back to main window if needed.
    - **Implement context menus** on tabs by subclassing `DraggableTabBar` further or connecting to its signals.
    - **Handle closing of detached windows**: if the last tab is closed, close the window.
    - prevent closing of main window, of project tab. maybe closing main window pops up message. 

- give projects names. right now the YAML header in the sessions refer to the project name but there isn't even one, haha.

- actually use the YAML header. Store a title and description.
- Poll YAML header for these, and also number of slices, turn Available Sessions list into tree showing all.
- Add option to create title and summary automatically.

- Two options in the slice list: Fork from here, and delete this and all subsequent slices. Forking creates bi-directional links in metadata. 

- slice count and session summary line in metadata (option to auto-generate?). 

- saved 'Baked" refernces to session files, and future commandpipe outputs, to file. either inline in document, or re-referenced to copies.

- Project tab should have hierarchy of sessions as a tree. 

- project configuration templates, especially regarding coding and initial codebase, makefiles. 

## later
- global config for generic prompts, default settings for temp sessions.

- rethink relation of session (maybe YAML header will be good for something) and projects, allowing drag of tabs onto desktop to create desktop links; means allowing command line options to automatically open project in order to open session.

- implement ollama backend

- remove hacky exclusion of toml source in `QStringList CommandPipeManager::scanSourceFiles() const` by explicitly adding project config filter line to exclude files. This includes also VK/ folder config files for .json. 

- use amalgamate script, with flags, as commandpipe for source inclusion?

- implement a response-parser with granular copy/paste buttons for fenced code, and even block-level copying?

- implement a system prompt to concretize formatting for easier code parsing?

- upgrade ai api
    - generalize to multiple providers
    - list available llms, allow switching
    - save llm settings in each response slice seperator. 

- figure out whether to use git rep or cmakelists.txt file for automatic selection of source files for source command pipe. 




## heap
update the toml parser to make sure it gets every variable set, especially for templates folder.

allow "forking" of sessions for lateral moves from earlier timestamps, perhaps where a "new" session can be made from a lateral move from the initial prompt.

add timezone to header of session files



