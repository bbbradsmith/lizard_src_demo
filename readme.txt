============================
 Lizard Source Code
 Copyright Brad Smith, 2018
============================


This code is released under the Creative Commons Attribution License (CC BY 4.0),
with the exception of the assets/ folder.

The assets/ folder is provided for reference only, and may not be reused or redistributed.
The name "Lizard", all characters, text, art, music, etc. may not be reused without permission.

Attribution should include the name of its original author, and the website link:
Author: Brad Smith
Website: http://lizardnes.com

For the full license terms, see: license.txt


The tools provided are intended for use on Windows operating systems.
With some effort Linux or Mac might be used as well, but the tools for this are not provided here.
Where prerequisite downloads are listed the version used is given, but later versions are probably fine.


This is provided without guarantee or offer of support.
Its author is not liable for any result of use or misuse of this software.
This is not intended as a tutorial, nor a guide for how to make your own game.

You may feel free to send me questions, but I cannot guarantee a reply.
If you find a use for this stuff please tell me about it.


This GitHub distribution does not include the reference assets/ folder, due to its incompatible license.
To obtain a copy of this data, and a pre-built Lizard Tool, download lizard_src_demo.zip from:
https://rainwarrior.itch.io/lizard


lizard@lizardnes.com
http://lizardnes.com


====================
 Guide: Lizard Tool
====================

The Lizard Tool is used to edit the material in the assets/ folder.

A Windows build is included at: release/lizard_tool.exe
Run edit_campaign.bat to launch the tool and open the game package: assets/campaign.lpk

The package file (.lpk) references 4 kinds of data, also contained under the assets/ folder:

1. Room - The Lizard world is a large grid of rooms.
2. CHR - A group of 64 graphics tiles.
3. Palette - A set of 4 colours.
4. Sprite - A collection of tiles references and positions that make up a single character sprite.

Each type of data has its own editor, which will be described below.
These four types of data are given in lists in the main window of the tool.
Data can be edited by selecting its entry and clicking the "Edit" button, or double clicking on the entry.
Rooms can more easily be opened from the Map view.

Multiple editor windows can be open at once.
In most cases, updating data in one will be reflected in other open editors that reference that data.

In the "Help" menu is an "About" command that lists keyboard commands for each editor.

In the "File" menu is an option to "Export", which will regenerate some files in: assets/export/
These exported files are used to build the game (either for NES or PC, both are exported at once).
The "Test NES" or "Test PC" options do an export followed by running test_nes.bat or test_pc.bat.
These batch files will build the game, then run it if successful.
See "Guide: Source" below for build requirements.

A warning: this tool is not really designed to be friendly to every user, only to me.
Every feature of this tool only exists because I thought it would help me finish making Lizard.
Many features may use unintuitive controls, or seem strangely limited.
Only the room editor has an undo feature, partly because I had source control as a backup for accidental changes.
I also never implemented a search tool, because the asset data formats are all plain text and I can just grep them.

-----------
 Tool: Map
-----------

In the "Help" menu is a "Map" option which opens up a grid of rooms map view.

Each room on the map appears as one or two coloured blocks on the grid. Left click to open that room.

Right click to switch between the two "sides" of the map ("Recto" and "Verso").

A room's position on the map can be set in the room editor under "coords".

------------
 Tool: Room
------------

The world of Lizard is made up of Rooms.

The top left shows the whole room, which in-game is two screens wide.
Left clicking on this field will draw with the currently selected background tile.
Right clicking will select the tile under the mouse cursor.
Edits to this field can be undone by pressing 'Z'.

Each room can reference 4 CHR blocks for background tiles, and 4 for sprite tiles.
Similarly it can reference 4 palettes for each of these as well.
Normally the font CHR should appear in the second background CHR slot.
An empty reference (none) will retain whatever CHR was in that slot before entering the room (makes loading faster).

Any background tiles from the first 2 blocks will represent "empty" space that the player passes though,
any background tiles from the latter 2 blocks will create "solid" space that the player cannot walk through.
Press 'C' or click the "collide" box to visualize the solid tiles.

The colours for the background have a 16x16 pixel granularity (called "attributes").
Press 'A' or click the "attribute" box to visualize and edit only the attribute data.
Alternatively, you can hold 'Left-Shift' to paint with the current colour without changing the tiles.

