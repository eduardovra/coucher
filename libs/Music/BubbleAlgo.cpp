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


#include "BubbleAlgo.h"

#include <cassert>
#include <cmath>
#include <deque>
#include <iostream>
#include <limits>
using namespace std;
#include <CppAddons/Math.h>
using namespace Math;

#include "Music.h"

#define MUSIC_DEBUG
#ifdef MUSIC_DEBUG
	#define LOG(a)	a
#else
	#define LOG(a)	
#endif


namespace Music
{
	int gcd(int a, int b)
	{
		if (b == 0) return a;

		return gcd(b, a%b);
	}
	int gcd(vector<int> v)
	{
		if(v.empty())	return -1;

		int s = v[0];
		for(size_t i=1; i<v.size(); i++)
			s = gcd(s, v[i]);

		return s;
	}

	void BubbleAlgo::init()
	{
		m_min_length = int(GetSamplingRate()/h2f(GetSemitoneMax()));
		m_max_length = int(GetSamplingRate()/h2f(GetSemitoneMin()));
		setMinMaxLength(m_min_length, m_max_length);
		m_bubbles.resize(m_max_length);
		m_waves.resize(m_max_length);
		m_waves_best.resize(m_max_length);

		cerr << "BubbleAlgo::init [" << m_min_length << ";" << m_max_length << "]" << endl;

		m_error_threshold = 0.1;
		m_conv_threshold = 0.1;
		double gauss_factor = 2.0;
		size_t latency_factor = 2;

		double u = Usefull(Win_Sinc(gauss_factor));

		for(size_t s=m_min_length; s<m_max_length; s++)
		{
			m_bubbles[s].s = s;
			m_bubbles[s].length = s;
			while(m_bubbles[s].length<100)
				m_bubbles[s].length += s;

			m_waves[s].resize(latency_factor*s);
			m_waves_best[s].resize(m_waves[s].size());
//			double c = - 2.0*Math::Pi * m_freq / sampling_rate;
			double c = - 2.0*Math::Pi / s;
			double d = (2.0/m_waves[s].size());

			for(size_t j=0; j<m_waves[s].size(); j++)
				m_waves[s][j] = exp(complex<double>(0.0, c*j)) * d * win_sinc(j/double(m_waves[s].size()), gauss_factor)/u;

			complex<double> b(0.0,0.0);
			for(int i=s-1; i>=0; i--)
			{
				for(size_t l=0; l<latency_factor; l++)
					b += m_waves[s][i+l*s]*sin(2.0*Math::Pi*(i+l*s)/s);
				m_waves_best[s][i] = normm(b);
//				if(s==109)
//					cerr<<"nv="<<m_waves_best[s][i]<<endl;
			}
		}

		cerr << "BubbleAlgo::init " << m_waves.back().size() << endl;
	}

	BubbleAlgo::BubbleAlgo()
	: Algorithm(0.1)
	{
		init();
	}

	//	double err = abs(buff[s] - buff[0])/((buff[s]+buff[0])/2);
	//	double err = abs(buff[s] - buff[0])/(max(abs(buff[s]),abs(buff[0])));

