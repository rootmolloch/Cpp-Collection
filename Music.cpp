#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <stdint.h>
#include <cmath>
#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <vector>
//#include <gtk/gtk.h>


using namespace std;

#define	MELODYFACTOR 0.9




float myPow(float x,float y) {
	return (x < 0 ? -1 : 1) * pow(abs(x),y);
}

float modWave(float x) {
	x = x;
	if(fmod(x,0.5) < 0.25)
		return 8 * fmod(x,0.25) - 1;
	else
		return 1 - 8 * fmod(x,0.25);
}


struct Sine1 {
	float f;

	Sine1(const float _f) {
		f = _f;
	}

	double operator()(double x) const {
		return myPow(sin(x * M_PI * 2),f);
	}
};


struct Sine2 {
	float f;

	Sine2(const float _f) {
		f = _f;
	}

	double operator()(double x) const {
		return myPow(modWave(x),f);
	}
};



struct RandomFunction {
	float *as;
	float *bs;
	float power;
	size_t numWaves;
		
	RandomFunction(size_t _numWaves) {
		numWaves = _numWaves;
		as = new float[numWaves];
		bs = new float[numWaves];
	}
	
	void reset(float _power) {
		power = _power;
		for(int i = 0;i < numWaves;++i) {
			as[i] = pow(2,-i / 12.0) * (1.0 + 0.1 * rand() / RAND_MAX);
			bs[i] = (float)rand() / RAND_MAX;
		}
		for(int i = 0;i < numWaves;++i)  cout << round(as[i] * 100) / 100.0 << " / ";
		cout << endl;
	}
	
	template<typename waveFunction>
	float operator()(float x,const waveFunction &wf) {
		float y = 0;
		float s = 0;
		for(int j = 0;j < numWaves;++j) {
			s += as[j];
			y += as[j] * wf((j + 1.0) * (x + bs[j]));
		}
		y /= s;
		return y;
	}
	
	~RandomFunction() {
		delete as;
		delete bs;
	}
};


struct Instrument {
	float a;
	pthread_t th;
	bool doPlay;
	uint16_t tone;
	float hold;
	float att;
	RandomFunction rf;
	
	static const size_t numChannels = 50;
	static const int bufferSize = 64000;
	static int16_t **buffers;
	static int bufferIndex;
	static int index;
	static void *thePlayThread(void *arg);
	static bool firstInstance;
	static snd_pcm_t *h;


	Instrument(int numWaves) : rf(numWaves) {
		if(firstInstance) {
			buffers = new int16_t*[numChannels];
			for(int i = 0;i < numChannels;++i) buffers[i] = new int16_t[bufferSize];
			pthread_create(&th,NULL,thePlayThread,NULL);
			firstInstance = false;
			for(int i = 0;i < numChannels;++i) 
				for(int j = 0;j < bufferSize;++j) 
					buffers[i][j] = 0;
		}
	}
	
	void reset(int _a,float _att,float _hold,float power) {
		a = pow(2,_a / 12.0);
		att = _att;
		hold = _hold;
		rf.reset(0.4);
	}
	
	template<typename waveFunction>
	void play(uint16_t tone,float fac,const waveFunction &wf,float distortion) {
		int bi = bufferIndex++;
		if(bufferIndex >= numChannels) bufferIndex = 0;
		float fr = pow(2.0,(tone - 49.0) / 12.0) * 440.0 / 44100.0;
		float min = 1000;
		float max = -1000;
		
		float xs[bufferSize];
		float sum = 0;
				
		for(int i = 0;i < bufferSize;i++) {
			float x = rf(i * fr,wf);
			x *= cos(fr * i * a);
			x = atan(distortion * myPow(x,1.0 / distortion)) / M_PI / 2;

			double y = -atan(10.0 * i / 44100.0 - M_PI * hold ) / M_PI + 0.5;
			x *= y;
			y = atan(10.0 * i / 44100.0 - M_PI * att) / M_PI + 0.5;
			x *= y;

			//x = 0.2 / (0.0001 + exp(- 1 * x * x));
			
			/*
			if(x > 0.1) x = 0.1;
			if(x < -0.1) x = -0.1;
			x *= 1.0;
			*/
			
			xs[i] = x;
			sum += abs(x);
		}
		sum /= bufferSize;
		sum *= 10;
		
		int ind = index;
		for(int i = 0;i < bufferSize;i++) {
			buffers[bi][(i + ind) % bufferSize] = round(5000 * xs[i] * fac * sum);
		}
	}
};

