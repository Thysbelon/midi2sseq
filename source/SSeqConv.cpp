#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <strings.h>
#include "SSeqConv.h"

#include <typeinfo>

const int8_t NOT_SET = 0x7f;

typedef struct
{
	uint wait;
	uint pos;
	//uint dataentry;
	int8_t RPNtype[2]={NOT_SET, NOT_SET};
	bool notewait=false;
} TrackStat;

uint64_t whileCheck = 0;

static inline uint CnvTime(uint time, uint tpb)
{
	return (time * 48) / tpb;
}

bool SSeqConv::ConvertMidi(MidiReader& midi)
{
	int ntracks = midi.tracks.size();
	int tpb = midi.GetTicksPerBeat();
	TrackStat* tracksts = (TrackStat*) calloc(ntracks, sizeof(TrackStat));
	int finishtracks = 0;
	memset(chnusage, 0, sizeof(chnusage));

	for (uint time = 0; finishtracks < ntracks; time ++)
	{
		uint cnvtime = CnvTime(time, tpb);

		for (int i = 0; i < ntracks; i ++)
		{
			//printf("i: %d\n", i);
			
			TrackStat* trackst = tracksts + i;

			if (trackst->wait)
			{
				trackst->wait --;
				if (trackst->wait) continue;
			}

			vector<MidiEvent>& track = midi.tracks[i];
			if (track.size() == trackst->pos) continue;

			while (!trackst->wait)
			{
				whileCheck++;
				if (whileCheck % 10000 == 0){printf("whileCheck: %lu. wait: %u. pos: %u.\n", whileCheck, trackst->wait, trackst->pos);}
				
				MidiEvent& midiev = track[trackst->pos];

				CnvEvent ev;
				ev.time = cnvtime;
				ev.duration = 0;
				ev.cmd = CNV_UNKNOWN;

				switch(midiev.cmd)
				{
					case EV_WAIT:
					{
						trackst->wait = midiev.delta;
						break;
					}

					case EV_NOTEON:
					{
						ev.cmd = CNV_NOTE;
						ev.param1 = midiev.note;
						ev.param2 = midiev.vel;
						chn[midiev.chn].push_back(ev);
						chnusage[midiev.chn] = 1;
						relData[midiev.chn].insert(HeldNote(midiev.note, chn[midiev.chn].size() - 1));
						break;
					}

					case EV_NOTEOFF:
					{
						HeldNoteIter it;
						pair<HeldNoteIter, HeldNoteIter> ret = relData[midiev.chn].equal_range(midiev.note);
						for (it = ret.first; it != ret.second; it ++)
						{
							CnvEvent& oldev = chn[midiev.chn][(*it).second];
							oldev.duration = ev.time - oldev.time;
						}
						relData[midiev.chn].erase(midiev.note);
						break;
					}

					case EV_CONTROLLER:
					{
						//uint& dataentry = trackst->dataentry;
						//int8_t RPNtype[]={NOT_SET, NOT_SET};
						switch(midiev.number)
						{
							case 6:
								//if (dataentry == 0)
								// TODO: write a command line tool to reorder same position cc100, cc101, and cc6 in the midi file, so that they are always in the correct order
								if (trackst->RPNtype[0]==0) {
									if (trackst->RPNtype[1]==0){
										printf("found pitch bend range. val: %u\n", midiev.val);
										ev.cmd = CNV_PITCHBENDRANGE;
										ev.param1 = midiev.val;
									} else if (trackst->RPNtype[1]==2){
										printf("found transpose. val: %u\n", midiev.val);
										ev.cmd = CNV_TRANSPOSE;
										ev.param1 = midiev.val - 32; // TODO: The transpose value can be negative, but ev.param1 is an unsigned char. Double check that the byte written to the sseq is correct.
										// TEST
									}
								}
								trackst->RPNtype[0]=NOT_SET; trackst->RPNtype[1]=NOT_SET;
								break;
							case 7:
								ev.cmd = CNV_VOL;
								ev.param1 = midiev.val;
								break;
							case 10:
								ev.cmd = CNV_PAN;
								ev.param1 = midiev.val;
								break;
							case 11:
								ev.cmd = CNV_EXPR;
								ev.param1 = midiev.val;
								break;
							case 100:
								//dataentry &= ~0x7F; // bitwise "and" assignment operator https://www.w3schools.com/c/tryc.php?filename=demo_oper_ass7 , and bitwise
								//dataentry |= (uint)midiev.val;
								trackst->RPNtype[0]=midiev.val;
								break;
							case 101:
								//dataentry &= ~(0x7F<<7);
								//dataentry |= (uint)midiev.val << 7;
								trackst->RPNtype[1]=midiev.val;
								break;
							// start of new entries. TEST
							case 1:
								ev.cmd = CNV_MODDEPTH;
								ev.param1 = midiev.val;
								break;
							case 5: // portamento time
								ev.cmd = CNV_PORTAMENTOTIME;
								ev.param1 = midiev.val;
								break;
							case 14:
								ev.cmd = CNV_PRIORITY;
								ev.param1 = midiev.val;
								break;
							// GotaSequenceLib reads undefined CC20 as pitch bend range. should this program also do that?
							case 21: // undefined midi cc corresponding to sseq mod speed
								ev.cmd = CNV_MODSPEED;
								ev.param1 = midiev.val;
								break;
							case 22: // undefined midi cc corresponding to sseq mod type
								ev.cmd = CNV_MODTYPE;
								ev.param1 = midiev.val;
								break;
							case /*23*/ 3: // undefined midi cc corresponding to sseq mod range. TODO: what is mod range? How is it different from mod depth?
								ev.cmd = CNV_MODRANGE;
								ev.param1 = midiev.val;
								break;
							case 26: // undefined midi cc corresponding to sseq mod delay
								ev.cmd = CNV_MODDELAY;
								ev.param1 = midiev.val;
								break;
							case 126: // mono // TODO: study how DS games use mono; improve this code
								if (trackst->notewait == false) {
									printf("inserted notewait (true).\n");
									ev.cmd = CNV_NOTEWAIT;
									ev.param1 = 1;
									trackst->notewait = true;
								}
								break;
							case 127: // poly
								if (trackst->notewait == true){
									printf("inserted notewait (false).\n");
									ev.cmd = CNV_NOTEWAIT;
									ev.param1 = 0;
									trackst->notewait = false;
								}
								break;
							case 84: // portamento control
								ev.cmd = CNV_PORTAMENTOCTRL;
								ev.param1 = midiev.val;
								break;
							case 65:
								ev.cmd = CNV_PORTAMENTOSWITCH;
								ev.param1 = midiev.val;
								break;
							case 73:
								ev.cmd = CNV_ATTACKRATE;
								ev.param1 = midiev.val;
								break;
							case 75:
								ev.cmd = CNV_DECAYRATE;
								ev.param1 = midiev.val;
								break;
							case 76:
								ev.cmd = CNV_SUSTAINRATE;
								ev.param1 = midiev.val;
								break;
							case 72:
								ev.cmd = CNV_RELEASERATE;
								ev.param1 = midiev.val;
								break;
							/*
							case 9: // undefined
								ev.cmd = CNV_SWEEPPITCH;
								ev.param1 = midiev.val;
								break;
							*/
							
						}
						if (ev.cmd) chn[midiev.chn].push_back(ev); //, chnusage[midiev.chn] = 1;
						break;
					}

					case EV_PITCHBEND:
					{
						// WARNING: midi pitch bend is a 14-bit integer, while sseq pitch bend is a 16-bit signed integer. Conversion cannot be completely lossless.
						ev.cmd = CNV_PITCHBEND;
						int pb = ((int)midiev.valwide - 0x2000) / 64;
						ev.param1 = ((uint)pb) & 0xFF;
						chn[midiev.chn].push_back(ev);
						//chnusage[midiev.chn] = 1;
						break;
					}

					case EV_PATCH: // a.k.a. program change
					{
						ev.cmd = CNV_PATCH;
						ev.param1 = midiev.patch;
						chn[midiev.chn].push_back(ev);
						//chnusage[midiev.chn] = 1;
						break;
					}

					case EV_MARKER:
					{
						if (strcasecmp(midiev.text, "loopStart") == 0)
						{
							ev.cmd = CNV_LOOPSTART;
							for (int j = 0; j < 16; j ++) chn[j].push_back(ev); // push the loopStart event to every sseq channel.
						}else if (strcasecmp(midiev.text, "loopEnd") == 0)
						{
							ev.cmd = CNV_LOOPEND;
							for (int j = 0; j < 16; j ++) chn[j].push_back(ev);
						} else { // TEST
							std::string stringMarker(midiev.text);
							if (stringMarker.substr(0, 7) == "Random:"){
								std::string valString=stringMarker.substr(7);
								size_t commaPos;
								//printf("valString: %s\n", valString.c_str());
								
								commaPos=valString.find(",");
								//printf("valString.substr(0, commaPos): %s\n", valString.substr(0, commaPos).c_str());
								const uint8_t commandByte=static_cast<uint8_t>(std::stoul(valString.substr(0, commaPos), nullptr, 16));
								valString.erase(0,commaPos + 1);
								commaPos=valString.find(",");
								int16_t randMin=std::stoi(valString.substr(0, commaPos));
								valString.erase(0,commaPos + 1);
								int16_t randMax=std::stoi(valString);
								
								ev.cmd=CNV_RANDOM;
								ev.param1 = static_cast<uint8_t>(commandByte);
								ev.paramwide=(ushort)randMin;
								ev.paramwide2=(ushort)randMax;
								
								printf("random: commandByte: 0x%X, randMin: %d, randMax: %d. i: %d\n", commandByte, randMin, randMax, i);
								//printf("random: ev.param1: 0x%X, ev.paramwide: %d, ev.paramwide2: %d. i: %d\n", ev.param1, (int16_t)ev.paramwide, (int16_t)ev.paramwide2, i);
							} else if (stringMarker.substr(0, 7) == "UseVar:") {
								std::string valString=stringMarker.substr(7);
								size_t commaPos;
								
								commaPos=valString.find(",");
								uint8_t commandByte=std::stoul(valString.substr(0, commaPos), nullptr, 16);
								valString.erase(0,commaPos + 1);
								uint8_t varNum=std::stoi(valString);
								
								ev.cmd=CNV_USEVAR;
								ev.param1=(uchar)commandByte;
								ev.param2=(uchar)varNum;
								
								printf("UseVar: commandByte: %X, varNum: %u. i: %d\n", commandByte, varNum, i);
							} else if (stringMarker.substr(0, 3) == "If:") {
								std::string valString=stringMarker.substr(3);
								uint8_t commandByte=std::stoul(valString, nullptr, 16);
								ev.cmd=CNV_IF;
								ev.param1=(uchar)commandByte;
								
								printf("If: commandByte: %X. i: %d\n", commandByte, i);
							} else if (stringMarker.substr(0, 4) == "Var:") { // both assignment and comparison fall under this umbrella
								std::string valString=stringMarker.substr(4);
								size_t commaPos;
								
								commaPos=valString.find(",");
								uint8_t varNum=std::stoi(valString.substr(0, commaPos));
								valString.erase(0,commaPos + 1);
								commaPos=valString.find(",");
								std::string operation=valString.substr(0, commaPos);
								valString.erase(0,commaPos + 1);
								int16_t value=std::stoi(valString);
								
								const char* varMethodName[] = {
									"=", "+=", "-=", "*=", "/=", "[Shift]", "[Rand]", "", 
									"==", ">=", ">", "<=", "<", "!="
								};
								int operIndex=0;
								for (operIndex=0; operIndex<14; operIndex++){
									if (operation == (std::string)varMethodName[operIndex]) {
										break;
									}
								}
								ev.cmd = CNV_SETVAR + operIndex; // CNV vars must be in the correct order in SSeqConv.h
								ev.param1=(uchar)varNum;
								ev.paramwide = (ushort)value;
								
								printf("var com %d: varNum: %u, value: %d. i: %d\n", CNV_SETVAR + operIndex, varNum, value, i);
							} else if (stringMarker.substr(0, 4) == "Tie:") {
								std::string valString=stringMarker.substr(4);
								bool value = (valString == "On" || valString == "on") ? true : false;
								
								ev.cmd = CNV_TIE;
								ev.param1 = (uchar)value;
								
								printf("Tie: value: %d. i: %d\n", value, i);
							} else if (stringMarker.substr(0, 9) == "PrintVar:") {
								std::string valString=stringMarker.substr(9);
								uint8_t varNum=std::stoi(valString);
								
								ev.cmd = CNV_PRINTVAR;
								ev.param1 = (uchar)varNum;
								
								printf("PrintVar: varNum: %u. i: %d\n", varNum, i);
							} else if (stringMarker.substr(0, 11) == "SweepPitch:") {
								std::string valString=stringMarker.substr(11);
								int16_t value=std::stoi(valString);
								printf("sweep pitch: value: %d. i: %d\n", value, i);
								
								ev.cmd = CNV_SWEEPPITCH;
								ev.paramwide = (ushort)value;
							}
							int insertAt = i - 1;/*midi channels start at zero, track numbers start at 1*/
							if (insertAt < 0) insertAt=0;
							if (ev.cmd) chn[insertAt].push_back(ev); // in this code, most events are pushed to the output track of the same number as their midi channel, but markers are assigned to midi tracks, not channels. Marker needs to be pushed to the output track that corresponds to the current midi track. Midi creator needs to make sure that the channel numbers of their midi matches the track numbers TODO: change all other midichn->outtrack assignments to miditrack->outtrack
						}
						break;
					}
			
					case EV_TEMPO:
					{
						ev.cmd = CNV_TEMPO;
						ev.paramwide = (ushort) midiev.tempo;
						chn[0].push_back(ev);
						chnusage[0] = 1;
						break;
					}
					
					case EV_SYSEX:
					{
						// TEST
						ev.cmd = CNV_MASTERVOL;
						ev.param1 = midiev.val;
						chn[0].push_back(ev); // midi sysex events cannot be assigned to a specific channel. And it shouldn't matter which channel the CNV_MASTERVOL event is placed on.
						chnusage[0] = 1;
						printf("master vol: value: %u.\n", midiev.val);
						break;
					}
				}

				trackst->pos ++;
				if (track.size() == trackst->pos)
				{
					finishtracks ++;
					break;
				}
			}
		}
	}

	free(tracksts);
	return true;
}

