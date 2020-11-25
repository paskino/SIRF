import sirf.Reg as reg
import sirf.STIR as pet

resampler = reg.NiftyResample() 
resampler.set_interpolation_type_to_linear()
data_dir = '/home/sirfuser/data/cardiac_resp'
PETtemplate_path = '{}/{}'.format(data_dir,
'pet/recons/ungated_pdhg/ungated_pdhg_Reg-FGP_TV-alpha5.0_nGates1_nSubsets1_pdhg_wPrecond_gamma1.0_wAC_wNorm_wRands-riters100_noMotion_iters_0.nii')
PETtemplate = pet.ImageData(PETtemplate_path)
MRimage_path = '{}/{}'.format(data_dir,
'mr/rec_im_mcir_fista_8ms_tv.nii') 
MRimage = pet.ImageData(MRimage_path)
# Set up PET template: no need to be the real data, just the image size etc you need.
resampler.set_reference_image(PETtemplate) 
resampler.set_floating_image(MRimage)
resampler.process()
resampledMR=resampler.get_output()
reg.NiftiImageData(resampledMR).write(
                "{}/{}".format(data_dir,'/pet/resampled_MR'))