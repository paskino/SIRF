# -*- coding: utf-8 -*-

"""MCIR for PET with primal-dual algorithms.

Usage:
  PET_MCIR_PD [--help | options]

Options:
  -T <pattern>, --trans=<pattern>   transformation pattern, * or % wildcard
                                    (e.g., tm_ms*.txt). Enclose in quotations.
  -t <str>, --trans_type=<str>      transformation type (tm, disp, def)
                                    [default: tm]
  -S <pattern>, --sino=<pattern>    sinogram pattern, * or % wildcard
                                    (e.g., sino_ms*.hs). Enclose in quotations.
  -a <pattern>, --attn=<pattern>    attenuation pattern, * or % wildcard
                                    (e.g., attn_ms*.hv). Enclose in quotations.
  -R <pattern>, --rand=<pattern>    randoms pattern, * or % wildcard
                                    (e.g., rand_ms*.hs). Enclose in quotations.
  -n <norm>, --norm=<norm>          ECAT8 bin normalization file
  -e <int>, --epoch=<int>           num epochs [default: 10]
  -r <string>, --reg=<string>       regularisation ("None","FGP_TV","explicit_TV", ...)
                                    [default: None]
  -o <outp>, --outp=<outp>          output file prefix [default: recon]
  --outpath=<string>                output folder path
  --param_path=<string>             param folder path
  --nxny=<nxny>                     image x and y dimension [default: 127]
  --dxdy=<dxdy>                     image x and y spacing
                                    (default: determined by scanner)
  -I <str>, --initial=<str>         Initial estimate
  --visualisations                  show visualisations
  --nifti                           save output as nifti
  --gpu                             use GPU projector
  -v <int>, --verbosity=<int>       STIR verbosity [default: 0]
  -s <int>, --save_interval=<int>   save every x iterations [default: 10]
  --descriptive_fname               option to have descriptive filenames
  --update_obj_fn_interval=<int>    frequency to update objective function
                                    [default: 1]
  --alpha=<val>                     regularisation strength (if used)
                                    [default: 0.5]      
  --reg_iters=<val>                 Number of iterations for the regularisation
                                    subproblem [default: 100]
  --precond                         Use preconditioning
  --numSegsToCombine=<val>          Rebin all sinograms, with a given number of
                                    segments to combine. Increases speed.
  --numViewsToCombine=<val>         Rebin all sinograms, with a given number of
                                    views to combine. Increases speed.
  --normaliseDataAndBlock           Normalise raw data and block operator by
                                    multiplying by 1./normK.
  --algorithm=<string>              Which algorithm to run [default: spdhg]
  --numThreads=<int>                Number of threads to use [default: 14]
  --numSubsets=<int>                Number of physical subsets to use [default: 1]
  --gamma=<val>                     parameter controlling primal-dual trade-off (>1 promotes dual)
                                    [default: 1.]
  --PowerMethod_iters=<val>         number of iterations for the computation of operator norms
                                    with the power method [default: 10]
  --templateAcqData                 Use template acd data
"""

# SyneRBI Synergistic Image Reconstruction Framework (SIRF)
# Copyright 2020 University College London.
#
# This is software developed for the Collaborative Computational
# Project in Synergistic Reconstruction for Biomedical Imaging
# (formerly CCP PETMR)
# (http://www.ccpsynerbi.ac.uk/).
#
# Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#       http://www.apache.org/licenses/LICENSE-2.0
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from functools import partial
from os import path
import os
from glob import glob
from docopt import docopt
import matplotlib.pyplot as plt
import numpy as np

from sirf.Utilities import error, show_2D_array, examples_data_path
import sirf.Reg as reg
import sirf.STIR as pet
from ccpi.framework import BlockDataContainer, ImageGeometry, BlockGeometry
from ccpi.optimisation.algorithms import PDHG, SPDHG
from ccpi.optimisation.functions import \
    KullbackLeibler, BlockFunction, IndicatorBox, MixedL21Norm, ScaledFunction
from ccpi.optimisation.operators import \
    CompositionOperator, BlockOperator, LinearOperator, Gradient, ScaledOperator
from ccpi.plugins.regularisers import FGP_TV
from ccpi.filters import regularisers
#from ccpi.utilities import NUM_THREADS

__version__ = '0.1.0'
args = docopt(__doc__, version=__version__)

###########################################################################
# Global set-up
###########################################################################

# storage scheme
pet.AcquisitionData.set_storage_scheme('memory')
# Verbosity
pet.set_verbosity(int(args['--verbosity']))
if int(args['--verbosity']) == 0:
    msg_red = pet.MessageRedirector(None, None, None)
