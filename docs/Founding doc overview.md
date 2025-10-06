VibeKoder
====


## What is it?

VibeKoder is two complimentary parts: a Qt-based standalone app for non-conversational LLM-assisted computer programming, and a Kate plugin for automated integration of Vibecoder-provided diffs and code into the code-base being developed. Together, the two create a closed loop of code, allowing each prompt to the LLM to always include the latest code base.

With VibeKoder, vibecoding will become more structured and deliberate than could ever be possible through a linear chat front-end.

## What does "non-conversational" mean?

LLMs, by nature, do not actually engage in linear conversation; the "chat" interface is created by a constant recourse of the entire conversation from the beginning with each "response" they generate. Thus, when using a generic, linear chat front-end to code with an LLM, every single query is repeated again and again to arrive at a sensible answer regarding the current state of the program. Eventually problems can arrise as the users code-base drifts from what the LLM can piece together of its current state from its recourse through all changes since its initial state, or when the LLMs context window runs out.

Instead of trying to continue to squeeze functional code out of this ever-lengthening, brittle stack of piecemeal conversational fragments, VibeKoder changes the paradigm away from the anthropomorphic conversational paradigm. Every prompt to the LLM will be assembled fresh, out of up-to-date development documentation: code, intentions of the current phase or task, and stage of implementation of that phase or task: planning, coding, compilation or linking errors, runtime errors from console output, or user testing.

The "conversations" that the programmer has with the LLM will be short-lived, lasting only the duration of each task. Lessons learned and ideas gleaned from those conversations will be integrated back into the developer documentation before the conversation "starts over" with continuation onto the next task or stage of development.

To anthropomorphize the LLM for a second: it'd be as if the LLM were many, many, many different developers each being hired to read documentation and code help code one feature from scratch, before being abandoned. This, rather than the LLM being one developer loyally retained and growing in knowledge from continued work.

The LLM always hears "Welcome aboard! Here's my large code-base, here's the developer docs and plans, here's where we are in those plans. Please help me with this one feature." And once implemented, it all starts again with the newer codebase and the new feature.

It's like someone, during a long trial, showing up to every day in court with a fresh lawyer, free of the stress or stigma from the preceding trial days.

The developer, then, is always in charge of the "training" of the LLM by keeping development docs in line and up to date, themselves remaining master of the codebase.

## What is "vibecoding"?

Vibecoding is a new fad where developers and, especially, non-developers ask "LLMs" to spit out code which performs what they want to do, and then see if they can get it to work. Such vibecoders have varying levels of competency and knowledge of both programming and software engineering, and their work varies greatly—mostly toward producing buggy and unpredictable trash.

Vibe coding is very organic compared to the structured development of regular code. Traditional coding takes time to be typed and tested. LLMs, by contrast, produces ample code; too much code, a veritable stream of endless codes on tap.

We might, then, liken vibe coding to gardening: it is the cultivation of blooms of code. The vibe coder plants seeds, and gathers and shapes and prunes the excess of living, quasi-biological material which springs forth. Or, perhaps, like a florist, the vibe-coder selects and evaluates and cuts and sutures and arranges into bouquets the bounty of code resulting from interaction with the LLM.

## How vibecoding could be better

My aim with VibeKoder is to empower such vibe-coders by lifting away the layers of abstraction which make LLMs "easy to use", but obscure their actual operating principles and, thus, hinder the coders ability to get a feel for working with them as they actually are. The chat-layer, which simulates human conversation, introduces distortions into the sense perceptions necessary to acquire an embodied and learned intuition about the LLM as-such, especially the degradations in quality of so-called "conversation" which occur when conversations get too long.

An LLM is like a bell; it is a percussion instrument to be struck at a certain place, a certain way, and in response it lets out a single resonant chime in return. Yes, feeding that resultant chime back into the LLM again for a second, third, or fourth strike can continue to lead to useful returns, but eventually the ouraboros shrivels.

By getting away from the ubiquitous chat interface, and offering an interface to re-configure and minutely adjust each "first" strike as fresh, the resonances from our bell can be played with, heard, tested, recorded, compared, and used freely in parallel. The vibe-coder can learn to "play" the LLM "instrument" better, each strike being a freshly delivered optimized blow.

The onus for "playing" the LLM, then, moves back onto the coder.

## The affordances

So, in detail, what features does VibeKoder provide, in its interface, to facilitate this new workflow?

### Compilation of the prompt

The prompt which precedes the task at hand is compiled from documents in the project folder. These include the code-base, either in full or in part, vision and planning docs, and interaction and code style docs. Each new prompt the user chooses to send can be variously configured; prompts about low-level features can omit docs about the GUI and vice versa. Simple questions about included libraries ("does such-and-such library support...?") can omit entirely most of the program details completely.