You can right click on a tile or palette in the selector to open the editor for the corresponding CHR/palette.

Properties:
- Coords: Where this room appears on the map. (Not used by the game, only for visual organization on the map.)
- Recto: This can either be on the Recto or Verso side of the map.
- Ship: Used to mark finished rooms, so I can see which ones still need work/review before the game ships.
- Music: Select music for this room. This also determines colour on the map.
- Water: The level of water. Below this level on the screen is considered underwater.
         If this is not 255, a "splasher" dog is expected in slot 10.
         Use "collide" to visualize.
- Scrolling: If unchecked, the room is only one screen wide (non-scrolling) and only the left half is seen.
- Doors: There are 8 entry points, and 8 possible linked rooms.
         The entry points normally correspond to the connected room (but do not have to).
         Right click on a linked room to open that room in a new window.

"Dogs" are the game objects in Lizard. There are 16 slots for invidual dogs in each room.
Each dog has a type, and X/Y location, and a parameter (0-255). The meaning of the parameter is particular to each type.
There is also a comment field after each dog which can be used to make notes.
Some dogs (e.g. sprite0) use a text parameter in the comment in lieu of the numerical parameter.
Some dogs (e.g. coin) will fill in the parameter automatically during export.
A dog will be visualized in the tool by the sprite with the same name (optionally with _ suffix to make it a tool-only sprite).

Since the dog type list is unsorted, the way I usually select a dog type is by clicking that field
then pressing the first letter of the type I want repeatedly to cycle through dogs that start with that letter.

"Assume CHR/palettes" can be used to copy CHR/palette references from an existing room.
The last opened room will be the default selection, so it may be easiest to open that room with the map first.

"Auto Link" will set up links to the 4 adjacent rooms on the map, as well as 4 connecting pass dogs at the edges of the screen.

"Unlink" will remove links to this room. (Useful when trying to delete it.)

The password selector gives you a password for this room with the selected Lizard. You will appear at door 0.

Test NES or Test PC will export all data but with the starting door connected to the current room, with the chosen lizard.
After exporting, it will run test_nes.bat or test_pc.bat which will build the the game and run it.
See "Guide: Source" below for build requirements.

"Strip" will remove unused CHR or palettes.

Keyboard commands:
- Z undo.
- [/] select the previous/next tile
- A/G/S/D/C toggle attribute/grid/sprites/doors/collide
- , followed by a right click: set selection rectangle upper left
- . followed by a right click: set selection rectangle lower left
- F fill selection rectangle with current tile
- K copy selection rectangle
- L followed by right click to paste the copied rectangle (top left at the click location)
- E followed by right click to flood fill current tile wherever it matches the clicked tile (also works in attribute mode)
- R followed by right click to fill with current tile until it matches a boundary of the selected tile.
    i.e. draw a shape outline with the current tile, then press R and right click inside it to fill the whole shape, ignoring previous contents.
- N disable/clear multi-fill selection.
- M add currently selected tile to multi-fill selection. Up to 8 tiles can be seleced. E fill will randomly use tiles from the multi-fill set.
- T followed by right click to fill with the "Volcano Zone" pattern. (Expects CHR ag0 in the 4th slot.)
- B followed by right click: replaces all instances of tile clicked on with the currently selected tile.
- ~ toggles typing mode. Left click to set the cursor, and then the keyboard will type letters directly into the background.
- Esc cancels an action and deselects the rectangle.

-----------
 Tool: CHR
-----------

This is a 1KB block of data that stores 256 graphics tiles, each 8x8 pixels of 4 colours.

- Click on a tile at the top to select one to edit.
- Left click on the magnified tile to set a pixel with the drawing colour.
- Right click on the magnified tile to select that pixel as the current drawing colour.
- Click on an adjacent magnified tile to switch to it.
- Click on a palette colour below the tiles to select a drawing colour.

Keyboard commands:
- [/] select the previous/next tile.
- 1/2/3/4 select drawing colours.
- C copy current tile
- P paste over current file
- F fill current tile with solid colour

A CHR entry whose name ends with _ will be suppressed by the exporter.
This is useful for creating CHR tiles that can be used in the tool only, without taking up space in the game.

