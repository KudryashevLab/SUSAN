%% Create the Tomograms Info for unbinned stacks

N = 4;  % number of tilt series (TS) to process together
K = 59; % maximal number of projections in processed TS
tomos = SUSAN.Data.TomosInfo(N,K);
apix = 2.62;            % angstroms per pixel
tsiz = [3710 3710 1400]; % pixels (should be the same for all processed TS)

for i = 1:N
    tomo_base = sprintf('../data/mixedCTEM_tomo%d',i);
    
    tomos.tomo_id(i) = i;
    
    tomos.set_stack (i,[tomo_base '.b1.ali.mrc']);
    tomos.set_angles(i,[tomo_base '.tlt']);
    
    tomos.pix_size(i,:)  = apix;
    tomos.tomo_size(i,:) = tsiz;
end
tomos.save('tomos_raw_b1.tomostxt');

%% Create a 2D grid to estimate the CTF

tomos = SUSAN.read('tomos_raw_b1.tomostxt');
sampling = 180;
grid = SUSAN.Data.ParticlesInfo.grid2D(sampling,tomos);
grid.save('grid_ctf.ptclsraw');

%% Create CtfEstimator

tic;
patch_size = 400;
ctf_est = SUSAN.Modules.CtfEstimator(patch_size);
ctf_est.binning = 0;
ctf_est.gpu_list = [0 1]; % ID's of GPUs to use
ctf_est.resolution.min = 30;    % angstroms
ctf_est.resolution.max = 8.5;     % angstroms
ctf_est.defocus.min = 10000;    % angstroms
ctf_est.defocus.smax = 50000;   % angstroms
tomos_ctf = ctf_est.estimate('ctf_grid','grid_ctf.ptclsraw','tomos_raw_b1.tomostxt');
tomos_ctf.save('tomos_tlt_b1.tomostxt');
disp(toc);

%% Create the Tomograms Info for binned stacks

N = 4;  % number of tilt series (TS) to process together
K = 59; % maximal number of projections in processed TS
tomos = SUSAN.Data.TomosInfo(N,K);
% Put below values for unbinned data
apix = 2.62;            % angstroms per pixel
tsiz = [3710 3710 1400]; % pixels (should be the same for all processed TS)

binnings = [2]; % used binnings

for bin=binnings
    apix_binned = apix * bin;
    tsiz_binned = ceil(tsiz / bin); 
    for i = 1:N
        tomo_base = sprintf('../data/mixedCTEM_tomo%d',i);

        tomos.tomo_id(i) = i;

        tomos.set_stack  (i,[tomo_base '.b' num2str(bin) '.ali.mrc']);
        tomos.set_angles (i,[tomo_base '.tlt']);
        tomos.set_defocus(i,sprintf('ctf_grid/Tomo%03d/defocus.txt',i),'Basic');

        tomos.pix_size(i,:)  = apix_binned;
        tomos.tomo_size(i,:) = tsiz_binned;
    end
    tomos.save(['tomos_tlt_b' num2str(bin) '.tomostxt']);
end

%% Create the Particles Info for binned data

tbl = SUSAN.read('picked_bin2.tbl');

tomos = SUSAN.read('tomos_tlt_b2.tomostxt');

ptcls = SUSAN.Data.ParticlesInfo(tbl,tomos);
ptcls.save('prj_001.ptclsraw');

%% Check average
avgr = SUSAN.Modules.Averager;
avgr.gpu_list = [0 1];
avgr.threads_per_gpu = 2;
avgr.set_ctf_correction('wiener');
avgr.rec_halves = false;
avgr.set_symmetry('c1');
avgr.padding = 0;
avgr.reconstruct('rec_ite0_bin2','tomos_tlt_b2.tomostxt','prj_001.ptclsraw',88);

%% Create the binned Reference Info

refs = SUSAN.Data.ReferenceInfo.create(1);

refs(1).map  = 'template_bin2.mrc';
refs(1).mask = 'mask_sph_bin2.mrc';

refs.save(refs,'prj_001_b2.refstxt');

%% Prepare and configure the Project

box_size_binned = 88;

mngr = SUSAN.Project.Manager('prj_001_b2', box_size_binned);

% Below are listed parameter values which 
% are the same for all rounds of alignment iterations
mngr.initial_reference = 'prj_001_b2.refstxt';
mngr.initial_particles = 'prj_001.ptclsraw';
mngr.tomogram_file     = 'tomos_tlt_b2.tomostxt';

mngr.gpu_list = [0 1];

mngr.aligner.set_ctf_correction('on_reference');
mngr.aligner.set_normalization('zm1s');
mngr.aligner.padding = 0;
mngr.averager.set_ctf_correction('phase_flip');

mngr.cc_threshold = 0.9; % cc threshold for reconstruction
mngr.alignment_type = 3; % 3 here means to make 3D alignment
mngr.aligner.bandpass.highpass = 0;

