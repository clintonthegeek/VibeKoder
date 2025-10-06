Okay, we are going to refactor the `SessionTabWidget`. Here is the idea:

The user needs to be able to make major edits to the session file from the prompt slice tree, using a right-click context menu or button.

This means introducing a modality for saving the session file which is seperate from sending. Right now, writes are only made to the session file when a new user-role prompt-slice is sent with the "Send" button. We will introduce a "Save" button beside the "Send" button which will update the file without sending.

Another important new modality is whether or not the last slice in the session, when it is selected, is a user-role slice, or an assistant-role slice.

When the last slice is a user-role slice, and it is selected, then the `m_appendUserPrompt` box should take up all the space; there is no need for the `m_sliceViewer` to be visible at all.

If a response from the server has just finished coming in, and the last slice is thus an assistant-role slice, and it is selected, then now the `m_sliceViewer` can pop into view, sharing the space with the `m_appendUserPrompt` below it. It also makes no sense to have an enabled Send or Save button unless the m_appendUserPrompt is non-empty, meaning the user has started to write something. 

So the user types something and hits save. Well, now the last slice is a user-role slice, but it has not been sent, and so no incoming assistant-role slice is forthcoming. What happens? The `m_appendUserPrompt` now takes up the entire space which was previously shared with the prior-selected assistant-role slice!

Every time the last slice in the session is a user-role slice, and that last slice is selected, then there is no `m_sliceViewer` necessary, since no un-editable, read-only slice is selected.

Now here's the interesting part: logically, given the back and forth between user-role slices and assistant-role slices, if the last slice is a user-role slice, then the *penultimate* prompt-slice *must* be an assistant-role slice or the initial system-role slice. And so, selecting this penultimate slice will give us back the split view where both the `m_sliceViewer` of the penulatimate assistant/system-role slice shares the space with the `m_appendUserPrompt` box beneath it showing the last slice, the editable user-role slice. When any slice further back is selected, then, it is should be shown in a `m_sliceViewer` and no `m_appendUserPrompt` box should be shown.

But that doesn't mean that they are not editable! No sir. In fact, we must here provide opportunities to either branch laterally into a new session (to be implemented later), to regenerate an assistant-role slice, or, if the slice is a user-role slice, to delete everything afterward and *then* enable editing.

Here, we must introduce a new action: the "Delete After" action, available in the context menu of the prompt slice tree.

"Delete After" erases every slice which follows after the acted-upon slice (in memory, at first). The modality of the `SessionTabWidget` now changes to reflect whatever the role of the new last slice dictates, as spelled out above. The save button is enabled, and the refresh button is also enabled, allowing an "undo". Once either Save or Send is pressed, however, the session file is overwritten and the deleted slices are lost.
