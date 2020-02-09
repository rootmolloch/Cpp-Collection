#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cmath>
#include "myBib.hpp"



using namespace std;


typedef SatModel<1024> MyModel;
typedef Algorithm<MyModel,100> MyAlgorithm;



struct Maxterm : vector<int> {
	double compute(const MyModel &m) const {
		double prod = 1;
		
		for(const auto x : *this) {
			if(x > 0) 
				prod *= 1 - m.xs[x - 1];
			else 
				prod *= m.xs[-x - 1];
		}
		
		return prod;
	}
};



struct Sat : vector<Maxterm> {
	double compute(const MyModel &m) const {
		double sum = 0;
		
		for(const auto mt : *this) {
			sum += mt.compute(m);
		}
		
		return sum;
	}
};


MyModel simplify(const MyModel &m0) {
	MyModel m;
	
	for(int i = 0;i < 1024;++i) {
		if(m0.xs[i] < 0.5) 
			m.xs[i] = 0;
		else
			m.xs[i] = 1;
	}
	
	return m;
}


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

	MyAlgorithm algorithm(1);
	
	while(true) {
		double z = 200 - sat.compute(algorithm.current());
		
		if(algorithm.putReward(z)) {
			MyModel x = algorithm.linearCombination();
			algorithm.mutate(x);
			
			cout << sat.compute(simplify(x)) << endl;
		}
	}
}