# Number of threads
#numThreads = args['--numThreads'] if args['--numThreads'] else NUM_THREADS
numThreads = args['--numThreads'] 
pet.set_max_omp_threads(numThreads)

if args['--templateAcqData']:
    template_acq_data = pet.AcquisitionData('Siemens_mMR', span=11, max_ring_diff=15, view_mash_factor=1)



def main():
    """Run main function."""

    use_gpu = True if args['--gpu'] else False

    ###########################################################################
    # Parse input files
    ###########################################################################

    [num_ms, trans_files, sino_files, attn_files, rand_files] = \
        get_filenames(args['--trans'],args['--sino'],args['--attn'],args['--rand'])

    ###########################################################################
    # Read input
    ###########################################################################

    [trans, sinos_raw, attns, rands_raw] = \
        read_files(trans_files, sino_files, attn_files, rand_files, args['--trans_type'])

    sinos = pre_process_sinos(sinos_raw, num_ms)
    rands = pre_process_sinos(rands_raw, num_ms)

    ###########################################################################
    # Initialise recon image
    ###########################################################################

    image = get_initial_estimate(sinos,use_gpu)

    ###########################################################################
    # Set up resamplers
    ###########################################################################

    if trans is None:
        resamplers = None
    else:
        resamplers = [get_resampler(image, trans=tran) for tran in trans]

    ###########################################################################
    # Resample attenuation images (if necessary)
    ###########################################################################

    resampled_attns = resample_attn_images(num_ms, attns, trans, use_gpu, image)
    print ("resampled_attns", len (resampled_attns))

    ###########################################################################
    # Set up acquisition models (one per motion state)
    ###########################################################################

    acq_models, masks = set_up_acq_models(
        num_ms, sinos, rands, resampled_attns, image, use_gpu)

    ###########################################################################
    # Set up reconstructor
    ###########################################################################

    if args['--reg']=='explicit_TV':
        [F, G, K, normK, tau, sigma, use_axpby, prob, gamma] = set_up_explicit_reconstructor(
            use_gpu, num_ms, image, acq_models, resamplers, masks, sinos, rands) 
    else:
        [F, G, K, normK, tau, sigma, use_axpby, prob, gamma] = set_up_reconstructor(
            use_gpu, num_ms, acq_models, resamplers, masks, sinos, rands)

    ###########################################################################
    # Get output filename
    ###########################################################################

    outp_file = get_output_filename(
        attn_files, normK, sigma, tau, sino_files, resamplers, use_gpu)

    ###########################################################################
    # Get algorithm
    ###########################################################################

    algo, num_iter = get_algo(F, G, K, normK, tau, sigma, gamma, use_axpby, prob, outp_file,image)

    ###########################################################################
    # Create save call back function
    ###########################################################################

    save_callback = get_save_callback_function(outp_file, num_iter)

    ###########################################################################
    # Run the reconstruction
    ###########################################################################

    while True:
        algo.max_iteration += num_iter
        algo.run(num_iter, verbose=True, very_verbose=True,
             callback=save_callback)
        display_results(algo.get_output().as_array())
        # stop = input("Shall we stop? [y/n]")
        stop = 'y'
        print (stop)
        if stop == 'y':
            print ("breaking")
            break
    # Display results
    display_results(algo.get_output().as_array(), slice_num=54)




def get_filenames(trans, sino, attn, rand):
    """Get filenames."""
    trans_pattern = str(trans).replace('%', '*')
    sino_pattern = str(sino).replace('%', '*')
    attn_pattern = str(attn).replace('%', '*')
    rand_pattern = str(rand).replace('%', '*')    
    if sino_pattern is None:
        raise AssertionError("--sino missing")
    trans_files = sorted(glob(trans_pattern))
    sino_files = sorted(glob(sino_pattern))
    attn_files = sorted(glob(attn_pattern))
    rand_files = sorted(glob(rand_pattern))

    num_ms = len(sino_files)
    # Check some sinograms found
    if num_ms == 0:
        raise AssertionError("No sinograms found at {}!".format(sino_pattern))
    # Should have as many trans as sinos
    if len(trans_files) > 0 and num_ms != len(trans_files):
        raise AssertionError("#trans should match #sinos. "
                             "#sinos = " + str(num_ms) +
                             ", #trans = " + str(len(trans_files)))
    # If any rand, check num == num_ms
    if len(rand_files) > 0 and len(rand_files) != num_ms:
        raise AssertionError("#rand should match #sinos. "
                             "#sinos = " + str(num_ms) +
                             ", #rand = " + str(len(rand_files)))

    # For attn, there should be 0, 1 or num_ms images
    if len(attn_files) > 1 and len(attn_files) != num_ms:
        raise AssertionError("#attn should be 0, 1 or #sinos")

    return [num_ms, trans_files, sino_files, attn_files, rand_files]