CHR can be imported/exported as a BMP image, to be modified in another editor.
When importing, it will use the closest matching colours to the currently selected palette.

"Assume palettes" will let you select a group of palettes from an existing room.
The last opened room will be the default selection, so it may be easiest to open that room with the map first.

---------------
 Tool: Palette
---------------

A palette has 4 entries for colour. Click an entry, click a colour.
In most cases the first colour should be black ($0F).

--------------
 Tool: Sprite
--------------

A sprite is a list of 8x8 tiles and their positions relative to a centre point (0,0).
Each tile can have 1 of 4 palettes (0-3), and can be flipped vertically or horizontally.

The count field is how many tiles this sprite has.
16 tile entries are shown in the editor, but the -16/+16 buttons will select more pages (up to 64 tiles).

Normally palette 0 and 1 are reserved by the player, so most sprites will use palettes 2 or 3.

The "vpal" option will apply the logical OR of a dog's slot to the palettes for this sprite,
so palette 2 will produce 2 on an even slot, or 3 on an odd slot.
This makes it easier to manage combinations of sprites in a room.

The "bank" option can be 0, 1 or 2.
The sprite data is spread over 3 banks in the NES version,
and code in each of these banks can only access one of these sprite sets (see source guide below).

The L/R/U/D buttons will shift the whole sprite by one pixel.
"Assume CHR/pal" will let you select CHR/palette references from an existing room.
The last opened room will be the default selection, so it may be easiest to open that room with the map first.

Sprites whose name ends with _ will not be exported, but can still be seen in the room editor.
Sprites whose name ends with __ will additionally be hidden from map renders.

-----------------
 Tool: Animation
-----------------

In the "Help" menu is an "Animation" option which opens an animation preview window.
This tool lets you preview a sequence of sprites animating while you edit their CHR data in another window.

Type sprites by name into the rows, and change the count at the top to set the length of the animation loop.

"Assume CHR/palettes" will let you select CHR/palette references from an existing room.
The last opened room will be the default selection, so it may be easiest to open that room with the map first.

-------------------------------
 Building the Tool from Source
-------------------------------

Prerequisites:
- Visual Studio 2013 Express: https://visualstudio.microsoft.com/vs/older-downloads/
- wxWidgets 2.8.12: http://www.wxwidgets.org/

Libraries and include files for wxWidgets must be placed in the wx/ folder. (See wx/readme.txt)

Open lizard.sln and build the lizard_tool project. The release version is used by edit_campaign.bat.
(In theory this could be built for other supported wxWidgets platforms, but no build scripts are provided.)

All of the relevant source code files are prefixed by "tool_", and also "enums.h" is shared with the game.
Mostly there is 1 CPP file for each editor window described above.

tool_main.cpp is just responsible for starting the program (by opening a "Package" editor window).

tool_data.cpp is not an editor, but a container for all the data in the game.
It has all the export code for generating output data for the PC/NES build, output to: assets/export/


========================
 Guide: Music and Sound
========================

Prerequisites:
- Python 3.4: https://www.python.org/downloads/
- FamiTracker 0.4.2: http://famitracker.com/downloads.php

Place FamiTracker.exe in the ft/ folder.

Run: music_export.py

This will search assets/src_music for music and sound effect files to compile.
Music files have the naming convention: music_##.ftm
Sound effects have the naming convention: sfx_##.ftm
The numbering for sfx is in hexadecimal, but for music it is decimal (sorry for the inconsistency).

These will be compiled to several files in assets/export/
- data_music.h
- data_music.cpp
- data_music.s
- data_music_enums.s
- music_stats.txt

Examine music_stats.txt to see how much space is used by each file.

All data, including the player code, is expected to fit in a single 32KB bank.

The exported music can be compiled into an NSF/NSFe music file.
See: nes/compile_nsf.bat (not in demo)

---------------
 Sound Effects
---------------

The sound effect is expected to be a series of notes played at speed 1 with a blank instrument.
All notes must appear only on square 1 or the noise channel, but not both.
The Vxx effect may be used to set duty or periodic noise. No other effects may be used.
The volume channel may be used.
A note cut --- is used to mark the end of the sound.

-------
 Music
-------

Music may use all channels except DPCM.

Tempo must be set to 150, but speed can be used.

The volume column is functional.

