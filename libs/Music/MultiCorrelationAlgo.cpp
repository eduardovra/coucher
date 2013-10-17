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


#include "MultiCorrelationAlgo.h"

#include <cassert>
#include <cmath>
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
	void MultiCorrelationAlgo::init()
	{
		for(size_t i=0; i<size(); i++)
		{
			if(m_corrs[i]!=NULL)	delete m_corrs[i];

			m_corrs[i] = new Correlation(m_latency_factor, i+GetSemitoneMin());
		}
	}

	MultiCorrelationAlgo::MultiCorrelationAlgo(int latency_factor, double test_complexity)
	: Transform(0.0, 0.0)
	, m_latency_factor(latency_factor)
	{
		assert(GetSamplingRate()>0);

		m_max_harm = 3;

		m_test_complexity = test_complexity;

		m_corrs.resize(size());
		for(size_t i=0; i<size(); i++)
			m_corrs[i] = NULL;
		init();
	}
	void MultiCorrelationAlgo::setLatencyFactor(double latency_factor)
	{
		m_latency_factor = latency_factor;
		for(size_t i=0; i<size(); i++)
			m_corrs[i]->m_latency_factor = latency_factor;
	}
	bool MultiCorrelationAlgo::is_minima(int ih)
	{
		if(ih+1>=0 && ih+1<int(size()))
			if(m_components[ih+1]<=m_components[ih])
				return false;
		if(ih-1>=0 && ih-1<int(size()))
			if(m_components[ih-1]<=m_components[ih])
				return false;
		return true;
	}
	int MultiCorrelationAlgo::getFondamentalWaveLength() const
	{
		return int(GetSamplingRate()/h2f(m_first_fond+GetSemitoneMin(), GetAFreq()));
	}
	void MultiCorrelationAlgo::apply(const deque<double>& buff)
	{
		assert(GetSamplingRate()>0);
		for(size_t i=0; i<size(); i++)
		{
			m_components[i] = 1.0;
			m_is_fondamental[i] = false;
		}

		m_first_fond = -1;

//		if(buff.size()<max(double((m_max_harm+1)*m_test_complexity*m_corrs[0]->m_s), (m_latency_factor+1)*m_corrs[0]->m_s))
		if(buff.empty() || buff.size()<(m_test_complexity+m_latency_factor+1)*m_corrs[0]->m_s)
			return;

		double v = 0.0;
		for(size_t i=0; i<buff.size() && v<=getVolumeTreshold() && i<m_corrs[0]->m_s; i++)
			v = max(v, abs(buff[i]));

		if(v>getVolumeTreshold())
		{
			// compute all components
			m_components_max = 0.0;
			double min_comp = 1000000;
			double max_sum = 0.0;
			for(int ih=int(size())-1; ih>=0; ih--)
			{
				m_corrs[ih]->receive(buff, 0);
				m_components[ih] = m_corrs[ih]->m_error;
				m_components_max = max(m_components_max, m_components[ih]);
			}

			// test components
			for(int ih=int(size())-1; ih>=0; ih--)
			{
				bool ok = true;

				bool crit_min = true;
				// criteria: the fond and his first harmonics are minimas
				if(ok)	ok =
							is_minima(ih) &&
							is_minima(ih-12) &&
							(is_minima(ih-19) || is_minima(ih-19-1) || is_minima(ih-19+1)) &&
							is_minima(ih-24);

				crit_min = ok;

				bool crit_small_enough = true;
				if(ok)
				{
					if(m_components[ih]/m_components_max>getComponentsTreshold())
						crit_small_enough = false;
					ok = crit_small_enough;
				}

//				bool crit_incr_err = true;
				/*
				// criteria: error always increase along harmonics
				//				and each harmonics get at minimum the error of
				//				the previous plus the error of the fondamental
				if(ok)
				{
					double old_err = m_components[ih];
					for(int k=2; crit_incr_err && k<=m_max_harm; k++)
					{
						int w = k*m_corrs[ih]->m_s;
						double kerr = 0.0;
						for(int i=0; kerr <= old_err && i<w; i++)
							kerr += abs(buff[i] - buff[i+w]);

//						ok = kerr > old_err + m_components[ih];
						crit_incr_err = kerr > old_err;
						old_err = kerr;
					}
					ok = crit_incr_err;
				}
				*/

//				bool crit_cross_zero = true;
				/*
				// criteria: wave should cross the zero value
				if(ok)
				{
					bool cross = true;
					for(int s=0; cross && s<int(m_corrs[ih]->m_s); s++)
					{
						double sg = Math::sgn(buff[s]);
						bool same_side = true;
						for(int i=0; same_side && i<int(m_corrs[ih]->m_s); i++)
							same_side = Math::sgn(buff[s+i])==sg;

						cross = !same_side;
					}

					ok = crit_cross_zero = cross;
				}
				*/

//				bool crit_integ_cst = true;
				// criteria: integral should be nearly constant while shifting
				// TODO
				// (the previous criteria seems sufficient to remove high comp.)

//				LOG(if(crit_min)
//					cerr << "ih=" << ih <<
//					" harm_min=(("<<is_minima(ih-12)<<","<<is_minima(ih-12-1)<<","<<is_minima(ih-12+1)<<"),("<<
//						is_minima(ih-19)<<","<<is_minima(ih-19-1)<<","<<is_minima(ih-19+1)<<"),("<<
//						is_minima(ih-24)<<","<<is_minima(ih-24-1)<<","<<is_minima(ih-24+1)<<"))"<< crit_min <<
//					" increase=" << crit_incr_err <<
//					" cross_zero=" << crit_cross_zero <<
//					" integ_cst=" << crit_integ_cst <<
//					" ok=" << ok << " c=" << m_components[ih] << endl;)

				// if all criteria are ok
				if(ok)
				{
					double sum = 0.0;
					int n=0;
					int i=0;
					double wh = 1.0;
					sum += wh*(m_components_max-m_components[ih]); n++;
					i=12;	if(ih-i>=0)	{sum+=wh*(m_components_max-m_components[ih-i]);	n++;}
					i=19;	if(ih-i>=0)	{sum+=wh*(m_components_max-m_components[ih-i]);	n++;}
					i=24;	if(ih-i>=0)	{sum+=wh*(m_components_max-m_components[ih-i]);	n++;}

					LOG(cerr << "ih=" << ih << " sum=" << sum << endl;)

					// get the "best"
					if(sum>max_sum)
					{
						size_t step = size_t(m_corrs[ih]->m_s/m_test_complexity);
						if(step<1)	step = 1;
						for(size_t s=0; ok && s<m_corrs[ih]->m_s; s+=step)
						{
							if(ih-1>=0){
								m_corrs[ih-1]->receive(buff, s);
								m_components[ih-1] = m_corrs[ih-1]->m_error;
							}
							if(ih+1<int(size())){
								m_corrs[ih+1]->receive(buff, s);
								m_components[ih+1] = m_corrs[ih+1]->m_error;
							}
							m_corrs[ih]->receive(buff, s);
							m_components[ih] = m_corrs[ih]->m_error;
							ok = is_minima(ih);
						}

						if(ok)
						{
							max_sum = sum;
							min_comp = m_components[ih];
							m_first_fond = ih;
						}
					}
				}
			}

			if(m_first_fond!=-1)
				m_is_fondamental[m_first_fond] = true;

			LOG(cerr << "m_first_fond=" << m_first_fond << endl;)
		}
	}
	MultiCorrelationAlgo::~MultiCorrelationAlgo()
	{
		for(size_t i=0; i<m_corrs.size(); i++)
			delete m_corrs[i];
	}
}