def read_files(trans_files, sino_files, attn_files, rand_files, trans_type):
    """Read files."""
    if not trans_files:
        trans = None
    else:
        if trans_type == "tm":
            trans = [reg.AffineTransformation(file) for file in trans_files]
        elif trans_type == "disp":
            trans = [reg.NiftiImageData3DDisplacement(file)
                     for file in trans_files]
        elif trans_type == "def":
            trans = [reg.NiftiImageData3DDeformation(file)
                     for file in trans_files]
        else:
            raise error("Unknown transformation type")

    sinos_raw = [pet.AcquisitionData(file) for file in sino_files]
    attns = [pet.ImageData(file) for file in attn_files]
    
    # fix a problem with the header which doesn't allow
    # to do algebra with randoms and sinogram
    rands_arr = [pet.AcquisitionData(file).as_array() for file in rand_files]
    rands_raw = [ s * 0 for s in sinos_raw ]
    for r,a in zip(rands_raw, rands_arr):
        r.fill(a)
    
    return [trans, sinos_raw, attns, rands_raw]




def pre_process_sinos(sinos_raw, num_ms):
    """Preprocess raw sinograms.

    Make positive if necessary and do any required rebinning."""
    # If empty (e.g., no randoms), return
    if not sinos_raw:
        return sinos_raw
    # Loop over all sinograms
    sinos = [0]*num_ms
    for ind in range(num_ms):
        # If any sinograms contain negative values
        # (shouldn't be the case), set them to 0
        sino_arr = sinos_raw[ind].as_array()
        if (sino_arr < 0).any():
            print("Input sinogram " + str(ind) +
                  " contains -ve elements. Setting to 0...")
            sinos[ind] = sinos_raw[ind].clone()
            sino_arr[sino_arr < 0] = 0
            sinos[ind].fill(sino_arr)
        else:
            sinos[ind] = sinos_raw[ind]
        # If rebinning is desired
        segs_to_combine = 1
        if args['--numSegsToCombine']:
            segs_to_combine = int(args['--numSegsToCombine'])
        views_to_combine = 1
        if args['--numViewsToCombine']:
            views_to_combine = int(args['--numViewsToCombine'])
        if segs_to_combine * views_to_combine > 1:
            sinos[ind] = sinos[ind].rebin(segs_to_combine, views_to_combine)
            # only print first time
            if ind == 0:
                print("Rebinned sino dimensions: {sinos[ind].dimensions()}")

    return sinos



def get_initial_estimate(sinos, use_gpu):
    """Get initial estimate."""

    # from the arguments
    initial_estimate = args['--initial']
    nxny = int(args['--nxny'])
    

    if initial_estimate:
        image = pet.ImageData(initial_estimate)
    elif args['--templateAcqData']:
        image = sinos[0].create_uniform_image(0., (127, 220, 220))
        image.initialise(dim=(127, 220, 220), vsize=(2.03125, 1.7080754, 1.7080754))
    else:
        # Create image based on ProjData
        image = sinos[0].create_uniform_image(0.0, (nxny, nxny))
        # If using GPU, need to make sure that image is right size.
        if use_gpu:
            dim = (127, 320, 320)
            spacing = (2.03125, 2.08626, 2.08626)
        # elif non-default spacing desired
        elif args['--dxdy']:
            dim = image.dimensions()
            dxdy = float(args['--dxdy'])
            spacing = (image.voxel_sizes()[0], dxdy, dxdy)
        if use_gpu or args['--dxdy']:
            image.initialise(dim=dim,
                             vsize=spacing)
            image.fill(0.0)

    return image


def get_resampler(image, ref=None, trans=None):
    """Return a NiftyResample object for the specified transform and image."""
    if ref is None:
        ref = image
    resampler = reg.NiftyResample()
    resampler.set_reference_image(ref)
    resampler.set_floating_image(image)
    resampler.set_padding_value(0)
    resampler.set_interpolation_type_to_linear()
    if trans is not None:
        resampler.add_transformation(trans)
    return resampler


