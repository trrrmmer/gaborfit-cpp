#include "gabor/eeg_clean.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace gabor::eeg {
namespace {
constexpr double pi = 3.141592653589793238462643383279502884;

double percentile(std::vector<double> values, double percent) {
  if (values.empty()) return std::numeric_limits<double>::quiet_NaN();
  std::sort(values.begin(), values.end());
  // MATLAB prctile uses sample positions (k-0.5)/N and clamps the tails.
  const double rank = std::clamp((percent / 100.0) * static_cast<double>(values.size()) - 0.5,
                                 0.0, static_cast<double>(values.size() - 1));
  const auto low = static_cast<std::size_t>(std::floor(rank));
  const auto high = static_cast<std::size_t>(std::ceil(rank));
  if (low == high) return values[low];
  return values[low] * (static_cast<double>(high) - rank) +
         values[high] * (rank - static_cast<double>(low));
}

std::vector<double> median_filter_zero_padded(const std::vector<double>& x, std::size_t width) {
  std::vector<double> output(x.size()), work(width);
  if (x.empty() || width == 0) return output;
  const std::ptrdiff_t left = static_cast<std::ptrdiff_t>(width / 2);
  for (std::size_t i = 0; i < x.size(); ++i) {
    for (std::size_t j = 0; j < width; ++j) {
      const auto source = static_cast<std::ptrdiff_t>(i) + static_cast<std::ptrdiff_t>(j) - left;
      work[j] = source >= 0 && source < static_cast<std::ptrdiff_t>(x.size()) ? x[source] : 0.0;
    }
    const std::size_t middle = width / 2;
    std::nth_element(work.begin(), work.begin() + middle, work.end());
    if (width % 2) output[i] = work[middle];
    else {
      const double upper = work[middle];
      const double lower = *std::max_element(work.begin(), work.begin() + middle);
      output[i] = 0.5 * (lower + upper);
    }
  }
  return output;
}

struct Peaks { std::vector<double> values; std::vector<std::size_t> indices; };

Peaks local_maxima(const std::vector<double>& values, double threshold, std::size_t gap) {
  Peaks result;
  std::vector<std::pair<std::size_t,std::size_t>> ranges;
  std::size_t i = 0;
  while (i < values.size()) {
    while (i < values.size() && !(values[i] > threshold)) ++i;
    if (i == values.size()) break;
    const std::size_t start = i;
    while (i + 1 < values.size() && values[i + 1] > threshold) ++i;
    ranges.emplace_back(start, i++);
  }
  std::vector<std::pair<std::size_t,std::size_t>> merged;
  for (const auto& range : ranges) {
    if (!merged.empty() && range.first - merged.back().second < gap) merged.back().second = range.second;
    else merged.push_back(range);
  }
  for (const auto& range : merged) {
    auto begin = values.begin() + static_cast<std::ptrdiff_t>(range.first);
    auto end = values.begin() + static_cast<std::ptrdiff_t>(range.second + 1);
    const auto maximum = std::max_element(begin, end);
    result.indices.push_back(static_cast<std::size_t>(maximum - values.begin()));
    result.values.push_back(*maximum);
  }
  return result;
}

Peaks detect_large_peaks_impl(double fs, const std::vector<double>& signal, double window_s,
                              double quartile_factor, bool no_flip) {
  Peaks output;
  const auto width = static_cast<std::size_t>(std::max(1.0, std::round(window_s * fs)));
  const auto increment = static_cast<std::size_t>(std::max(1.0, std::round(width * 0.75)));
  for (std::size_t first = 0; first + width <= signal.size(); first += increment) {
    const std::vector<double> window(signal.begin() + static_cast<std::ptrdiff_t>(first),
                                     signal.begin() + static_cast<std::ptrdiff_t>(first + width));
    const double maximum = *std::max_element(window.begin(), window.end());
    std::size_t best = 0, best_count = 0;
    for (std::size_t k = 0; k < 50; ++k) {
      const double threshold = static_cast<double>(k) / 49.0 * maximum;
      const std::size_t count = local_maxima(window, threshold, 10).indices.size();
      if (count > best_count) { best_count = count; best = k; }
    }
    const double threshold = static_cast<double>(best) / 49.0 * maximum;
    const Peaks candidates = local_maxima(window, threshold, 5);
    if (!candidates.values.empty()) {
      const double q50 = percentile(candidates.values, 50), q75 = percentile(candidates.values, 75);
      const double large = q50 + quartile_factor * (q75 - q50);
      for (std::size_t k = 0; k < candidates.values.size(); ++k) {
        if (candidates.values[k] > large && candidates.indices[k] < increment) {
          output.values.push_back(candidates.values[k]); output.indices.push_back(first + candidates.indices[k]);
        }
      }
    }
  }
  if (!no_flip) {
    const std::size_t count = std::min(signal.size(), 2 * width);
    std::vector<double> reversed(count);
    for (std::size_t i = 0; i < count; ++i) reversed[i] = signal[signal.size() - 1 - i];
    Peaks tail = detect_large_peaks_impl(fs, reversed, window_s, quartile_factor, true);
    std::vector<std::pair<std::size_t,double>> converted;
    for (std::size_t i = 0; i < tail.indices.size(); ++i)
      converted.emplace_back(signal.size() - 1 - tail.indices[i], tail.values[i]);
    std::sort(converted.begin(), converted.end());
    const std::size_t last = output.indices.empty() ? 0 : output.indices.back();
    for (const auto& item : converted)
      if (output.indices.empty() || item.first > last) { output.indices.push_back(item.first); output.values.push_back(item.second); }
  }
  return output;
}

std::vector<std::size_t> prune_close(std::vector<std::size_t> peaks,
                                     const std::vector<double>& signal, double minimum_samples) {
  while (peaks.size() > 1) {
    std::vector<bool> close(peaks.size(), false), remove(peaks.size(), false);
    for (std::size_t i = 1; i < peaks.size(); ++i) close[i] = peaks[i] - peaks[i-1] < minimum_samples;
    if (std::none_of(close.begin(), close.end(), [](bool value){return value;})) break;
    for (std::size_t i = 1; i < close.size(); ++i) {
      if (close[i] && (i == 1 || !close[i-1])) {
        if (std::abs(signal[peaks[i]]) > std::abs(signal[peaks[i-1]])) remove[i-1] = true;
        else remove[i] = true;
      }
    }
    std::vector<std::size_t> retained;
    for (std::size_t i = 0; i < peaks.size(); ++i) if (!remove[i]) retained.push_back(peaks[i]);
    peaks.swap(retained);
  }
  return peaks;
}

std::pair<std::size_t,std::size_t> suppress_outside_zero_crossings(std::vector<double>& y,
                                                                   std::size_t crossing_count) {
  const std::size_t n = y.size();
  std::size_t first_range = static_cast<std::size_t>(std::floor(0.375*n));
  std::size_t last_range = std::min(n-1, static_cast<std::size_t>(std::floor(0.625*n)));
  auto peak = std::max_element(y.begin()+static_cast<std::ptrdiff_t>(first_range),
                               y.begin()+static_cast<std::ptrdiff_t>(last_range+1),
                               [](double a,double b){return std::abs(a)<std::abs(b);});
  std::size_t peak_index = static_cast<std::size_t>(peak-y.begin());
  const bool negative = y[peak_index] < 0;
  if (negative) for (double& value:y) value=-value;
  std::size_t valid_first=0, valid_last=n-1;
  for (int direction : {-1,1}) {
    double last_value=1; std::size_t found=0; std::ptrdiff_t k=static_cast<std::ptrdiff_t>(peak_index);
    while (k>=0 && k<static_cast<std::ptrdiff_t>(n)) {
      const double value=y[static_cast<std::size_t>(k)];
      if (value*last_value<=0) { ++found; last_value=-last_value; if(found==crossing_count) break; }
      k+=direction;
    }
    if(found==crossing_count) { if(direction<0) valid_first=static_cast<std::size_t>(k); else valid_last=static_cast<std::size_t>(k); }
  }
  for(std::size_t i=0;i<n;++i) if(i<valid_first||i>valid_last)y[i]=0;
  if(negative)for(double& value:y)value=-value;
  return {valid_first,valid_last};
}

std::vector<double> event_window(const std::vector<double>& signal,
                                 const std::vector<std::size_t>& peaks, std::size_t index,
                                 std::ptrdiff_t half_width) {
  const std::ptrdiff_t peak=static_cast<std::ptrdiff_t>(peaks[index]);
  const std::ptrdiff_t start=peak-half_width,end=peak+half_width;
  std::vector<double> y(static_cast<std::size_t>(2*half_width+1),0.0);
  std::ptrdiff_t left_limit=std::max<std::ptrdiff_t>(0,start),right_limit=std::min<std::ptrdiff_t>(signal.size()-1,end);
  if(index>0 && static_cast<std::ptrdiff_t>(peaks[index-1])>=start)
    left_limit=(static_cast<std::ptrdiff_t>(peaks[index-1])+peak)/2+1;
  if(index+1<peaks.size() && static_cast<std::ptrdiff_t>(peaks[index+1])<=end)
    right_limit=(peak+static_cast<std::ptrdiff_t>(peaks[index+1])+1)/2-1;
  for(std::ptrdiff_t source=left_limit;source<=right_limit;++source)y[static_cast<std::size_t>(source-start)]=signal[static_cast<std::size_t>(source)];
  return y;
}

struct InitialProblem { Parameters initial{},lower{},upper{}; bool valid=false; };
double peak_threshold(const std::vector<double>& y,double factor) {
  const auto median=median_filter_zero_padded(y,100);std::vector<double> difference(y.size());
  for(std::size_t i=0;i<y.size();++i)difference[i]=y[i]-median[i];
  const double maximum=*std::max_element(difference.begin(),difference.end());std::size_t best=0,best_count=0;
  for(std::size_t k=0;k<50;++k){auto p=local_maxima(difference,static_cast<double>(k)/49*maximum,10);if(p.indices.size()>best_count){best_count=p.indices.size();best=k;}}
  auto peaks=local_maxima(difference,static_cast<double>(best)/49*maximum,5);
  if(peaks.values.empty())return std::numeric_limits<double>::quiet_NaN();
  return percentile(peaks.values,50)+factor*(percentile(peaks.values,75)-percentile(peaks.values,50));
}
InitialProblem make_problem(const std::vector<double>& x,const std::vector<double>& y) {
  InitialProblem p;if(y.size()<2)return p;
  std::vector<double> negative(y.size());std::transform(y.begin(),y.end(),negative.begin(),[](double v){return -v;});
  double low=-peak_threshold(negative,2);double high=peak_threshold(y,2);
  low=std::max(*std::min_element(y.begin(),y.end())*.95,low);high=std::min(*std::max_element(y.begin(),y.end())*.95,high);
  std::vector<std::size_t> valid;double sum=0,cog=0;
  for(std::size_t i=0;i<y.size();++i)if(y[i]>high||y[i]<low){valid.push_back(i);sum+=std::abs(y[i]);cog+=x[i]*std::abs(y[i]);}
  if(valid.empty()||!(sum>0))return p;
  cog/=sum;double variance=0;
  for(auto i:valid)variance+=(x[i]-cog)*(x[i]-cog)*std::abs(y[i])/sum;
  if(valid.size()==1){auto nearest=static_cast<std::size_t>(std::min_element(x.begin(),x.end(),[&](double a,double b){return std::abs(a-cog)<std::abs(b-cog);})-x.begin());double delta=nearest+1<x.size()?x[nearest+1]-x[nearest]:x[nearest]-x[nearest-1];variance=(delta/2)*(delta/2);}
  if(!(variance>0))return p;
  const double sigma=std::sqrt(variance);
  auto maximum=std::max_element(valid.begin(),valid.end(),[&](auto a,auto b){return std::abs(y[a])<std::abs(y[b]);});const double amplitude=std::abs(y[*maximum]);
  p.lower={{amplitude/4,1/(32*sigma),-pi,sigma/2,cog/2}};p.upper={{amplitude*1.75,1/(2*sigma),pi,sigma*1.5,cog*1.5}};
  if(y[*maximum]<0){p.lower[0]=-amplitude*1.75;p.upper[0]=-amplitude/4;}
  for(std::size_t i=0;i<5;++i)p.initial[i]=(p.lower[i]+p.upper[i])/2;
  p.valid=true;return p;
}

std::vector<std::size_t> cluster_outliers(const std::vector<std::size_t>& peaks,const std::vector<double>& amplitudes) {
  if(peaks.size()<2)return {};
  struct Cluster{std::vector<std::size_t> members;};std::vector<Cluster> clusters;
  for(std::size_t i=0;i<amplitudes.size();++i)clusters.push_back({{i}});
  std::vector<std::vector<Cluster>> levels(amplitudes.size()+1);levels[clusters.size()]=clusters;
  std::vector<double> merge_distances;
  while(clusters.size()>1){double best=std::numeric_limits<double>::infinity();std::size_t ai=0,bi=1;
    for(std::size_t a=0;a<clusters.size();++a)for(std::size_t b=a+1;b<clusters.size();++b){double distance=0;for(auto i:clusters[a].members)for(auto j:clusters[b].members)distance=std::max(distance,std::abs(amplitudes[i]-amplitudes[j]));if(distance<best){best=distance;ai=a;bi=b;}}
    merge_distances.push_back(best);clusters[ai].members.insert(clusters[ai].members.end(),clusters[bi].members.begin(),clusters[bi].members.end());clusters.erase(clusters.begin()+static_cast<std::ptrdiff_t>(bi));levels[clusters.size()]=clusters;
  }
  double best_gap=-std::numeric_limits<double>::infinity(),threshold=std::numeric_limits<double>::infinity();
  for(std::size_t requested=2;requested<=6;++requested){std::size_t count=std::min(requested,amplitudes.size());if(count<amplitudes.size()){const std::size_t merge_index=amplitudes.size()-count-1;const double cut=merge_distances[merge_index];std::size_t included=merge_index+1;while(included<merge_distances.size()&&std::abs(merge_distances[included]-cut)<=1e-12*std::max(1.0,std::abs(cut)))++included;count=amplitudes.size()-included;}const auto& level=levels[count];std::vector<std::pair<double,const Cluster*>> ordered;for(const auto& c:level){std::vector<double> v;for(auto i:c.members)v.push_back(amplitudes[i]);ordered.push_back({percentile(v,50),&c});}std::sort(ordered.begin(),ordered.end(),[](auto&a,auto&b){return a.first<b.first;});if(ordered.size()<2)continue;std::vector<double> last,previous;for(auto i:ordered.back().second->members)last.push_back(amplitudes[i]);for(auto i:ordered[ordered.size()-2].second->members)previous.push_back(amplitudes[i]);double gap=percentile(last,10)-percentile(previous,90);if(gap>best_gap){best_gap=gap;threshold=(*std::min_element(last.begin(),last.end())+*std::max_element(previous.begin(),previous.end()))/2;}}
  std::vector<std::size_t> output;for(std::size_t i=0;i<peaks.size();++i)if(amplitudes[i]>threshold)output.push_back(peaks[i]);return output;
}

Peaks detect_large_peaks_cluster_impl(double fs, const std::vector<double>& signal,
                                      double window_s, bool no_flip) {
  Peaks output;
  const auto width=static_cast<std::size_t>(std::max(1.0,std::round(window_s*fs)));
  const auto increment=static_cast<std::size_t>(std::max(1.0,std::round(width*.75)));
  for(std::size_t first=0;first+width<=signal.size();first+=increment){
    const std::vector<double> window(signal.begin()+static_cast<std::ptrdiff_t>(first),
                                     signal.begin()+static_cast<std::ptrdiff_t>(first+width));
    const double maximum=*std::max_element(window.begin(),window.end());
    std::size_t best=0,best_count=0;
    for(std::size_t k=0;k<50;++k){const auto candidates=local_maxima(window,static_cast<double>(k)/49*maximum,10);if(candidates.indices.size()>best_count){best_count=candidates.indices.size();best=k;}}
    const auto candidates=local_maxima(window,static_cast<double>(best)/49*maximum,5);
    if(candidates.values.size()>1){
      const auto selected=cluster_outliers(candidates.indices,candidates.values);
      for(const auto local:selected)if(local<increment){output.indices.push_back(first+local);output.values.push_back(window[local]);}
    }
  }
  if(!no_flip){
    const std::size_t count=std::min(signal.size(),2*width);std::vector<double> reversed(count);
    for(std::size_t i=0;i<count;++i)reversed[i]=signal[signal.size()-1-i];
    const auto tail=detect_large_peaks_cluster_impl(fs,reversed,window_s,true);
    std::vector<std::pair<std::size_t,double>> converted;
    for(std::size_t i=0;i<tail.indices.size();++i)converted.emplace_back(signal.size()-1-tail.indices[i],tail.values[i]);
    std::sort(converted.begin(),converted.end());const std::size_t last=output.indices.empty()?0:output.indices.back();
    for(const auto& item:converted)if(output.indices.empty()||item.first>last){output.indices.push_back(item.first);output.values.push_back(item.second);}
  }
  return output;
}
} // namespace

