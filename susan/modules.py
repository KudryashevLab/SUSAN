###########################################################################
# This file is part of the Substack Analysis (SUSAN) framework.
# Copyright (c) 2018-2021 Ricardo Miguel Sanchez Loayza.
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
# 
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
###########################################################################

import os as _os
import susan.utils.datatypes as _dt

def _get_gpu_str(list_gpus_ids):
    gpu_str = ','.join( str(num) for num in  list_gpus_ids )
    return gpu_str

###############################################################################

class Aligner:
    def __init__(self):
        self.list_gpus_ids     = [0]
        self.threads_per_gpu   = 1
        self.bandpass          = _dt.bandpass(0,-1,2)
        self.dimensionality    = 3
        self.extra_padding     = 0
        self.allow_drift       = True
        self.halfsets_independ = False
        self.use_cc_sigma      = False
        self.cone              = _dt.search_params(0,1)
        self.inplane           = _dt.search_params(0,1)
        self.refine            = _dt.refine_params(0,1)
        self.offset            = _dt.offset_params([4,4,4],1,'ellipsoid')
        self.padding_type      = 'noise'
        self.normalize_type    = 'zero_mean_one_std'
        self.ctf_correction    = 'on_reference'
        self.pseudo_symmetry   = 'c1'
        self.ssnr              = _dt.ssnr(1,0.01)
        self.mpi               = _dt.mpi_params('srun -n %d ',1)
        self.verbosity         = 0
        
    def set_angular_search(self,c_r=0,c_s=1,i_r=0,i_s=1):
        self.cone.span    = c_r
        self.cone.step    = c_s
        self.inplane.span = i_r
        self.inplane.step = i_s
        
    def set_offset_search(self,off_range,off_step=1,off_type='ellipsoid'):
        if not off_type in ['ellipsoid','cylinider']:
            raise ValueError('Invalid offset type. Only "ellipsoid" or "cylinder" are valid')
        
        if isinstance(off_range,int) or isinstance(off_range,float):
            self.offset.span = (off_range,off_range,off_range)
        elif len(off_range) == 3:
            self.offset.span = off_range
        elif len(off_range) == 2:
            self.offset.span = (off_range[0],off_range[0],off_range[1])
        else:
            raise ValueError('Offset range can have up to 3 elements.')
        self.offset.step = off_step
        self.offset.kind = off_type
        
    def _validate(self):
        if not self.dimensionality in [2,3]:
            raise ValueError('Invalid dimensionality type. Only 3 or 3 are valid')
        
        if not self.offset.kind in ['ellipsoid','cylinder']:
            raise ValueError('Invalid offset type. Only "ellipsoid" or "cylinder" are valid')
        
        if not self.padding_type in ['zero','noise']:
            raise ValueError('Invalid padding type. Only "zero" or "noise" are valid')
        
        if not self.normalize_type in ['none','zero_mean','zero_mean_one_std','zero_mean_proj_weight']:
            raise ValueError('Invalid normalization type. Only "none", "zero_mean", "zero_mean_one_std" or "zero_mean_proj_weight" are valid')
        
        if not self.ctf_correction in ['none','on_reference','on_substack','wiener_ssnr','wiener_white','cfsc']:
            raise ValueError('Invalid ctf correction type. Only "none", "on_reference", "on_substack", "wiener_ssnr", "wiener_white" or "cfsc" are valid')
        
        if not self.offset.step > 0 or not self.cone.step > 0 or not self.inplane.step > 0:
            raise ValueError('The steps values must be larger than 0')
        
        if self.offset.span[0] < self.offset.step or self.offset.span[1] < self.offset.step  or self.offset.span[2] < self.offset.step :
            raise ValueError('Offset: Step cannot be larger than Range/Span')

        if self.cone.span == 0:
            self.cone.step = 1
        else:
            if self.cone.span < self.cone.step:
                raise ValueError('Cone: Step cannot be larger than Range/Span')

        if self.inplane.span == 0:
            self.inplane.step = 1
        else:
            if self.inplane.span < self.inplane.step:
                raise ValueError('Inplane: Step cannot be larger than Range/Span')

    def get_args(self,ptcls_out,refs_file,tomos_file,ptcls_in,box_size):
        self._validate()
        n_threads = len(self.list_gpus_ids)*self.threads_per_gpu
        gpu_str   = _get_gpu_str(self.list_gpus_ids)
        args =        ' -tomos_file '      + tomos_file
        args = args + ' -ptcls_in '        + ptcls_in
        args = args + ' -ptcls_out '       + ptcls_out
        args = args + ' -refs_file '       + refs_file
        args = args + ' -n_threads %d'     % n_threads
        args = args + ' -gpu_list '        + gpu_str
        args = args + ' -box_size %d'      % box_size
        args = args + ' -pad_size %d'      % self.extra_padding
        args = args + ' -pad_type '        + self.padding_type
        args = args + ' -norm_type '       + self.normalize_type
        args = args + ' -ctf_type '        + self.ctf_correction
        args = args + ' -ssnr_param %f,%f' % (self.ssnr.F,self.ssnr.S)
        args = args + ' -bandpass %f,%f'   % (self.bandpass.highpass,self.bandpass.lowpass)
        args = args + ' -rolloff_f %f'     % self.bandpass.rolloff
        args = args + ' -p_symmetry '      + self.pseudo_symmetry
        args = args + ' -ali_halves %d'    % self.halfsets_independ
        args = args + ' -use_sigma %d'     % self.use_cc_sigma
        args = args + ' -cone %f,%f'       % (self.cone.span,self.cone.step)
        args = args + ' -inplane %f,%f'    % (self.inplane.span,self.inplane.step)
        args = args + ' -refine %d,%d'     % (self.refine.factor,self.refine.levels)
        args = args + ' -off_type '        + self.offset.kind
        args = args + ' -off_params %f,%f,%f,%f' % (self.offset.span[0],self.offset.span[1],self.offset.span[2],self.offset.step)
        args = args + ' -type %d'          % self.dimensionality
        args = args + ' -verbosity %d'     % self.verbosity
        return args
    
    def align(self,ptcls_out,refs_file,tomos_file,ptcls_in,box_size):
        cmd = 'susan_aligner ' + self.get_args(ptcls_out, refs_file, tomos_file, ptcls_in, box_size)
        rslt = _os.system(cmd)
        if not rslt == 0:
            raise NameError('Error executing the alignment: ' + cmd)
    
    def align_mpi(self,ptcls_out,refs_file,tomos_file,ptcls_in,box_size):
        cmd = self.mpi.gen_cmd() + ' ' + _os.path.dirname(_os.path.abspath(__file__)) + '/bin/susan_aligner_mpi ' + self.get_args(ptcls_out, refs_file, tomos_file, ptcls_in, 
box_size)
        rslt = _os.system(cmd)
        if not rslt == 0:
            raise NameError('Error executing the alignment: ' + cmd)

