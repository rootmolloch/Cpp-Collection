#include "myBib.hpp"
#include <vector>
#include <cmath>
#include <iostream>



using namespace std;


template<size_t N>
struct Equation {
	static const size_t n = N;
	float as[N];
	float bs[N];
	
	Equation() {
		for(int i = 0;i < n;++i) {
			as[i] = (float)rand() / RAND_MAX - 0.5;
			bs[i] = (float)rand() / RAND_MAX - 0.5;
		}
	}
	
	Equation<N> mutate() {
	}
	
	Equation<N> operator*(const double x) const {
		Equation<N> eq;
		
		for(int i = 0;i < n;++i) {
			eq.as[i] = as[i] * x;
			eq.bs[i] = bs[i] * x;
		}
		
		return eq;
	}

	Equation<N> operator+(const Equation<N> &e0) const {
		Equation<N> eq;
		
		for(int i = 0;i < n;++i) {
			eq.as[i] = as[i] + e0.as[i];
			eq.bs[i] = bs[i] + e0.bs[i];
		}
		
		return eq;
	}

	Equation<N> operator-(const Equation<N> &e0) const {
		Equation<N> eq;
		
		for(int i = 0;i < n;++i) {
			eq.as[i] = as[i] - e0.as[i];
			eq.bs[i] = bs[i] - e0.bs[i];
		}
		
		return eq;
	}
	
	double abs() {
		double sum = 0;
		
		for(int i = 0;i < n;++i) {
			sum += as[i] * as[i];
			sum += bs[i] * bs[i];
		}
		
		return sqrt(sum);
	}
	
	Equation<N> clamp() {
		return *this;
	}
};



struct Double {
	double x;
	
	Double() {
		x = 20 * ((double)rand() / RAND_MAX - 0.5);
	}
	
	Double mutate() {
		x += 0.9 * ((double) rand() / RAND_MAX - 0.5);
		return *this;
	}
	
	Double operator*(const double f) const {
		Double y;
		
		y.x = x * f;
		
		return y;
	}
	
	Double operator+(const Double &f) const {
		Double y;
		
		y.x = x + f.x;
		
		return y;
	}

	Double operator-(const Double &f) const {
		Double y;
		
		y.x = x - f.x;
		
		return y;
	}
	
	double abs() {
		return x < 0 ? -1 : 1;
	}
	
	Double clamp() {
		return *this;
	}
};



typedef Equation<10> MyEquation;
typedef Algorithm<Double,50> MyAlgorithm;




double computeReward0(const MyEquation &me,double z,double z1) {
	double sum = 0;
	for(int j = 0;j < MyEquation::n;++j) sum += me.as[j] * pow(z,j);

	double sum1 = 0;
	for(int j = 0;j < MyEquation::n;++j) sum1 += me.bs[j] * pow(z1,j);

	return abs(sum - sum1);
}




double computeReward(const MyEquation &me,double z0) {
}




int main(int argc, char **argv) {
	srand(time(0));

	double z0 = 0;
	
	MyAlgorithm alg(1);
	MyEquation eq;
	
	for(int i = 0;i < 10000;++i) {
		if(alg.putReward(computeReward0(eq,z0,alg.current().x))) {
			Double d = alg.linearCombination();
			
			cout << "->" << d.x << " --> " << computeReward0(eq,z0,d.x) << endl;
			
			alg.mutate(d);
		}
	}
	
	return 0;
}