int16_t **Instrument::buffers;
int Instrument::bufferIndex = 0;
int Instrument::index = 0;
void *Instrument::thePlayThread(void *arg) {
	int16_t buf[2000];
	int index2 = 0;
	
	while(true) {
		index += 1000;
		if(index >= bufferSize) index = 0;

		for(int i = 0;i < 1000;++i) {
			int16_t x = 0;
			
			for(int j = 0;j < numChannels;++j) {
				x += buffers[j][i + index2];
				buffers[j][i + index2] = 0;
			}
			
			buf[2 * i] = x;
			buf[2 * i + 1] = x;
		}

		int err;
		if ((err = snd_pcm_writei (h, buf, 1000)) != 1000) {
			fprintf (stderr, "write to audio interface failed (%s)\n",
				 snd_strerror (err));
			exit (1);
		}
		
		index2 += 1000;
		if(index2 >= bufferSize) index2 = 0;
	}
}
bool Instrument::firstInstance = true;
snd_pcm_t *Instrument::h;



vector<int> scale;
int n0;



void makeScale() {
	vector<int> xs;
	
	// Randomize ?
	/*
	for(int i = 0;i < 12;++i) xs.push_back(i);
	for(int i = 0;i < 5;++i) xs.erase(xs.begin() + ((long)xs.size() * rand() / RAND_MAX));
	*/
	
	//Or C-Dur ?
	xs.clear();
	xs.push_back(0);
	xs.push_back(2);
	xs.push_back(4);
	xs.push_back(5);
	xs.push_back(7);
	xs.push_back(9);
	xs.push_back(11);
	
	scale.clear();
	for(int i = 0;i < 4;++i) 
		for(auto x : xs)
			scale.push_back(x + 12 * i + 20);
			
	for(auto x : scale) cout << x << ",";
	cout << endl;

	n0 = scale[(long)4.0 * rand() / RAND_MAX] - scale[0];
}



template<typename WaveFunction>
struct Pattern {
	Instrument instrument;
	bool *pattern;
	int tone;
	int index;
	int size;
	WaveFunction wf;
	
	void reset(const int tone,const int percentage) {
	
		this->tone = tone;

		for(int i = 0;i < size;i += 1) {
			pattern[i] = false;
		}
		
		for(int i = 0;i < size;i += 4) {
			if(100.0 * rand() / RAND_MAX < percentage) {
				pattern[i] = true;
				
				if(10.0 * rand() / RAND_MAX < 5) pattern[clamp(i - 2)] = true;
				if(10.0 * rand() / RAND_MAX < 2) pattern[clamp(i - 1)] = true;
				if(10.0 * rand() / RAND_MAX < 5) pattern[clamp(i + 2)] = true;
				if(10.0 * rand() / RAND_MAX < 2) pattern[clamp(i + 1)] = true;
			}
		}

		for(int i = 0;i < size;i += 1) {
			cout << pattern[i] << " / ";
		}
		cout << endl;
	}

	Pattern(const int tone,const int _size,const WaveFunction &_wf) : wf(_wf) , instrument(3) {
		size = _size;
		pattern = new bool [size];
		index = 0;
		reset(tone,60);
	}
	
	int clamp(int t) {
		while(t < 0) t += size;
		while(t >= size) t -= size;
		return t;
	}
	
	bool play(float volume) {
		if(pattern[index]) {
			switch(index % 4) {
				case 0:
					instrument.play(tone,volume * 1.0,wf,20);
					break;
				case 1:
				case 3:
					instrument.play(tone,volume * 0.8,wf,20);
					break;
				case 2:
					instrument.play(tone,volume * 0.9,wf,20);
					break;
			}
		}
		
		index += 1;
		if(index >= size) {
			index = 0;
			return true;
		}
		
		return false;
	}
};




template<typename MelodyWave,typename InstrumentWave>
struct Melody {
	Instrument instrument;
	vector<int> melody;
	int prev;
	int index;
	RandomFunction rf;
	MelodyWave mw;
	InstrumentWave iw;
	
	Melody(const MelodyWave &_mw,const InstrumentWave &_iw) : 
		rf(3), mw(_mw), iw(_iw), instrument(1) {
		reset();
		index = 0;
	}
	
	void reset() {
		rf.reset(0.2);
		
		prev = 0;
		melody.clear();

		float xs[48];
		float min = 10000;
		float max = -1;
		for(int i = 0;i < 48;++i) {
			xs[i] = rf((float)i / 47.0,mw);
			if(xs[i] > max) max = xs[i];
			if(xs[i] < min) min = xs[i];
		}
		
		cout << min << " / " << max << endl;
		
		for(int i = 0;i < 48;++i) {
			int j = round(12 * (xs[i] - min) / (max - min) + 8) + n0;
			cout << j << " .... ";
			melody.push_back(scale[j]);
		}
		cout << endl;
		
		for(int i = 0;i < 32;++i) {
			int j = melody.size() * rand() / RAND_MAX;
			melody.insert(melody.begin() + j,melody[j]);
		}
		
		for(auto x : melody) cout << x << " , ";
		cout << endl;
		
		index = 0;
	}
	
