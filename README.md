# midi2sseq

## Introduction

This is a program that converts standard MIDI files into the SSEQ format.

<!--
Build prerequisites
-------------------

- Windows: MinGW
- Everything else: edit the makefile to remove the .exe extension first (TODO: autodetect)

-->

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

- Use [VGMTrans](https://github.com/vgmtrans/vgmtrans) to find the song you want to edit and to rip the sound bank to sf2
- Copy and paste the nds file to the same folder, and name the copy something like "Game (header).nds"
- Run ndstool with the command `./ndstool -x "Game.nds" -d rom-files -9 arm9.bin -7 arm7.bin -y9 arm9overlay.bin  -y7 arm7overlay.bin  -y overlay-files -h "Game (header).nds"` to extract the rom's files
- Run [SDATTool](https://github.com/froggestspirit/SDATTool) with `python3 SDATTool.py -u sound_data.sdat sdat_out` to extract the SDAT's files
- Run [sseq2mid](https://github.com/Thysbelon/sseq2mid)
- Edit the midi in your preferred midi editor
- Run [midi2sseq](https://github.com/Thysbelon/midi2sseq/releases/latest). Delete the original sseq in the folder and give your modified version the same filename as the original. Place the outputted sseq in the folder containing the original SDAT's contents (or a copy of that folder).
- Run SDATTool with `python3 SDATTool.py -b sound_data_modified.sdat sdat_out_modified` to build the files into a new SDAT
- Place the new SDAT into the folder containing the rom's files. Build a new rom by running ndstool with the command `./ndstool -c "Game-modified.nds" -d rom-files-modified -9 arm9.bin -7 arm7.bin -y9 arm9overlay.bin  -y7 arm7overlay.bin  -y overlay-files -h "Game (header).nds"`
