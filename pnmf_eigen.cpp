#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>

//these library for checking whether string can be translated into values
#include <cctype>
#include <algorithm>

//#define EIGEN_NO_DEBUG
//#define EIGEN_DONT_PARALLELIZE
//#define EIGEN_MPL2_ONLY

#include "Eigen/Dense"
#include <random>

#define MAXITER 1000

#include "exportCsv.h"

void  exportData(std::string filename,vector<MatrixXd> data)
{
 ofstream ofs(filename);

 vector<MatrixXd>::iterator it;
 for(it = data.begin(); it != data.end(); it++ )
 {
  for (int i = 0; i < (*it).rows(); i++)
  {
   ofs << (*it)(i, 0);
   if (i != (*it).rows()-1) {
    ofs << ",";
   }

  }
  ofs << endl;
 }

 ofs.close();

 return;
}

//rows=150,cols=4 for iris data

//calculate the i-divergence
double i_div(Eigen::MatrixXd X, Eigen::MatrixXd Y)
{
  //check the dimensions of both matrix
  if (X.rows() != Y.rows() || X.cols() != Y.cols() )
  {
    std::cout << "Input matrixes are not the same dimension!" << std::endl;
    return -1.0;
  }
  double sum = 0;
  for(int i = 0; i < X.rows(); i++){
    for(int j = 0; j < X.cols(); j++){
      sum += X(i,j) * log(X(i,j)/Y(i,j)) - X(i,j) + Y(i,j);
    }
  }
  return sum;
}
//calculate the i-divergence with probability
double i_div(Eigen::MatrixXd X, Eigen::MatrixXd Y, Eigen::MatrixXd M)
{
  //check the dimensions of both matrix
  if (X.rows() != Y.rows() || X.rows() != M.rows() || X.cols() != Y.cols() || X.cols() != M.cols())
  {
    std::cout << "Input matrixes are not the same dimension!" << std::endl;
    return -1.0;
  }
  double sum = 0;
  for(int i = 0; i < X.rows(); i++){
    for(int j = 0; j < X.cols(); j++){
      if(M(i,j) && X(i,j) && Y(i,j)){
        sum += X(i,j) * log(X(i,j)/Y(i,j)) - X(i,j) + Y(i,j);
      }
    }
  }
  return sum;
}

//calculate the euclid distance with probability
double euc_err(Eigen::MatrixXd X, Eigen::MatrixXd Y)
{
  if (X.rows() != Y.rows() || X.cols() != Y.cols() )
  {
    std::cout << "Input matrixes are not the same dimension!" << std::endl;
    return -1.0;
  }
  return (Y - X).squaredNorm();
}
//calculate the euclid distance
double euc_err(Eigen::MatrixXd X, Eigen::MatrixXd Y, Eigen::MatrixXd M)
{
  if (X.rows() != Y.rows() || X.cols() != Y.cols() )
  {
    std::cout << "Input matrixes are not the same dimension!" << std::endl;
    return -1.0;
  }
  /*
  int sum = 0;
  for(int i = 0; i < X.rows(); i++){
    for(int j = 0; j < X.cols(); j++){
      if(M(i,j) == 1){
        sum += (Y-X)(i,j)*(Y-X)(i,j);
      }
    }
  }
  return sum;
  */
  return (Y-X).squaredNorm();
}

std::vector<std::string> split(const std::string &str, char sep)
{
    std::vector<std::string> v;
    std::stringstream ss(str);
    std::string buffer;
    while( std::getline(ss, buffer, sep) ) {
        v.push_back(buffer);
    }
    return v;
}

//this function priginally by https://gist.github.com/infusion/43bd2aa421790d5b4582
Eigen::MatrixXd readCSV(std::string file, int rows, int cols) {

  using namespace std;

  std::ifstream in(file);
  std::string line;
  std::string token;
  std::size_t pos;

  int row = 0;
  int col = 0;

  Eigen::MatrixXd res = Eigen::MatrixXd(rows, cols);

  if (in.is_open()) {
    std::getline(in, line);
    while (std::getline(in, line)) {
      col = 0;
      vector<std::string> elems = split(line, ',');

      for(int i = 0; i < elems.size(); i++){
        pos = elems[i].find(".");
        token = elems[i];
        if(pos != string::npos){
          token.replace(pos, 1, "");
        }
        if(std::all_of(token.begin(), token.end(), ::isdigit)){ //lambda式が使えなかった
          res(row, col) = stof(elems[i]); //atofではダメだった
          col++;
          //cout << "elems[i] is:" << elems[i]  << ", token :" << token << endl;
        }
      }
      row++;
    }
    in.close();
  }
  return res;
}

//refresh the euclid distance function
void refresh_euc(Eigen::MatrixXd &X, Eigen::MatrixXd &T, Eigen::MatrixXd &V){
  Eigen::MatrixXd Y = T * V;
  T.array() = T.array() * (X * V.transpose()).array() / 
                     (T * V * V.transpose()).array();
  V.array() = V.array() * (V.transpose() * X).array() /
                     (T.transpose() * T * V).array();
  /*
  for(int i = 0; i < X.rows(); i++){
    for(int k = 0; k < T.cols(); k++){
      double numer = 0;
      double denom = 0;
      for(int j = 0; j < X.cols(); j++){
        numer += X(i,j) * V(k,j);
        denom += Y(k,j) * V(i,j);
      }
      T(i,k) = T(i,k) * numer / denom;
    }
  }
  for(int k = 0; k < V.rows(); k++){
    for(int j = 0; j < X.cols(); j++){
      double numer = 0;
      double denom = 0;
      for(int i = 0; i < X.rows(); i++){
        numer += X(i,j) * T(i,k);
        denom += Y(i,k) * T(i,j);
      }
      V(k,j) = V(k,j) * numer / denom;
    }
  }
  */
  return;
}

