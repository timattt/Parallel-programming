#include "helper.h"
#include "def.h"
#include <vector>
#include <array>
#include <algorithm>
#include <iterator>
#include <functional>

#define tau 0.2
#define h 1.0
bool printTime = false; // change only by inner option --time

int main (int argc , char* argv [])
{
    auto [size , rank] = mpi::init ();
    auto scale = processStart (argc , argv);
    if (rank == mpi::rank_main)
        std::cout << "\n\nSize = " << scale.size << "\nTau = " << scale.tau << "\nX = " << scale.h << "\n";

    auto phi = [](double x) -> double { return x; };
    auto psi = [](double t) -> double { return t; };
    auto f = [](double x , double t) -> double { return x + t; };

    auto solution = solveParallel (scale , phi , psi , f , size , rank);
    if (rank == mpi::rank_main)
    {
        auto start_linear_time = MPI_Wtime();
        auto check = solveLinear (scale , phi , psi , f);
        auto end_linear_time = MPI_Wtime();
        if (!printTime) {
            std::cout << "\nLINEAR SOLUTION\n\n";
            dump (check , scale.size , scale.size);
        }
        else
            std::cout << "LINEAR TIME: " << end_linear_time - start_linear_time << "\n";
    }

    quit ();
}

scale_t processStart (int argc , char* argv [])
{
    scale_t result;
    po::options_description desc ("Allowed options");
    desc.add_options ()
        ("help,h" , "Information about options")
        ("time" , "Time checker parallel")
        ("size,s" , po::value<size_t> () , "Size of result matrix")
        ("tau,t" , po::value<double> () , "Time step")
        ("x,x" , po::value<double> () , "Coordinate step");

    po::variables_map vm;
    po::store (po::parse_command_line (argc , argv , desc) , vm);
    po::notify (vm);

    if (vm.count ("help"))
    {
        std::cout << desc;
        std::exit (0);
    }
    if (vm.count ("size"))
        result.size = vm["size"].as<size_t> ();
    if (vm.count ("tau"))
        result.tau = vm["tau"].as<double> ();
    if (vm.count ("x"))
        result.h = vm["x"].as<double> ();
    if (vm.count ("time"))
        printTime = true;

    return result;
}

void dumpShort (const std::vector<std::vector<double>>& data , int sizeX , int sizeT)
{
    for (int i = 0; i < sizeT; ++i)
    {
        for (int j = 0; j < sizeX; ++j)
            std::cout << data[j][i] << "\t| ";
        std::cout << "\n";
    }
}

void dump (const std::vector<std::vector<double>>& data , int sizeX , int sizeT)
{
    std::cout << "Axes\n  ---t\n|\nx\n";
    dumpShort (data , sizeX , sizeT);
}

void fillDataShort (std::function<double (double , double)> f ,
                    std::vector<std::vector<double>>& data ,
                    scale_t scale , int sizeX , int curT , int offset)
{
    double tauh = (scale.h + scale.tau) / (2 * scale.h * scale.tau);
    for (int x = 0; x < sizeX - 1; ++x)
        data[curT + 1][x + 1] = (
            f (scale.tau * (0.5 + curT) , scale.h * (0.5 + x + offset)) +
            (data[curT][x] + data[curT][x + 1] - data[curT + 1][x]) / 2.0 / scale.tau +
            (data[curT][x] - data[curT][x + 1] + data[curT + 1][x]) / 2.0 / scale.h
            ) / tauh;
}

// Mathematical function to calculate rectangle
void fillData (std::function<double (double , double)> f ,
               std::vector<std::vector<double>>& data ,
               scale_t scale , int sizeX , int offset)
{
    for (int t = 0; t < scale.size - 1; ++t)
        fillDataShort (f , data , scale , sizeX , t , offset);
}

std::vector<std::vector<double>>
solveLinear (scale_t scale ,
             std::function<double (double)> phi ,
             std::function<double (double)> psi ,
             std::function<double (double , double)> f)
{
    double tauh = (scale.h + scale.tau) / (2 * scale.h * scale.tau);

    // first arg = time, second arg = coord
    std::vector<std::vector<double>> data (scale.size);
    std::for_each (data.begin () , data.end () , [scale](std::vector<double>& arr) {arr.resize (scale.size); });

    // beg ------- boundary conditions ------------
    std::generate (data[0].begin () , data[0].end () , [n = 0 , &phi , scale] () mutable
    { return phi (scale.h * n++); });
    std::for_each (data.begin () + 1 , data.end () , [n = 1 , &psi , scale] (std::vector<double>& arr) mutable
    { arr[0] = psi (scale.tau * n++); });
    // end ------- boundary conditions ------------
    fillData (f , data , scale , scale.size);
    return data;
}

/*
1) We are going to create the smallest 1st rank to create speed for next threads
2) The bigger thread -> the less (or equal) size

It leads to splitting. Example:
    Size = 14, threads = 4 -> ceil(14 - 1 / 4) = 4
        Every thread except the first one 4.
        The first one 14 - 4 * 3 = 2
*/