def resample_attn_images(num_ms, attns, trans, use_gpu, image):
    """Resample attenuation images if necessary."""
    resampled_attns = None
    if trans is None:
        resampled_attns = attns
    else:
        if len(attns) > 0:
            resampled_attns = [0]*num_ms
            # if using GPU, dimensions of attn and recon images have to match
            ref = image if use_gpu else None
            for i in range(num_ms):
                # if we only have 1 attn image, then we need to resample into
                # space of each gate. However, if we have num_ms attn images,
                # then assume they are already in the correct position, so use
                # None as transformation.
                tran = trans[i] if len(attns) == 1 else None
                # If only 1 attn image, then resample that. If we have num_ms
                # attn images, then use each attn image of each frame.
                attn = attns[0] if len(attns) == 1 else attns[i]
                resam = get_resampler(attn, ref=ref, trans=tran)
                resampled_attns[i] = resam.forward(attn)
    return resampled_attns

def set_up_acq_models(num_ms, sinos, rands, resampled_attns, image, use_gpu):
    """Set up acquisition models."""
    print("Setting up acquisition models...")

    # From the arguments
    algo = str(args['--algorithm'])
    nsub = int(args['--numSubsets']) if args['--numSubsets'] and algo=='spdhg' else 1
    norm_file = args['--norm']
    verbosity = int(args['--verbosity'])
  

    if not use_gpu:
        acq_models = [pet.AcquisitionModelUsingRayTracingMatrix() for k in range(nsub * num_ms)]
    else:
        acq_models = [pet.AcquisitionModelUsingNiftyPET() for k in range(nsub * num_ms)]
        for acq_model in acq_models:
            acq_model.set_use_truncation(True)
            acq_model.set_cuda_verbosity(verbosity)

    # create masks if nsub >1
    if nsub>1:
        im_one = image.clone().allocate(1.)
        masks = []
    else:
        masks = None


    # If present, create ASM from ECAT8 normalisation data
    asm_norm = None
    if norm_file:
        if not path.isfile(norm_file):
            raise error("Norm file not found: " + norm_file)
        asm_norm = pet.AcquisitionSensitivityModel(norm_file)

    # Loop over each motion state
    for ind in range(num_ms):
        # Create attn ASM if necessary
        asm_attn = None
        if resampled_attns:
            s = sinos[ind]
            ra = resampled_attns[ind]
            am = pet.AcquisitionModelUsingRayTracingMatrix()
            asm_attn = get_asm_attn(s,ra,am)

        # Get ASM dependent on attn and/or norm
        asm = None
        if asm_norm and asm_attn:
            if ind == 0:
                print("ASM contains norm and attenuation...")
            asm = pet.AcquisitionSensitivityModel(asm_norm, asm_attn)
        elif asm_norm:
            if ind == 0:
                print("ASM contains norm...")
            asm = asm_norm
        elif asm_attn:
            if ind == 0:
                print("ASM contains attenuation...")
            asm = asm_attn
                
        # Loop over physical subsets
        for k in range(nsub):
            current = k * num_ms + ind

            if asm:
                acq_models[current].set_acquisition_sensitivity(asm)
            #KT we'll set the background in the KL function below
            #KTif len(rands) > 0:
            #KT    acq_models[ind].set_background_term(rands[ind])

            # Set up
            acq_models[current].set_up(sinos[ind], image)    
            acq_models[current].num_subsets = nsub
            acq_models[current].subset_num = k 

            # compute masks if needed
            if nsub>1 and ind==0:
                mask = acq_models[current].direct(im_one)
                masks.append(mask)

    return acq_models, masks

def get_asm_attn(sino, attn, acq_model):
    """Get attn ASM from sino, attn image and acq model."""
    asm_attn = pet.AcquisitionSensitivityModel(attn, acq_model)
    # temporary fix pending attenuation offset fix in STIR:
    # converting attenuation into 'bin efficiency'
    asm_attn.set_up(sino)
    bin_eff = pet.AcquisitionData(sino)
    bin_eff.fill(1.0)
    asm_attn.unnormalise(bin_eff)
    asm_attn = pet.AcquisitionSensitivityModel(bin_eff)
    return asm_attn