Allowed effects:
- Bxx to set a loop point. This is required for all tracks.
- D00 to create patterns of various length.
- Fxx to change the speed. (Tempo changes not supported.)

Instrument evelope macros are supported, excepting hi-pitch.
Using arpeggio and pitch envelopes in the same instrument may result in behaviour that differs from Famitracker.
Note release === is not supported, and release envelopes are not supported.

Pitch envelope may be used as a way to achieve vibrato without the 4xx effect.
A second note and a second instrument may be used to (inefficiently) substitute for release envelopes.

Identical patterns and instruments are automatically merged to save data.

The Lizard Tool has no knowledge of the exported music.
It would have to be rebuilt to accomodate new track names, or additional tracks.
See MUSIC_LIST in tool_data.cpp and TOOL_MUSIC_COUNT in enums.h

This export tool will catch several types of errors but probably fails to check a lot of cases.
Since I know what its limits are, I did not spend much time testing how it handles things it can't do.

The export generates a music_stats.txt file which can be used to see how much data each track is using.


=============
 Guide: Text
=============

Prerequisites:
- Python 3.4: https://www.python.org/downloads/

Run: text_export.py

This will compile assets/src_text/text_english.txt to several files in assets/export/
- text_set.h
- text_default.cpp
- text_set.s
- text_default.s
- text_default_rotate.s

See assets/src_text/text_english.txt for notes on the input data.

This script places almost all of the text in the game in a large table,
and the game references this text through a single interface (see lizard_text.h).
Because the code directly uses these enumeration values, you will need to remove the corresponding code if removing any text.

There was a system in place for providing translated versions of the text, but it has been partly disabled for the source release.
The full translation system involves patching CHR and room data, which is very dependent on the specific data in the assets/ folder,
and since the assets/ folder is not resuable or licensed for open source it seemed inappropriate to include.
It also makes it harder to modify the game, because changing certain things in it will break the translation system;
due to its nature it had to be written after the game assets were complete and finalized.

If you are interested in volunteering to translate Lizard into a language not yet supported, please contact me for details:

lizard@lizardnes.com


=============
 Guide: Dogs
=============

All game objects in this game are referred to as "dogs".

Aside from the tile collision, some dogs have movable collision rectangles known as "blockers".
There are 4 blockers allowed in any room at one time, and the blocker assigned to a dog is generally its slot & 3.

There's a bunch of rules for combinations of things, which slots can be used for various dogs, etc.
For the most part doing the wrong thing will look visually wrong in the tool,
or give an error when trying to export.
In some cases I may have missed enforcement of a rule, however,
especially with bosses where there may be only one way to put together the dogs for that room anyway.

Coins and iceblocks have their index/flag automatically assigned.
Other objects with a flag need to be manually assigned. (See enums.h / enums.s.)


Bank 0: (dogs0.s)

none - Empty dog.
door - Door entry point, parameter 0-7 is one of the door links.
pass - Like a door but automatically activates on touch.
pass_x - A pass for the edge of the screen for making a horizontal transition.
pass_y - A pass for the edge of the screen for making a vertical transition.
password_door - A door for continuing, goes wherever the current password goes (if valid).
lizard_empty_left - Empty lizard facing left, parameter is lizard type.
lizard_empty_right - Empty lizard facing right.
lizard_dismounter - Animation helper that must accompany lizard_empty_left/right.
splasher - Animation helper that must be used in slot 10 when water level is < 255.
disco - Lounge lizard's disco ball.
water_palette - Animate palettes 1 with water colours.
grate - A small sprite grate to let water through (blocker).
grate90 - Rotated grate (blocker).
water_flow - Pushes the lizard to the left (used at top of waterfall).
rainbow_palette - Cycles the colours of the palette given by parameter.
pump - Animated pump for steam zone, purely cosmetic.
secret_steam - Animated steam for where lava meets the water.
ceiling_free - Rooms normally have collision on rows 30/31, this removes that to make vertical wrap possible (Void Zone).
block_column - Blocks a vertical column 4 pixels wide, used in some rooms to protect the edge (blocker).
save_stone - Creates a checkpoint if touched.
coin - A collectable coin.
monolith - Readable by the lizard of knowledge.
iceblock - Will melt for the lizard of heat. Not a blocker, uses tile collision instead. Iceblock must be 8-pixel aligned.
vator - A moving platform, pattern depends on parameter:
        0 sine X
        1 sine Y
        2 always up, no clip, wrap
        3 always down, no clip, wrap
        4 circular ride top
        5 circular ride bottom
