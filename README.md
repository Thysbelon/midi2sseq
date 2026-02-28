# midi2sseq

## Introduction

This is a program that converts standard MIDI files into the SSEQ format.

<!--
Build prerequisites
-------------------

- Windows: MinGW
- Everything else: edit the makefile to remove the .exe extension first (TODO: autodetect)

-->

## Demonstration

This software can be used to make [SiIvagunner mashups, known as "high quality rips"](https://siivagunner.wiki/wiki/Rip), of Nintendo DS music that sound accurate to the sound of the DS.   
An example of a SiIvagunner-rip that used midi2sseq in its creation is ["Tiny Town"](https://www.youtube.com/watch?v=n07W7zOkKKs), which was created by myself. [The project files for this SiIvagunner-rip can be found here](https://github.com/Thysbelon/midi2sseq/tree/gh-pages/tiny-town-x-quartz-quadrant-good).

## Supported events

- Note on/off
- Volume, pan & expression
- Pitch bend & pitch bend range
- Patch change
- Tempo
- Loop points via `loopStart` and `loopEnd` markers
- Hopefully everything in [sequence.md](https://github.com/Thysbelon/midi2sseq/blob/master/sequence.md)

Most midi event to sseq command conversions are implemented as Midi undefined CC and text markers. These are documented in the [README of my fork of sseq2mid](https://github.com/Thysbelon/sseq2mid?tab=readme-ov-file).

<!--

To do
-----

- Implement more MIDI commands
- Some SSEQ commands are not implemented yet (modulation, portamento, pitch sweep): please help!

-->

## How to Edit NDS Music

Ready-to-run copies of ndstool can be downloaded from [Leseratte's website](https://wii.leseratte10.de/devkitPro/other-stuff/ndstool/). Scroll to the bottom and download the latest version for your OS. Decompress it, which will give you a **folder** named "ndstool-version-OS_arch.pkg". Go into the folder, then you should find ndstool on the path "opt/devkitpro/tools/bin/".

[Guide on how to open command prompt in a certain folder on Windows](https://www.howtogeek.com/789662/how-to-open-a-cmd-window-in-a-folder-on-windows/).

The terminal commands listed below are examples; please replace placeholders with the names of your files.  
Instances of `./ndstool` are for linux, Windows users should type `.\ndstool`.    
**The instructions below are not step-by-step, they are only an outline**, because the instructions skip a few minor steps relating to file and folder management.

1. Use [VGMTrans](https://github.com/vgmtrans/vgmtrans) to find the song you want to edit and to rip the sound bank to sf2.
2. Copy and paste the nds file to the same folder, and name the copy something like "Game (header).nds".
3. Run ndstool with the command `./ndstool -x "Game.nds" -d rom-files -9 arm9.bin -7 arm7.bin -y9 arm9overlay.bin  -y7 arm7overlay.bin  -y overlay-files -h "Game (header).nds"` to extract the rom's files.
4. Run [SDATTool](https://github.com/froggestspirit/SDATTool) with `python3 SDATTool.py -u sound_data.sdat sdat_contents` to extract the SDAT's files.
5. Run [sseq2mid](https://github.com/Thysbelon/sseq2mid) to convert the sseq file containing the song you wish to edit to midi.
6. Edit the midi in your preferred midi editor. Please use a midi editor that supports multiple tracks in midi files (type 1 midi files). This is needed so that text markers corresponding to sseq commands are placed on the correct tracks. The [sseq2mid](https://github.com/Thysbelon/sseq2mid) README contains additional info on song editing.
7. Run [midi2sseq](https://github.com/Thysbelon/midi2sseq/releases/latest) to convert your edited midi to sseq.<!-- Delete the original sseq in the folder and give your modified version the same filename as the original. Place the outputted sseq in the folder containing the original SDAT's contents (or a copy of that folder). --> Replace the original sseq in the SDAT folder with your modified sseq (it should have the same filename and location).
8. Run SDATTool with `python3 SDATTool.py -b sound_data_modified.sdat sdat_contents_modified` to build the files into a new SDAT.
9. In the folder containing the rom's files, replace the original SDAT with your modified SDAT.<!--Place the new SDAT into the folder containing the rom's files.--> Build a new rom by running ndstool with the command `./ndstool -c "Game-modified.nds" -d rom-files-modified -9 arm9.bin -7 arm7.bin -y9 arm9overlay.bin  -y7 arm7overlay.bin  -y overlay-files -h "Game (header).nds"`.
