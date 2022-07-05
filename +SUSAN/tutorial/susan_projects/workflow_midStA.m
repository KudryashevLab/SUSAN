%% Create average as the unbinned template
avgr = SUSAN.Modules.Averager;
avgr.gpu_list = [0 1];
avgr.threads_per_gpu = 2;
avgr.set_ctf_correction('wiener');
avgr.rec_halves = false;
avgr.set_symmetry('c1');
avgr.padding = 0;
avgr.reconstruct('template_bin1','tomos_tlt_b2.tomostxt','prj_002.ptclsraw',176);

%% Create the unbinned Reference Info

refs = SUSAN.Data.ReferenceInfo.create(1);

refs(1).map  = 'template_bin1_class001.mrc';
refs(1).mask = 'mask_sph_bin1.mrc';

refs.save(refs,'prj_002_b1.refstxt');

%% Prepare and configure the Project

box_size_unbinned = 176;

mngr = SUSAN.Project.Manager('prj_002_b1', box_size_unbinned);

% Below are listed parameter values which 
% are the same for all rounds of alignment iterations
mngr.initial_reference = 'prj_002_b1.refstxt';
mngr.initial_particles = 'prj_002.ptclsraw';
mngr.tomogram_file     = 'tomos_tlt_b1.tomostxt';

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
    mngr.aligner.set_offset_ellipsoid(10,1);
    mngr.aligner.bandpass.lowpass = 40;
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
avgr.reconstruct('rec_ite1_bin1','tomos_tlt_b1.tomostxt','prj_002_b1/ite_0001/particles.ptclsraw',176);

%% Round 2 with 6 Iterations (2-7)
% This round estimates translational
% & rotational alignment with offsets
tic;

mngr.aligner.halfsets = true;
mngr.fsc_threshold = 0.143;
lp = 45;
lp_step = 2;
for i = 2:7
    mngr.aligner.drift = true;
    as = atan2d(1,lp);
    mngr.aligner.set_angular_search(4*as,as,4*as,as);
    mngr.aligner.set_angular_refinement(1,2);
    mngr.aligner.set_offset_ellipsoid(6,1);
    mngr.aligner.bandpass.lowpass = lp;
    lp = mngr.execute_iteration(i);
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
avgr.reconstruct('rec_ite7_bin1','tomos_tlt_b1.tomostxt','prj_002_b1/ite_0007/particles.ptclsraw',176);

%% Round 3 with 3 Iterations (8-10)
% This round estimates translational
% & rotational alignment with offsets
tic;
mngr.aligner.halfsets = true;
mngr.fsc_threshold = 0.143;
lp = 52;
for i = 8:10
    mngr.aligner.drift = true;
    as = atan2d(1,lp);
    mngr.aligner.set_angular_search(4*as,as,4*as,as);
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
avgr.reconstruct('rec_ite10_bin1','tomos_tlt_b1.tomostxt','prj_002_b1/ite_0010/particles.ptclsraw',176);

%% Load particles of the last iteration and save them

prtcls = mngr.get_ptcls(10);
prtcls.position = prtcls.position + prtcls.ali_t;
prtcls.ali_t(:) = 0;
prtcls.save('prj_003.ptclsraw');

%% Show results: show FSC of the last iteration

figure;
mngr.show_fsc(10);

%% Show results: display reconstruction of the last iteration
% N.B.: For this section you need to install Dynamo 
% and activate it in the current MATLAB section at command line!
dtmshow( dbandpass(mngr.get_map(10),[0 45 5]) );