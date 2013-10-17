// Copyright 2004 "Gilles Degottex"

// This file is part of "Music"

// "Music" is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 2.1 of the License, or
// (at your option) any later version.
// 
// "Music" is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


#ifndef _ScoreGenerator_h_
#define _ScoreGenerator_h_

#include <assert.h>
#include <vector>
#include <deque>
#include <iostream>
#include <algorithm>
using namespace std;

#include <CppAddons/Math.h>
#include <CppAddons/Random.h>
//#include <CppAddons/String.h>

#include "Music.h"
#include "Instrument.h"

namespace Music
{

//! generate a randomly generated score with \ref Instrument
/*!
 * \param buffer the outputed wave
 * \param score the corresponding outputed score
 * \param instrs the instruments
 * \param lenght lenght of the score in seconds		: R+*
 * \param dataBySecond number of data by second		: ~{11000, 22000, 44100}
 * \param AFreq frequency of the A3					: ~440.0
 * \param minHT minimum halftone					: Z		// TODO <0
 * \param maxHT maximum halftone					: Z
 * \param nbNotes maximum number of notes in lenght			: N*
 * \param nbInstr maximum number of instruments played at the same time		: N*
*/
inline void GenerateScoreFromInstruments(
		deque<double>& buffer,
		vector< vector<int> >& score,
		vector<Instrument*>& instrs,
		double lenght,
		int dataBySecond,
		double AFreq,
		int minHT,
		int maxHT,
		int nbNotes,
		int nbInstr)
{
	int size = int(lenght*dataBySecond);
	int nbHT = maxHT-minHT+1;
	int note_size = size / nbNotes;
	//		cout << "AFreq=" << AFreq << " dataBySecond=" << dataBySecond << endl;
	//		cout << "nbNotes=" << nbNotes << " lenght=" << lenght << " minHT=" << minHT << " maxHT=" << maxHT << endl;
	//		cout << "size=" << size << " nbHT=" << nbHT << endl;

	// TODO compute upHT so every harm are present in the range of minHT maxHT
	//	int maxNbHarm = 0;
	//	for(int i=0; i<instrs.size(); i++)
	//	{
	//		if(instrs[i]->getFreqs()!=NULL)
	//		{
	//			int n = instrs[i]->getFreqs()->size();
	//			maxNbHarm = std::max(maxNbHarm, n);
	//		}
	//	}
	//	int upHT = (int)f2h(AFreq*(maxNbHarm+1), AFreq);
	//	if(upHT>maxHT-minHT)	throw string("not enough HT for covering all harmonics");
	//	cout << upHT << endl;

	//	cout << "make score ... " << instrs.size() << endl;
	int t=0;
	if(score.size()!=(size_t)(nbNotes*note_size))
		score.resize(nbNotes*note_size);

	assert((size_t)nbInstr<=instrs.size());

	for(int i=0; i<nbNotes; i++)
	{
		vector<int> hs(instrs.size());
		vector< Math::polar<double> > ps(instrs.size());
		vector<bool> play(instrs.size());
		for(size_t w=0; w<play.size(); w++)	play[w] = false;
		int count_playing=0;
		while(count_playing<nbInstr)
		{
			int index = int(Random::s_random.nextDouble()*play.size());
			if(!play[index])
			{
				play[index] = true;
				count_playing++;
			}
		}
		for(size_t v=0; v<hs.size(); v++)
		{
//			if(play[v])
			{
//			hs[v] = minHT+g_random.nextInt(maxHT-minHT+1);
//			hs[v] = minHT+g_random.nextInt(maxHT-minHT-upHT+1);
// TODO !!!!!!!!!!!!!!
//			int up = (int)f2h(AFreq*(instrs[v]->getFreqs()->size()), AFreq);
//			int up = instrs[v]->getFreqs()->size();
			int up = 36;
//			cout << "up=" << up << endl;
//			assert(up<maxHT-minHT);
			hs[v] = minHT+Random::s_random.nextInt(maxHT-minHT-up+1);
//			hs[v] = minHT+g_random.nextInt(maxHT-minHT+1);
			ps[v] = Math::polar<double>(Random::s_random.nextDouble()/nbInstr, Random::s_random.nextDouble()*2*Math::Pi);
//			ps[v] = Math::polar<double>(1.0/nbInstr, g_random.nextDouble()*2*Math::Pi);
//			cout << "halftone choosen " << hs[v] << endl;
			}

//			cout << ps[v].mod << " ";
		}
//		cout << endl;

//		if(i==5)hs[0]=-12;
//		else	hs[0]=-48;

//		for(int v=0; v<hs.size(); v++)
//			cout << "halftone choosen " << hs[v] << " play=" << play[v] << endl;

		for(int j=0; j<note_size; j++)
		{
			double buff = 0.0;
			for(size_t v=0; v<instrs.size(); v++)
				if(play[v])
					buff += ps[v].mod*instrs[v]->gen_value(hs[v], ps[v].arg);

			buffer.push_back(buff);

			vector<int> score_at(nbHT);
			for(size_t h=0; h<score_at.size(); h++)
				score_at[h] = 0;

			for(size_t v=0; v<instrs.size(); v++)
				if(play[v] && ps[v].mod>0.05)					  // TODO 0.1: note is here if we hear it !
					score_at[hs[v]-minHT]++;

			score[t] = score_at;
			t++;
		}
	}
}

//! generate a randomly generated sinusoid score
/*!
 * \param buffer the outputed wave
 * \param score the outputed score
 * \param lenght lenght of the score in seconds		: R+*
 * \param dataBySecond number of data by second		: ~{11000, 22000, 44100}
 * \param AFreq frequency of the A3					: ~440.0
 * \param minHT minimum halftone					: Z		// TODO <0
 * \param maxHT maximum halftone					: Z
 * \param nbNotes number of notes in lenght			: N*
 * \param nbVoice number of voices					: N*
*/
inline void GenerateScore(	deque<double>& buffer,
		vector< vector<int> >& score,
		double lenght,
		int dataBySecond,
		double AFreq,
		int minHT,
		int maxHT,
		int nbNotes,
		int nbVoice)
{
	int size = int(lenght*dataBySecond);
	int nbHT = maxHT-minHT+1;
	int note_size = size / nbNotes;
	//	cout << "AFreq=" << AFreq << " dataBySecond=" << dataBySecond << endl;
	//	cout << "nbNotes=" << nbNotes << " lenght=" << lenght << " minHT=" << minHT << " maxHT=" << maxHT << endl;
	//	cout << "size=" << size << " nbHT=" << nbHT << " nbVoice=" << nbVoice << endl;

	//	cout << "make score ... \t\t" << flush;
//	int n=0;
	for(int i=0; i<nbNotes; i++)
	{
		vector<int> hs(nbVoice);
		for(size_t v=0; v<hs.size(); v++)
		{
			hs[v] = minHT+Random::s_random.nextInt(maxHT-minHT+1);
			//			cout << "halftone choosen " << hs[v] << endl;
		}

		vector<double> decals(nbVoice);
		for(size_t v=0; v<decals.size(); v++)
			decals[v] = Random::s_random.nextDouble()*Math::Pi/2;
		//			decals[v] = g_random.nextDouble()*2*Math::Pi;

		//		cout << "halftone selected=" << h << " freq=" << freq << endl;
		for(int j=0; j<note_size; j++)
		{
			double buff = 0.0;
			for(size_t v=0; v<hs.size(); v++)
				buff += sin( ((2.0*Math::Pi)/dataBySecond)*h2f(hs[v], AFreq)*double(j) + decals[v]);

//			int index = i*note_size+j;
			// add data index and wav value

			buffer.push_back(buff);
			// add score halftones (solution)
			vector<int> score_at(nbHT);
			for(int vh=0; vh<nbHT; vh++)
			{
				int ampl = 0;
				for(size_t v=0; v<hs.size(); v++)
					if(hs[v]==vh+minHT)
						ampl++;
				score_at[vh] = ampl;
			}
			score.push_back(score_at);
			//			n = String::undoable_out_percent(cout, 100.0f*index/size, n);
		}
	}
	//	String::undoable_out_clear(cout, n);
	//	cout << "done   " << endl;
}

}

#endif // _ScoreGenerator_h_