def set_up_reconstructor(use_gpu, num_ms, acq_models, resamplers, masks, sinos, rands=None):
    """Set up reconstructor."""

    # From the arguments
    algo = str(args['--algorithm'])
    regularizer = str(args['--reg'])
    r_iters = int(args['--reg_iters'])
    r_alpha = float(args['--alpha'])
    nsub = int(args['--numSubsets']) if args['--numSubsets'] and algo=='spdhg' else 1
    precond = True if args['--precond'] else False
    param_path = str(args['--param_path'])
    normalise = True if args['--normaliseDataAndBlock'] else False
    gamma = float(args['--gamma'])
    

    if not os.path.exists(param_path):
        os.mkdir(param_path)

    if normalise:
        raise error('options {} and regularization={} are not yet implemented together'.format(normalise, regularizer))

    # We'll need an additive term (eta). If randoms are present, use them
    # Else, use a scaled down version of the sinogram
    etas = rands if rands else [sino * 0 + 1e-5 for sino in sinos]

    # Create composition operators containing linear
    # acquisition models and resamplers,
    # and create data fit functions
    
    if nsub == 1:
        if resamplers is None:
            #KT C = [am.get_linear_acquisition_model() for am in acq_models]
            C = [am for am in acq_models]
        else:
            C = [CompositionOperator(
                    #KTam.get_linear_acquisition_model(),
                    am,
                    res, preallocate=True)
                    for am, res in zip(*(acq_models, resamplers))]
        fi = [KullbackLeibler(b=sino, eta=eta) for sino, eta in zip(sinos, etas)]
    else:
        C = [am for am in acq_models]
        fi = [None] * (num_ms * nsub)
        for (k,i) in np.ndindex((nsub,num_ms)):
            # resample if needed
            if resamplers is not None:            
                C[k * num_ms + i] = CompositionOperator(
                    #KTam.get_linear_acquisition_model(),
                    C[k * num_ms + i],
                    res[i], preallocate=True)
            fi[k * num_ms + i] = KullbackLeibler(b=sinos[i], eta=etas[i], mask=masks[k].as_array(),use_numba=True)


    if regularizer == "FGP_TV":
        r_tolerance = 1e-7
        r_iso = 0
        r_nonneg = 1
        r_printing = 0
        device = 'gpu' if use_gpu else 'cpu'
        G = FGP_TV(r_alpha, r_iters, r_tolerance,
                   r_iso, r_nonneg, r_printing, device)
        if precond:
            FGP_TV.proximal = precond_proximal
    elif regularizer == "None":
        G = IndicatorBox(lower=0)
    else:
        raise error("Unknown regularisation")
        
    
    F = BlockFunction(*fi)
    K = BlockOperator(*C)

    if not precond:
        if algo == 'pdhg':
            # we want the norm of the whole physical BlockOp
            normK = get_proj_norm(BlockOperator(*C),param_path)
            sigma = gamma/normK
            tau = 1/(normK*gamma)
        elif algo == 'spdhg':
            # we want the norm of each component
            normK = get_proj_normi(BlockOperator(*C),nsub,param_path)
            # we'll let spdhg do its default implementation
            sigma = None
            tau = None
        use_axpby = True
    else:
        normK=None
        # this from Vaggelis notebook
        tmp_tau = K.adjoint(K.range_geometry().allocate(1.)).power(-1)
        tmp_tau_np = tmp_tau.as_array()
        tmp_tau_np[tmp_tau_np==np.inf]=1e-5
        if not resamplers is None:
            # get the min of the backward proj without registration
            am = acq_models[0]
            xa = am.adjoint(am.range_geometry().allocate(1.)).as_array()
            vmin = num_ms * np.min(xa[xa>0])
            # replace everything above 1/vmin
            tmp_tau_np[tmp_tau_np > (1. / vmin)] = 1. / vmin
        tau = tmp_tau*0.
        tau.fill(tmp_tau_np)
        tmp_sigma = [Ki.direct(Ki.domain_geometry().allocate(1.)).power(-1) for Ki in K]
        sigma = [tmp_sigmai*0. for tmp_sigmai in tmp_sigma]
        for (tmp_sigmai, sigmai) in zip(tmp_sigma, sigma):
            tmp_sigma_np = tmp_sigmai.as_array()
            tmp_sigma_np[tmp_sigma_np==np.inf]=1e-5
            sigmai.fill(tmp_sigma_np)
        sigma = BlockDataContainer(*sigma)
        # trade-off parameter
        sigma *= gamma
        tau *= (1/gamma)
        use_axpby = False

    if algo == 'spdhg':
        prob = [1./ len(K)] * len(K)
    else:
        prob = None

    return [F, G, K, normK, tau, sigma, use_axpby, prob, gamma]

