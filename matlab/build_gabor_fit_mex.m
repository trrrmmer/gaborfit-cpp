function output=build_gabor_fit_mex(buildDirectory)
%BUILD_GABOR_FIT_MEX Build the experimental C ABI wrapper after CMake build.
root=fileparts(fileparts(mfilename('fullpath')));
if nargin<1||isempty(buildDirectory),buildDirectory=fullfile(root,'build');end
source=fullfile(root,'matlab','gabor_fit_mex.cpp');include=['-I' fullfile(root,'include')];library=['-L' buildDirectory];
arguments={'-R2018a',include,source,library,'-lgaborfit','-outdir',fullfile(root,'matlab')};
if isunix && ~ismac
    arguments=[arguments,{'LDFLAGS=$LDFLAGS -Wl,-rpath,\$ORIGIN'}];shared=fullfile(buildDirectory,'libgaborfit.so');
    runtimeLibraries=fullfile(buildDirectory,'libgaborfit.so*');
elseif ismac
    arguments=[arguments,{'LDFLAGS=$LDFLAGS -Wl,-rpath,@loader_path'}];shared=fullfile(buildDirectory,'libgaborfit.dylib');
    runtimeLibraries=fullfile(buildDirectory,'libgaborfit*.dylib');
else
    shared=fullfile(buildDirectory,'Release','gaborfit.dll');if ~exist(shared,'file'),shared=fullfile(buildDirectory,'gaborfit.dll');end
    runtimeLibraries=shared;
end
mex(arguments{:});
if exist(shared,'file'),copyfile(runtimeLibraries,fullfile(root,'matlab'),'f');end
output=fullfile(root,'matlab',['gabor_fit_mex.' mexext]);
end