bool SSeqConv::SaveToFile(const char* filename)
{
	ushort usagemask = 0;
	int ntracks = 0;
	for (int i = 0; i < 16; i ++)
		if (chnusage[i])
		{
			printf("Channel %d, %ld event(s)\n", i+1, chn[i].size());
			usagemask |= 1 << i;
			ntracks ++;
		}

	if (!ntracks) return false;

	FileClass f(filename, "wb");
	if (f.openerror()) return false;

	f.WriteUInt(0x51455353); // SSEQ
	f.WriteUInt(0x0100feff);
	f.WriteUInt(0); // temp size
	f.WriteUShort(16); // struct size
	f.WriteUShort(1); // # of blocks

	f.WriteUInt(0x41544144); // DATA
	f.WriteUInt(0); // size of file - 16
	f.WriteUInt(0x1C); // offset of data

	f.WriteUChar(0xFE); // Multitrack
	f.WriteUShort(usagemask); // usage mask

	int tracktableoff = f.Tell();

	// Skip over track table
	f.Seek(5 * (ntracks - 1), SEEK_CUR);

	CnvTrack* tracks = new CnvTrack[ntracks];

	for (int i = 0, j = 0; i < 16; i ++)
	if (chnusage[i])
	{
		tracks[j].chnnum = i;
		tracks[j].offset = f.Tell() - 0x1C;
		tracks[j].trackdata = &chn[i];
		printf("Saving channel %d...\n", i+1);
		if (!SaveTrack(f, tracks[j]))
		{
			delete [] tracks;
			return false;
		}
		j ++;
	}

	// Come back to track table
	f.Seek(tracktableoff, SEEK_SET);

	for (int i = 1; i < ntracks; i ++)
	{
		f.WriteUChar(0x93);
		f.WriteUInt(tracks[i].chnnum | (tracks[i].offset << 8));
	}

	delete [] tracks;

	// Fix the size values
	f.Seek(0, SEEK_END);
	int fsize = f.Tell();
	f.Seek(8, SEEK_SET);
	f.WriteUInt(fsize);
	f.Seek(20, SEEK_SET);
	f.WriteUInt(fsize - 16);

	return true;
}

