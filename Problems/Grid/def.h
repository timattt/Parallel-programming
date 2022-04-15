#pragma once
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include "helper.h"

namespace po = boost::program_options;

struct scale_t
{
    size_t size = 10;
    double tau = 1.0;
    double h = 1.0;
};

scale_t processStart (int argc , char* argv []);

void dumpShort (const std::vector<std::vector<double>>& data , int size_x , int size_t);
void dump (const std::vector<std::vector<double>>& data , int size_x , int size_t);

void fillDataShort (std::function<double (double, double)> f ,
                    std::vector<std::vector<double>>& data ,
                    scale_t scale , int sizeX , int curT , int offset = 0);
void fillData (std::function<double (double, double)> f ,
               std::vector<std::vector<double>>& data ,
               scale_t scale , int sizeX , int offset = 0);

std::vector<std::vector<double>>
solveLinear (scale_t scale ,
             std::function<double (double)> phi ,
             std::function<double (double)> psi ,
             std::function<double (double , double)> f);

std::vector<std::vector<double>>
solveParallel (scale_t scale ,
             std::function<double (double)> phi ,
             std::function<double (double)> psi ,
             std::function<double (double , double)> f ,
             int size , int rank);

void mainThreadWork (scale_t scale ,
                    std::vector<std::vector<double>> data ,
                    std::function<double (double , double)> f ,
                    int common_size , int size);
void commonThreadWork (scale_t scale ,
                    std::vector<std::vector<double>> data ,
                    std::function<double (double , double)> f ,
                    int offset , int rank);
void lastThreadWork (scale_t scale ,
                    std::vector<std::vector<double>> data ,
                    std::function<double (double , double)> f ,
                    int offset , int rank);