These docs evolve along-side the code-base, like an external prose support to back-up the current state of the code. The vibe-coder, then, will be in charge of managing this "stack" of development docs; this is the task they must take charge of in exchange for being relieved of most responsibilities of programming.

This prompt compilation stage will be seen, by the user, as a stack of checkable headers, each of which can be expanded into a larger granularity.

### Automatic tracking of structured development plans

A grand plan for the program ought to be drawn out, although is often changed as the product takes shape. Fortunately, the structured nature of markdown allows easy organization of the text to be maintained.

An organically grown program must be boot-strapped with some basic shell code full of stubs and place-holders; each of those must be tracked and eventually got around to later for fleshing out.

Development at this scale must be incremental: large features are built out of many small features. Various branches of different, inter-operating aspects must often be developed in parallel, each tracked and integrated carefully.

At a smaller level, the imposition of some local structure upon whatever feature the coder "feels like" developing, then, is also useful. Each new piece needs to be planned, developed, compiled, and tested.

In "planning" mode, the LLM can aid in creating plans which are broken into incremental stages and tasks. As these stages or tasks are begun or completed, their status can be updated in the documentation.

### Codebase amalgamation with `git` integration

The prompt stack will usually begin, after a short prompt, with the full or partial amalgamated codebase for the project. Assuming a C++ project, this will comprise header for the complete program, and `CMakeLists.txt` and `.cpp` files for the part of the program being considered or hacked upon.

And then, during "development" of some localized feature, its implementation tasks are checked off and paired with the `git` diffs for the associated work. Once a feature is complete, the change is committed back into the codebase which is shown at the top of the prompt, and the development target in the planning docs moved to the next stage or step.

This way, even if the coder is asking for the LLM to reproduce entire code-blocks redundantly in order to get just a line or two of change, only that line or two is reflected in the `diff` which goes into the next prompt, optimizing context-window use. 

And by using git branches, the prompts can always be begun with the latest code base, or as an earlier state, and then piecemeal as an explicit breadcrumb list of diff, corresponding to development. The documentation will always tie completion of certain goals, stages, or tasks with corresponding commits.

### varying levels of scope

As said above, the prompt can contain the full project code, or any small sub-section, selectable through checkboxes for header files or source files, resources, docs, etc. in a prompt stack.

Narrow, focused selections upon parts of the code base are efficient for programming and debugging. Wide, full prompts containing the entire codebase are useful for larger-scale questions about direction, program architecture, the refactoring or major components, planning of future development, and the like.

The ability to go back and forth easily between prompts of both nature, using the occasional wide-scale prompt to get information to change planning, vision, and development docs which are used in more narrow-scale prompts is a game-changer for vibe-coding workflows.

### Kate integration.

Most LLM integrations are for professional IDEs. I'd like to have an integration for Kate, the humble power-tool text editor for KDE. It would need to do what most LLM integrations do: allow one-click integration of diffs and code changes into the source files of the open project.

There must be some kind of inter-process communication, then, between the kate plugin and the standalone VibeKoder app. Whether this is through app sockets, or the creation of temp files in a jointly-monitored folder, the two halves of the process need to allow coordination about which blocks of code from the LLM are paired with which stage or task in the development docs, and with which git commits.

Further, should the coder decide to do some manual, direct coding on their part, the resulting diffs need to also be tracked and integrated into the development docs.

### Power clipboard

And, until that is developed, the VibeKoder app must allow robust copy/paste and export functions of its own. LLMs usually produce markdown, putting code into triple back-tick blocks. Those whole blocks might be copy/pasted, or maybe just certain code-blocks, determined by syntax or even indentation, are desirable to copy piece-meal. The view of the LLM response which contains these code-blocks should allow such partial clipboard functionality, if this workflow is desired over the Kate integration.

---

## What Next?

There are obviously a lot of loose ends and open questions in this idea. Are the commit messages derived directly and automatically from the planning docs? Are the planning docs themselves tracked in the same `git` repo as the code? What about the various prompt-stack configurations?

For now, I'd like you to please help me firm up the semantics of my proposal, and develop a basic architecture for VibeKoder using these semantics. All the various terms I'm using need specific definitions. For instance, do we have a "pointer" indicating the current stage or task being worked on in our plan? What is its nature?

Please zoom out and give me an overview of the project as you see it, arranged in a way to make it most sensible and thorough. Share ambiguous areas where they arrise, left empty, so they can be filled. And ask questions as you feel the need.























