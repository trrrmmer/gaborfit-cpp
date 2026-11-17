#include "gabor/eeg_clean_c.h"
#include "gabor/eeg_clean.hpp"
#include <algorithm>
#include <cstring>
#include <new>

struct gaborfit_eeg_context { gabor::eeg::Cleaner cleaner; gabor::eeg::Result last; explicit gaborfit_eeg_context(size_t n):cleaner(n){} };
namespace {
void copy_text(char* out,size_t n,const std::string& value){if(!n)return;const auto count=std::min(n-1,value.size());std::memcpy(out,value.data(),count);out[count]='\0';}
void copy_parameters(double out[5],const gabor::Parameters& in){std::copy(in.begin(),in.end(),out);}
}
extern "C" {
uint32_t gaborfit_eeg_abi_version(void){return GABORFIT_EEG_ABI_VERSION;}
void gaborfit_eeg_options_defaults(gaborfit_eeg_options* value){if(!value)return;std::memset(value,0,sizeof(*value));value->abi_version=GABORFIT_EEG_ABI_VERSION;value->struct_size=sizeof(*value);gabor::eeg::Options o;value->median_filter_duration_s=o.median_filter_duration_s;value->analysis_window_width_s=o.analysis_window_width_s;value->minimum_peak_distance_s=o.minimum_peak_distance_s;value->fitting_window_width_s=o.fitting_window_width_s;value->maximum_event_width_s=o.maximum_event_width_s;value->pass_count=o.pass_count;}
gaborfit_eeg_context* gaborfit_eeg_context_create(size_t n){try{return new gaborfit_eeg_context(n);}catch(...){return nullptr;}}
void gaborfit_eeg_context_destroy(gaborfit_eeg_context* value){delete value;}
int32_t gaborfit_eeg_clean(gaborfit_eeg_context* context,const double* signal,size_t count,double fs,const gaborfit_eeg_options* value,double* cleaned,gaborfit_eeg_result* out){
 if(!context||!value||!cleaned||!out||value->abi_version!=GABORFIT_EEG_ABI_VERSION||value->struct_size<sizeof(*value)){return -1;}
 std::memset(out,0,sizeof(*out));out->abi_version=GABORFIT_EEG_ABI_VERSION;out->struct_size=sizeof(*out);
 try{gabor::eeg::Options o;o.median_filter_duration_s=value->median_filter_duration_s;o.analysis_window_width_s=value->analysis_window_width_s;o.minimum_peak_distance_s=value->minimum_peak_distance_s;o.fitting_window_width_s=value->fitting_window_width_s;o.maximum_event_width_s=value->maximum_event_width_s;o.pass_count=value->pass_count;o.retain_stage_signals=value->retain_stage_signals!=0;context->last=context->cleaner.clean(signal,count,fs,o);out->status=context->last.valid?0:-1;out->sample_count=context->last.cleaned.size();out->pass_count=context->last.passes.size();copy_text(out->diagnostic,sizeof(out->diagnostic),context->last.diagnostic);if(context->last.cleaned.size()==count)std::copy(context->last.cleaned.begin(),context->last.cleaned.end(),cleaned);for(const auto& p:context->last.passes)for(const auto& e:p.events){++out->attempted_events;if(e.accepted)++out->accepted_events;if(static_cast<int32_t>(e.solver_status)<0)++out->failed_events;}return out->status;}catch(const std::exception& e){out->status=-3;copy_text(out->diagnostic,sizeof(out->diagnostic),e.what());return -3;}catch(...){out->status=-3;copy_text(out->diagnostic,sizeof(out->diagnostic),"unknown native exception");return -3;}
}
size_t gaborfit_eeg_last_event_count(const gaborfit_eeg_context* context){if(!context)return 0;size_t n=0;for(const auto& p:context->last.passes)n+=p.events.size();return n;}
int32_t gaborfit_eeg_last_event(const gaborfit_eeg_context* context,size_t index,gaborfit_eeg_event* out){if(!context||!out)return -1;const gabor::eeg::Event* found=nullptr;size_t current=0;for(const auto& p:context->last.passes){for(const auto& e:p.events){if(current++==index){found=&e;break;}}if(found)break;}if(!found)return -1;std::memset(out,0,sizeof(*out));out->pass_index=found->pass_index;out->peak_index=found->peak_index;out->window_start=found->window_start;out->window_end=found->window_end;copy_parameters(out->initial,found->initial);copy_parameters(out->lower,found->lower);copy_parameters(out->upper,found->upper);copy_parameters(out->parameters,found->parameters);out->solver_status=static_cast<int32_t>(found->solver_status);out->rejection=static_cast<int32_t>(found->rejection);out->accepted=found->accepted;out->iterations=found->iterations;out->evaluations=found->evaluations;out->attempts=found->attempts;out->qc_triggered=found->qc_triggered;out->rescue_accepted=found->rescue_accepted;out->bound_active=found->bound_active;out->objective=found->objective;out->explained_variance=found->explained_variance;return 0;}
size_t gaborfit_eeg_last_pass_count(const gaborfit_eeg_context* context){return context?context->last.passes.size():0;}
int32_t gaborfit_eeg_last_pass(const gaborfit_eeg_context* context,size_t index,gaborfit_eeg_pass* out){if(!context||!out||index>=context->last.passes.size())return -1;const auto& p=context->last.passes[index];std::memset(out,0,sizeof(*out));out->pass_index=p.pass_index;out->threshold_peak_count=p.threshold_peaks.size();out->duration_valid_peak_count=p.duration_valid_peaks.size();out->selected_peak_count=p.selected_peaks.size();out->attempted_event_count=p.events.size();for(const auto& e:p.events)out->accepted_event_count+=e.accepted?1:0;return 0;}
}