%% Round 1 with 1 Iteration (1)
% This round estimates pure translational alignment w/o offsets
tic;

for i = 1
    mngr.aligner.drift = false;
    mngr.aligner.set_angular_search(0,1,0,1);
    mngr.aligner.set_offset_ellipsoid(20,1);
    mngr.aligner.bandpass.lowpass = 20;
    mngr.execute_iteration(i);
end
disp(toc);

%% Check average
avgr = SUSAN.Modules.Averager;
avgr.gpu_list = [0 1];
avgr.threads_per_gpu = 2;
avgr.set_ctf_correction('wiener');
avgr.rec_halves = false;
avgr.set_symmetry('c1');
avgr.padding = 0;
avgr.reconstruct('rec_ite1_bin2','tomos_tlt_b2.tomostxt','prj_001_b2/ite_0001/particles.ptclsraw',88);

%% Round 2 with 3 Iterations (2-4)
% This round estimates translational
% & rotational alignment with offsets
tic;

mngr.aligner.halfsets = 1;
mngr.fsc_threshold = 0.143;
lp = 10;
for i = 2:4
    mngr.aligner.drift = true;
    as = atan2d(1,lp);
    mngr.aligner.set_angular_search(8*as,as,8*as,as);
    mngr.aligner.set_angular_refinement(1,2);
    mngr.aligner.set_offset_ellipsoid(6,1);
    mngr.aligner.bandpass.lowpass = lp;
    lp = mngr.execute_iteration(i);
end
disp(toc);

%% Check average
avgr = SUSAN.Modules.Averager;
avgr.gpu_list = [0 1];
avgr.threads_per_gpu = 2;
avgr.set_ctf_correction('wiener');
avgr.rec_halves = false;
avgr.set_symmetry('c1');
avgr.padding = 0;
avgr.reconstruct('rec_ite4_bin2','tomos_tlt_b2.tomostxt','prj_001_b2/ite_0004/particles.ptclsraw',88);

%% Round 3 with 3 Iterations (5-7)
% This round estimates translational
% & rotational alignment with offsets
tic;
mngr.aligner.halfsets = false;
mngr.fsc_threshold = 0.143;
lp = 30;
lp_step = 4;
for i = 5:7
    mngr.aligner.drift = true;
    as = atan2d(1,lp);
    mngr.aligner.set_angular_search(8*as,as,8*as,as);
    mngr.aligner.set_angular_refinement(0,1);
    mngr.aligner.set_offset_ellipsoid(6,1);
    mngr.aligner.bandpass.lowpass = lp;
    mngr.execute_iteration(i);
    lp = lp + lp_step;
end
disp(toc);

%% Check average
avgr = SUSAN.Modules.Averager;
avgr.gpu_list = [0 1];
avgr.threads_per_gpu = 2;
avgr.set_ctf_correction('wiener');
avgr.rec_halves = false;
avgr.set_symmetry('c1');
avgr.padding = 0;
avgr.reconstruct('rec_ite7_bin2','tomos_tlt_b2.tomostxt','prj_001_b2/ite_0007/particles.ptclsraw',88);

%% Round 4 with 3 Iterations (8-10)
% This round estimates translational
% & rotational alignment with offsets
tic;
mngr.aligner.halfsets = false;
mngr.fsc_threshold = 0.143;
lp = 45;
lp_step = 5;
for i = 8:10
    mngr.aligner.drift = true;
    as = atan2d(1,lp);
    mngr.aligner.set_angular_search(8*as,as,8*as,as);
    mngr.aligner.set_angular_refinement(0,1);
    mngr.aligner.set_offset_ellipsoid(6,1);
    mngr.aligner.bandpass.lowpass = lp;
    mngr.execute_iteration(i);
    lp = lp + lp_step;
end
disp(toc);

%% Check average
avgr = SUSAN.Modules.Averager;
avgr.gpu_list = [0 1];
avgr.threads_per_gpu = 2;
avgr.set_ctf_correction('wiener');
avgr.rec_halves = false;
avgr.set_symmetry('c1');
avgr.padding = 0;
avgr.reconstruct('rec_ite10_bin2','tomos_tlt_b2.tomostxt','prj_001_b2/ite_0010/particles.ptclsraw',88);

%% Load particles of the last iteration and save them

prtcls = mngr.get_ptcls(10);
prtcls.position = prtcls.position + prtcls.ali_t;
prtcls.ali_t(:) = 0;
prtcls.save('prj_002.ptclsraw');

%% Show results: show FSC of the last iteration
%mngr = SUSAN.Project.Manager('prj_001_b2');
figure;
mngr.show_fsc(10);

%% Show results: display reconstruction of the last iteration
% N.B.: For this section you need to install Dynamo 
% and activate it in the current MATLAB section at command line!
dtmshow( dbandpass(mngr.get_map(10),[0 45 5]) );