noise - Creates a background noise sound (e.g. blowing wind), X = noise frequency 0-15, Y = volume 0-15.
snow - Particle snow effect.
rain - Particle rain effect.
rain_boss - Particle rain that uses a different palette.
drip - Dripping water.
hold_screen - Used for a screen that's held for time or until button press (map, ending, etc.).
              Waits parameter seconds, or for button press. parameter=255 means infinite hold.
              Y=255 is used for tip screen to return to the previous room.
boss_door - Entrance to a boss. parameter=0-5 selects which boss it belongs to.
            If the boss is unbeaten (or parameter=6), this door will enter link 1.
            If the boss is beaten, this door will instead go to the "start_again" room.
boss_door_rain - Variant of boss door for rain.
boss_door_exit - Not a door, but just an animated closing boss door for when you step into a boss chamber.
boss_door_exeunt - Like boss_door_exit but does not use it glowing palette, instead uses human skin colour. (Ran out of free palettes in some cases.)
boss_rush - A boss_door but it skips the opening animation, and fails to appear if the boss is beaten.
other - The companion you meet after beating a boss, interacting with them using your lizard power goes to a soft ending room.
ending - Triggers execution of the soft ending sequence.
river_exit - Forces player to surf toward the right so they can't botch the ending after the river boss.
bones - Not placeable, but creatures that are killed turn to bones. This dog shows a given sprite and falls off the screen.
easy - Animated "EASY" text that comes down on the title screen.
sprite0 - Displays a named sprite. Use the text comment to name it. (Parameter is auto-filled, bank is auto-selected by transmuting to sprite1/2 as necessary.)
sprite2 - Like sprite0 but for bank 2 sprites. Bank is automatically adjusted.
hintd - A hint arrow that appears if using the knowledge power. Pointing down.
hintu - Hint arrow pointing up.
hintl - Hint arrow pointing left.
hintr - Hint arrow pointing right.
hint_penguin - Penguing but with a hint arrow pointing down to its head, to show that you can jump on it to progress.
bird - Sitting bird that flies away when you get near.
frog - Green frog. Harmless.
grog - Blue frog. Poisonous!
panda - A panda, you can jump over it (hard) or on its head (easier).
goat - Jumps up and down between the ceiling and floor.
dog - A dog that paces back and forth and jumps at you.
      Can jump over a 5 tile gap, but not 6.
      Should be corralled either by empty space on either side of its platform, or tiles blocking the floor in front of it (6 tiles thick).
      Make sure it has overhead space for jumping, as it does not have collision physics.
