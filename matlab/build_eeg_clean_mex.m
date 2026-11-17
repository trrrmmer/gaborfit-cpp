function output=build_eeg_clean_mex(buildDirectory)
%BUILD_EEG_CLEAN_MEX Build the experimental one-call native EEG cleaner.
root=fileparts(fileparts(mfilename('fullpath')));
if nargin<1||isempty(buildDirectory),buildDirectory=fullfile(root,'build');end
source={fullfile(root,'matlab','eeg_clean_mex.cpp'),fullfile(root,'src','eeg_clean.cpp')};
arguments={'-R2018a',['-I' fullfile(root,'include')],source{:}, ...
    ['-L' buildDirectory],'-lgaborfit','-outdir',fullfile(root,'matlab')};
if isunix && ~ismac
    arguments=[arguments,{'LDFLAGS=$LDFLAGS -Wl,-rpath,\$ORIGIN'}];
    shared=fullfile(buildDirectory,'libgaborfit.so');runtimeLibraries=fullfile(buildDirectory,'libgaborfit.so*');
elseif ismac
    arguments=[arguments,{'LDFLAGS=$LDFLAGS -Wl,-rpath,@loader_path'}];
    shared=fullfile(buildDirectory,'libgaborfit.dylib');runtimeLibraries=fullfile(buildDirectory,'libgaborfit*.dylib');
else
    shared=fullfile(buildDirectory,'Release','gaborfit.dll');
    if ~exist(shared,'file'),shared=fullfile(buildDirectory,'gaborfit.dll');end
    runtimeLibraries=shared;
end
mex(arguments{:});
if exist(shared,'file'),copyfile(runtimeLibraries,fullfile(root,'matlab'),'f');end
output=fullfile(root,'matlab',['eeg_clean_mex.' mexext]);
end
