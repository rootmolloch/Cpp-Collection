#include <vector>
#include <limits>
#include <cstdlib>
#include <cmath>
#include <iostream>


template<size_t numVars>
struct SatModel {
	double xs[numVars];
	
	SatModel() {
		for(int i = 0;i < numVars;++i) xs[i] = (double)rand() / RAND_MAX;
	}
	
	SatModel<numVars> mutate() {
		double delta[numVars];
		double sum = 0;
		for(int i = 0;i < numVars;++i) {
			delta[i] = (double)rand() / RAND_MAX - 0.5;
			sum += delta[i] * delta[i];
		}
		sum = sqrt(sum);
		
		for(int i = 0;i < numVars;++i) {
			xs[i] += 0.1 * delta[i] / sum;
			
			if(xs[i] > 1) xs[i] = 1;
			if(xs[i] < 0) xs[i] = 0;
		}
		return *this;
	}
	
	SatModel<numVars> operator*(const double x) const {
		SatModel<numVars> m;
		for(int i = 0;i < numVars;++i) m.xs[i] = xs[i] * x;					
		return m;
	}
	
	SatModel<numVars> operator+(const SatModel<numVars> &m0) const {
		SatModel<numVars> m;
		for(int i = 0;i < numVars;++i) m.xs[i] = xs[i] + m0.xs[i];					
		return m;
	}
	
	SatModel<numVars> operator-(const SatModel<numVars> &m0) const {
		SatModel<numVars> m;
		for(int i = 0;i < numVars;++i) m.xs[i] = xs[i] - m0.xs[i];					
		return m;
	}
	
	double abs() {
		double sum = 0;
		for(int i = 0;i < numVars;++i) sum += xs[i] * xs[i];					
		return sqrt(sum);
	}
	
	SatModel<numVars> clamp() const {
		SatModel<numVars> m;
		for(int i = 0;i < numVars;++i) {
			if(xs[0] > 1) 
				m.xs[i] = 1;
			else if(xs[0] < 0)
				m.xs[i] = 0;
			else
				m.xs[i] = xs[i];
		}
		return m;
	}
};


template<size_t numInputs,size_t numNeurons,size_t numOutputs>
struct NN  {
	double inputWeights[numInputs][numNeurons];
	double weights[numNeurons][numNeurons];
	double biasWeights[numNeurons];
	double potentials[numNeurons];
	double reward;

	void init() {
		reward = 0;
		for(int i = 0;i < numNeurons;++i) potentials[i] = 0;
	}
	
	NN() {
		for(int i = 0;i < numNeurons;++i) 
			biasWeights[i] = 2.0 * rand() / RAND_MAX - 1;
		for(int i = 0;i < numNeurons;++i) 
			for(int j = 0;j < numInputs;++j) 
				inputWeights[j][i] = 2.0 * rand() / RAND_MAX - 1;
		for(int i = 0;i < numNeurons;++i) 
			for(int j = 0;j < numNeurons;++j) 
				weights[j][i] = 2.0 * rand() / RAND_MAX - 1;
		init();
	}

	void compute(double *inputs,double *outputs) {
		double activation[numNeurons];
		
		for(int k = 0;k < numNeurons;++k) {
			for(int i = 0;i < numNeurons;++i) 
				activation[i] = biasWeights[i];
			for(int i = 0;i < numNeurons;++i) 
				for(int j = 0;j < numInputs;++j) 
					activation[i] += inputWeights[j][i] * inputs[j];
			for(int i = 0;i < numNeurons;++i) 
				for(int j = 0;j < numNeurons;++j) 
					activation[i] += weights[j][i] * potentials[j];

			for(int i = 0;i < numNeurons;++i) potentials[i] = atan(activation[i]);
		}
		
		for(int i = 0;i < numOutputs;++i) outputs[i] = potentials[i];
	}
	
