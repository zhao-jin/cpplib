# remove duplicated and single character lines from history
shell sed "/^.$/d" ~/.gdbhistory | uniq > ~/.gdbhistory.new && mv ~/.gdbhistory.new ~/.gdbhistory

# load and save history
set history file ~/.gdbhistory
set history save

# print C++ virtual function table
set print vtbl on

# print object in pretty format
set print pretty on

# print object according to vtable (RTTI)
set print object

# set tui mode window border style
set tui border-kind ascii