std::vector<std::vector<double>>
solveParallel (scale_t scale ,
             std::function<double (double)> phi ,
             std::function<double (double)> psi ,
             std::function<double (double , double)> f ,
             int size , int rank)
{
    std::vector<std::vector<double>> data (scale.size);
    int common_size = std::ceil ((float)(scale.size - 1) / (float)size);
    int size_main = scale.size - common_size * (size - 1);
    if (rank == mpi::rank_main)
    {
        // Prepare data to solve
        std::for_each (data.begin () , data.end () , [n = 0 , &psi , scale , size_main] (std::vector<double>& arr) mutable
        { arr.resize (size_main , 0); arr[0] = psi (scale.tau * n++); });
        std::generate (data[0].begin () + 1 , data[0].end () , [n = 1 , &phi , scale] () mutable
        { return phi (scale.h * n++); });

        mainThreadWork (scale , data , f , common_size , size);
    }
    else
    {
        double offset = size_main + common_size * (rank - 1) - 1;
        std::for_each (data.begin () , data.end () , [common_size](std::vector<double>& arr) { arr.resize (common_size + 1 , 0); });
        std::generate (data[0].begin () , data[0].end () , [n = 0 , &phi , scale , offset] () mutable
        { return phi (scale.h * (offset + n++)); });

        if (rank != size - 1)
            commonThreadWork (scale , data , f , offset , rank);
        else
            lastThreadWork (scale , data , f , offset , rank);
    }
    return data;
}

void mainThreadWork (scale_t scale ,
                    std::vector<std::vector<double>> data ,
                    std::function<double (double , double)> f ,
                    int common_size , int size)
{
    auto start_parallel_time = MPI_Wtime();
    double tauh = (scale.h + scale.tau) / (2 * scale.h * scale.tau);
    for (int t = 0; t < data.size () - 1; ++t)
    {
        fillDataShort (f , data , scale , data[0].size () , t);
        act (MPI_Send (&data[t + 1].back () , 1 , MPI_DOUBLE , /*rank*/1 , 0 , MPI_COMM_WORLD));
    }

    std::vector<std::vector<std::vector<double>>> data_rank (size - 1);
    for (int i = 0; i < size - 1; ++i)
    {
        data_rank[i].resize (scale.size);
        std::for_each (data_rank[i].begin () , data_rank[i].end () , [common_size , i](auto& vec) {
            vec.resize (common_size);
            act (MPI_Recv (vec.data () , common_size , MPI_DOUBLE , i + 1 , 1 , MPI_COMM_WORLD , MPI_STATUS_IGNORE));
        });
    }

    auto end_parallel_time = MPI_Wtime();
    if (!printTime) {
        std::cout << "\nMPI SOLUTION:\n\n";
        dump (data , scale.size , data[0].size ());
        for (const auto& cur_data : data_rank)
            dumpShort (cur_data , scale.size , common_size);
    }
    else
        std::cout << "Parallel time: " << end_parallel_time - start_parallel_time << "\n";
}
void commonThreadWork (scale_t scale ,
                    std::vector<std::vector<double>> data ,
                    std::function<double (double , double)> f ,
                    int offset , int rank)
{
    double tauh = (scale.h + scale.tau) / (2 * scale.h * scale.tau);
    for (int t = 0; t < data.size () - 1; ++t)
    {
        act (MPI_Recv (data[t + 1].data () , 1 , MPI_DOUBLE , rank - 1 , 0 , MPI_COMM_WORLD , MPI_STATUS_IGNORE));
        fillDataShort (f , data , scale , data[0].size () , t , offset);
        act (MPI_Send (&data[t + 1].back () , 1 , MPI_DOUBLE , /*rank*/rank + 1 , 0 , MPI_COMM_WORLD));
    }
    std::for_each (data.begin () , data.end () , [](auto& vec) {
        act (MPI_Send (vec.data () + 1 , vec.size () - 1 , MPI_DOUBLE , 0 , 1 , MPI_COMM_WORLD));
    });
}
void lastThreadWork (scale_t scale ,
                    std::vector<std::vector<double>> data ,
                    std::function<double (double , double)> f ,
                    int offset , int rank)
{
    double tauh = (scale.h + scale.tau) / (2 * scale.h * scale.tau);
    for (int t = 0; t < data.size () - 1; ++t)
    {
        act (MPI_Recv (data[t + 1].data () , 1 , MPI_DOUBLE , rank - 1 , 0 , MPI_COMM_WORLD , MPI_STATUS_IGNORE));
        fillDataShort (f , data , scale , data[0].size () , t , offset);
    }
    std::for_each (data.begin () , data.end () , [](auto& vec) {
        act (MPI_Send (vec.data () + 1 , vec.size () - 1 , MPI_DOUBLE , 0 , 1 , MPI_COMM_WORLD));
    });
}