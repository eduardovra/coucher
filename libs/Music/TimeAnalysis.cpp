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


#include "TimeAnalysis.h"

#include <cassert>
#include <cmath>
#include <deque>
#include <iostream>
#include <limits>
using namespace std;
#include <CppAddons/Math.h>
using namespace Math;

#include "Music.h"

//#define MUSIC_DEBUG
#ifdef MUSIC_DEBUG
#define LOG(a)	a
#else
#define LOG(a)	
#endif

namespace Music
{
	double InterpolatedWaveLength(const std::deque<double>& queue, int left, int right)
	{
		double l = left - queue[left]/(queue[left+1]-queue[left]);

		double r = right - queue[right]/(queue[right+1]-queue[right]);

		return r - l;
	}

	/*
	 * on peut imaginer des cas qui mettent en échec cette procédure:
	 *	on selectionne un zéro qui n'en n'est pas un une longeur d'onde plus
	 *	tard et si un autre zéro se trouve dans la zone de tolérance la longeur
	 *	ainsi calculée entre ces deux zéro (qui ne se correspondent donc pas) sera fausse.
	 *	example: une fréquence très basse avec une seule harmonique très très
	 *	haute.
	 *	- il faut utiliser des zéros significatifs ... et ... et ... et voilà.
	 *	- ou encore écarter les solutions trop élognées de la moyenne (il ne
	 *	faudrait pas utiliser la moyenne, mais plutôt une autre fonction
	 *	(souvient plus du nom, demander à Jan))
	 */
	double GetAverageWaveLengthFromApprox(const std::deque<double>& queue, size_t approx, int n, double AFreq, int sampling_rate)
	{
		if(AFreq!=0.0f)		assert(sampling_rate>0);

		double wave_length = 0.0f;
		if(queue.size()<2)	return 0.0f;

		deque<int> ups;									// the upper peeks

		// parse the whole buffer, for n zeros
		for(int i=0; int(ups.size())<n && i+1<int(queue.size()); i++)
			if(queue[i]<=0 && queue[i+1]>0)				// if it cross the axis
				ups.push_back(i);

		if(ups.size()<1)	return 0.0f;

		int count = 0;

//		cerr << "approx=" << approx << " wave_length=(";
		for(int i=0; i<int(ups.size()); i++)
		{
			int i_seek = int(ups[i] + approx);

			if((size_t)i_seek<queue.size())
			{
				int lower_i_seek = i_seek;
				int higher_i_seek = i_seek;

				if(!(queue[i_seek]<=0 && queue[i_seek+1]>0))
				{
					while(!(queue[lower_i_seek]<=0 && queue[lower_i_seek+1]>0))
						lower_i_seek--;

					while(!(queue[higher_i_seek]<=0 && queue[higher_i_seek+1]>0))
						higher_i_seek++;

					if(i_seek-lower_i_seek < higher_i_seek-i_seek)
						i_seek = lower_i_seek;
					else
						i_seek = higher_i_seek;
				}

				bool ok=true;
				if(AFreq!=0.0f)
				{
					double ht = f2hf(double(sampling_rate)/approx, AFreq);
					int low_bound = int(sampling_rate/h2f(ht+1, AFreq));
					int high_bound = int(sampling_rate/h2f(ht-1, AFreq));
					ok = i_seek>ups[i]+low_bound && i_seek<ups[i]+high_bound;
				}
				if(ok)
				{
//					cerr << "["<<ups[i]<<"{"<<low_bound<<"<"<<lower_i_seek-ups[i]<<","<<higher_i_seek-ups[i]<<"<"<<high_bound<<"}"<< i_seek - ups[i] << "] ";

					wave_length += InterpolatedWaveLength(queue, ups[i], i_seek);

					count++;
				}
			}
		}

		wave_length /= count;

//		cerr << ")=" << wave_length << ":" << 48000*1.0/wave_length << "(" << count << ")" << endl;

		return wave_length;
	}

	void GetWaveSample(const std::deque<double>& queue, size_t wave_length, std::deque<double>& sample)
	{
		assert(wave_length>0);
		if(queue.size()<2*wave_length)	return;

		// find the highest peek in the second period
		int left = 0;
		double max_vol = 0;
		for(int i=int(wave_length); i<int(queue.size()) && i<int(2*wave_length); i++)
		{
			if(queue[i]>max_vol)
			{
				max_vol = queue[i];
				left = i;
			}
		}

		// go back to the previous zero
		while(left>=0 && !(queue[left]<=0 && queue[left+1]>0))
			left--;

		// get the right bound
		int right = int(left + wave_length);

		// adjust the right bound to the nearest zero
		int left_right = right;
		int right_right = right;
		while(left_right>=0 && !(queue[left_right]<=0 && queue[left_right+1]>0))
			left_right--;
		while(right_right+1<int(queue.size()) && !(queue[right_right]<=0 && queue[right_right+1]>0))
			right_right++;
		if(right-left_right < right_right-right)
			right = left_right;
		else
			right = right_right;

		// fill in the sample
		sample.clear();	
		for(int i=left; i<int(queue.size()) && i<right; i++)
			sample.push_back(queue[i]);
	}
}

