#include <stdio.h>
#include "MidiReader.h"
#include "SSeqConv.h"

int usage(const char* prog)
{
	printf("Usage:\n\n%s midifile sseqfile\n", prog);
	return 0;
}

int error(const char* msg)
{
	printf("Error: %s\n", msg);
	return 1;
}

int main(int argc, char* argv[])
{
	if (argc != 3) return usage(argv[0]);

	SSeqConv cnv;

	{
		MidiReader midi;
		printf("Reading midi file...\n");
		if (!midi.Load(argv[1])) return error("Invalid MIDI file!");
		printf("Converting midi...\n");
		if (!cnv.ConvertMidi(midi)) return error("Cannot convert MIDI events!");
		// Free the midi data
	}

	printf("Saving sseq file...\n");
	if (!cnv.SaveToFile(argv[2])) return error("Cannot save to file!");

	return 0;
}
