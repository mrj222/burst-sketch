#ifndef STAIRCASE_H
#define STAIRCASE_H

#include <vector>

#define inf 2147483647
#define tol 80


struct Point { 
	uint64_t x;
	double y;
};

class Staircase{

    public:
        Staircase() {}
        ~Staircase() {}

		void set_param(uint32_t a, uint32_t b) {
			eta = a;
			buffer_size = b;
		}

        void clear() {
			pt.clear();
    		buffer.clear();
		}

        double estimate(uint64_t x) {
			auto t = &pt;
    		if (!buffer.empty() && x >= buffer[0].x) {
        		t = &buffer;
    		}
    		auto pos = upper_bound(t->begin(), t->end(), x, [](unsigned v, Point p) {return v < p.x;});
    		if (pos == t->begin()) {
        		return 0;
    		}
    		else {
				auto x2 = pos->x;
				auto y2 = pos->y;
				auto x1 = (pos - 1)->x;
				auto y1 = (pos - 1)->y;
        		return y1 + (y2 - y1) / (x2 - x1) * (x - x1);
    		}
		}

        void feed(Point p) {
			if (!buffer.empty() && buffer[buffer.size() - 1].x >= p.x) {
        		assert(buffer[buffer.size() - 1].x == p.x);
        		buffer[buffer.size() - 1].y = p.y;
    		}
    		else {
        		if (!buffer.empty()) {
            		assert(buffer[buffer.size() - 1].y <= p.y);
       			}
        		if (buffer.size() == buffer_size) {
            		dp();
        		}
        		buffer.push_back(p);
    		}
		}

		double burstiness(uint64_t t, uint64_t tau) {
            auto c = estimate(t);
            if (t < tau) {
                return c;
            }
            else  {
                auto b = estimate(t - tau);
                if (t < 2 * tau) {
                    return c - 2 * b;
                }
                else {
                    auto a = estimate(t - 2 * tau);
                    return c - 2 * b + a;
                }
            }
        }

    private:
        unsigned buffer_size;
        unsigned eta;
        std::vector<Point> pt;
        std::vector<Point> buffer;
        void dp() {

			assert(buffer.size() >= 2);
	
		    uint64_t **f = (uint64_t **)malloc(buffer.size() * sizeof(uint64_t *));
    		int **p = (int **)malloc(buffer.size() * sizeof(int *));
    		for (auto i = 0; i < buffer.size(); ++i) {
        		f[i] = (uint64_t *)malloc((eta + 1) * sizeof(uint64_t));
        		p[i] = (int *)malloc((eta + 1) * sizeof(int));
    		}

    		auto m = buffer.size() - 1;

    		f[0][0] = 0;
    		for (auto i = 0U; i < m; ++i) {
        		assert(buffer[i+1].y >= buffer[i].y);
        		assert(buffer[i+1].x >= buffer[i].x);
        		f[0][0] += (buffer[i+1].x - buffer[i].x) * (buffer[i].y - buffer[0].y);
    		}

    		auto min_cost = inf;
    		auto min_i = 0;

    		for (auto i = 1U; i < m; ++i) {

        		f[i][0] = inf;
        		for (auto j = 1U; j <= MIN(i, eta); ++j) {
            		f[i][j] = inf;
            		for (auto k = j - 1; k < i; ++k) {
                		auto delta = (buffer[m].x - buffer[i].x) * (buffer[i].y - buffer[k].y);
                		auto t = f[k][j-1] - delta;
                		if (t < f[i][j]) {
                    		p[i][j] = k;
                    		f[i][j] = t;
                		}
            		}
        		}
        		if (i >= eta && f[i][eta] <= min_cost) {
            		min_cost = f[i][eta];
            		min_i = i;
        		}
    		}
    		std::vector<int> tmp;
    		tmp.reserve(eta + 2);
    		auto i = min_i;
    		auto j = eta;
    		tmp.push_back(m);
    		while (i > 0) {
        		tmp.push_back(i);
        		i = p[i][j];
        		--j;
    		}
    		tmp.push_back(0);
    		for (auto it = tmp.rbegin(); it != tmp.rend(); ++it) {
        		pt.push_back(buffer[*it]);
    		}
    		for (auto i = 0; i < buffer.size(); ++i) {
        		free(f[i]);
        		free(p[i]);
    		}
    		free(f);
    		free(p);
    		buffer.clear();
		}
};

#endif