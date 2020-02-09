#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cmath>


using namespace std;


struct Maxterm : vector<int> {
	double compute(const double *m) const {
		double prod = 1;
		
		for(const auto x : *this) {
			if(x > 0) 
				prod *= 1 - m[x - 1];
			else 
				prod *= m[-x - 1];
		}
		
		return prod;
	}
};



struct Sat : vector<Maxterm> {
	double compute(const double *m) const {
		double sum = 0;
		
		for(const auto mt : *this) {
			sum += mt.compute(m);
		}
		
		return sum;
	}
};




int main(int argc,char **argv) {
	srand(time(0));
	
	if(argc != 2) {
		cerr << "usage : sat1 <cnf>" << endl;
		return -1;
	}
	
	ifstream ifs(argv[1]);
	char line[1024];
	Sat sat;
	Maxterm mt;
	int maxx = 0;
	
	while(ifs.getline(line,1024).good()) {
		string str(line);
		stringstream sstr(str);
		
		if((str[0] >= '0' && str[0] <= '9') || str[0] == '-' || str[0] == ' ') {
			int x;
			
			do {
				sstr >> x;
				if(abs(x) > maxx) maxx = abs(x);
			
				if(x != 0) {
					mt.push_back(x);
				} else if(mt.size() > 0) {
					sat.push_back(mt);
					mt.clear();
				}
			} while(x != 0);
		}
	}
	
	cout << sat.size() << " / " << maxx << " / " << sat[0].size() << " / " << endl;

	double xs[maxx];
	for(int i = 0;i < maxx;++i) xs[i] = (double)rand() / RAND_MAX;
	double vs[maxx];
	for(int i = 0;i < maxx;++i) vs[i] = 0.01 * (double)rand() / RAND_MAX - 0.005;
	double as[maxx];
	for(int i = 0;i < maxx;++i) as[i] = 0.01 * (double)rand() / RAND_MAX - 0.005;
	double bs[maxx];
	for(int i = 0;i < maxx;++i) bs[i] = 0.01 * (double)rand() / RAND_MAX - 0.005;
	
	double prev = 0;
	
	while(true) {
		double x = sat.compute(xs);
		cout << x << endl;
		
		for(auto mt : sat) {
			double v = mt.compute(xs);
			
			for(auto x : mt) {
				if(x > 0) {
					bs[x - 1] += 0.00001 * v;
				} else {
					bs[-x - 1] -= 0.00001 * v;
				}
			}
		}
		
		prev = x;
		
		for(int i = 0;i < maxx;++i) {
			as[i] += bs[i];
			vs[i] += as[i];
			xs[i] += vs[i];
			if(xs[i] > 1) {
				xs[i] = 1;
				vs[i] = 0;
			}
			if(xs[i] < 0) {
				xs[i] = 0;
				vs[i] = 0;
			}
			vs[i] *= 0.99;
		}
	}
}