###############################################################################

class Averager:
    def __init__(self):
        self.list_gpus_ids     = [0]
        self.threads_per_gpu   = 1
        self.bandpass          = _dt.bandpass(0,-1,2)
        self.extra_padding     = 0
        self.rec_halfsets      = False
        self.padding_type      = 'noise'
        self.normalize_type    = 'zero_mean_one_std'
        self.ctf_correction    = 'wiener'
        self.symmetry          = 'c1'
        self.ssnr              = _dt.ssnr(1,0.01)
        self.inversion         = _dt.inversion_params(10,0.75)
        self.mpi               = _dt.mpi_params('srun -n %d ',1)
        self.verbosity         = 0
        
    def _validate(self):
        if not self.padding_type in ['zero','noise']:
            raise NameError('Invalid padding type. Only "zero" or "noise" are valid')
        
        if not self.normalize_type in ['none','zero_mean','zero_mean_one_std','zero_mean_proj_weight']:
            raise NameError('Invalid normalization type. Only "none", "zero_mean", "zero_mean_one_std" or "zero_mean_proj_weight" are valid')
        
        if not self.ctf_correction in ['none','phase_flip','wiener','wiener_ssnr']:
            raise NameError('Invalid ctf correction type. Only "none", "phase_flip", "wiener" ot "wiener_ssnr" are valid')
            
    def get_args(self,out_pfx,tomos_file,ptcls_in,box_size):
        self._validate()
        if self.bandpass.lowpass <= 0:
            self.bandpass.lowpass = box_size/2-1
        n_threads = len(self.list_gpus_ids)*self.threads_per_gpu
        gpu_str   = _get_gpu_str(self.list_gpus_ids)
        args =        ' -tomos_file '      + tomos_file
        args = args + ' -out_prefix '      + out_pfx
        args = args + ' -ptcls_file '      + ptcls_in
        args = args + ' -n_threads %d'     % n_threads
        args = args + ' -gpu_list '        + gpu_str
        args = args + ' -box_size %d'      % box_size
        args = args + ' -pad_size %d'      % self.extra_padding
        args = args + ' -pad_type '        + self.padding_type
        args = args + ' -norm_type '       + self.normalize_type
        args = args + ' -ctf_type '        + self.ctf_correction
        args = args + ' -ssnr_param %f,%f' % (self.ssnr.F,self.ssnr.S)
        args = args + ' -w_inv_iter %d'    % self.inversion.ite
        args = args + ' -w_inv_gstd %f'    % self.inversion.std
        args = args + ' -bandpass %f,%f'   % (self.bandpass.highpass,self.bandpass.lowpass)
        args = args + ' -rolloff_f %f'     % self.bandpass.rolloff
        args = args + ' -symmetry '        + self.symmetry
        args = args + ' -rec_halves %d'    % self.rec_halfsets
        args = args + ' -verbosity %d'     % self.verbosity
        return args
    
    def reconstruct(self,out_pfx,tomos_file,ptcls_in,box_size):
        cmd = 'susan_reconstruct ' + self.get_args(out_pfx,tomos_file,ptcls_in,box_size)
        rslt = _os.system(cmd)
        if not rslt == 0:
            raise NameError('Error executing the reconstruction: ' + cmd)
    
    def reconstruct_mpi(self,out_pfx,tomos_file,ptcls_in,box_size):
        cmd = self.mpi.gen_cmd() + ' ' + _os.path.dirname(_os.path.abspath(__file__)) + '/bin/susan_reconstruct_mpi' + self.get_args(out_pfx,tomos_file,ptcls_in,box_size)
        rslt = _os.system(cmd)
        if not rslt == 0:
            raise NameError('Error executing the reconstruction: ' + cmd)

