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


#include "FreqAnalysis.h"

#include <assert.h>
#include <iostream>
#include <fstream>
using namespace std;
#include <fftw3.h>
using namespace Math;
#include "Music.h"

namespace Music
{
	void SingleResConvolutionTransform::init()
	{
		if(GetSamplingRate()<=0)	return;

		for(size_t h=0; h<size(); h++)
			if(m_convolutions[h]!=NULL)
				delete m_convolutions[h];

		m_components.resize(GetNbSemitones());
		m_convolutions.resize(GetNbSemitones());
		m_formants.resize(GetNbSemitones());
		for(size_t h=0; h<size(); h++)
			m_convolutions[h] = new Convolution(m_latency_factor, m_gauss_factor, int(h)+GetSemitoneMin());
	}

	SingleResConvolutionTransform::SingleResConvolutionTransform(double latency_factor, double gauss_factor)
	: Transform(0.0, 0.0)
	, m_latency_factor(latency_factor)
	, m_gauss_factor(gauss_factor)
	{
		m_convolutions.resize(size());
		m_formants.resize(size());
		for(size_t h=0; h<size(); h++)
			m_convolutions[h] = NULL;
		init();
	}
	void SingleResConvolutionTransform::apply(const deque<double>& buff)
	{
		for(size_t h=0; h<size(); h++)
		{
			m_is_fondamental[h] = false;
			m_convolutions[h]->apply(buff);
			m_formants[h] = m_convolutions[h]->m_formant;
			m_components[h] = normm(m_formants[h]);
		}
	}
	SingleResConvolutionTransform::~SingleResConvolutionTransform()
	{
		for(size_t i=0; i<m_convolutions.size(); i++)
			delete m_convolutions[i];
	}

// NeuralNetGaussAlgo
	void NeuralNetGaussAlgo::init()
	{
		cerr << "NeuralNetGaussAlgo::init" << endl;

		SingleResConvolutionTransform::init();

//		m_fwd_plan = rfftw_create_plan(m_size, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE | FFTW_OUT_OF_PLACE | FFTW_USE_WISDOM);
	}
	NeuralNetGaussAlgo::NeuralNetGaussAlgo(double latency_factor, double gauss_factor)
	: SingleResConvolutionTransform(latency_factor, gauss_factor)
	{
		init();
	}

	void NeuralNetGaussAlgo::apply(const deque<double>& buff)
	{
//		cerr << "NeuralNetGaussAlgo::apply " << m_components_treshold << endl;

		m_components_max = 0.0;
		for(size_t h=0; h<size(); h++)
		{
			m_convolutions[h]->apply(buff);
			m_formants[h] = m_convolutions[h]->m_formant;
			m_components[h] = normm(m_formants[h]);
			m_components_max = max(m_components_max, m_components[h]);
		}

		m_first_fond = UNDEFINED_SEMITONE;
		for(size_t h=0; h<size(); h++)
		{
			m_is_fondamental[h] = false;

			if(m_components[h]/m_components_max>m_components_treshold && m_first_fond==UNDEFINED_SEMITONE)
			{
				m_first_fond = h;
				m_is_fondamental[h] = true;
			}
		}
	}

