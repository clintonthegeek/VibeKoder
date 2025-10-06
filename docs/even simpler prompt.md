# VibeKoder Design Doc

## What is VibeKoder?

VibeKoder is a Qt/C++ program client to the OpenAI API designed around the workflow of "percussing" or "striking" available LLMs with textual strings and the capture of responses. It explicitly undercuts the conversational, linear chat interface.

The primary metaphor is that of a percussion instrument, like a drumkit or a bell. Depending on where and how you strike a percussion instrument, it returns a different and complimentary resonant sound. While the elaboration of a linear conversation is easily achievable in VibeKoder, it is only one direction of movement. Orthogonal to linear conversation, the user is afforded lateral moves through dynamic changes in the initial prompt, via differently configurations or live updated from linked files or inputs. 

For instance, instead of beginning the prompt with source code, and then having a long conversation where proposed edits are made and merged, slowly drifting from the initial source, the initial source can be updated at any point by a lateral move which directly sources the latest code base. This movement is entirely controlled by the user in a conscious and deliberate way.

### Compilation of the prompt

The prompt which precedes the task at hand is compiled from documents in the project folder. These include the code-base, either in full or in part, vision and planning docs, and interaction and code style docs. Each new prompt the user chooses to send can be variously configured; prompts about low-level features can omit docs about the GUI and vice versa. Simple questions about included libraries ("does such-and-such library support...?") can omit entirely most of the program details completely.

These docs evolve along-side the code-base, like an external prose support to back-up the current state of the code. The vibe-coder, then, will be in charge of managing this "stack" of development docs; this is the task they must take charge of in exchange for being relieved of most responsibilities of programming.

This prompt compilation stage will be seen, by the user, as a stack of configurable include calls, each of which can be expanded into a larger granularity.