def set_up_explicit_reconstructor(use_gpu, num_ms, image, acq_models, resamplers, masks, sinos, rands=None):
    """Set up reconstructor."""

    # From the arguments
    algo = str(args['--algorithm'])
    r_alpha = float(args['--alpha'])
    nsub = int(args['--numSubsets']) if args['--numSubsets'] and algo=='spdhg' else 1
    precond = True if args['--precond'] else False
    param_path = str(args['--param_path'])
    normalise = True if args['--normaliseDataAndBlock'] else False
    gamma = float(args['--gamma'])

    if precond:
        raise error('Options precond and explicit TV are not yet implemented together')

    # We'll need an additive term (eta). If randoms are present, use them
    # Else, use a scaled down version of the sinogram
    etas = rands if rands else [sino * 0 + 1e-5 for sino in sinos]

    # Create composition operators containing linear
    # acquisition models and resamplers,
    # and create data fit functions

    if nsub == 1:
        if resamplers is None:
            #KT C = [am.get_linear_acquisition_model() for am in acq_models]
            C = [am for am in acq_models]
        else:
            C = [CompositionOperator(
                    #KTam.get_linear_acquisition_model(),
                    am,
                    res, preallocate=True)
                    for am, res in zip(*(acq_models, resamplers))]
        fi = [KullbackLeibler(b=sino, eta=eta) for sino, eta in zip(sinos, etas)]
    else:
        C = [am for am in acq_models]
        fi = [None] * num_ms * nsub
        for (k,i) in np.ndindex((nsub,num_ms)):
            # resample if needed
            if resamplers is not None:            
                C[k * num_ms + i] = CompositionOperator(
                    #KTam.get_linear_acquisition_model(),
                    C[k * num_ms + i],
                    res[i], preallocate=True)
            fi[k * num_ms + i] = KullbackLeibler(b=sino[i], eta=eta[i], mask=masks[k].as_array(),use_numba=True)

    # define gradient
    Grad = Gradient(image, backend='c', correlation='SpaceChannel')
    normGrad = get_grad_norm(Grad,param_path)

    # define data fit
    data_fit = MixedL21Norm()
    MixedL21Norm.proximal = MixedL21Norm_proximal

    if algo == 'pdhg':
        # we want the norm of the whole physical BlockOp
        normProj = get_proj_norm(BlockOperator(*C),param_path)
        if normalise:
            C_rs = [ScaledOperator(Ci,1/normProj) for Ci in C]
            Grad_rs = ScaledOperator(Grad,1/normGrad)
            C_rs.append(Grad_rs)
            f_rs = [ScaledFunction(KullbackLeibler(b=1/normProj*sino, eta=1/normProj*eta),normProj) 
                    for sino, eta in zip(sinos, etas)]
            f_rs.append(ScaledFunction(data_fit,r_alpha * normGrad))
            normK = np.sqrt(2)
        else:
            C.append(Grad)
            f = [KullbackLeibler(b=sino, eta=eta) for sino, eta in zip(sinos, etas)]
            f.append(ScaledFunction(data_fit,r_alpha))
            normK = np.sqrt(normProj**2 + normGrad**2)
        sigma = gamma/normK
        tau = 1/(normK*gamma)
        prob = None
            
    elif algo == 'spdhg':
        # we want the norm of each component
        normProj = get_proj_normi(BlockOperator(*C),nsub,param_path)
        if normalise:
            C_rs = [ScaledOperator(Ci,1/normProji) for Ci, normProji in zip(C,normProj)]
            Grad_rs = ScaledOperator(Grad,1/normGrad)
            C_rs.append(Grad_rs)
            f_rs = [ScaledFunction(KullbackLeibler(b=1/normProji*sino, eta=1/normProji*eta),normProji) 
                    for sino, eta, normProji in zip(sinos, etas, normProj)]
            f_rs.append(ScaledFunction(data_fit,r_alpha * normGrad))
            normK = [1.] * len(C_rs)
            prob = [1./(2 * (len(C_rs)-1))] * (len(C_rs)-1) + [1./2]
        else:
            C.append(Grad)
            f = [KullbackLeibler(b=sino, eta=eta) for sino, eta in zip(sinos, etas)]
            f.append(ScaledFunction(data_fit,r_alpha))
            normK = normProj.append(normGrad)
            prob = [1./(2 * (len(C)-1))] * (len(C)-1) + [1./2]
        # we'll let spdhg do its default stepsize implementation
        sigma = None
        tau = None        
    else:
        raise error("algorithm '{}' is not implemented".format(algo))

    G = IndicatorBox(lower=0)

    if normalise:
        F = BlockFunction(*f_rs)
        K = BlockOperator(*C_rs)
    else:
        F = BlockFunction(*f)
        K = BlockOperator(*C)
    use_axpby = True

    return [F, G, K, normK, tau, sigma, use_axpby, prob, gamma]


