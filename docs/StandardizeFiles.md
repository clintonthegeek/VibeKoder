The program basically works. Now I need to standardize the schema for file systems.

Here is what our config looks like:

```
# VibeKoder Project config

[api]
access_token = "sk-proj-etc"
model = "gpt-4.1-mini"
max_tokens = 800
temperature = 0.3
top_p = 1.0
frequency_penalty = 0.0
presence_penalty = 0.0

[folders]
root = "/home/clinton/Sync/VibeKoder/build"
docs = "docs"             # Relative to root, folder with design docs, style guides, etc.
src = "src"               # Source code folder
include_docs = ["docs/design.md", "docs/styles.md"]

[filetypes]
source = [".cpp", ".h", ".hpp"]
docs = [".md", ".txt"]

[command_pipes]
git_diff = "git diff"
make_output = "make"

[session]
template = "templates/session_template.md"
session_folder = "sessions"
```

It seems that we should move the root to the actual project root, not the build directory.

We need a folder for `sessions` and `templates`. 

The `[template]` section is not yet read and doesn't make sense. These folders should be defined in `[folders]`.

## include docs
we don't need to list specific docs, we should list folders, or an array of folders, and have files of all file types be automatically and recursively detected in folders subfolders.

## template folder
In here are templates for creating new sessions from scratch.

When a template is opened, the program must display a TreeWidgetView

## session folder
We need per-session sub-folders with timestamp names. In each folder, the "root" session is begun, and has its own dedicated docs and src folder named after it (session 001 is 001.md, and folders 001-doc and 001-src are created) wherein cached versions of included docs are duplicated.

Any time that an include first parsed, it it re-written from, say `<!-- include: docs/Vision.md -->` to read `<!-- cached: docs/Vision.md -->`, and thereafter will be read from cache. The user can manually rewrite `cached` to `include` if he wants to refresh that particular include.

At some point, the user is able to "branch" from a response there into a new derivitive session; the derivitive session can either inheret the cached documents or refresh and automatically change all `cached` to `includes`, leading to the creation of a new session-cache folders.

Derivative documents have their origin document appended to their filename, so next might be document `002-001.md`. If refreshing includes, folder `002/docs` and `002/src` are made. if using cached includes from origin, then no such folder/subfolder is made and the cache is the origin session cache.

## command pipes

We need a commandpipe for executing the program. Also, they should be arrays, providing paths for execution relative to root

## new project config


```
# VibeKoder Project config

[api]
access_token = "sk-proj-etc"
model = "gpt-4.1-mini"
max_tokens = 800
temperature = 0.3
top_p = 1.0
frequency_penalty = 0.0
presence_penalty = 0.0

[folders]
root = "/home/clinton/Sync/VibeKoder"
docs = "docs"             # Relative to root, folder with design docs, style guides, etc.
src = "src"               # Source code folder
sessions = "sessions"     # Session storage
templates = "templates"   # Session Templates

include_docs = ["docs", "~/src/docs"] # explicit home directory ref outside project root

[filetypes]
source = [".cpp", ".h", ".hpp"]
docs = [".md", ".txt"]

[command_pipes]
git_diff = ["git diff", "."]
make_output = ["make", "build"]
execute = ["VibeKoder", "build"]

[session]
template = "templates/session_template.md"
session_folder = "sessions"
```
