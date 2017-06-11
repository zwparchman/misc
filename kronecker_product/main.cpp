//#define cimg_display 0

#include <iostream>
#include <vector>
#include <exception>
#include <cmath>
#include <utility>
#include <assert.h>

#include <boost/program_options.hpp>

#include "MappedArray.hpp"

#include "Timer.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow" 
#include "CImg.h"
#pragma GCC diagnostic pop

namespace zack{

template<typename T>
T clamp( T val, std::pair<T,T>range){
  assert( range.first <= range.second);

  return std::max( std::min( val, range.second), range.first);
}

template <typename T>
double normalize( T val, std::pair<T,T> range){
  assert( range.first <= range.second);

  auto diff = abs(range.second - range.first);
  return (val - range.first ) / (double)diff;
}

template <typename T>
T lerp( double val, std::pair<T,T> range){

  T diff = (range.second - range.first);
  T off = diff * val;
  return (range.first) + off;
}

template <typename T, typename R>
R remap( T val, std::pair<T,T> range1, std::pair<R,R> range2){
  assert( range1.first <= range1.second);

  double v = normalize( val, range1);
  return lerp( v, range2);
}

}


struct Matrix_exception: public std::exception{};

template <typename T>
struct Matrix{
    MappedArray<T> arr;

    size_t rows, cols;

    Matrix(Matrix &other):
        arr(other.rows * other.cols, "Matrix_XXXXXX"),
        rows(other.rows),
        cols(other.cols)
    {
#pragma omp parallel for
        for(size_t i=0; i<arr.size; i++){
            arr[i]=other.arr[i];
        }
    }

    Matrix(std::initializer_list<std::initializer_list<T>> lst):
            arr(lst.size()*(lst.begin()->size()),"kronmul_XXXXXX"){
        cols = lst.size();
        rows = lst.begin()->size();
        size_t index=0;
        for( auto row_data : lst){
            if( row_data.size() != rows ){
                throw Matrix_exception();
            }

            for( auto val : row_data ){
                arr[index]=val;
                index++;
            }
        }
    }

    void close(){
        arr.close();
        rows=0;
        cols=0;
    }

    template <typename F>
    void do_power(F val){
#pragma omp parallel for simd
        for(size_t i=0; i<rows; i++){
            for(size_t j=0; j<cols;j++){
                T &cur = (*this)[i][j];
                cur = std::pow(cur, val);
            }
        }
    }

    Matrix(size_t rr, size_t cc, std::string backing) : 
        arr(rr*cc,backing),
        rows(rr), 
        cols(cc)
    {
    }

    static Matrix kronecker_product( Matrix<T> &a, Matrix<T> &b, std::string backing){
        Matrix ret(a.rows*b.rows, a.cols *b.cols, backing);

#pragma omp parallel for
        for( size_t brow=0; brow < b.rows; brow++){
            for( size_t bcol=0; bcol < b.cols; bcol++){
                T mul = b[brow][bcol];
                for( size_t arow=0; arow < a.rows; arow++){
                    size_t ret_row = brow * a.cols+arow;
                    for( size_t acol=0; acol < a.cols; acol++){
                        size_t ret_col = bcol*a.cols+acol;
                        ret[ret_row][ret_col] = a[arow][acol] * mul;
                    }
                }
            }
        }
        return ret;
    }

    T& wrapped_get( size_t r, size_t c){
        return (*this)[r%rows][c%cols];
    }

    T* operator[](size_t r){
        return &arr[r*cols];
    }
};

using namespace std;

Matrix<float> kpower(Matrix<float> a, int power, std::string fname){
    Matrix<float>b(a);
    Matrix<float>c(a);

    for(int i=0; i<power; i++){
        std::string backing="kpower_XXXXXX";
        if(i+1==power){
            backing=fname;
        }
        c = Matrix<float>::kronecker_product(a,b, backing);
        a.close();
        a=c;
    }
    a.do_power(1/(float)power);
    return a;
}

uint8_t c2f(float val){
    return zack::remap( val, make_pair(0.0f, 1.0f), make_pair(0,255));
}

int main(int argc, char **argv){

    std::string out_name;
    int raise;

    namespace po = boost::program_options;

    po::options_description desc("allowed options");
    desc.add_options()
        ("help,h", "print help")
        ("display,d", "display the image with X")
        ("bmp", po::value<std::string>(&out_name)->default_value(""), "the file to write to")
        ("iterations,i", po::value<int>(&raise)->default_value(3), "iterations for multiplication")
        ;


    po::variables_map args;
    po::store(po::parse_command_line(argc, argv, desc), args);
    args.notify();

    if( args.count("iterations") ) raise = args["iterations"].as<int>();
    if( args.count("bmp") )out_name = args["bmp"].as<std::string>();

    if( args.count("help")){
        std::cout<<desc<<std::endl;
        return 0;
    }

    const float h = 0.8;
    const float l = 0.01;
    Matrix<float> a1 {{h,h,h},
                      {l,l,h},
                      {l,h,h}};
    Matrix<float> a2 {{l,l,h},
                      {l,h,l},
                      {l,l,l}};
    Matrix<float> a3 {{h,l,h},
                      {l,h,l},
                      {l,l,h}};

    Timer power_timer;
    power_timer.start();

    Matrix<float>r{{1}},g{{1}},b{{1}};
#pragma omp sections
    {
#pragma omp section
        r = kpower(a1, raise, "red");
#pragma omp section
        g = kpower(a2,raise, "green");
#pragma omp section
        b = kpower(a3,raise, "blue");
    }
    power_timer.stop();

    std::cout<<"Time taken to generate data: "<<std::to_string(power_timer.getTime())<<" seconds"<<std::endl;


    using namespace cimg_library;
    CImg<uint8_t> image(r.rows, r.cols, 1, 3) ;

#pragma omp parallel for
    for(size_t i=0; i<r.rows; i++){
        for(size_t j=0; j<r.cols; j++){
            const uint8_t rr = c2f(r[i][j] );
            const uint8_t gg = c2f( g[i][j] );
            const uint8_t bb = c2f( b[i][j] );
            image(i,j,0,0)=rr;
            image(i,j,0,1)=gg;
            image(i,j,0,2)=bb;
        }
    }
    if(args.count("display")){
        image.display();
    }

    if(out_name.size()){
        image.save_bmp(out_name.c_str());
    }
}