	NeuralNetGaussAlgo::~NeuralNetGaussAlgo()
	{
	}

// MonophonicAlgo
	MonophonicAlgo::MonophonicAlgo(double latency_factor, double gauss_factor)
	: SingleResConvolutionTransform(latency_factor, gauss_factor)
	{
		init();
	}
	int MonophonicAlgo::getSampleAlgoLatency() const
	{
		return m_convolutions[0]->size();
	}
	void MonophonicAlgo::apply(const deque<double>& buff)
	{
		for(size_t h=0; h<m_is_fondamental.size(); h++)
			m_is_fondamental[h] = false;

		m_volume_max = 0.0;
		m_components_max = 0.0;
		m_first_fond = -1;

//		cout << "buff size=" << buff.size() << " size=" << m_convolutions[m_convolutions.size()-1]->size() << endl;

		int h;
		for(h=size()-1; h>=0 && buff.size()>=m_convolutions[h]->size(); h--)
		{
			size_t i=0;
			if(h!=int(size())-1) i=m_convolutions[h+1]->size();
			for(; i<m_convolutions[h]->size(); i++)	
				m_volume_max = max(m_volume_max, abs(buff[i]));

			if(m_volume_max > getVolumeTreshold())
			{
				m_convolutions[h]->apply(buff);

				double formant_mod = normm(m_convolutions[h]->m_formant);

//				cerr<<formant_mod<<" "<<getComponentsTreshold()<<endl;

//				if(formant_mod > getComponentsTreshold())
				{
// 					m_components[h] = min(formant_mod, max_value);
					m_components[h] = formant_mod;
					m_components_max = max(m_components_max, m_components[h]);
					m_first_fond = h;
				}
//				else m_components[h] = 0.0;
			}
			else m_components[h] = 0.0;
		}
		for(;h>=0; h--)
			m_components[h] = 0.0;

		// smash all components below a treshold of the max component
//		for(size_t h=0; h<size(); h++)
//			if(m_components[h] < m_dominant_treshold*m_components_max)
//				m_components[h] = 0.0;
//
		// the note is the first resulting component
//		for(size_t h=0; m_first_fond==-1 && h<size(); h++)
//			if(m_components[h] > 0.0)
//				m_first_fond = h;

		// correction: the note is the nearest maximum of m_note
//		if(m_first_fond!=-1)
//			while(m_first_fond+1<int(size()) && m_components[m_first_fond+1] > m_components[m_first_fond])
//				m_first_fond++;

//		cerr << "m_first_fond=" << m_first_fond << endl;
	}

#if 0
	
	TwoVoiceMHT::TwoVoiceMHT(double AFreq, int dataBySecond, double rep, double win_factor, int minHT, int maxHT)
	: m_mht(new MHT(AFreq, dataBySecond, rep, win_factor, minHT, maxHT))
	, m_last_sol(m_mht->m_components.size())
	{
		int nbHT = maxHT - minHT + 1;
		m_components.resize(nbHT);

		for(size_t i=0; i<m_last_sol.size(); i++)
			m_last_sol[i] = complex<double>(0.0,0.0);
	}
	void TwoVoiceMHT::apply(deque<double>& buff)
	{
		//	cout << "TwoVoiceMHT::apply" << endl;

		m_mht->apply(buff);

		//	double earingTreshold = 0.05;
		//	double modArgTreshold = 0.2;
		//	double modModTreshold = 0.2;
		//	ComputeDiffs(m_mht->m_components, fp, argpfp, modfp);

		//	int count = 0;
		for(size_t h=0;h<m_components.size(); h++)
		{
			//		if(m_mht->m_components[h]!=complex<double>(0.0,0.0) && count<2)
			{
				m_components[h] = m_mht->m_components[h];

				//			count++;
				//			if(fabs(argpfp[0][h])>modArgTreshold)	count++;
			}
			//		else	m_components[h] = complex<double>(0.0,0.0);
		}

		//	m_last_sol = m_mht->m_components;
	}
	TwoVoiceMHT::~TwoVoiceMHT()
	{
		delete m_mht;
	}