Cleaner::Cleaner(std::size_t reserve_samples):fitter_(reserve_samples){}
Result Cleaner::clean(const double* signal,std::size_t count,double fs,const Options& options){
  Result result;if(!signal||count<5||!std::isfinite(fs)||fs<=0||options.pass_count<1){result.diagnostic="invalid signal, sampling frequency, or pass count";return result;}
  result.cleaned.assign(signal,signal+count);for(double value:result.cleaned)if(!std::isfinite(value)){result.diagnostic="signal contains nonfinite samples";return result;}
  for(std::int32_t pass=0;pass<options.pass_count;++pass){PassDiagnostics diagnostics;diagnostics.pass_index=pass+1;
    const std::size_t median_width=static_cast<std::size_t>(std::floor(options.median_filter_duration_s*fs))+1;auto median=median_filter_zero_padded(result.cleaned,median_width);std::vector<double> high(count);for(std::size_t i=0;i<count;++i)high[i]=result.cleaned[i]-median[i];
    auto positive=detect_large_peaks_impl(fs,high,options.analysis_window_width_s,-1000,false);std::vector<double> inverse(count);std::transform(high.begin(),high.end(),inverse.begin(),[](double v){return-v;});auto negative=detect_large_peaks_impl(fs,inverse,options.analysis_window_width_s,-1000,false);std::vector<std::size_t> candidates=positive.indices;candidates.insert(candidates.end(),negative.indices.begin(),negative.indices.end());std::sort(candidates.begin(),candidates.end());candidates=prune_close(candidates,high,options.minimum_peak_distance_s*fs);diagnostics.threshold_peaks=candidates;
    const std::ptrdiff_t half=static_cast<std::ptrdiff_t>(std::floor(std::floor(options.fitting_window_width_s*fs)/2));std::vector<std::size_t> duration_peaks;std::vector<double> amplitudes;
    for(std::size_t k=0;k<candidates.size();++k){auto window=event_window(high,candidates,k,half);auto range=suppress_outside_zero_crossings(window,3);const auto width=range.second-range.first;if(static_cast<double>(width)<options.maximum_event_width_s*fs){duration_peaks.push_back(candidates[k]);std::ptrdiff_t start=static_cast<std::ptrdiff_t>(candidates[k])-half+static_cast<std::ptrdiff_t>(range.first),end=static_cast<std::ptrdiff_t>(candidates[k])-half+static_cast<std::ptrdiff_t>(range.second);start=std::max<std::ptrdiff_t>(0,start);end=std::min<std::ptrdiff_t>(count-1,end);auto mm=std::minmax_element(high.begin()+start,high.begin()+end+1);amplitudes.push_back(*mm.second-*mm.first);}}
    diagnostics.duration_valid_peaks=duration_peaks;std::vector<double> sparse_amplitudes(count,0.0);for(std::size_t i=0;i<duration_peaks.size();++i)sparse_amplitudes[duration_peaks[i]]=amplitudes[i];auto selected=detect_large_peaks_cluster_impl(fs,sparse_amplitudes,options.analysis_window_width_s,false).indices;selected=prune_close(selected,high,options.minimum_peak_distance_s*fs);diagnostics.selected_peaks=selected;
    std::vector<double>x(static_cast<std::size_t>(2*half+1));for(std::size_t i=0;i<x.size();++i)x[i]=i/fs;
    for(std::size_t k=0;k<selected.size();++k){Event event;event.pass_index=pass+1;event.peak_index=selected[k];event.window_start=static_cast<std::ptrdiff_t>(selected[k])-half;event.window_end=static_cast<std::ptrdiff_t>(selected[k])+half;auto target=event_window(high,selected,k,half);suppress_outside_zero_crossings(target,3);auto problem=make_problem(x,target);if(!problem.valid){event.rejection=Rejection::invalid_problem;diagnostics.events.push_back(event);continue;}event.initial=problem.initial;event.lower=problem.lower;event.upper=problem.upper;FitRequest request;request.samples={target.data(),x.data(),nullptr,target.size(),0};request.initial=problem.initial;request.bounds={problem.lower,problem.upper};auto fit=fitter_.fit(request);event.parameters=fit.parameters;event.solver_status=fit.status;event.objective=fit.objective;event.iterations=fit.iterations;event.evaluations=fit.evaluations;event.attempts=fit.attempts;event.qc_triggered=fit.qc_triggered;event.rescue_accepted=fit.rescue_accepted;event.bound_active=fit.bound_active;if(static_cast<int>(fit.status)<0){event.rejection=Rejection::numerical_failure;diagnostics.events.push_back(event);continue;}std::vector<double>waveform(x.size());for(std::size_t i=0;i<x.size();++i)waveform[i]=Fitter::evaluate(x[i],fit.parameters);double maximum=0;for(double v:waveform)maximum=std::max(maximum,std::abs(v));std::size_t first=0,last=waveform.size()-1;while(first<waveform.size()&&!(std::abs(waveform[first])>.1*maximum))++first;while(last>0&&!(std::abs(waveform[last])>.1*maximum))--last;if(first<waveform.size()&&static_cast<double>(last-first+1)>options.maximum_event_width_s*fs){event.rejection=Rejection::waveform_too_wide;diagnostics.events.push_back(event);continue;}double target2=0,residual2=0;for(std::size_t i=0;i<target.size();++i){target2+=target[i]*target[i];double d=waveform[i]-target[i];residual2+=d*d;}event.explained_variance=1-residual2/target2;if(event.explained_variance<0){event.rejection=Rejection::negative_explained_variance;diagnostics.events.push_back(event);continue;}const double marked=half/fs;if(std::exp(-.5*std::pow((marked-fit.parameters[4])/fit.parameters[3],2))<.2){event.rejection=Rejection::eccentric_envelope;diagnostics.events.push_back(event);continue;}event.accepted=true;event.rejection=Rejection::none;for(std::ptrdiff_t global=std::max<std::ptrdiff_t>(0,event.window_start);global<=std::min<std::ptrdiff_t>(count-1,event.window_end);++global)result.cleaned[static_cast<std::size_t>(global)]-=waveform[static_cast<std::size_t>(global-event.window_start)];diagnostics.events.push_back(event);}
    if(options.retain_stage_signals){diagnostics.running_median=std::move(median);diagnostics.high_pass=std::move(high);}result.passes.push_back(std::move(diagnostics));}
  result.valid=true;result.diagnostic="ok";return result;
}
} // namespace gabor::eeg
