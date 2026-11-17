#include "gabor/eeg_clean.hpp"
#include "gabor/eeg_clean_c.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
}
int main(){
 try{
  constexpr double fs=500;constexpr std::size_t n=1000;
  std::vector<double> signal(n,0.0);
  for(std::size_t i=0;i<n;++i)signal[i]=0.04*std::sin(2*3.141592653589793*7*i/fs);
  const std::size_t centres[]={100,175,250,325,400,475,550,625,700,775,850};
  for(std::size_t event=0;event<11;++event){const double amplitude=event==5?12.0:1.0;
   for(std::size_t i=0;i<n;++i){double dt=(static_cast<double>(i)-centres[event])/fs;
    signal[i]+=amplitude*std::exp(-.5*std::pow(dt/.008,2))*std::cos(2*3.141592653589793*75*dt+.2);}}
  gabor::eeg::Options options;options.analysis_window_width_s=1;options.minimum_peak_distance_s=.04;
  options.maximum_event_width_s=.06;options.pass_count=2;options.retain_stage_signals=true;
  gabor::eeg::Cleaner cleaner(n);auto first=cleaner.clean(signal.data(),signal.size(),fs,options);
  auto second=cleaner.clean(signal.data(),signal.size(),fs,options);
  require(first.valid,"cleaning failed");require(first.passes.size()==2,"pass count mismatch");
  require(first.cleaned==second.cleaned,"cleaning is not deterministic");
  require(!first.passes.front().threshold_peaks.empty(),"detector found no candidates");
  require(first.passes.front().running_median.size()==n,"stage signal diagnostics missing");
  for(double value:first.cleaned)require(std::isfinite(value),"nonfinite cleaned sample");
  require(!gabor::eeg::Cleaner().clean(nullptr,n,fs).valid,"invalid input accepted");
  require(!cleaner.clean(signal.data(),signal.size(),0.0,options).valid,"zero sampling rate accepted");
  auto invalid_options=options;invalid_options.pass_count=0;
  require(!cleaner.clean(signal.data(),signal.size(),fs,invalid_options).valid,"zero pass count accepted");
  auto nonfinite=signal;nonfinite[5]=std::numeric_limits<double>::quiet_NaN();
  require(!cleaner.clean(nonfinite.data(),nonfinite.size(),fs,options).valid,"nonfinite signal accepted");
  gaborfit_eeg_options c_options{};gaborfit_eeg_options_defaults(&c_options);
  c_options.analysis_window_width_s=options.analysis_window_width_s;
  c_options.minimum_peak_distance_s=options.minimum_peak_distance_s;
  c_options.maximum_event_width_s=options.maximum_event_width_s;c_options.pass_count=2;
  auto* context=gaborfit_eeg_context_create(n);require(context!=nullptr,"C context creation failed");
  std::vector<double> c_cleaned(n);gaborfit_eeg_result c_result{};
  require(gaborfit_eeg_clean(context,signal.data(),n,fs,&c_options,c_cleaned.data(),&c_result)==0,"C cleaning failed");
  require(c_cleaned==first.cleaned,"C and C++ cleaned outputs differ");
  require(gaborfit_eeg_last_event_count(context)==c_result.attempted_events,"C event count differs");
  if(c_result.attempted_events){gaborfit_eeg_event event{};require(gaborfit_eeg_last_event(context,0,&event)==0,"C event retrieval failed");}
  gaborfit_eeg_context_destroy(context);
  for(double alternate_fs:{250.0,1000.0,2000.0}){
    const std::size_t alternate_n=static_cast<std::size_t>(2*alternate_fs);
    std::vector<double> alternate(alternate_n);
    for(std::size_t i=0;i<alternate_n;++i)alternate[i]=std::sin(2*3.141592653589793*11*i/alternate_fs);
    gabor::eeg::Options alternate_options;alternate_options.analysis_window_width_s=.5;
    auto alternate_result=cleaner.clean(alternate.data(),alternate.size(),alternate_fs,alternate_options);
    require(alternate_result.valid,"variable sampling-rate cleaning failed");
    require(alternate_result.cleaned.size()==alternate.size(),"variable-length output mismatch");
  }
  std::cout<<"Native EEG cleaning smoke test passed; pass-1 selected="
           <<first.passes.front().selected_peaks.size()<<" events="<<first.passes.front().events.size()<<'\n';
  return 0;
 }catch(const std::exception& exception){std::cerr<<"TEST FAILURE: "<<exception.what()<<'\n';return 1;}
}
