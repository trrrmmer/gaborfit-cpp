#include "gabor/eeg_clean.hpp"
#include <cmath>
#include <iostream>
#include <vector>

int main() {
  constexpr double pi=3.14159265358979323846,fs=1000.0;
  std::vector<double> signal(3000);
  for(std::size_t i=0;i<signal.size();++i){
    const double t=i/fs;signal[i]=0.03*std::sin(2*pi*9*t);
    for(double centre:{0.55,1.2,1.85,2.45}){const double dt=t-centre;
      signal[i]+=3.0*std::exp(-.5*std::pow(dt/.008,2))*std::cos(2*pi*70*dt+.2);}
  }
  gabor::eeg::Options options;options.analysis_window_width_s=1.0;
  options.minimum_peak_distance_s=.04;options.maximum_event_width_s=.06;
  options.pass_count=2;
  gabor::eeg::Cleaner cleaner(signal.size());
  const auto result=cleaner.clean(signal.data(),signal.size(),fs,options);
  if(!result.valid){std::cerr<<result.diagnostic<<'\n';return 1;}
  std::size_t attempted=0,accepted=0;
  for(const auto& pass:result.passes)for(const auto& event:pass.events){++attempted;accepted+=event.accepted?1:0;}
  std::cout<<"samples="<<result.cleaned.size()<<" attempted="<<attempted
           <<" accepted="<<accepted<<" passes="<<result.passes.size()<<'\n';
  return 0;
}
