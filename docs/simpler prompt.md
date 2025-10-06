Simpler seed prompt
=====

Hello, you are going to help me build VibeCoder.

VibeCoder is a Qt/C++ based client to the OpenAI API, useful for assistance in programming.

What it is *not* is a traditional chat client. Although it *will* allow for chat usage as such, it will regularly be used for sending prompts which are compiled differently than through linear conversations.

*copy paste theory here*

## glossary
terroms from old doc

prompt-slice: term to disambiguate the latest thing said by a user in a prompt-reply conversational turn away from the total prompt which constitutes a single context window delivered to the LLM.

Session: A session is what, in a chat client, would be called a chat instance. A session is a recorded instance of a prompt stack which has lead to a response from the LLM. Sessions, at that point, become potentially non-linear; the conversation can be branched laterally by the the user a) modifying earlier prompt-slices to gain a new response, or b) the contraction and compression of a series of prompt-reply turns into a paraphrase and a simple diff.

Also, in a session, a new initial prompt can be recompiled according to new options, and with updated linked files.

The session takes the form of a tree-like structure.



## version one of VibeCoder

Most of the complexity of this software will be managed in the users' own design choices for their prompts and linked files. Much as how any open-ended system of a few building blocks can allow virtually infinite permuations of its fundamental units, we need only provide a few robust capabilities to enable the vibe coder to flourish.

What I need, first and foremost, is an interface to the API which allows for the saving of different sessions, for the tagging and insertion of linked files (code and markdown) inline, and for the ability to duplicate sessions.

I need the ability to save sessions.

I need a way to quickly compile prompts.



## future plans
check boxes and radio buttons to configure prompts: we can have selectors which automatically change words which are inside braces, seperated by bar symbols within the boxy of our instruction prose.

console environment: just like how a user uses a `venv` to change python paths automatically, we can have a command which drops the users, from the console propgram of their choice, into a bash prompt modifed to capture all program output and track their compilation and execution debug output and debuggero utput automatically.