	NN<numInputs,numNeurons,numOutputs> mutate() {
		for(int i = 0;i < numNeurons;++i) 
			biasWeights[i] += 0.2 * rand() / RAND_MAX - 0.1;
		for(int i = 0;i < numNeurons;++i) 
			for(int j = 0;j < numInputs;++j) 
				inputWeights[j][i] += 0.2 * rand() / RAND_MAX - 0.1;
		for(int i = 0;i < numNeurons;++i) 
			for(int j = 0;j < numNeurons;++j) 
				weights[j][i] += 0.2 * rand() / RAND_MAX - 0.1;
		
		return *this;
	}
	
	NN<numInputs,numNeurons,numOutputs> operator*(const double x) const {
		NN<numInputs,numNeurons,numOutputs> nn;
		
		for(int i = 0;i < numInputs;++i) for(int j = 0;j < numNeurons;++j) nn.inputWeights[i][j] = inputWeights[i][j] * x;
		for(int i = 0;i < numNeurons;++i) for(int j = 0;j < numNeurons;++j) nn.weights[i][j] = weights[i][j] * x;
		for(int i = 0;i < numNeurons;++i) nn.biasWeights[i] = biasWeights[i] * x;
			
		return nn;
	}
	
	NN<numInputs,numNeurons,numOutputs> operator+(const NN<numInputs,numNeurons,numOutputs> &nn) const {
		NN<numInputs,numNeurons,numOutputs> nn2;
		
		for(int i = 0;i < numInputs;++i) for(int j = 0;j < numNeurons;++j) nn2.inputWeights[i][j] = nn.inputWeights[i][j] + inputWeights[i][j];
		for(int i = 0;i < numNeurons;++i) for(int j = 0;j < numNeurons;++j) nn2.weights[i][j] = nn.weights[i][j] + weights[i][j];
		for(int i = 0;i < numNeurons;++i) nn2.biasWeights[i] = nn.biasWeights[i] + biasWeights[i];
			
		return nn2;
	}
	
	NN<numInputs,numNeurons,numOutputs> operator-(const NN<numInputs,numNeurons,numOutputs> &nn) const {
		NN<numInputs,numNeurons,numOutputs> nn2;
		
		for(int i = 0;i < numInputs;++i) for(int j = 0;j < numNeurons;++j) nn2.inputWeights[i][j] = nn.inputWeights[i][j] - inputWeights[i][j];
		for(int i = 0;i < numNeurons;++i) for(int j = 0;j < numNeurons;++j) nn2.weights[i][j] = nn.weights[i][j] - weights[i][j];
		for(int i = 0;i < numNeurons;++i) nn2.biasWeights[i] = nn.biasWeights[i] - biasWeights[i];
			
		return nn2;
	}
	
	NN<numInputs,numNeurons,numOutputs> clamp() const {
		return *this;
	}
	
	double abs() {
		double sum = 0;

		for(int i = 0;i < numInputs;++i) for(int j = 0;j < numNeurons;++j) sum += inputWeights[i][j] * inputWeights[i][j];
		for(int i = 0;i < numNeurons;++i) for(int j = 0;j < numNeurons;++j) sum += weights[i][j] * weights[i][j];
		for(int i = 0;i < numNeurons;++i) sum += biasWeights[i] * biasWeights[i];
		
		return sqrt(sum);
	}
};


struct Physics2D {
	struct Point2D {
		double x,y,vx,vy,decayx,decayy;
		
		Point2D(double _x,double _y,double _decayx,double _decayy) {
			vx = 0;
			vy = 0;
			x = _x;
			y = _y;
			decayx = _decayx;
			decayy = _decayy;
		}
	};
	
	struct Line2D {
		Point2D *p1, *p2;
		double len;
		
		Line2D(Point2D *_p1,Point2D *_p2, double l) {
			p1 = _p1;
			p2 = _p2;
			len = l;
		}
	};

	std::vector<Point2D *> points;
	std::vector<Line2D *> lines;
	
