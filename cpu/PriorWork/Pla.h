#ifndef pla_h
#define pla_h

#include <limits>
#include <vector>
#include <cmath>
#include <iostream>
#include <cassert>

#define eps 1e-10

struct point {
    uint64_t x;
    double y;
};

class PLA{

    public:

		PLA() {
            result.clear();
            buffer_lower = lower = std::numeric_limits<double>::lowest();
            buffer_upper = upper = std::numeric_limits<double>::max();
            buffer_initialized = initialized = false;
        }
        
        ~PLA() {}

        struct segment{
            unsigned long long start, end;
            double slope;
            double intercept;
        };

        void set_param(double x) {
            tollerance = x;
        }

        void clear() {
            result.clear();
            buffer_lower = lower = std::numeric_limits<double>::lowest();
            buffer_upper = upper = std::numeric_limits<double>::max();
            buffer_initialized = initialized = false;
        }

        double estimate(uint64_t t) {
            if (buffer_initialized) {
                if (t >= buffer_last.x) {
                    return buffer_last.y;
                } else if (t >= buffer_begin.x) {
                    auto slope = (buffer_lower + buffer_upper) / 2;
                    auto intercept = buffer_begin.y - slope * buffer_begin.x;
                    return valueAtTime(slope, intercept, t);
                }
                int l = 0;
                int r = result.size() - 1;
                while (l <= r) {
                    int m = (l + r) / 2;
                    if (result[m].start <= t && result[m].end >= t) {
                        return valueAtTime(result[m].slope, result[m].intercept, t);
                    } else if (result[m].start > t) {
                        r = m - 1;
                    } else {
                        l = m + 1;
                    }
                }
            }
            return 0.0;
        }

        void feed(point current) {
            // std::cout << buffer_initialized << std::endl;
            if (buffer_initialized && current.x - buffer_last.x > eps) {
                lower = buffer_lower;
                upper = buffer_upper;
                last = buffer_last;
                begin = buffer_begin;
                initialized = buffer_initialized;
            }
            else if (!result.empty() && fabs(result.back().start - begin.x) < eps) {
                result.pop_back();
            }

            if (!initialized) {
                buffer_last = buffer_begin = current;
                buffer_initialized = true;
                return;
            }

            assert(current.x - last.x > - eps);

            auto lowerPrime = MAX(lower, pointSlope(begin, {current.x, current.y - tollerance}));
            auto upperPrime = MIN(upper, pointSlope(begin, {current.x, current.y + tollerance}));

            if (lowerPrime <= upperPrime) {
                buffer_lower = lowerPrime;
                buffer_upper = upperPrime;
            } else {
                //((upper+lower)/2)*(t-begin.x)+(begin.y)
                auto slope = (lower + upper) / 2;
                auto intercept = begin.y - slope * begin.x;
                result.push_back({begin.x, last.x, slope, intercept});

                buffer_begin = {last.x, valueAtTime(slope, intercept, last.x)};
                buffer_lower = pointSlope(buffer_begin, {current.x, current.y - tollerance});
                buffer_upper = pointSlope(buffer_begin, {current.x, current.y + tollerance});
            }  
            buffer_last = current;
        }


        std::vector<segment> result;

    private:

        int tollerance;

        double lower, upper;
        //available slope range : [lower, upper]
        point begin;
        //current time segment start point : begin
        point last;
        //last point
        
        bool initialized;

        double buffer_lower, buffer_upper;
        point buffer_begin, buffer_last;
        bool buffer_initialized;


        inline double valueAtTime(double slope, double intercept, unsigned long long time) {
            return slope * time + intercept;
        }

        inline double pointSlope(point pt1, point pt2) {
            assert(pt2.x > pt1.x);
            return (pt2.y - pt1.y) / (pt2.x - pt1.x);
        }
};

#endif
