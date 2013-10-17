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


#include "ScoreGenerator.h"

#include <iostream>
//#include <fstream>
//#include <sstream>
//#include <deque>
using namespace std;

#include <CppAddons/Math.h>
//#include <CppAddons/String.h>
//using namespace String;
#include <CppAddons/Random.h>

#include "Music.h"

namespace Music
{

void GenerateScore(	vector<double>& buffer,			// the score's wav
				vector< vector<int> >& score,	// the score
				double lenght,					// lenght of the score in seconds	: R+*
				int dataBySecond,				// number of data by second			: ~{11000, 22000, 44100}
				double AFreq,					// frequency of A					: ~440.0
				int minHT,						// minimum halftone					: Z		// TODO <0
				int maxHT,						// maximum halftone					: Z
				int nbNotes,					// number of notes in lenght		: N*
				int nbVoice)					// number of voices					: N*
{
	static Random rdm;

	int size = int(lenght*dataBySecond);
	int nbHT = maxHT-minHT+1;
	int note_size = size / nbNotes;
	cout << "AFreq=" << AFreq << " dataBySecond=" << dataBySecond << endl;
	cout << "nbNotes=" << nbNotes << " lenght=" << lenght << " minHT=" << minHT << " maxHT=" << maxHT << endl;
	cout << "size=" << size << " nbHT=" << nbHT << " nbVoice=" << nbVoice << endl;

	cout << "make score ... \t\t" << flush;
//	int n=0;
	for(int i=0; i<nbNotes; i++)
	{
		vector<int> hs(nbVoice);
		for(size_t v=0; v<hs.size(); v++)
			hs[v] = minHT+rdm.nextInt(maxHT-minHT+1);

//		cout << "halftone selected=" << h << " freq=" << freq << endl;
		for(int j=0; j<note_size; j++)
		{
			double buff = 0.0;
			for(size_t v=0; v<hs.size(); v++)
				buff += sin( ((2.0*Math::Pi)/dataBySecond)*h2f(hs[v], AFreq)*double(j) );

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
//			n = undoable_out_percent(cout, 100.0f*index/size, n);
		}
	}
//	undoable_out_clear(cout, n);
	cout << "done   " << endl;
}

}

