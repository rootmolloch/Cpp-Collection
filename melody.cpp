#include <iostream>
#include <fstream>
#include <unistd.h>
#include <pwd.h>
#include <sstream>
#include <cmath>


using namespace std;



struct {
	int tones[64];
	int index;
} config;


void init() {
	for(int i = 0;i < 32;++i) {
		config.tones[i] = 24 * (sin(i * M_PI * 2 / 32.0) / 2 + 0.5) + 25;
	}
	
	for(int i = 0;i < 32;++i) {
		int j = rand() % (32 + i);
		for(int k = 32 + i;k > j;--k) config.tones[k] = config.tones[k - 1];
	}

	config.index = 0;
}




int main(int argc,char **argv) {
	srand(time(0));

	struct passwd *pw = getpwuid(getuid());
	stringstream cfg;
	cfg << pw->pw_dir << "/.melody";

	ifstream ifs(cfg.str().c_str());

	if(!ifs.good()) {
		init();
	} else {
		ifs >> config.index;
		for(int i = 0;i < 64;++i) ifs >> config.tones[i];
	}
	
	cout << round(pow(2,(config.tones[config.index] - 49) / 12.0) * 440) << endl;
		
	config.index += 1;
	if(config.index >= 64) {
		init();
	}
	
	ofstream ofs(cfg.str().c_str());
	if(!ofs.good()) return -1;
	ofs << config.index << " "; 
	for(int i = 0;i < 64;++i) ofs << config.tones[i] << " ";

	return 0;
}

