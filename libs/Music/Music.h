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


#ifndef _Music_h_
#define _Music_h_

#include <math.h>
#include <string>
#include <list>
using namespace std;
#include <CppAddons/Math.h>
#include <CppAddons/StringAddons.h>

namespace Music
{
	enum NotesName{LOCAL_ANGLO, LOCAL_LATIN};
	extern NotesName s_notes_name;
	inline NotesName GetNotesName()					{return s_notes_name;}
	inline void SetNotesName(NotesName type)		{s_notes_name = type;}

	extern int s_tonality;
	inline int GetTonality()						{return s_tonality;}
	inline void SetTonality(int tonality)			{s_tonality = tonality;}

	extern int s_sampling_rate;
	inline int GetSamplingRate()					{return s_sampling_rate;}
	void SetSamplingRate(int sampling_rate);

	extern double s_AFreq;
	inline double GetAFreq()						{return s_AFreq;}
	void SetAFreq(double AFreq);

	extern const int UNDEFINED_SEMITONE;
	extern int s_semitone_min;
	extern int s_semitone_max;
	inline int GetSemitoneMin()						{return s_semitone_min;}
	inline int GetSemitoneMax()						{return s_semitone_max;}
	inline int GetNbSemitones()						{return s_semitone_max-s_semitone_min+1;}
	void SetSemitoneBounds(int semitone_min, int semitone_max);

	struct SettingsListener
	{
		virtual void samplingRateChanged()			{}
		virtual void AFreqChanged()					{}
		virtual void semitoneBoundsChanged()		{}

		SettingsListener();
		virtual ~SettingsListener();
	};

	extern list<SettingsListener*>	s_settings_listeners;
	void AddSettingsListener(SettingsListener* l);
	void RemoveSettingsListener(SettingsListener* l);

//! convert frequency to a float number of half-tones from A3
/*!
 * \param freq the frequency to convert to \f$\in R+\f$ {Hz}
 * \param AFreq tuning frequency of the A3 (Usualy 440) {Hz}
 * \return the float number of half-tones from A3 \f$\in R\f$
 */
inline double f2hf(double freq, double AFreq=GetAFreq())		{return 12.0*(log(freq)-log(AFreq))/log(2.0);}
// TODO VERIF
// le ht doit être le ht le plus proche de freq !! et pas un simple arrondi en dessous de la valeur réel !!
//! convert frequency to number of half-tones from A3
/*!
 * \param freq the frequency to convert to \f$\in R+\f$ {Hz}
 * \param AFreq tuning frequency of the A3 (Usualy 440) {Hz}
 * \return the number of half-tones from A3. Rounded to the nearest half-tones(
 * not a simple integer convertion of \ref f2hf ) \f$\in R\f$
 */
inline int f2h(double freq, double AFreq=GetAFreq())
{
	double ht = f2hf(freq, AFreq);
	if(ht>0)	return int(ht+0.5);
	if(ht<0)	return int(ht-0.5);
	return	0;
}
//! convert number of half-tones to frequency
/*!
 * \param ht number of half-tones to convert to \f$\in Z\f$
 * \param AFreq tuning frequency of the A3 (Usualy 440) {Hz}
 * \return the converted frequency
 */
inline double h2f(double ht, double AFreq=GetAFreq())			{return AFreq * pow(2.0, ht/12.0);}

//! convert half-tones from A3 to the corresponding note name
/*!
 * \param ht number of half-tones to convert to \f$\in Z\f$
 * \param local 
 * \return his name (Do, Re, Mi, Fa, Sol, La, Si; with '#' if needed)
 */
inline string h2n(int ht, NotesName local=GetNotesName(), int tonality=GetTonality(), bool show_oct=true)
{
	ht += tonality;

	int oct = 3;
	while(ht<0)
	{
		ht += 12;
		oct--;
	}
	while(ht>11)
	{
		ht -= 12;
		oct++;
	}

	if(ht>2)	oct++;
	if(oct<=0)	oct--;
	
//	char coct[3];
//	sprintf(coct, "%d", oct);
//	string soct = coct;

	string soct;
	if(show_oct)
		soct = StringAddons::toString(oct);

	if(local==LOCAL_ANGLO)
	{
		if(ht==0)		return "A"+soct;
		else if(ht==1)	return "A#"+soct;
		else if(ht==2)	return "B"+soct;
		else if(ht==3)	return "C"+soct;
		else if(ht==4)	return "C#"+soct;
		else if(ht==5)	return "D"+soct;
		else if(ht==6)	return "D#"+soct;
		else if(ht==7)	return "E"+soct;
		else if(ht==8)	return "F"+soct;
		else if(ht==9)	return "F#"+soct;
		else if(ht==10)	return "G"+soct;
		else if(ht==11)	return "G#"+soct;
	}
	else
	{
		if(ht==0)		return "La"+soct;
		else if(ht==1)	return "La#"+soct;
		else if(ht==2)	return "Si"+soct;
		else if(ht==3)	return "Do"+soct;
		else if(ht==4)	return "Do#"+soct;
		else if(ht==5)	return "Re"+soct;
		else if(ht==6)	return "Re#"+soct;
		else if(ht==7)	return "Mi"+soct;
		else if(ht==8)	return "Fa"+soct;
		else if(ht==9)	return "Fa#"+soct;
		else if(ht==10)	return "Sol"+soct;
		else if(ht==11)	return "Sol#"+soct;
	}

	return "R27";
}

inline int n2h(const std::string& note, NotesName local=LOCAL_ANGLO, int tonality=GetTonality())
{
	// TODO !
	return -1;
}

//! gauss fonction
/*!
 * \param x \f$\in ]-\infty,\infty[\f$
 */
inline double gauss(double x)
{
	return exp(-Math::Pi*x*x);
}

//! usefull gauss fonction
/*!
 * \param x \f$\in [0,1]\f$
 * \param f width factor
 */
inline double win_gauss(double x, double f)
{
	return gauss((x-0.5)*f);
}

//! object fonction of \ref win_gauss
struct Win_Gauss
{
	double m_g;

	Win_Gauss(double g) : m_g(g)	{}

	double operator()(double x)		{return win_gauss(x, m_g);}
};

//! compute integrale of fn \f$\in [0,1]\f$
/*! with a Simpson algorithm
 */
template<typename Fn>
inline double Usefull(Fn fn, double simpsonStep=0.001)
{
	return Math::Simpson(0.0, 1.0, fn, simpsonStep);
}

inline double ComputeM(int i)
{
	return abs(1.0/(-1+pow(2, i/12.0)));
}

inline double sinc(double x)
{
	return sin(Math::Pi*x)/(Math::Pi*x);
}

inline double win_sinc(double x, double f)
{
	if(x==0.5) return 1.0;
	double s = sinc((x-0.5)*f);
	return s*s;
}

struct Win_Sinc
{
	double m_f;

	Win_Sinc(double f) : m_f(f)	{}

	double operator()(double x)		{return win_sinc(x, m_f);}
};

//! convert cartesian coordinates to polar coordinates
inline pair<double, double> cart2pol(double x, double y)
{
	double m = sqrt(x*x+y*y);

	if(m == 0.0)	return make_pair(m, 0.0);

	double a = acos(x/m);

	if(y < 0.0)		return make_pair(m, -a);

	return make_pair(m, a);
}

}

#endif // _Music_h_