	/*
	 * * normalize errors ?
	 * for too high results
	 *   * longer correlation length
	 *     6 is not statisticaly a valuable average size ...
	 *   * random shifting
	 *     + kill high errors
	 *     - more latency (*1.5 or more)
	 * for too low results
	 *   * divide by num of present multiples in the sort
	 *
	 * max is not stable enough
	 * difficult to use conv because there is sound with fondamental with zero energy
	 */
	void BubbleAlgo::apply(const deque<double>& buff)
	{
		m_wave_length = 0;

		if(buff.size()<m_waves.back().size())	return;

		LOG(cerr<<"BubbleAlgo::apply min_length="<<m_min_length<<" max_length="<<m_max_length<<endl;)

		Type max_vol = 0.0;
		for(size_t i=0; i<m_max_length; i++)
			max_vol = max(Type(max_vol), Type(buff[i]));

		m_error_threshold = 0.33;
		m_conv_threshold = 0.0;
		// use relative thresholds
		double err_threshold = m_error_threshold*max_vol;
		size_t err_drop = 0, err_drop_t = 0;
		double conv_threshold = m_conv_threshold*max_vol;
		size_t conv_drop = 0;
		size_t score_drop = 0, score_drop_t = 0;
		size_t sgn_drop = 0, sgn_drop_t = 0;
		size_t mult_drop = 0, mult_drop_t = 0;

		// init
		m_sort.clear();
		m_best_set.clear();
		size_t map_size = 0;
		size_t max_n = 0;
		for(size_t s=m_min_length; s<m_max_length; s++)
		{
			m_bubbles[s].score = 0.0;
			m_bubbles[s].err = 0.0;
			m_bubbles[s].conv = complex<double>(0.0,0.0);
			m_bubbles[s].count = 0;
			m_bubbles[s].todrop = false;
			m_bubbles[s].gcd_count = 0;

			if(sgn(buff[s])==sgn(buff[0]) || sgn(buff[s+1])==sgn(buff[1]))
			{
				m_bubbles[s].err = diff(buff, s, 0);
//				m_bubbles[s].conv = m_waves[s][0] * buff[0];
//				m_bubbles[s].conv += m_waves[s][s] * buff[s];
				m_bubbles[s].count++;
//				m_bubbles[s].score = max(m_bubbles[s].err, 1.0-normm(m_bubbles[s].conv));
				m_bubbles[s].score = m_bubbles[s].err;

//				if(m_bubbles[s].err<err_threshold && normm(m_bubbles[s].conv)>conv_threshold/s)
				if(m_bubbles[s].err<err_threshold)
				{
					m_sort[m_bubbles[s].score].push_back(s);
					map_size++;
					max_n += s;
				}
			}
		}
		size_t init_map_size = map_size;
		LOG(cerr<<"map size="<<map_size<<" ("<<int(log(float(map_size))+1)<<")  droped="<<(m_max_length-m_min_length)-map_size<<endl;)

		if(map_size==0)	return;

		// seek
		bool search = true;
		size_t n=0;
		size_t f=0;
		size_t current_best = 0;
		while(!m_sort.empty() && search)
		{
			n++;

			// extract the next bubble to raise, lower or drop in the chart
			size_t s = (*(m_sort.begin())).second.front();
			(*(m_sort.begin())).second.pop_front();
			map_size--;
			if((*(m_sort.begin())).second.empty())
				m_sort.erase(m_sort.begin());

			// 2. should be dropped by multiplicity
			if(m_bubbles[s].todrop)
			{
				mult_drop++;
				mult_drop_t += n;
			}
			// 1. if sign is the same (or nearly)
			else if(!(sgn(buff[s+m_bubbles[s].count])==sgn(buff[m_bubbles[s].count]) || sgn(buff[s+m_bubbles[s].count+1])==sgn(buff[m_bubbles[s].count+1])))
			{
				sgn_drop++;
				sgn_drop_t += n;
			}
			else
			{
				m_bubbles[s].err += diff(buff, s+m_bubbles[s].count, m_bubbles[s].count);
//				if(m_bubbles[s].count<s)
//				{
//					m_bubbles[s].conv += m_waves[s][m_bubbles[s].count] * buff[m_bubbles[s].count];
//					m_bubbles[s].conv += m_waves[s][m_bubbles[s].count+s] * buff[m_bubbles[s].count+s];
//				}
				m_bubbles[s].count++;
//				m_bubbles[s].score = max((m_bubbles[s].err/m_bubbles[s].count), 1.0-normm(m_bubbles[s].conv));
				m_bubbles[s].score = m_bubbles[s].err/m_bubbles[s].count;

				// if finished
				if(m_bubbles[s].count>=m_bubbles[s].length)
				{
					if(m_bubbles[s].err/m_bubbles[s].count>err_threshold)
						err_drop++;
//					else if(normm(m_bubbles[s].conv)<conv_threshold)
//						conv_drop++;
					else
					{
						m_best_set[m_bubbles[s].score].push_back(s);
						m_bubbles[s].n = n;
						m_bubbles[s].f = f;
						f++;
						if(f==1 || m_bubbles[s].score<m_bubbles[current_best].score)
							current_best = s;

						// drop nearby and all mult bubble
						size_t ms=2*s;
						if(s+1<m_max_length)	m_bubbles[s+1].todrop = true;
						if(s-1>=m_min_length)	m_bubbles[s-1].todrop = true;
						while(ms<m_max_length)
						{
							m_bubbles[ms].todrop = true;
							if(ms+1<m_max_length)	m_bubbles[ms+1].todrop = true;
							if(ms-1>=m_min_length)	m_bubbles[ms-1].todrop = true;
							ms += s;
						}
						size_t d=2;
						m_bubbles[s].gcd_count++;
//						int old_ds = s;
						while(s/d>=m_min_length)
						{
							size_t ds1 = s/d;
							size_t ds2 = 1+ds1;
//							size_t ds2 = ds1;
//							for(int i=old_ds-1; i>ds2; i--)
//								m_bubbles[ds1].todrop = true;
//							old_ds = ds1;
							m_bubbles[ds1].gcd_count++;
							if(m_bubbles[ds1].gcd_count<f)
								m_bubbles[ds1].todrop = true;
							m_bubbles[ds2].gcd_count++;
							if(m_bubbles[ds2].gcd_count<f)
								m_bubbles[ds2].todrop = true;

							d++;
						}
					}
				}
				// if there is no chance for s to be lower than error threshold
//				else if(m_bubbles[s].err/s>err_threshold)
//				{
//					err_drop++;
//					err_drop_t += n;
//				}
				else
				{
					// all test past, can continue
					m_sort[m_bubbles[s].score].push_back(s);
					map_size++;
				}
			}

//			if(f>0)	search = false;

			// all possible multiples have been finished
			if(f>m_max_length/m_min_length)	search = false;
		}

		int best_tot_c = 0;

		size_t gcds = 0;

		for(map<Type,list<size_t> >::iterator it=m_best_set.begin(); it!=m_best_set.end(); ++it)
		{
			for(list<size_t>::iterator itl=(*it).second.begin(); itl!=(*it).second.end(); ++itl)
			{
				size_t s = *itl;
//				if(m_bubbles[s].gcd_count>1)
				{
					LOG(cerr<<m_bubbles[s].n<<"("<<int(100*float(m_bubbles[s].n)/n)<<"%) "<<m_bubbles[s].f<<": finished score="<<m_bubbles[s].score<<" ("<<m_bubbles[s].err/m_bubbles[s].count<<","<<1.0-normm(m_bubbles[s].conv)<<") s="<<s<<" "<<m_bubbles[s].gcd_count<<" "<<m_bubbles[s].todrop<<" :"<<h2n(f2h(48000.0f/s))<<endl;)
				best_tot_c += m_bubbles[s].count;

				}
				if(m_bubbles[s].gcd_count>=f)
					if(gcds<s)	gcds = s;
			}
		}

		LOG(
		cerr<<n<<": f=" << f << " bests count="<<int(10000*float(best_tot_c)/n)/100.0f<<"% max n="<<max_n<<endl;
		cerr<<"finished map size="<<map_size<<" ("<<int(log(float(map_size))+1)<<")" << endl;

		cerr<<"drops=[err="<<err_drop<<"("<<100*err_drop/init_map_size<<"%)";
		if(err_drop>0)	cerr<<"(pos:"<<100*err_drop_t/err_drop/n<<"%)";
		cerr<<",conv="<<conv_drop;
		cerr<<",score="<<score_drop<<"("<<100*score_drop/init_map_size<<"%)";
		if(score_drop>0)	cerr<<"(pos:"<<100*score_drop_t/score_drop/n<<"%)";
		cerr<<",sgn="<<sgn_drop<<"("<<100*sgn_drop/init_map_size<<"%)";
		if(sgn_drop>0)	cerr<<"(pos:"<<100*sgn_drop_t/sgn_drop/n<<"%)";
		cerr<<",mult="<<mult_drop<<"("<<100*mult_drop/init_map_size<<"%)";
		if(mult_drop>0)	cerr<<"(pos:"<<100*mult_drop_t/mult_drop/n<<"%)";
		cerr<<"]"<<endl;
		)

		if(f==0)
			m_wave_length = 0;
		else
		{
//			m_wave_length = (*m_best_set.begin()).second.front();
//
//			if(m_bubbles[m_wave_length].err/m_bubbles[m_wave_length].count>err_threshold
//					|| normm(m_bubbles[m_wave_length].conv)<conv_threshold)
//				m_wave_length = 0;

			m_wave_length = gcds;
		}

		LOG(cerr << "Final " << n << ": wave length=" << m_wave_length << endl;)
	}
}

