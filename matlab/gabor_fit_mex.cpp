#include "mex.h"
#include "gabor/gabor_c.h"
#include <algorithm>

namespace {
gaborfit_context* persistent_context = nullptr;
void release_context() {
  gaborfit_context_destroy(persistent_context);
  persistent_context = nullptr;
}
gaborfit_context* context_for(size_t sample_count) {
  if (!persistent_context) {
    persistent_context = gaborfit_context_create(sample_count);
    if (!persistent_context) mexErrMsgIdAndTxt("gaborfit:allocation", "Failed to create native fitting context");
    mexAtExit(release_context);
  }
  return persistent_context;
}
}

void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
  if (nrhs < 5 || nrhs > 6)
    mexErrMsgIdAndTxt("gaborfit:arguments", "Usage: result=gabor_fit_mex(y,fs_or_t,initial,lower,upper[,weights])");
  const size_t n=mxGetNumberOfElements(prhs[0]);
  if (!mxIsDouble(prhs[0]) || mxIsComplex(prhs[0]) || n<5) mexErrMsgIdAndTxt("gaborfit:signal","Invalid signal");
  for (int k=2;k<5;++k) if (!mxIsDouble(prhs[k]) || mxGetNumberOfElements(prhs[k])!=5)
    mexErrMsgIdAndTxt("gaborfit:parameters","initial/lower/upper must contain five doubles");
  gaborfit_request request{};gaborfit_request_defaults(&request);request.signal=mxGetDoubles(prhs[0]); request.sample_count=n;
  if (mxGetNumberOfElements(prhs[1])==1) request.sampling_frequency_hz=mxGetScalar(prhs[1]);
  else { if (mxGetNumberOfElements(prhs[1])!=n) mexErrMsgIdAndTxt("gaborfit:time","Time length mismatch"); request.times=mxGetDoubles(prhs[1]); }
  if (nrhs==6) { if(mxGetNumberOfElements(prhs[5])!=n) mexErrMsgIdAndTxt("gaborfit:weights","Weight length mismatch"); request.weights=mxGetDoubles(prhs[5]); }
  std::copy(mxGetDoubles(prhs[2]),mxGetDoubles(prhs[2])+5,request.initial);
  std::copy(mxGetDoubles(prhs[3]),mxGetDoubles(prhs[3])+5,request.lower);
  std::copy(mxGetDoubles(prhs[4]),mxGetDoubles(prhs[4])+5,request.upper);
  request.max_iterations=100;request.fallback_iterations=200;request.x_tolerance=1e-5;
  request.f_tolerance=1e-5;request.gradient_tolerance=1e-8;request.initial_damping=1e-3;
  request.deterministic_fallback=1;request.quality_control=1;
  gaborfit_result result{};gaborfit_fit(context_for(n),&request,&result);
  const char* fields[]={"status","parameters","objective","residual_rms","iterations","evaluations","attempts","qc_triggered","rescue_accepted","bound_active","diagnostic"};
  plhs[0]=mxCreateStructMatrix(1,1,11,fields);
  mxSetField(plhs[0],0,"status",mxCreateDoubleScalar(result.status));
  mxArray* parameters=mxCreateDoubleMatrix(1,5,mxREAL);std::copy(result.parameters,result.parameters+5,mxGetDoubles(parameters));mxSetField(plhs[0],0,"parameters",parameters);
  mxSetField(plhs[0],0,"objective",mxCreateDoubleScalar(result.objective));mxSetField(plhs[0],0,"residual_rms",mxCreateDoubleScalar(result.residual_rms));
  mxSetField(plhs[0],0,"iterations",mxCreateDoubleScalar(result.iterations));mxSetField(plhs[0],0,"evaluations",mxCreateDoubleScalar(result.evaluations));
  mxSetField(plhs[0],0,"attempts",mxCreateDoubleScalar(result.attempts));mxSetField(plhs[0],0,"qc_triggered",mxCreateLogicalScalar(result.qc_triggered));
  mxSetField(plhs[0],0,"rescue_accepted",mxCreateLogicalScalar(result.rescue_accepted));mxSetField(plhs[0],0,"bound_active",mxCreateLogicalScalar(result.bound_active));
  mxSetField(plhs[0],0,"diagnostic",mxCreateString(result.diagnostic));
  (void)nlhs;
}

