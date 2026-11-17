#include "mex.h"
#include "gabor/eeg_clean.hpp"
#include <memory>
#include <string>

namespace {
double field(const mxArray* value,const char* name,double fallback){if(!value||!mxIsStruct(value))return fallback;const mxArray* item=mxGetField(value,0,name);return item&&mxGetNumberOfElements(item)==1?mxGetScalar(item):fallback;}
void require(bool condition,const char* id,const char* text){if(!condition)mexErrMsgIdAndTxt(id,"%s",text);}
}
void mexFunction(int nlhs,mxArray* plhs[],int nrhs,const mxArray* prhs[]){
 require(nrhs>=2&&nrhs<=3,"gaborfit:eeg:arguments","Usage: [cleaned, diagnostics] = eeg_clean_mex(signal, fs, options)");
 require(mxIsDouble(prhs[0])&&!mxIsComplex(prhs[0]),"gaborfit:eeg:signal","signal must be a real double vector");
 const mwSize n=mxGetNumberOfElements(prhs[0]);require(n>=5,"gaborfit:eeg:signal","signal requires at least five samples");
 const double fs=mxGetScalar(prhs[1]);gabor::eeg::Options options;const mxArray* supplied=nrhs==3?prhs[2]:nullptr;
 options.median_filter_duration_s=field(supplied,"median_filter_duration_s",options.median_filter_duration_s);
 options.analysis_window_width_s=field(supplied,"analysis_window_width_s",field(supplied,"analyse_window_width_s",options.analysis_window_width_s));
 options.minimum_peak_distance_s=field(supplied,"minimum_peak_distance_s",field(supplied,"min_tpeak_diff_s",options.minimum_peak_distance_s));
 options.fitting_window_width_s=field(supplied,"fitting_window_width_s",field(supplied,"cutoff_window_width_s",options.fitting_window_width_s));
 options.maximum_event_width_s=field(supplied,"maximum_event_width_s",field(supplied,"max_event_width_s",options.maximum_event_width_s));
 options.pass_count=static_cast<std::int32_t>(field(supplied,"pass_count",field(supplied,"run_count",options.pass_count)));
 gabor::eeg::Cleaner cleaner(n);const auto result=cleaner.clean(mxGetDoubles(prhs[0]),n,fs,options);require(result.valid,"gaborfit:eeg:clean",result.diagnostic.c_str());
 plhs[0]=mxCreateDoubleMatrix(mxGetM(prhs[0]),mxGetN(prhs[0]),mxREAL);std::copy(result.cleaned.begin(),result.cleaned.end(),mxGetDoubles(plhs[0]));
 if(nlhs>1){const char* names[]={"pass_count","attempted_events","accepted_events","failed_events"};plhs[1]=mxCreateStructMatrix(1,1,4,names);std::size_t attempted=0,accepted=0,failed=0;for(const auto& pass:result.passes)for(const auto& event:pass.events){++attempted;accepted+=event.accepted?1:0;failed+=static_cast<int>(event.solver_status)<0?1:0;}mxSetField(plhs[1],0,names[0],mxCreateDoubleScalar(result.passes.size()));mxSetField(plhs[1],0,names[1],mxCreateDoubleScalar(attempted));mxSetField(plhs[1],0,names[2],mxCreateDoubleScalar(accepted));mxSetField(plhs[1],0,names[3],mxCreateDoubleScalar(failed));}
}
