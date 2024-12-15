#include <stdio.h>
#include <stdlib.h>
#include "MidiReader.h"
#include "endian.h"

#define MIDIMAGIC  0x4D546864
#define MIDIHEADERSIZE 6
#define TRACKMAGIC 0x4D54726B

bool MidiReader::Load(const char* filename)
{
	FileClass f(filename, "rb");
	if (f.openerror()) return false;
	f.SetBigEndian();

	if (f.ReadUInt() != MIDIMAGIC) return false;
	if (f.ReadUInt() != MIDIHEADERSIZE) return false;
	ushort format = f.ReadUShort();
	if (format != 0 && format != 1) return false; // No format 2 for you yet
	ushort trkcnt = f.ReadUShort();
	ushort timediv = f.ReadUShort();
	if (timediv & 0x8000) return false; // No SMPTE time division for you yet
	tpb = timediv;

	for (int i = 0; i < trkcnt; i ++)
	{
		printf("track %d\n", i);
		tracks.push_back(vector<MidiEvent>());
		vector<MidiEvent>& curtrack = tracks.back();
		LoadTrack(f, curtrack);
		if (curtrack.size() == 0) tracks.pop_back();
	}

	return true;
}

bool MidiReader::LoadTrack(FileClass& f, vector<MidiEvent>& track)
{
	printf("Loading track...\n");
	if (f.ReadUInt() != TRACKMAGIC) return false;
	f.Seek(4, SEEK_CUR);

	int lastcmd = 0, cmd;
	uint64_t infForCheck = 0;
	for(;;) // This is an infinite loop
	{
		infForCheck++;
		if (infForCheck % 100000 == 0){
			//printf("infForCheck: %lu. cmd: 0x%X.\n", infForCheck, cmd);
			printf("infForCheck: %lu.\n", infForCheck);
			//if (cmd == 0) {break;}
		}
		
		//printf("ReadVL...\n");
		uint delta = f.ReadVL();
		//printf("ReadVL finished.\n");
		if (delta)
		{
			MidiEvent ev;
			ev.cmd = EV_WAIT;
			ev.delta = delta;
			track.push_back(ev);
		}
		MidiEvent ev;
		ev.cmd = EV_UNKNOWN;
		cmd = f.ReadUChar();
		if (cmd < 0x80) f.Seek(-1, SEEK_CUR), cmd = lastcmd;
		else lastcmd = cmd;

		if (cmd != 0xFF && cmd != 0xF0 && cmd != 0xF7)
		{
			ev.chn = cmd & 0xF;
			switch(cmd & 0xF0)
			{
				case 0x90: // event code
					ev.cmd = EV_NOTEON;
					ev.note = f.ReadUChar();
					ev.vel = f.ReadUChar();
					if (ev.vel == 0) ev.cmd = EV_NOTEOFF;
					break;

				case 0x80:
					ev.cmd = EV_NOTEOFF;
					ev.note = f.ReadUChar();
					ev.vel = f.ReadUChar();
					break;

				case 0xA0:
					ev.cmd = EV_AFTERTOUCH;
					ev.note = f.ReadUChar();
					ev.val = f.ReadUChar();
					break;

				case 0xB0:
					ev.cmd = EV_CONTROLLER;
					ev.number = f.ReadUChar();
					ev.val = f.ReadUChar();
					break;

				case 0xC0:
					ev.cmd = EV_PATCH;
					ev.patch = f.ReadUChar();
					break;

				case 0xD0:
					ev.cmd = EV_AFTERTOUCH2;
					ev.aft = f.ReadUChar();
					break;

				case 0xE0:
					ev.cmd = EV_PITCHBEND;
					ev.valwide = (ushort)f.ReadUChar() | ((ushort)f.ReadUChar() << 7);
					break;
			}
		}else if (cmd == 0xFF) // seems like all meta events use FF
		{
			int metacmd = f.ReadUChar();
			int metasize = f.ReadVL();
			void* buf = NULL;
			if (metasize)
			{
				buf = malloc(metasize+1);
				f.ReadRaw(buf, metasize);
				((char*)buf)[metasize] = 0; // done for strings
			}

			if (metacmd == 0x2F) break; // breaks the infinite loop

			switch(metacmd)
			{
				case 1:
					ev.cmd = EV_TEXT;
					ev.text = (const char*) buf;
					buf = NULL;
					break;

				case 6:
					ev.cmd = EV_MARKER;
					ev.text = (const char*) buf;
					buf = NULL;
					break;

				case 81:
					ev.cmd = EV_TEMPO;
					ev.tempo = 60000000 / be_eswap_u24(*((uint*)buf));
					break;

				default:
					continue;
			}

			if (buf) free(buf);
		} else if (cmd == 0xF0 || cmd == 0xF7) {
			ev.cmd = EV_SYSEX;
			// byte sysexMasterVolume[] = { 0xF0, 0x7F, 0x7F, 0x04, 0x01, 0x00, (byte) volume, 0xF7 }
			// volume is in the range 0-127
			// a length byte is inserted in between the first and second byte of sysexMasterVolume.
			// this sysex implementation only supports one sysex command: master volume.
			// TODO: limit to F0 <length byte> 7F 7F 04 01 00 volume F7
			for (int i=0; i<6; i++){
				f.ReadUChar(); // discard 6 bytes.
			}
			ev.val = f.ReadUChar();
			f.ReadUChar(); // discard final f7 byte.
		}

		track.push_back(ev);
	}

	return true;
}