def PowerMethod(operator, x_init=None):
    '''Power method to calculate iteratively the Lipschitz constant
    
    :param operator: input operator
    :type operator: :code:`LinearOperator`
    :param iterations: number of iterations to run
    :type iteration: int
    :param x_init: starting point for the iteration in the operator domain
    :returns: tuple with: L, list of L at each iteration, the data the iteration worked on.
    '''
    # From the arguments
    iterations = int(args['--PowerMethod_iters'])

    # Initialise random
    if x_init is None:
        x0 = operator.domain_geometry().allocate('random')
    else:
        x0 = x_init.copy()
        
    x1 = operator.domain_geometry().allocate()
    y_tmp = operator.range_geometry().allocate()
    s = []
    # Loop
    i = 0
    while i < iterations:
        operator.direct(x0,out=y_tmp)
        operator.adjoint(y_tmp,out=x1)
        x1norm = x1.norm()
        if hasattr(x0, 'squared_norm'):
            s.append( x1.dot(x0) / x0.squared_norm() )
        else:
            x0norm = x0.norm()
            s.append( x1.dot(x0) / (x0norm * x0norm) ) 
        x1.multiply((1.0/x1norm), out=x0)
        print ("current squared norm: {}".format(s[-1]))
        i += 1
    return np.sqrt(s[-1]), [np.sqrt(si) for si in s], x0

def precond_proximal(self, x, tau, out=None):

    """Modify proximal method to work with preconditioned tau"""
    pars = {'algorithm': FGP_TV,
            'input': np.asarray(x.as_array()/tau.as_array(),
                                dtype=np.float32),
            'regularization_parameter': self.lambdaReg,
            'number_of_iterations': self.iterationsTV,
            'tolerance_constant': self.tolerance,
            'methodTV': self.methodTV,
            'nonneg': self.nonnegativity,
            'printingOut': self.printing}

    res = regularisers.FGP_TV(pars['input'],
                                    pars['regularization_parameter'],
                                    pars['number_of_iterations'],
                                    pars['tolerance_constant'],
                                    pars['methodTV'],
                                    pars['nonneg'],
                                    self.device)[0]
    if out is not None:
        out.fill(res)
    else:
        out = x.copy()
        out.fill(res)
    out *= tau
    return out 
    
def MixedL21Norm_proximal(self, x, tau, out):
        
    r"""Returns the value of the proximal operator of the MixedL21Norm function at x.

    .. math :: \mathrm{prox}_{\tau F}(x) = \frac{x}{\|x\|_{2}}\max\{ \|x\|_{2} - \tau, 0 \}

    where the convention 0 · (0/0) = 0 is used.

    """
    # Note: we divide x/tau so the cases of both scalar and 
    # datacontainers of tau to be able to run

    tmp = (x/tau).pnorm(2)
    tmp_ig = 0.0 * tmp
    (tmp - 1).maximum(0.0, out = tmp_ig)
    tmp_ig.multiply(x, out = out)
    # out = out / tmp 
    # with 0/0 = 0
    tmp_array = tmp.as_array()
    for outi in out:
        outi_array = outi.as_array()
        np.divide(outi_array, tmp_array, out=outi_array, where=(outi_array!=0))
        outi.fill(outi_array)

def get_proj_norm(K,param_path):
    # load or compute and save norm of whole operator
    file_path = '{}/normK.npy'.format(param_path)
    if os.path.isfile(file_path):
        print('Norm file {} exists, load it'.format(file_path))
        normK = float(np.load(file_path, allow_pickle=True))
    else:
        print('Norm file {} does not exist, compute it'.format(file_path))
        normK = PowerMethod(K)[0]
        # save to file
        np.save(file_path, normK, allow_pickle=True)
    return normK

def get_proj_normi(K,nsub,param_path):
    # load or compute and save norm of each sub-operator
    # (over motion states and subsets)
    file_path = '{}/normK_nsub{}.npy'.format(param_path, nsub)
    if os.path.isfile(file_path):
        print('Norm file {} exists, load it'.format(file_path))
        normK = np.load(file_path, allow_pickle=True).tolist()
    else: 
        print('Norm file {} does not exist, compute it'.format(file_path))
        normK = [PowerMethod(Ki)[0] for Ki in K]
        # save to file
        np.save(file_path, normK, allow_pickle=True)
    return normK

def get_grad_norm(Grad,param_path):
    file_path = '{}/normGrad.npy'.format(param_path)
    if os.path.isfile(file_path):
        print('Norm file {} exists, load it'.format(file_path))
        normG = np.load(file_path, allow_pickle=True)
    else:
        print('Norm file {} does not exist, compute it'.format(file_path))
        normG = PowerMethod(Grad)[0]
        # save to file
        np.save(file_path, normG, allow_pickle=True)
    return normG