//refresh the i-divergence function
void refresh_i(Eigen::MatrixXd &X, Eigen::MatrixXd &T, Eigen::MatrixXd &V){
  Eigen::MatrixXd Y = T * V;

  for(int i = 0; i < X.rows(); i++){
    for(int k = 0; k < T.cols(); k++){
      double numer = 0;
      double denom = 0;
      for(int j = 0; j < X.cols(); j++){
        numer += X(i,j) / Y(i,j) * V(k,j);
        denom += V(k,j);
      }
      T(i,k) = T(i,k) * numer / denom;
    }
  }
  Y = T * V;
  for(int k = 0; k < V.rows(); k++){
    for(int j = 0; j < X.cols(); j++){
      double numer = 0;
      double denom = 0;
      for(int i = 0; i < X.rows(); i++){
        numer += X(i,j) / Y(i,j) * T(i,k);
        denom += T(i,k);
      }
      V(k,j) = V(k,j) * numer / denom;
    }
  }
  return;
}
//refresh the i-divergence function with probability
void refresh_i(Eigen::MatrixXd &X, Eigen::MatrixXd &T, Eigen::MatrixXd &V, Eigen::MatrixXd &M){
  Eigen::MatrixXd Y = T * V;

  for(int i = 0; i < X.rows(); i++){
    for(int k = 0; k < T.cols(); k++){
      double numer = 0;
      double denom = 0;
      for(int j = 0; j < X.cols(); j++){
        if(Y(i,j) != 0.0){
          numer += M(i,j)*X(i,j) / Y(i,j) * V(k,j);
          denom += M(i,j)*V(k,j);
        }
      }
      if (denom != 0.0) {
        T(i,k) = T(i,k) * numer / denom;
      }else{
        T(i,k) = 0.0;
      }
    }
  }
  Y = T * V;
  for(int k = 0; k < V.rows(); k++){
    for(int j = 0; j < X.cols(); j++){
      double numer = 0;
      double denom = 0;
      for(int i = 0; i < X.rows(); i++){
        if(Y(i,j) != 0.0){
          numer += M(i,j)*X(i,j) / Y(i,j) * T(i,k);
          denom += M(i,j)*T(i,k);
        }
      }
      if (denom != 0.0) {
        V(k,j) = V(k,j) * numer / denom;
      }else{
        V(k,j) = 0.0;
      }
    }
  }
  return;
}

int main(int argc, char* argv[]){
  using namespace Eigen;
  using namespace std;

  if(argc != 5){
    std::cout << "Usage:" << argv[0] << " [inputfile] [rows] [columns] [dimensions]" << std::endl;
    return -1;
  }

  MatrixXd X1 = readCSV(argv[1], atoi(argv[2]), atoi(argv[3])); //this sentence makes values in X1

  //for randomization
  std::random_device rnd;
  std::mt19937 mt(rnd());
  std::uniform_real_distribution<> rand01(0.0, 1.0);

  MatrixXd M1 = MatrixXd(X1.rows(),X1.cols());
  for (int i = 0; i < X1.rows(); i++){
    for (int j = 0; j < X1.cols(); j++){
      M1(i,j) = (rand01(mt)>=0.5) ? 1.0:0.0;
    }
  }
  MatrixXd M2 = MatrixXd::Ones(M1.rows(),M1.cols()) - M1;
  //cout << M1 << endl;
  int k = atoi(argv[4]);

  //the initiation of T,V
  MatrixXd T1 = MatrixXd(X1.rows(),k);
  MatrixXd V1 = MatrixXd(k,X1.cols());
  //the initialization of T,V with rand values [0.0,1.0]
  for (int i = 0; i < X1.rows(); i++){
    for (int j = 0; j < k; j++){
      T1(i,j) = rand01(mt);
    }
  }
  for (int i = 0; i < X1.cols(); i++){
    for (int j = 0; j < k; j++){
      V1(j,i) = rand01(mt);
    }
  }
  //for checking the change of div on each iteration
  ofstream ofs("iter1.csv");
  ofstream ofs1("iter2.csv");
  //ofs << "0," << i_div(X1,T1*V1, M1) << endl;
  //ofs1 << "0," << i_div(X1,T1*V1, M2) << endl;
  ofs << "0," << euc_err(X1,T1*V1, M1) << endl;
  ofs1 << "0," << euc_err(X1,T1*V1, M2) << endl;

  for(int i = 1; i < MAXITER; i++){
    //refresh_i(X1,T1,V1,M1);
    refresh_euc(X1,T1,V1);
    ofs << i << "," << i_div(X1,T1*V1, M1) << endl;
    //ofs1 << i << "," << i_div(X1,T1*V1, M2) << endl;
  }

  ofstream ofs2("dataT1.csv");
  for(int i=0; i < T1.rows(); i++){
    for(int j=0; j < T1.cols()-1; j++){
      ofs2 << T1(i,j) << ",";
    }
    ofs2 << T1(i,T1.cols()-1) << endl;
  }
  ofstream ofs3("dataV1.csv");
  for(int i=0; i < V1.rows(); i++){
    for(int j=0; j < V1.cols()-1; j++){
      ofs3 << V1(i,j) << ",";
    }
    ofs3 << V1(i,V1.cols()-1) << endl;
  }
  ofstream ofs4("dataM.csv");
  for(int i=0; i < M1.rows(); i++){
    for(int j=0; j < M1.cols()-1; j++){
      ofs4 << M1(i,j) << ",";
    }
    ofs4 << M1(i,V1.cols()-1) << endl;
  }


  return 0;
}