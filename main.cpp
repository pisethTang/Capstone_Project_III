#include <iostream>
#include <Eigen/Dense> // This is the include path within the Eigen library

int main()
{
    Eigen::Matrix3d m; // A 3x3 matrix of doubles
    m << 1, 2, 3,
         4, 5, 6,
         7, 8, 9;

    Eigen::Vector3d v(1, 2, 3);

    std::cout << "Matrix m:\n" << m << std::endl;
    std::cout << "\nVector v:\n" << v << std::endl;
    std::cout << "\nMatrix * Vector:\n" << m * v << std::endl;

    return 0;
}