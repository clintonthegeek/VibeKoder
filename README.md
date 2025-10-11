# VibeKoder Design Doc

> LLMs are not, as I have said, agents or people you can talk to. They are percussion instruments, like drums or bells. Whenever you strike them with words, the tokens derived from those words resonate in a vast echo-chamber, and the echoes which return are turned back into new sentences again.
>
>This makes them an amazing technology. Just as the globe is the world in your hands, an LLM is *language itself* turned into a material artifact. You put words into it, those words reverberate across all language, and more words ring out!
>
>That technology: the LLM as percussion instrument, a poetry chamber, is what kids ought to be learning. As long as they are not learning that, they are being lied to. Poisoned. Seduced by the last people you want knowing their secrets, profiling their psychology, guiding their development, or taking their money.
>
> ([source](https://lessmad.substack.com/p/normie-ai-talk-is-pure-poison))

## What is VibeKoder?

VibeKoder is a Qt/C++ client to the OpenAI API, designed around the workflow of "percussing" or "striking" LLMs with textual strings, and capturing the responses. It explicitly undercuts the conversational, linear chat interface.

![A Screenshot of VibeKoder](docs/VK.png?raw=true "VibeKoder")

The primary metaphor is that of a percussion instrument, like a drumkit or a bell. Depending on where and how you strike a percussion instrument, it returns a different and complementary resonant sound.

While the elaboration of a linear conversation is easily achievable in VibeKoder, it is only one direction of movement: orthogonal to linear conversation, the user is afforded lateral moves through dynamic changes in the initial prompt, via differently configurations or live updated from linked files or inputs. 

For instance, instead of beginning the prompt with source code, and then having a long conversation where proposed edits are made and merged, slowly drifting from the initial source, the initial source can be updated at any point by a lateral move which directly sources the latest code base.

This movement is entirely controlled by the user in a conscious and deliberate way.

### Compilation of the prompt

The prompt which precedes the task at hand is compiled from documents in the project folder. These include the code-base, either in full or in part, vision and planning docs, and interaction and code style docs.

Each new prompt the user chooses to send can be variously configured; prompts about low-level features can omit docs about the GUI and vice versa. Simple questions about included libraries ("does such-and-such library support...?") can omit entirely most of the program details completely.

The documents evolve along-side the code-base, like an external prose support to back-up the current state of the code. The vibe-coder, then, will be in charge of managing this "stack" of development docs, in exchange for being relieved of most responsibilities of coding.

This prompt compilation stage will be seen, by the user, as a stack of configurable include calls, each of which can be expanded into a larger granularity.

### Target Usecase and terminology.

For now, the target use case will be for programming tasks. Specifically, I'd like the notion of a *project* to mean a single codebase, and I'd like a *session* to correspond to a recorded instance of a prompt stack which has lead to a response from the LLM. Sessions, at that point, become potentially non-linear; the conversation can be branched laterally by the the user a) modifying earlier prompt-slices to gain a new response, or b) the contraction and compression of a series of prompt-reply turns into a paraphrase and a simple diff.

*Include Docs* will be all document snippets which the user might include in a prompt stack, such a design docs, best practices, reference materials.

*Command pipes* will be commands which generate console output which will be integrated into the prompts dynamically. These might include calls to `git diff` in the codebase to capture recent changes, or `make` commands which lead to console output, or the debug output of the application itself. 

*Prompt slice* means the last appended slice of a prompt representing what the user says to the model in a conversational turn.

All the main work will, for now, be done in text files edited by the users favourite text editor. VibeKoder itself will store configuration (API key, codebase, command pipes, and include documents root) in the project file, and each session file will refer to the project file for configuration options and then allow assembly of the stack.

After making a new project file, the user will start a new session, being provided with a template prompt stack document saved under an initial ordinal filename in the session folder. Using their favourite text editor, the user will comment or uncomment the include lines for various documents and sources. 

Upon sending the compiled prompt, the variable parts of the document will be replaced with their static, compiled contents. For instance, references to source documents will be replaced by the source documents themselves. Further, the response from the LLM returned from the API returned will be appended to the bottom of the document. The user can now carrry on in conversation by appending their next prompt-slice.

The user will be afforded options to run several commands with `QProcess::start()` and and capture their textual output directly into the file from `readAllStandardOutput()` and `readAllStandardError()`.

The user can carry on in linear conversation this way, until they decide to perform a lateral movement, leading to a new template prompt stack document, saved under the next incremental ordinal file name. 

### Miscellaneous Implementation Notes

I have recently rewritten the entire system configuration logic to use dynamically generated JSON from a prime `schema.json` file. This is used to create an app-wide `config.json` file in the users config folder (for instance, `~/.config/VibeKoder`) and that config file has defaults which will be used to create new project files. 

Thereafter, settings are easily accessible app-wide by dynamical getters, as so: 

```cpp
QStringList docFileTypes = m_project->getValue("filetypes.docs").toStringList();
QVariantMap cmdPipeMap = m_project->getValue("command_pipes").toMap();
QString someString = m_project->setValue("someNewValue");
```