	RemoveSyncMHT::RemoveSyncMHT(double AFreq, int dataBySecond, double rep, double win_factor, int minHT, int maxHT)
	: m_mht(new MHT(AFreq, dataBySecond, rep, win_factor, minHT, maxHT))
	, m_last_sol(m_mht->m_components.size())
	{
		int nbHT = maxHT - minHT + 1;
		m_components.resize(nbHT);

		for(size_t i=0; i<m_last_sol.size(); i++)
			m_last_sol[i] = complex<double>(0.0,0.0);
	}
	void RemoveSyncMHT::apply(deque<double>& buff)
	{
		m_mht->apply(buff);

		double earingTreshold = 0.05;
		double syncArgTreshold = 0.3;	// 0.02 0.25 0.2
		//	double syncModTreshold = 0.2;	// 0.05 0.1 0.3

		double fourier_amplitude = 0.0;
		for(size_t h=0; h<m_mht->m_components.size(); h++)
			fourier_amplitude = max(fourier_amplitude, normm(m_mht->m_components[h]));
		vector<int> notes;

		for(size_t h=0; h<m_mht->m_components.size(); h++)                       // for each half tone
		{
			bool is_fond = false;

			if(normm(m_mht->m_components[h])>earingTreshold*fourier_amplitude) // if we can ear it
			{
				is_fond = true;

				// search for syncronisation with each discovered fondamentals
				for(size_t i=0; i<notes.size() && is_fond; i++)
				{
					double rk = m_mht->m_convolutions[h]->m_freq/m_mht->m_convolutions[notes[i]]->m_freq;
					int k = int(rk+0.5);
					if(abs(k-rk)<0.05) // TODO		// if k is nearly an integer, a potential harmonic
					{
						complex<double> ft = m_mht->m_components[notes[i]] / normm(m_mht->m_components[notes[i]]);
						complex<double> ftm1 = m_last_sol[notes[i]] / normm(m_last_sol[notes[i]]);
						complex<double> rpt = m_mht->m_components[h]/pow(ft, k);
						complex<double> rptm1 = m_last_sol[h]/pow(ftm1, k);
						//					if(h==25 && k==4)
						//						cout << abs(log(normm(rpt))-log(normm(rptm1))) << " ";
						//						cout << k << "=(arg=" << abs(arg(rpt)-arg(rptm1)) << " mod=" << abs(log(normm(rpt))-log(normm(rptm1))) << ") ";
						is_fond = abs(arg(rpt)-arg(rptm1)) > syncArgTreshold;
						//					is_fond = is_fond || abs(log(normm(rpt))-log(normm(rptm1))) > syncModTreshold;
					}

					//				is_fond = false;
				}

				if(is_fond) notes.push_back(h);       // it's a fondamentals
			}

			//		cout << endl;

			if(is_fond)		m_components[h] = m_mht->m_components[h];
			else			m_components[h] = complex<double>(0.0,0.0);
		}

		m_last_sol = m_mht->m_sol;
	}
	RemoveSyncMHT::~RemoveSyncMHT()
	{
		delete m_mht;
	}

#define FACTOR 4
	OneDataMultiplierMHT::OneDataMultiplierMHT(double AFreq, int dataBySecond, double rep, double win_factor, int minHT, int maxHT)
	: m_rep(int(rep))
	{
		int nbHT = maxHT - minHT + 1;
		m_components.resize(nbHT);
		m_convolutions.resize(nbHT);

		for(int h=minHT; h<=maxHT; h++)
			m_convolutions[h-minHT] = new SingleHalfTone(AFreq, dataBySecond, rep, win_factor, h);

		//	m_length = int(dataBySecond * 1.0/h2f(minHT, AFreq));
		m_length = int(rep/FACTOR * dataBySecond * 1.0/h2f(minHT, AFreq));
		m_size = int(rep * dataBySecond * 1.0/h2f(minHT, AFreq));

		m_fwd_plan = rfftw_create_plan(m_size, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE | FFTW_OUT_OF_PLACE | FFTW_USE_WISDOM);
		m_bck_plan = rfftw_create_plan(m_size, FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE | FFTW_OUT_OF_PLACE | FFTW_USE_WISDOM);
		m_in = new fftw_real[m_size];
		m_out = new fftw_real[m_size];
	}
	void OneDataMultiplierMHT::apply(deque<double>& buff)
	{
		if(buff.size()<(size_t)(m_length+1))	return;

		ofstream file_in("test_mult_in.dat");

		for(int i=0; i<m_size; i++)					// input interpolation
		{
			double x = i*double(m_length)/m_size;
			double y1 = buff[int(x)];
			double y2 = buff[1+int(x)];

			double d = x-int(x);

			m_in[i] = (y1*(1.0-d) + y2*d)/2;
			//		m_in[i] *= win_sinc(double(i)/m_size,2);

			file_in << i << " " << m_in[i] << endl;
		}

		rfftw_one(m_fwd_plan, m_in, m_out);			// transformation

		//	m_out[0] = m_out[0];	// done modify it

		/*	int factor = FACTOR;								// dilatation
			for(int k=m_size/2-1; k>=1; k--)
			{
			if(k%factor==0)
			{
			int i = k/factor;
			m_out[k] = m_out[i];
			m_out[m_size-k] = m_out[m_size-i];
			}
			else
			{
			m_out[k] = 0.0;
			m_out[m_size-k] = 0.0;
			}
			}*/

		double factor = FACTOR;								// dilatation with interpolation
		int k = m_size/2-1;
		for(;k>=1; k--)
		{
			double drk = k/factor;
			if(int(drk)==0)
			{
				m_out[k] = 0.0;
				m_out[m_size-k] = 0.0;
			}
			else
			{
				double dr = drk-int(drk);
				m_out[k] = (m_out[int(drk)]*(1.0-dr) + m_out[int(drk)+1]*dr)/2;

				double dik = m_size - drk;
				double di = dik-int(dik);
				m_out[m_size-k] = (m_out[int(dik)]*(1.0-di) + m_out[int(dik)-1]*di)/2;
			}
		}

		/*	int decal = 31;								// decal
			int k = m_size/2-1;
			for(;k>=1+decal; k--)
			{
			m_out[k] = m_out[k-decal];
			m_out[m_size-k] = m_out[m_size-(k-decal)];
			}
			for(;k>0; k--)
			{
			m_out[k] = 0.0;
			m_out[m_size-k] = 0.0;
			}*/

		rfftw_one(m_bck_plan, m_out, m_in);			// inverse transformation

		deque<double> mult_buff(m_size);			// fill a biger buffer

		ofstream file_out("test_mult_out.dat");
		for(int i=0; i<m_size; i++)
		{
			mult_buff[i] = m_in[i]/m_size;
			//		mult_buff[i] = m_in[i];
			file_out << i << " " << mult_buff[i] << endl;
		}

		for(size_t h=0; h<m_convolutions.size(); h++)			// do the convolutions
		{
			m_convolutions[h]->apply(mult_buff);
			m_components[h] = m_convolutions[h]->m_trans;
			//		cout << sqrt(norm(m_components[h])) << ":";
		}
		//	cout << endl;
	}