bool SSeqConv::SaveTrack(FileClass& f, CnvTrack& trinfo)
{
	// Notewait mode OFF
	f.WriteUChar(0xC7); f.WriteUChar(0); // Notewait should be determined by the midi // UPDATE: nevermind. The default is "notewait on". Notewait off events *are* successfully converted back and forth between sseq2mid and midi2sseq, *but* if the notewait off event happens *at the same time* as some notes, the order can get messed up and place the notes *before* the notewait off, which causes the notes to play after eachother instead of at the same time and makes the entire rest of that track go out of sync.
	// having midi2sseq write notewait off at the start of every song instead of leaving it to the midi *could* have negative consequences if a song is meant to play in notewait on mode, but this scenario seems unlikely. I think most DS composers used notewait off.
	vector<CnvEvent>& data = *trinfo.trackdata;
	uint lasttime = 0;
	int loopOff = f.Tell() - 0x1C; // just in case

	for(uint i = 0; i < data.size(); i ++)
	{
		CnvEvent& ev = data[i];
		uint delta = ev.time - lasttime;
		//printf("%d TIME %u DELTA %u %d/", i, ev.time, delta, ev.cmd);
		if (ev.time < lasttime)
		{
			printf("ERROR: Out of order events! %u < %u\n",  ev.time, lasttime);
			return false;
		}

		if (delta)
		{
			f.WriteUChar(0x80); // REST
			f.WriteVL(delta);
		}

		lasttime = ev.time;

		switch(ev.cmd)
		{
			case CNV_NOTE:
				if (ev.duration)
				{
					f.WriteUChar(ev.param1 & 0x7F);
					f.WriteUChar(ev.param2 & 0x7F);
					f.WriteVL(ev.duration);
				}
				break;
			case CNV_VOL:
				f.WriteUChar(0xC1); // VOLUME
				f.WriteUChar(ev.param1 & 0x7F);
				break;
			case CNV_PAN:
				f.WriteUChar(0xC0); // PAN
				f.WriteUChar(ev.param1 & 0x7F);
				break;
			case CNV_EXPR:
				f.WriteUChar(0xD5); // EXPRESSION
				f.WriteUChar(ev.param1 & 0x7F);
				break;
			case CNV_PITCHBEND:
				f.WriteUChar(0xC4); // PITCH BEND
				f.WriteUChar(ev.param1 & 0xFF);
				break;
			case CNV_PITCHBENDRANGE:
				f.WriteUChar(0xC5); // PITCH BEND RANGE
				f.WriteUChar(ev.param1 & 0x7F);
				break;
			case CNV_PATCH:
				f.WriteUChar(0x81); // PATCH
				f.WriteUChar(ev.param1 & 0x7F);
				break;
			case CNV_LOOPSTART:
				loopOff = f.Tell() - 0x1C;
				break;
			case CNV_LOOPEND:
				f.WriteUChar(0x94); // JUMP
				// TODO: add option in mid2sseq to specify whether it should do loops as a 0x94 jump, or as 0xD4 loop start and 0xFC loop end; and/or have sseq2mid specify what type the original sseq used.
				f.WriteUInt(loopOff);
				break;
			case CNV_TEMPO:
				if (ev.paramwide > 240)
				{
					printf("WARNING: tempo too fast (%d)\n", ev.paramwide);
					break;
				}
				f.WriteUChar(0xE1); // TEMPO
				f.WriteUShort(ev.paramwide);
				break;
			// start of new entries. TEST
			case CNV_TRANSPOSE:
				f.WriteUChar(0xC3);
				f.WriteUChar(ev.param1);
				break;
			case CNV_MODDEPTH:
				f.WriteUChar(0xCA);
				f.WriteUChar(ev.param1);
				break;
			case CNV_PORTAMENTOTIME:
				f.WriteUChar(0xCF);
				f.WriteUChar(ev.param1);
				break;
			case CNV_PRIORITY:
				f.WriteUChar(0xC6);
				f.WriteUChar(ev.param1);
				break;
			case CNV_MODSPEED:
				f.WriteUChar(0xCB);
				f.WriteUChar(ev.param1);
				break;
			case CNV_MODTYPE:
				f.WriteUChar(0xCC);
				f.WriteUChar(ev.param1);
				break;
			case CNV_MODRANGE:
				f.WriteUChar(0xCD);
				f.WriteUChar(ev.param1);
				break;
			case CNV_MODDELAY: // s16
				f.WriteUChar(0xE0);
				//f.WriteUChar(ev.param1);
				f.WriteUShort((ushort)ev.param1);
				break;
			case CNV_NOTEWAIT:
				f.WriteUChar(0xC7);
				f.WriteUChar(ev.param1);
				break;
			case CNV_PORTAMENTOCTRL:
				f.WriteUChar(0xC9);
				f.WriteUChar(ev.param1);
				break;
			case CNV_PORTAMENTOSWITCH:
				f.WriteUChar(0xCE);
				f.WriteUChar(ev.param1);
				break;
			case CNV_ATTACKRATE:
				f.WriteUChar(0xD0);
				f.WriteUChar(ev.param1);
				break;
			case CNV_DECAYRATE:
				f.WriteUChar(0xD1);
				f.WriteUChar(ev.param1);
				break;
			case CNV_SUSTAINRATE:
				f.WriteUChar(0xD2);
				f.WriteUChar(ev.param1);
				break;
			case CNV_RELEASERATE:
				f.WriteUChar(0xD3);
				f.WriteUChar(ev.param1);
				break;
			case CNV_SWEEPPITCH:
				f.WriteUChar(0xE3);
				//f.WriteUChar(ev.param1);
				//f.WriteUShort((ushort)ev.param1);
				f.WriteUShort(ev.paramwide);
				break;
			case CNV_RANDOM:
				f.WriteUChar(0xA0);
				f.WriteUChar(ev.param1);
				f.WriteUShort(ev.paramwide);
				f.WriteUShort(ev.paramwide2);
				break;
			case CNV_USEVAR:
				f.WriteUChar(0xA1);
				f.WriteUChar(ev.param1);
				f.WriteUChar(ev.param2);
				break;
			case CNV_IF:
				f.WriteUChar(0xA2);
				f.WriteUChar(ev.param1);
				break;
			/*
			case CNV_SETVAR:
				f.WriteUChar(0xB0);
				f.WriteUChar(ev.param1);
				f.WriteUShort(ev.paramwide);
				break;
			case CNV_ADDVAR:
				f.WriteUChar(0xB1);
				f.WriteUChar(ev.param1);
				f.WriteUShort(ev.paramwide);
				break;
			case CNV_SUBVAR:
				f.WriteUChar(0xB2);
				f.WriteUChar(ev.param1);
				f.WriteUShort(ev.paramwide);
				break;
			case CNV_MULTVAR:
				f.WriteUChar(0xB3);
				f.WriteUChar(ev.param1);
				f.WriteUShort(ev.paramwide);
				break;
			case CNV_DIVVAR:
				f.WriteUChar(0xB4);
				f.WriteUChar(ev.param1);
				f.WriteUShort(ev.paramwide);
				break;
			case CNV_SHIFTVAR:
				f.WriteUChar(0xB5);
				f.WriteUChar(ev.param1);
				f.WriteUShort(ev.paramwide);
				break;
			case CNV_RANDVAR:
				f.WriteUChar(0xB6);
				f.WriteUChar(ev.param1);
				f.WriteUShort(ev.paramwide);
				break;
			case CNV_EQVAR:
				f.WriteUChar(0xB8);
				f.WriteUChar(ev.param1);
				f.WriteUShort(ev.paramwide);
				break;
			case CNV_GRTEQVAR:
				f.WriteUChar(0xB9);
				f.WriteUChar(ev.param1);
				f.WriteUShort(ev.paramwide);
				break;
			case CNV_GRTVAR:
				f.WriteUChar(0xBA);
				f.WriteUChar(ev.param1);
				f.WriteUShort(ev.paramwide);
				break;
			case CNV_LESSEQVAR:
				f.WriteUChar(0xBB);
				f.WriteUChar(ev.param1);
				f.WriteUShort(ev.paramwide);
				break;
			case CNV_LESSVAR:
				f.WriteUChar(0xBC);
				f.WriteUChar(ev.param1);
				f.WriteUShort(ev.paramwide);
				break;
			case CNV_NOTVAR:
				f.WriteUChar(0xBD);
				f.WriteUChar(ev.param1);
				f.WriteUShort(ev.paramwide);
				break;
			*/
			case CNV_SETVAR:
			case CNV_ADDVAR:
			case CNV_SUBVAR:
			case CNV_MULTVAR:
			case CNV_DIVVAR:
			case CNV_SHIFTVAR:
			case CNV_RANDVAR:
			case CNV_EQVAR:
			case CNV_GRTEQVAR:
			case CNV_GRTVAR:
			case CNV_LESSEQVAR:
			case CNV_LESSVAR:
			case CNV_NOTVAR:
				f.WriteUChar((ev.cmd - CNV_SETVAR) + 0xB0);
				f.WriteUChar(ev.param1);
				f.WriteUShort(ev.paramwide);
				break;
			case CNV_TIE:
				f.WriteUChar(0xC8);
				f.WriteUChar(ev.param1);
				break;
			case CNV_PRINTVAR:
				f.WriteUChar(0xD6);
				f.WriteUChar(ev.param1);
				break;
			case CNV_MASTERVOL:
				f.WriteUChar(0xC2);
				f.WriteUChar(ev.param1);
				break;
		}
	}

	f.WriteUChar(0xFF); // END OF TRACK

	return true;
}
