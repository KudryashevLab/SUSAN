%% Create the Tomograms Info for unbinned stacks

N = 4;  % number of tilt series (TS) to process together
K = 59; % maximal number of projections in processed TS
tomos = SUSAN.Data.TomosInfo(N,K);
apix = 2.62;            % angstroms per pixel
tsiz = [3710 3710 880]; % pixels (should be the same for all processed TS)

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
ctf_est.gpu_list = [0 1]; % ID's of GPUs to use
ctf_est.resolution.min = 30;    % angstroms
ctf_est.resolution.max = 8.5;     % angstroms
ctf_est.defocus.min = 10000;    % angstroms
ctf_est.defocus.smax = 50000;   % angstroms
ctf_est.verbose = 2;
tomos_ctf = ctf_est.estimate('ctf_grid','grid_ctf.ptclsraw','tomos_raw_b1.tomostxt');
tomos_ctf.save('tomos_tlt_b1.tomostxt');
disp(toc);

%% Create the Tomograms Info for binned stacks

N = 4;  % number of tilt series (TS) to process together
K = 59; % maximal number of projections in processed TS
tomos = SUSAN.Data.TomosInfo(N,K);
% Put below values for unbinned data
apix = 2.62;            % angstroms per pixel
tsiz = [3710 3710 880]; % pixels (should be the same for all processed TS)

binnings = [2 4 8]; % used binnings

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

tbl = SUSAN.read('picked_b8.tbl');

tomos = SUSAN.read('tomos_tlt_b4.tomostxt');

% we need to re-calculate particles coordinates since we will use 
% a table with picked particles which is in bin 8 and
% a reference structure and corresponding map which is in bin 4
bin_factor = 8 / 4;
tbl(:,[24 25 26]) = bin_factor * tbl(:,[24 25 26]);

% generate random particle orientations
tbl(:,[7 8 9]) = 360 * rand(size(tbl(:,[7 8 9])));
ptcls = SUSAN.Data.ParticlesInfo(tbl,tomos);
ptcls.save('prj_001.ptclsraw');

%% Create the binned Reference Info

refs = SUSAN.Data.ReferenceInfo.create(1);

refs(1).map  = 'emd_3420_b4.mrc';
refs(1).mask = 'mask_sph_b4.mrc';

refs.save(refs,'prj_001_b4.refstxt');

%% Prepare and configure the Project

box_size_binned = 64;

mngr = SUSAN.Project.Manager('prj_001_b4', box_size_binned);

% Below are listed parameter values which 
% are the same for all rounds of alignment iterations
mngr.initial_reference = 'prj_001_b4.refstxt';
mngr.initial_particles = 'prj_001.ptclsraw';
mngr.tomogram_file     = 'tomos_tlt_b4.tomostxt';

mngr.gpu_list = [0 1];

mngr.aligner.set_ctf_correction('on_reference');
mngr.aligner.set_normalization('zm1s');
mngr.aligner.padding = 0;
mngr.averager.set_ctf_correction('phase_flip');

mngr.cc_threshold = 0.9; % cc threshold for reconstruction
mngr.alignment_type = 3; % 3 here means to make 3D alignment
mngr.aligner.bandpass.highpass = 0;
mngr.aligner.bandpass.lowpass = 21;

%% Round 1 with 1 Iteration (1)
% This round estimates pure translational alignment w/o offsets
tic;
for i = 1
    mngr.aligner.drift = false;
    mngr.aligner.set_angular_search(0,1,0,1);
    mngr.aligner.set_offset_ellipsoid(20,1);
    mngr.execute_iteration(i);
end
disp(toc);

%% Round 2 with 3 Iterations (2-4)
% This round estimates translational
% & rotational alignment with offsets
tic;
for i = 2:4
    mngr.aligner.drift = true;
    mngr.aligner.set_angular_search(36,4,36,4);
    mngr.aligner.set_angular_refinement(1,2);
    mngr.aligner.set_offset_ellipsoid(6,1);
    mngr.execute_iteration(i);
end
disp(toc);

%% Round 3 with 5 Iterations (5-9)
% This round estimates translational
% & rotational alignment with offsets
tic;
for i = 5:9
    mngr.aligner.drift = true;
    mngr.aligner.set_angular_search(8,2,8,2);
    mngr.aligner.set_angular_refinement(3,2);
    mngr.aligner.set_offset_ellipsoid(6,1);
    mngr.execute_iteration(i);
end
disp(toc);

%% Load particles of the last iteration (9) and save them

prtcls = mngr.get_ptcls(9);
prtcls.position = prtcls.position + prtcls.ali_t;
prtcls.ali_t(:) = 0;
prtcls.save('prj_002.ptclsraw');

%% Show results: show FSC of the last iteration (9)
figure;
mngr.show_fsc(9);

%% Show results: display reconstruction of the last iteration (9)
% N.B.: For this section you need to install Dynamo 
% and activate it in the current MATLAB section at command line!
dtmshow( dbandpass(mngr.get_map(7),[0 45 5]) );