	OneDataMultiplierMHT::~OneDataMultiplierMHT()
	{
		for(size_t i=0; i<m_convolutions.size(); i++)
			delete m_convolutions[i];

		rfftw_destroy_plan(m_fwd_plan);
		rfftw_destroy_plan(m_bck_plan);
		delete m_in;
		delete m_out;
	}

	IndicMultiHalfTone::IndicMultiHalfTone(double AFreq, int dataBySecond, double maxRep, double win_factor, int minHT, int maxHT)
	{
		int nbHT = maxHT - minHT + 1;
		m_components.resize(nbHT);
		m_convolutions.resize(nbHT);
		m_small_sht.resize(nbHT);

		for(int h=minHT; h<=maxHT; h++)
		{
			m_convolutions[h-minHT] = new SingleHalfTone(AFreq, dataBySecond, maxRep, win_factor, h);
			m_small_sht[h-minHT] = new SingleHalfTone(AFreq, dataBySecond, 1.0, win_factor, h);
		}
	}

	void IndicMultiHalfTone::apply(deque<double>& buff)
	{
		if(buff.size()<m_convolutions[0]->size()) return;

		for(size_t h=0; h<m_convolutions.size(); h++)
		{
			m_convolutions[h]->apply(buff);
			//		m_components[h] = m_convolutions[h]->m_trans;
			//		m_small_sht[h]->apply(buff);
			Math::polar<double> sol = m_convolutions[h]->m_trans;
			//		m_components[h] = m_small_sht[h]->m_trans;

			// place les plus petits au debut de la fenêtre (réduit l'amorti de la fin de la note)
			//		int start = 0;

			// place les plus petits au milieu, divise par deux la taille des amortis
			int start = int(m_convolutions[h]->size()-m_small_sht[h]->size())/2;

			// place les plus petits à  la fin de la fenêtre (réduit l'amorti du début de la note)
			//		int start = int((m_convolutions[h][0]->m_rep-m_convolutions[h][i]->m_rep)*lenght);

			int delta = m_small_sht[h]->size();
			double vol = 1.0;
			for(int i=0; i<delta; i++)
			{
				m_small_sht[h]->apply(buff, start+i);
				vol = min(vol, sqrt(norm(m_small_sht[h]->m_trans)));
			}

			//		sol.mod = (sol.mod >= 0.3) ? vol : 0.0;
			sol.mod = (sol.mod >= 0.1) ? 1.0 : 0.0;

			m_components[h] = Math::make_complex(sol);
		}
	}

	MultiResMultiHalfTone::MultiResMultiHalfTone(double AFreq, int dataBySecond, double maxRep, double win_factor, int minHT, int maxHT)
	{
		int nbHT = maxHT - minHT + 1;
		m_convolutions.resize(nbHT);
		m_components.resize(nbHT);

		for(int h=minHT; h<=maxHT; h++)
			for(double r=maxRep; r>=1; r-=4)
				m_convolutions[h-minHT].push_back(new SingleHalfTone(AFreq, dataBySecond, r, win_factor, h));
	}

