# WAD / game data policy

Do not commit or redistribute Doom WAD files in this repository/package.

Expected SD layout for Lilka multiboot:

/apps/doom/doom.bin
/apps/doom/doom.wad

`main.cpp` uses `lilka::multiboot.getFirmwarePath()` and searches for `doom*.wad` in the same SD folder as the launched `doom.bin`.

Use a legally obtained WAD file. Shareware Doom data may be used only under its own distribution terms. Commercial WADs must not be redistributed.
