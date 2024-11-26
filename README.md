midi2sseq
=========

Introduction
------------

This is a program that converts standard MIDI files into the SSEQ format.

<!--
Build prerequisites
-------------------

- Windows: MinGW
- Everything else: edit the makefile to remove the .exe extension first (TODO: autodetect)

-->

Supported events
----------------

- Note on/off
- Volume, pan & expression
- Pitch bend & pitch bend range
- Patch change
- Tempo
- Loop points via `loopStart` and `loopEnd` markers
- Hopefully everything in [sequence.md](https://github.com/Thysbelon/midi2sseq/blob/master/sequence.md)

Most midi event to sseq command conversions are implemented as Midi undefined CC and text markers. Currently, these are only documented in the [code of my fork of sseq2mid](https://github.com/Thysbelon/sseq2mid/blob/master/src/sseq2mid.c).

<!--

To do
-----

- Implement more MIDI commands
- Some SSEQ commands are not implemented yet (modulation, portamento, pitch sweep): please help!

-->