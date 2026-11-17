#include <Eigen/Core>
#include <iostream>

int main() {
  std::cout << "Eigen " << EIGEN_WORLD_VERSION << '.' << EIGEN_MAJOR_VERSION << '.'
            << EIGEN_MINOR_VERSION << '\n';
  static_assert(EIGEN_WORLD_VERSION == 3, "GaborFit requires Eigen 3.4.0");
  static_assert(EIGEN_MAJOR_VERSION == 4, "GaborFit requires Eigen 3.4.0");
  static_assert(EIGEN_MINOR_VERSION == 0, "GaborFit requires Eigen 3.4.0");
  return 0;
}