###############################################################################

class CtfEstimator:
    def __init__(self):
        self.list_gpus_ids     = [0]
        self.threads_per_gpu   = 1
        self.binning           = 0
        self.resolution_angs   = _dt.range_params(40,7)
        self.defocus_angstroms = _dt.range_params(10000,90000)
        self.tilt_search       = 3000
        self.refine_defocus    = _dt.search_params(2000,100)
        self.max_bfactor       = 600
        self.resolution_thres  = 0.75
        #self.mpi               = _dt.mpi_params('srun -n %d ',1)
        self.verbose           = 0
        #self.verbosity         = 0
        
    def _validate(self):
        if not self.refine_defocus.step > 0:
            raise ValueError('The steps values must be larger than 0')
        
        if self.refine_defocus.span < self.refine_defocus.step:
            raise ValueError('Refine Defocus: Step cannot be larger than Range/Span')

        #if self.resolution_angs.max_val < self.resolution_angs.min_val:
        #    raise ValueError('Resolution (angstroms): min is larger than max')

        if self.defocus_angstroms.max_val < self.defocus_angstroms.min_val:
            raise ValueError('Defocus (angstroms): min is larger than max')
            
    def get_args(self,out_dir,tomos_file,ptcls_in,box_size):
        self._validate()
        if out_dir[-1] == '/':
            out_dir = out_dir[:-1]
        n_threads = len(self.list_gpus_ids)*self.threads_per_gpu
        gpu_str   = _get_gpu_str(self.list_gpus_ids)
        args =        ' -tomos_in '        + tomos_file
        args = args + ' -data_out '        + out_dir
        args = args + ' -ptcls_file '      + ptcls_in
        args = args + ' -n_threads %d'     % n_threads
        args = args + ' -gpu_list '        + gpu_str
        args = args + ' -box_size %d'      % box_size
        args = args + ' -res_range %f,%f'  % (self.resolution_angs.min_val,self.resolution_angs.max_val)
        args = args + ' -res_thres %f'     % self.resolution_thres
        args = args + ' -def_range %f,%f'  % (self.defocus_angstroms.min_val,self.defocus_angstroms.max_val)
        args = args + ' -tilt_search %f'   % self.tilt_search
        args = args + ' -refine_def %f,%f' % (self.refine_defocus.span,self.refine_defocus.step)
        args = args + ' -binning %d'       % self.binning
        args = args + ' -bfactor_max %f'   % self.max_bfactor
        args = args + ' -verbose %d'       % self.verbose
        #args = args + ' -verbosity %d'     % self.verbosity
        return args
    
    def estimate(self,out_dir,tomos_file,ptcls_in,box_size):
        cmd = 'susan_estimate_ctf ' + self.get_args(out_dir,tomos_file,ptcls_in,box_size)
        rslt = _os.system(cmd)
        if not rslt == 0:
            raise NameError('Error executing the CTF estimation: ' + cmd)