wolf - Same as dog. (Literally the same code, just felt the sprite variation deserved a new name in the code.)
owl - Parameter=0 first dive is to left, =1 first dive is to right. Blocker.
armadillo - Armadillo.
beetle - Crab that walks left and right randomly that you can stand on. Blocker.
skeetle - Harmful. Not a blocker, can't be used at same time as beetle.
seeker_fish - Fish that follows you around.
manowar - Jellyfish that vaguely puffs in your direction.
snail - Snail that follows the wall.
snapper - Jumping crab.
voidball - Bouncing diagonal ball that wraps through the screen on all sides. (Use ceiling_free.)
ballsnake - Skeleton snake that turns randomly, wraps on all sides. (Use ceiling_free.)
medusa - Large bird, harmful. Parameter is roughly how low they can dive. I think they can touch 36 pixels below this value with their feet.
penguin - Penguin that you can briefly stand on.
mage - Creature that throws a ball of magic that follows you. Requires mage_ball two slots down.
mage_ball - Used in combination with mage, slot should be mage slot + 2.
ghost - Randomly apears and zooms toward the player.
piggy - Piggy the collects coins.
panda_fire - Panda that is burning. (Can't safely stand on its head.)
goat_fire - Goat that is burning. Unused.
dog_fire - Dog that is burning.
owl_fire - Owl that is burning. Can't safely stand on its head? Unused, and the regular owl can't be set on fire.
medusa_fire - Medusa that is burning. Unused.
arrow_left - Arrow that shoots left when you cross its path.
arrow_right - Arrow that shoots right when you cross its path.
saw - Rotating saw.
steam - Steam that shoots periodically. Parameter is the number of frames between bursts of steam.
sparkd - Spark travelling down.
sparku - Spark travelling up.
sparkl - Spark travelling left.
sparkr - Spark travelling right.
         Sparks must be aligned to an 8x8 tile.
         Maximum reach is 8 tiles, you can connect two sparks in a row to make a longer one.
         Connections between sparks are automatically found.
         Sparks are drawn behind the background, and will terminate after entering a solid tile.
         Bits 3-5 of parameter are automatically used to set starting phase for a seamless connection.
frob_fly - The fly from the Frob boss, shares most of its code with the mage_ball.

Bank 1: (dogs1.s)

password - Button for password editing. Parameter=0-4 indicates elemnt of password, parameter=255 makes it a lizard changer button.
lava_palette - Animates palette 3 with lava colours.
water_split - Makes water level apply only on the right half of the screen.
block - A pushable block. Blocker.
block_on - A pushable block that will permanently stop appearing after it has been pushed off an edge and it falls. Parameter is a flag number.
block_off - The pair for block_on, will appear instead only once the block_on has been pushed off and edge. Parameter is a flag number.
drawbridge - A gate with a button to open it. Blocker.
rope - A rope with a block hanging from it. Blocker. Parameter is flag. Pair with block off.
boss_flame - Flames that indicate the defeated bosses, and open the final boss door when all are lit.
river - Controls the river sequence.
river_enter - If you reach the right side of the screen using the Surf power, and the river boss is unbeaten, acts like a pass. (Parameter selects link.)
              Otherwise wiill go to the end_river_again room.
sprite1 - Like sprite0/sprite2 but for bank 1. Bank is automatically corrected.
beyond_star - A spinning icosahedron that turns your lizard into the Lizard of Beyond. Must go in slot 5.
beyond_end - Sets up palettes and monitors when the Lizard touches the beyond_star to end the game. ("Ascended" ending.)
other_end_left - End of game Lizard of Beyond companion, facing right.
other_end_right - End of game  Lizard of Beyond companion, facing right.
particle - White particle (like snow) that floats upward, repeating randomly.
info - Sets up the text for the info screen.
diagnostic - Sets up text for the diagnostic debug screen and animates the timer, etc.
metrics - Sets up the text for the metrics screen (computer in the Void Zone).
super_moose - A moose who has helpful tips. Blocker.
brad_dungeon - A mimic named Julie who thinks she's in a Brad Dungeon.
brad - It's me! Brad!
heep_head - Heep's head.
heep - Heep's body segements.
heep_tail - Heep's tail.
lava_left - Pushes the player left slowly if using Stone power, otherwise bones you.
lava_right - Same but to the right.
lava_left_wide - Etc.
lava_right_wide - Etc.
lava_left_wider - Etc.
lava_right_wider - Etc.
lava_down - Etc. keeps the Stone player from moving horizontally so they don't fall out of the lava stream.
lava_poop - Drops lava randomly from the top of the screen, splashes when it hits a colliding tile.
lava_mouth - The mouth of the lava flow.
bosstopus - Shaft, the octopus boss.
bosstopus_egg - Shaft's eggs.
cat - Bowie, the cat boss.
cat_smile - The smiles the cat throws.
cat_sparkle - Sparkles that follow the cat around.
cat_star - Dangerous stars that move horizontally across the screen. Parameter is speed.
raccoon - Raccon that drinks lava and spits it into the tube for launching.
raccoon_launcher - Launches lava up the tube, monitors/controls the boss state.
raccoon_lavaball - Ball of lava falling from the sky.
raccoon_valve - Valve that you can stand on to close.
frob - Nujan the giant frog.
frob_hand_left - Nujan's left hand.
frob_hand_right - Nujan's right hand.
frob_zap - Nujan's magic eye zaps.
frob_tongue - Nujan's tongue.
frob_block - A block you can push into Nujan's eyes.
frob_platform - Breakable blocks that you can stand on.
queen - The ghost of the Lizard Queen.
hare - Corinna, the boss of the Mountain Zone.
harecicle - Icicles that fall in Corinna's cave.
hareburn - Corinna's fire.
rock - River rock. (Not placeable.)
log - River log. (Not placeable.)
duck - Graveyard, the river duck. (Not placeable.)
ramp - River ramp. (Not placeable.)
river_seeker - Seeker fish adapted for the river. (Not placeable.)
barrel - River barrel, used to attack snek. (Not placeable.)
wave - River wave, thrown by snek. (Not placeable.)
snek_loop - One loop of the snek. (Not placeable.)
snek_head - The head of the snek. (Not placeable.)
snek_tail - The tail of the snek. (Not placeable.)
river_loop - Loops the boss portion of the river sequence, or proceeds to the next screen if beaten. (Not placeable.)
watt - Flaming platforms for the final boss.
waterfall - A waterfall you can climb to get into the Water Zone.

Bank 2: (dogs2.s)

tip - Responsible for drawing tip text to the screen when you've been bonesed in the same spot too many times.
wiquence - Controls the final boss sequence.
witch - The final boss, sitting at their computer.
book - The book next to the final boss.


===============
 Guide: Source
===============

Prerequisites:
- Windows: Visual Studio 2013 Express: https://visualstudio.microsoft.com/vs/older-downloads/
- Linux: GCC
- Mac: Xcode 8: https://developer.apple.com/xcode/
- Windows/Linux/Mac: SDL 2.0.7: https://www.libsdl.org/download-2.0.php
- NES: CC65 2.14: https://cc65.github.io/
- NES: Python 3.4: https://www.python.org/downloads/

If you only wish to test the NES build, only CC65 is needed. (Python is also used, but non-essential.)
If you only wish to test the Windows build, only Visual Studio and SDL are needed.

See SDL2/readme.txt for information about setting up SDL2 libraries.
See nes/cc65/readme.txt for information about setting up CC65.

Windows: Build the lizard project found inside lizard.sln.
Linux: Run the makefile.
Mac: Build the lizard project found in the xcode8/ folder.
NES: Run compile_nes.bat to build the game. Outputs to: nes/temp/

The NES and PC versions of the code usually have functions of corresponding names.
You can search the .s or .cpp files to find th equivalent implementations.
Usually the logic is better commented and easier to follow in the C++ version,
referring to it should help aid comprehension of the NES assembly code.
For the most part I implemented and iterated on things in C++ first, then ported them to NES.

The .s and .cpp files usually have similar names/content (excepting the lizard_ prefix),
though there are differences:
- lizard_dogs.cpp is split into dogs0.s / dogs1.s / dogs2.s on the NES because it does not fit in a single bank.
- lizard_game.cpp is equivalent to main.s, and lizard_main.cpp is instead a PC framework thing.
- lizard_game.cpp also contains a bunch of common routines which are split into multple files in the NES version.

There's no good reason for these differences, other than at some point the names diverged,
and I thought it would be a waste of time to reorganize merely for a more symmetrical look.
In general, this is the end result of a program that has changed and adapted a lot during its development.
Many things may look rough, or sub-optimal, but if I thought they were good enough to finish the game with
I left them alone, rather than chasing beautiful code for its own sake.
Apologies for wherever that makes it harder to follow.


PC code:
- lizard_audio.cpp - Simulation of the NES audio processing unit, and music/sound-effect player code.
- lizard_cheevo.cpp - Achievement tracking. (Only really used by Steam versions of the game.)
- lizard_config.cpp - PC game settings management.
- lizard_dogs.cpp - Implementation of all the dog objects.
- lizard_game.cpp - Main game modes, and a bunch of common game code.
- lizard_linux.cpp - Linux platform.
- lizard_log.cpp - Logging and playback system (useful for automated testing).
- lizard_mac.cpp - Mac platform.
- lizard_main.cpp - Main PC framework, interfaces with SDL.
- lizard_options.cpp - PC options menu.
- lizard_ppu.cpp - Simulation of the NES picture processing unit.
- lizard_text.cpp - Text and language routines.
- lizard_win32.cpp - Windows platform.
- lizard_mac.m - Mac platform objective C stuff.
- dogs_table.h - A big table of all the dogs as redefinable macros.
- dogs_tables.h - Compiles dogs_table.h into several tables.
- enums.h - Enumeration values used throughout the code.
- lizard_platform.h - Common header for platforms.
- lizard_version.h - The build version.
- assets/export/data.h - Exported data from Lizard Tool.
- assets/export/data.cpp - Exported data from Lizard Tool.
- assets/export/icon.h - Exported data from build_icon_h.py.
- assets/export/text_set.h - Exported data from text_export.py.
- assets/export/data_music.h - Exported data from music_export.py.
- assets/export/data_music.cpp - Exported data from music_export.py.

NES code:
- bank0/F.s - Bank management code for each of the 16 x 32KB banks.
- chr.s - CHR loading/unpacking code.
- collide.s - Common collision routines.
- common.s - Various common routines.
- common_df.s - Routines common to banks D and F.
- common_ef.s - Routines common to banks E and F.
- detect_pal.s - Routine for determining region type (NTSC/PAL/Dendy), by Damian Yerrick. See file for information.
- dogs_common.s - Routines common to all dogs.
- dogs_inc.s - Common include for all dogs (imports from dogs_common.s, macros, etc.)
- dogs0.s - Dog group 0. (Bank E.)
- dogs1.s - Dog group 1. (Bank D.)
- dogs2.s - Dog group 2. (Bank F.)
- draw.s - Sprite drawing routines.
- enums.s - Enumeration values used throughout the code.
- extra_a.s - Extra code for bank A.
- extra_b.s - Extra code for bank B.
- fixed.s - Fixed location code that appears in every bank.
- header.s - The iNES file header.
- lizard.s - The Lizard player character controller.
- main.s - The main game loops and modes.
- music.s - Music and sound effects.
- nsf.s - A wrapper for building an NSF music file. (not in demo)
- nsfe.s - A wrapper for building an NSFe music file. (not in demo)
- ram.s - All RAM allocations.
- room.s - Room loading/unpacking code.
- sound_common.s - Macros for playing a sound effect.
- text.s - Text routines.
- version.s - The build version.

The NES banks are arranged roughly like this:
0 - room0
1 - room1
2 - room2
3 - room3
4 - room4
5 - room5
6 - room6
7 - room7
8 - room8
9 - room9
A - CHR1
B - CHR0
C - music
D - dogs1
E - dogs0
F - main, lizard, dogs2

(See the bank#.s files for further clarification of each bank's contents.
There is some miscellaneous content stuffed into many of them.)


Each dog has 3 functions: init, tick, and draw.
- Init runs when the room is loaded.
- Tick runs once per frame if not paused.
- Draw runs once per frame.

These three functions expect the Y register for the dog's slot. (In hindsight: I should have used X.)
The dog_now variable also automatically contains the slot if needed for quick restoration.

The dog in slot 0 is always ticked before the others, which will be ticked in varying order.
The draw calls are organized similarly.


The PC program accepts any entry from its configuration file as a command line option.
E.g. "-music=0" is the same as the line "music=0" in lizard.ini.

There are some hidden configuration options that aren't saved to the configuration file.
Importantly: -lizard_of_debug=1
This enables a bunch of debugging extensions. (These are on by default in the debug build, but not in release.)
A debug menu will be added to the options menu, and cheat codes can now access all lizards.
It also enables some keyboard commands:
    C - display current CHR and palettes
    V - highlight sprites
    G - highlight tile collision
    H - highlight hitboxes and blockers
    D - display dogs info
    F - display framerate
    W/Pause - halt
    E - frame advance
    Q - slow
    TAB - fast
    F12 - screenshot
    F10 - toggle automatic screenshot each frame
    F11 - screenshot + frame advance

The screenshots and lizard.ini configuration file are saved under your appdata folder,
or equivalent on other platforms. Logs are also saved to this folder if enabled in the options menu.
Logs can be used to record and play back the game, very useful for automatically reproducing a bug or other condition.
The log will be saved to log.liz. You can open it with the command line,
or on Windows by dragging that file on top of the executable file.


There are probably many other things I should describe about the source code,
but I had to stop to keep this document from growing too large and unwieldy.
I think the basics are outlined above, and I'll leave if to you to figure out the rest.
Thanks for taking the time to look at it!


=========
 Contact
=========

lizard@lizardnes.com
http://lizardnes.com
