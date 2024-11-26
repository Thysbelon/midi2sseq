#pragma once
#include <vector>
#include <map>
#include "endian.h"
#include "MidiReader.h"

using std::multimap;
using std::pair;

typedef multimap<uchar, int> HeldNoteMap;
typedef HeldNoteMap::iterator HeldNoteIter;
typedef pair<uchar, int> HeldNote;

enum
{
	CNV_UNKNOWN = 0,

	CNV_NOTE, CNV_VOL, CNV_PAN, CNV_EXPR, CNV_PATCH, CNV_LOOPSTART, CNV_LOOPEND, CNV_TEMPO,
	CNV_PITCHBEND, CNV_PITCHBENDRANGE,
	CNV_TRANSPOSE,
	CNV_MODDEPTH,
	CNV_PORTAMENTOTIME,
	CNV_PRIORITY,
	CNV_MODSPEED,
	CNV_MODTYPE,
	CNV_MODRANGE,
	CNV_MODDELAY,
	CNV_NOTEWAIT,
	CNV_PORTAMENTOCTRL,
	CNV_PORTAMENTOSWITCH,
	CNV_ATTACKRATE,
	CNV_DECAYRATE,
	CNV_SUSTAINRATE,
	CNV_RELEASERATE,
	CNV_SWEEPPITCH,
	CNV_RANDOM,
	CNV_USEVAR,
	CNV_IF,
	CNV_SETVAR,
	CNV_ADDVAR,
	CNV_SUBVAR,
	CNV_MULTVAR,
	CNV_DIVVAR,
	CNV_SHIFTVAR,
	CNV_RANDVAR,
	CNV_B7, // placeholder for some math with CNV_SETVAR.
	CNV_EQVAR,
	CNV_GRTEQVAR,
	CNV_GRTVAR,
	CNV_LESSEQVAR,
	CNV_LESSVAR,
	CNV_NOTVAR,
	CNV_TIE,
	CNV_PRINTVAR,
	CNV_MASTERVOL
};

typedef struct
{
	uint time;
	uint duration;
	ushort cmd;
	union
	{
		struct { uchar param1, param2; };
		ushort paramwide;
		ushort paramwide2;
	};
} CnvEvent;

typedef struct
{
	int chnnum;
	int offset;
	vector<CnvEvent>* trackdata;
} CnvTrack;

class SSeqConv
{
	vector<CnvEvent> chn[16];
	HeldNoteMap relData[16];

	uchar chnusage[16];
	bool SaveTrack(FileClass&, CnvTrack&);
public:
	bool ConvertMidi(MidiReader&);
	bool SaveToFile(const char*);
};