###############################################################################

class CtfRefiner:
    def __init__(self):
        self.list_gpus_ids     = [0]
        self.threads_per_gpu   = 1
        self.bandpass          = _dt.bandpass(0,-1,2)
        self.dimensionality    = 3
        self.extra_padding     = 0
        self.padding_type      = 'noise'
        self.halfsets_independ = False
        self.estimate_dose_wgt = False
        self.defocus_angstroms = _dt.search_params(1000,100)
        self.angles            = _dt.search_params(2,1)
        self.mpi               = _dt.mpi_params('srun -n %d ',1)
        self.verbosity         = 0
        
    def _validate(self):
        if not self.padding_type in ['zero','noise']:
            raise ValueError('Invalid padding type. Only "zero" or "noise" are valid')
        
        if not self.defocus_angstroms.step > 0 or not self.angles.step > 0:
            raise ValueError('The steps values must be larger than 0')
        
        if self.defocus_angstroms.span < self.defocus_angstroms.step:
            raise ValueError('Defocus (Angstroms): Step cannot be larger than Range/Span')

        if self.angles.span < self.angles.step:
            raise ValueError('ANgles (degrees): Step cannot be larger than Range/Span')

    def get_args(self,ptcls_out,refs_file,tomos_file,ptcls_in,box_size):
        self._validate()
        n_threads = len(self.list_gpus_ids)*self.threads_per_gpu
        gpu_str   = _get_gpu_str(self.list_gpus_ids)
        args =        ' -tomos_file '      + tomos_file
        args = args + ' -ptcls_in '        + ptcls_in
        args = args + ' -ptcls_out '       + ptcls_out
        args = args + ' -refs_file '       + refs_file
        args = args + ' -n_threads %d'     % n_threads
        args = args + ' -gpu_list '        + gpu_str
        args = args + ' -box_size %d'      % box_size
        args = args + ' -pad_size %d'      % self.extra_padding
        args = args + ' -pad_type '        + self.padding_type
        args = args + ' -bandpass %f,%f'   % (self.bandpass.highpass,self.bandpass.lowpass)
        args = args + ' -rolloff_f %f'     % self.bandpass.rolloff
        args = args + ' -def_search %f,%f' % (self.defocus_angstroms.span,self.defocus_angstroms.step)
        args = args + ' -ang_search %f,%f' % (self.angles.span,self.angles.step)
        args = args + ' -use_halves %d'    % self.halfsets_independ
        args = args + ' -est_dose %d'      % self.estimate_dose_wgt
        args = args + ' -verbosity %d'     % self.verbosity
        return args
    
    def refine(self,ptcls_out,refs_file,tomos_file,ptcls_in,box_size):
        cmd = 'susan_ctf_refiner ' + self.get_args(ptcls_out, refs_file, tomos_file, ptcls_in, box_size)
        rslt = _os.system(cmd)
        if not rslt == 0:
            raise NameError('Error executing the refinement: ' + cmd)
    
    def refine_mpi(self,ptcls_out,refs_file,tomos_file,ptcls_in,box_size):
        cmd = self.mpi.gen_cmd() + ' ' + _os.path.dirname(_os.path.abspath(__file__)) + '/bin/susan_ctf_refiner_mpi ' + self.get_args(ptcls_out, refs_file, tomos_file, ptcls_in, 
box_size)
        rslt = _os.system(cmd)
        if not rslt == 0:
            raise NameError('Error executing the refinement: ' + cmd)