def get_output_filename(attn_files, normK, sigma, tau, sino_files, resamplers, use_gpu):
    """Get output filename."""

    # From the arguments
    outp_file = args['--outp']
    descriptive_fname = True if args['--descriptive_fname'] else False
    norm_file = args['--norm']
    includesRand = True if args['--rand'] else False
    algorithm = str(args['--algorithm'])
    precond = True if args['--precond'] else False
    regularisation = args['--reg']
    r_iters = int(args['--reg_iters'])
    r_alpha = float(args['--alpha'])
    gamma = float(args['--gamma'])
    nsub = int(args['--numSubsets']) if args['--numSubsets'] and algorithm=='spdhg' else 1
    normalise = True if args['--normaliseDataAndBlock'] else False

    if descriptive_fname:
        outp_file += "_Reg-" + regularisation
        if regularisation is not None:
            outp_file += "-alpha" + str(r_alpha)
        outp_file += "_nGates" + str(len(sino_files))
        outp_file += "_nSubsets" + str(nsub)
        outp_file += '_' + algorithm
        if not precond:
            outp_file += "_noPrecond"
        else:
            outp_file += "_wPrecond"
        outp_file += "_gamma" + str(gamma)
        if normalise:
            outp_file += "normalise"
        if len(attn_files) > 0:
            outp_file += "_wAC"
        if norm_file:
            outp_file += "_wNorm"
        if includesRand:
            outp_file += "_wRands"
        if use_gpu:
            outp_file += "_wGPU"
        if regularisation == 'FGP_TV':          
            outp_file += "-riters" + str(r_iters)
        if resamplers is None:
            outp_file += "_noMotion"
    return outp_file

def get_algo(F, G, K, normK, tau, sigma, gamma, use_axpby, prob, outp_file,init_image):

    # from the arguments:
    algorithm = str(args['--algorithm'])
    num_epoch = int(args['--epoch'])
    update_obj_fn_interval = int(args['--update_obj_fn_interval'])
    regularisation = args['--reg']

    """Get the reconstruction algorithm."""
    if algorithm == 'pdhg':
        num_iter = num_epoch
        algo = PDHG(
                f=F,
                g=G,
                operator=K,
                tau=tau,
                sigma=sigma, 
                x_init=init_image,
                use_axpby=use_axpby,
                max_iteration=num_epoch,           
                update_objective_interval=update_obj_fn_interval,
                log_file=outp_file+".log",
                )
    elif algorithm == 'spdhg':
        if regularisation == 'explicit_TV':
            num_iter = 2 * (len(K)-1) * num_epoch
        else:
            num_iter = len(K) * num_epoch
        algo = SPDHG(            
                f=F, 
                g=G, 
                operator=K,
                tau=tau,
                sigma=sigma,
                gamma=gamma,
                x_init=init_image,
                prob=prob,
                use_axpby=use_axpby,
                norms=normK,
                max_iteration=num_iter,         
                update_objective_interval=update_obj_fn_interval,
                log_file=outp_file+".log",
                )
    else:
        raise error("Unknown algorithm: " + algorithm)
    return algo, num_iter

def get_save_callback_function(outp_file, num_iter):
    """Get the save callback function."""

    # from the arguments
    save_interval = int(args['--save_interval'])
    nifti = True if args['--nifti'] else False
    outpath = str(args['--outpath'])


    if not os.path.exists(outpath):
        os.mkdir(outpath)
    save_interval = min(save_interval, num_iter)

    def save_callback(save_interval, nifti, outpath, outp_file,
                      num_iter, iteration,
                      last_objective, x):
        """Save callback function."""
        completed_iterations = iteration + 1
        if completed_iterations % save_interval == 0 or \
                completed_iterations == num_iter:
            if not nifti:
                x.write("{}/{}_iters_{}".format(outpath,outp_file, completed_iterations))
            else:
                reg.NiftiImageData(x).write(
                    "{}/{}_iters_{}".format(outpath,outp_file, completed_iterations))

    psave_callback = partial(
        save_callback, save_interval, nifti, outpath, outp_file, num_iter)
    return psave_callback

def display_results(out_arr, slice_num=None):
    """Display results if desired."""

    # from the arguments
    visualisations = True if args['--visualisations'] else False
    if visualisations:
        # show reconstructed image
        # out_arr = algo.get_output().as_array()
        if slice_num is None:
            z = out_arr.shape[0]//2
        else:
            z = slice_num
        show_2D_array('Reconstructed image', out_arr[z, :, :])
        plt.show()

    
def get_domain_sirf2cil(domain_sirf):
    
    return ImageGeometry(
            voxel_num_x = domain_sirf.shape[2], 
            voxel_num_y = domain_sirf.shape[1], 
            voxel_num_z = domain_sirf.shape[0],
            voxel_size_x = domain_sirf.voxel_sizes()[2], 
            voxel_size_y = domain_sirf.voxel_sizes()[1],
            voxel_size_z = domain_sirf.voxel_sizes()[0])


if __name__ == "__main__":
    main()