	void MultiResMultiHalfTone::apply(deque<double>& buff)
	{
		if(buff.size()<m_convolutions[0][0]->size()) return;

		for(size_t h=0; h<m_convolutions.size(); h++)
		{
			// integration sur plusieurs résolution (Multirésolution ? Ondelettes ?)
			m_convolutions[h][0]->apply(buff);
			Math::polar<double> sol = m_convolutions[h][0]->m_trans;

			for(size_t i=1; i<m_convolutions[h].size(); i++)
			{
				// place les plus petits au debut de la fenêtre (réduit l'amorti de la fin de la note)
				//						int start = 0;

				// place les plus petits au milieu, divise par deux la taille des amortis TODO marche pas
				int start = int(m_convolutions[h][0]->size()-m_convolutions[h][i]->size())/2;

				// place les plus petits à  la fin de la fenêtre (réduit l'amorti du début de la note)
				//			int start = int((m_convolutions[h][0]->m_rep-m_convolutions[h][i]->m_rep)*lenght);

				m_convolutions[h][i]->apply(buff, start);
				sol.mod = min(sol.mod, sqrt(norm(m_convolutions[h][i]->m_trans)));
			}

			m_components[h] = Math::make_complex(sol);
		}
	}

	MultiResMultiHalfTone::~MultiResMultiHalfTone()
	{
		for(size_t i=0; i<m_convolutions.size(); i++)
			for(size_t j=0; j<m_convolutions[i].size(); j++)
				delete m_convolutions[i][j];
	}

	TriResMultiHalfTone::TriResMultiHalfTone(double AFreq, int dataBySecond, double maxRep, double win_factor, int minHT, int maxHT)
	{
		int nbHT = maxHT - minHT + 1;
		m_convolutions.resize(nbHT);
		m_components.resize(nbHT);

		for(int h=minHT; h<=maxHT; h++)
			for(double r=maxRep; r>=1; r-=2)
				m_convolutions[h-minHT].push_back(new SingleHalfTone(AFreq, dataBySecond, r, win_factor, h));
	}

	void TriResMultiHalfTone::apply(deque<double>& buff)
	{
		if(buff.size()<m_convolutions[0][0]->size()) return;

		for(size_t h=0; h<m_convolutions.size(); h++)
		{
			m_convolutions[h][0]->apply(buff);
			Math::polar<double> sol = m_convolutions[h][0]->m_trans;

			for(size_t i=1; i<m_convolutions[h].size(); i++)
			{
				m_convolutions[h][i]->apply(buff, 0);
				sol.mod = min(sol.mod, sqrt(norm(m_convolutions[h][i]->m_trans)));
				m_convolutions[h][i]->apply(buff, m_convolutions[h][0]->size()-m_convolutions[h][i]->size());
				sol.mod = min(sol.mod, sqrt(norm(m_convolutions[h][i]->m_trans)));
			}

			m_components[h] = Math::make_complex(sol);
		}
	}

#if 0
	NeuralNetMHT::NeuralNetMHT(double AFreq, int dataBySecond, double rep, double win_factor, int minHT, int maxHT, const string& file_name)
	: m_mht(new MHT(AFreq, dataBySecond, rep, win_factor, minHT, maxHT))
	, m_nn(new LayeredNeuralNet<TypeNeuron>())
	{
		m_nbHT = maxHT - minHT + 1;
		m_components.resize(m_nbHT);

		m_nn->load(file_name.c_str());

		assert(m_nbHT==m_nn->getInputLayer()->getNeurons().size());
		assert(m_nbHT==m_nn->getOutputLayer()->getNeurons().size());

		cout << "topology: " << m_nn->getInputLayer()->getNeurons().size();
		for(LayeredNeuralNet<TypeNeuron>::LayerIterator it = ++(m_nn->m_layerList.begin()); it !=m_nn->m_layerList.end(); it++)
			cout << " => " << (*it)->getNeurons().size();
		cout << "   [" << m_nn->getNbWeights() << " weights]" << endl;
	}
	void NeuralNetMHT::apply(deque<double>& buff)
	{
		//	cout << "NeuralNetMHT::apply" << endl;

		m_mht->apply(buff);

		vector<double> inputs(m_nbHT);

		for(int h=0; h<m_nbHT; h++)
			inputs[h] = normm(m_mht->m_components[h]);

		m_nn->computeOutputs(inputs);

		for(int h=0; h<m_nbHT; h++)
			m_components[h] = complex<double>(m_nn->getOutputLayer()->getNeurons()[h].o);
	}
	NeuralNetMHT::~NeuralNetMHT()
	{
		delete m_nn;
		delete m_mht;
	}
#endif

#endif
}