	Point2D *agent;
	Point2D *pendulum;
	
	Physics2D() {
		agent = new Point2D(0.5,0.5,0.98,0);
		pendulum = new Point2D(0.55,1.0,0.98,0.98);
		
		points.push_back(agent);
		points.push_back(pendulum);
		
		lines.push_back(new Line2D(agent,pendulum,0.5));
	}
	
	void step() {
		for(auto l : lines) {
			double dx = l->p2->x - l ->p1->x;
			double dy = l->p2->y - l ->p1->y;
			double l1 = sqrt(dx * dx + dy * dy);
			double l2 = l1 - l->len;
			
			l->p1->vx += 0.1 * dx * l2 / l1;
			l->p1->vy += 0.1 * dy * l2 / l1;
			l->p2->vx -= 0.1 * dx * l2 / l1;
			l->p2->vy -= 0.1 * dy * l2 / l1;
		}

		for(auto p : points) {
			p->vy += 0.01;
			p->vx *= p->decayx;
			p->vy *= p->decayy;
			p->x += 0.1 * p->vx;
			p->y += 0.1 * p->vy;
		}
		
		if(agent->x > 0.9) {
			agent->x = 0.9;
			agent->vx = 0;
		}
		if(agent->x < 0.1) {
			agent->x = 0.1;
			agent->vx = 0;
		}
	}
};


template<typename T,size_t numIndividuals>
struct Algorithm {
	double rewards[numIndividuals];
	T individuals[numIndividuals];
	int index;
	size_t numSteps;
	int k;
	
	Algorithm(size_t numSteps) {
		this->numSteps = numSteps;
		for(int i = 0;i < numIndividuals;++i) rewards[i] = 0;
		k = 0;
		index = 0;
	}
	
	bool putReward(double reward) {
		rewards[index] += reward;
		k += 1;
		if(k >= numSteps) {
			k = 0;
			index += 1;
			if(index >= numIndividuals) {
				index = 0;
				return true;
			}
		}
		return false;
	}
	
	T linearCombination() const {
		double rmax = -std::numeric_limits<double>::max();
		double rmin = std::numeric_limits<double>::max();
		for(int i = 0;i < numIndividuals;++i) {
			if(rewards[i] > rmax) rmax = rewards[i];
			if(rewards[i] < rmin) rmin = rewards[i];
		}
		if(rmax == rmin) rmin = rmax - 0.1;
		
		double gmax = -std::numeric_limits<double>::max();
		double gmin = std::numeric_limits<double>::max();
		for(int i = 0;i < numIndividuals;++i) {
			double g = 0;
			for(int j = 0;j < numIndividuals;++j) {
				g += (individuals[i] - individuals[j]).abs();
			}
			if(g > gmax) gmax = g;
			if(g < gmin) gmin = g;
		}
		if(gmax == gmin) gmin = gmax - 0.1;
		
		T t;
		double fsum = 0;
		
		for(int i = 0;i < numIndividuals;++i) {
			double f = pow((rmax - rewards[i]) / (rmax - rmin),1);
			
			/*
			double g = 0;
			for(int j = 0;j < numIndividuals;++j) {
				g += (individuals[i] - individuals[j]).abs();
			}
			g = (g - gmin) / (gmax - gmin);
			*/
			fsum += f;
			t = t + individuals[i] * f;
		}
		
		std::cout << rmax << " / " << rmin << " / " << fsum << std::endl;
		
		return (t * (1.0 / fsum)).clamp();
	}
	
	void mutate(const T &x) {
		for(int i = 0;i < numIndividuals;++i) rewards[i] = 0;
		individuals[0] = x;
		for(int i = 1;i < numIndividuals / 2;++i) individuals[i] = T();
		for(int i = numIndividuals / 2;i < numIndividuals;++i) individuals[i] = T(x).mutate().clamp();
		
		k = 0;
		index = 0;
	}
	
	T &current() {
		return individuals[index];
	}
};