	bool play() {
		if((prev != melody[index]) || index % 8 == 0) 
			switch(index % 4) {
				case 0:
					instrument.play(melody[index],50 * 1.0,iw,0.8);
					break;
				case 1:
				case 3:
					instrument.play(melody[index],50 * 0.8,iw,0.8);
					break;
				case 2:
					instrument.play(melody[index],50 * 0.9,iw,0.8);
					break;
			}
			
		prev = melody[index];
		
		index += 1;
		if(index >= melody.size()) {
			index = 0;
			return true;
		}
		
		return false;
	}
};


bool doPlay = false;

void *timerThread(void *) {
	while(true) {
		doPlay = true;
		usleep(170000);
	}
}



main (int argc, char *argv[])
{
	srand(time(0));
	
	pthread_t musicThread;
	
	
	int i;
	int err;
	short buf[128];
	snd_pcm_t *playback_handle;
	snd_pcm_hw_params_t *hw_params;

	if ((err = snd_pcm_open (&playback_handle, argv[1], SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		fprintf (stderr, "cannot open audio device %s (%s)\n", 
			 argv[1],
			 snd_strerror (err));
		exit (1);
	}
	   
	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
		fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
			 snd_strerror (err));
		exit (1);
	}
			 
	if ((err = snd_pcm_hw_params_any (playback_handle, hw_params)) < 0) {
		fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
			 snd_strerror (err));
		exit (1);
	}

	if ((err = snd_pcm_hw_params_set_access (playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		fprintf (stderr, "cannot set access type (%s)\n",
			 snd_strerror (err));
		exit (1);
	}

	if ((err = snd_pcm_hw_params_set_format (playback_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		fprintf (stderr, "cannot set sample format (%s)\n",
			 snd_strerror (err));
		exit (1);
	}

	unsigned int rate = 44100;
	if ((err = snd_pcm_hw_params_set_rate_near (playback_handle, hw_params, &rate, 0)) < 0) {
		fprintf (stderr, "cannot set sample rate (%s)\n",
			 snd_strerror (err));
		exit (1);
	}

	if ((err = snd_pcm_hw_params_set_channels (playback_handle, hw_params, 2)) < 0) {
		fprintf (stderr, "cannot set channel count (%s)\n",
			 snd_strerror (err));
		exit (1);
	}

	if ((err = snd_pcm_hw_params (playback_handle, hw_params)) < 0) {
		fprintf (stderr, "cannot set parameters (%s)\n",
			 snd_strerror (err));
		exit (1);
	}

	snd_pcm_hw_params_free (hw_params);

	if ((err = snd_pcm_prepare (playback_handle)) < 0) {
		fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
			 snd_strerror (err));
		exit (1);
	}

	Instrument::h = playback_handle;
	
	/*
	Instrument<50> ins;
	usleep(100000);
	
	ins.play(49);
	usleep(10000);
	ins.play(29);
	*/
	
	makeScale();
	
	/*
	Melody<Sine2,Sine1> m(Sine2(MELODYFACTOR),Sine1(1));
	Melody<Sine2,Sine1> m2(Sine2(1.0 / MELODYFACTOR),Sine1(1));
	*/
	
	Melody<Sine2,Sine1> m(Sine2(1.2),Sine1(1.0));
	Melody<Sine2,Sine1> m2(Sine2(0.8),Sine1(1.0));
	
	Pattern<Sine1> p(20,64,Sine1(1));
	Pattern<Sine1> p2(25,64,Sine1(1));
	Pattern<Sine1> p3(30,64,Sine1(1));
	Pattern<Sine1> p4(35,64,Sine1(1));
	int k = 0;
	int k2 = 0;
	int k3 = 0;
	int k4 = 0;
	
	int a,b,c;
	int n = 4;
	
	p.reset(scale[a = 0],80);
	p2.reset(scale[b = rand() % n + 1 + a],70);
	p3.reset(scale[c = rand() % n + 1 + b],70);
	p4.reset(scale[rand() % n + 1 + c],70);
	m.instrument.reset(-96,0.2,1.2,0.02);
	m2.instrument.reset(-96,0.2,1.2,0.02);
	p.instrument.reset(-36,-3,0.7,0.2);
	p2.instrument.reset(-36,-3,0.8,0.2);
	p3.instrument.reset(-36,-3,0.8,0.2);
	p4.instrument.reset(-36,-3,0.8,0.2);
	
	pthread_t th;
	pthread_create(&th,NULL,timerThread,NULL);
	
	while(true) {
		while(!doPlay) {
			usleep(10000);
		}

		doPlay = false;
		
		m.play();
		m2.play();
		
		p.play(1.2);
		p2.play(1.2);
		p3.play(1.0);
		if(p4.play(1.0)) {
			k += 1;
			if(k >= 2) {
				k = 0;
				makeScale();
				m.reset();
				m2.reset();
			}
			p.reset(scale[a = 0],80);
			p2.reset(scale[b = rand() % n + 1 + a],70);
			p3.reset(scale[c = rand() % n + 1 + b],70);
			p4.reset(scale[rand() % n + 1 + c],70);
		}
	}
	
	snd_pcm_close (playback_handle);
	exit (0);
}
