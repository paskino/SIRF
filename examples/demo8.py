import math
import pylab
import sys
import time

sys.path.append('../../build/xGadgetron')
sys.path.append('../pGadgetron')

from pGadgetron import *
from pGadgets import *

try:
    # acquisitions will be read from this HDF file
    file = str(input('raw data file: '))
    input_data = ISMRMRDAcquisitions(file)

    print('---\n acquisition data norm: %e' % input_data.norm())

    processed_data = MR_remove_x_oversampling(input_data)

    print('---\n processed acquisition data norm: %e' % processed_data.norm())

    # perform reconstruction
    recon = SimpleReconstructionProcessor()
    recon.set_input(processed_data)
    recon.process()
    complex_images = recon.get_output()

    print('---\n reconstructed images norm: %e' % complex_images.norm())

    csms = MRCoilSensitivityMaps()

##    csm_file = str(input('csm file: '))
##    print('reading sensitivity maps...')
##    csms.read(csm_file)

    print('---\n ordering acquisitions...')
    input_data.order()
    print('---\n computing sensitivity maps...')
    csms.compute(input_data)

    # create acquisition model based on the acquisition parameters
    # stored in input_data and image parameters stored in interim_images
    am = AcquisitionModel(input_data, complex_images)

    am.set_coil_sensitivity_maps(csms)

    # use the acquisition model (forward projection) to produce 'acquisitions'
    acqs = am.forward(complex_images)

    print('---\n reconstructed images forward projection norm %e' % acqs.norm())

    # compute the difference between real and modelled acquisitions:
    #   diff = acqs - P acqs,
    # where P is the orthogonal projector onto input_data
    a = -acqs.dot(input_data) / input_data.dot(input_data)
    b = 1.0
    diff = AcquisitionsContainer.axpby(a, input_data, b, acqs)
    rr = diff.norm()/acqs.norm()
    print('---\n reconstruction residual norm (rel): %e' % rr)

    # apply the adjoint model (backward projection)
    imgs = am.backward(diff)

    print('---\n its backward projection norm: %e' % imgs.norm())

    # test that the backward projection is the adjoint of forward
    # on x = diff and y = interim_images
    # (note that x = (1 - P)F y, so the result must be numerically real)
    xFy = diff.dot(acqs)
    print('---\n (x, F y) = (%e, %e)' % (xFy.real, xFy.imag))
    Bxy = imgs.dot(complex_images)
    print('= (B x, y) = (%e, %e)' % (Bxy.real, Bxy.imag))

    # test images norm
    s = imgs.norm()
    ss = imgs.dot(imgs)
    print('---\n (B x, B x) = (%e, %e) = %e' % (ss.real, ss.imag, s*s))

    # test linear combination of images
    a = -1.0
    im_diff = ImagesContainer.axpby(a, imgs, b, imgs)
    print('---\n 0.0 = %e' % im_diff.norm())

    # extract real images from complex
    images = MR_extract_real_images(complex_images)

    # plot obtained images
    for i in range(images.number()):
        data = images.image_as_array(i)
        pylab.figure(i + 1)
        pylab.imshow(data[0,0,:,:])
        pylab.show()

except error as err:
    # display error information
    print ('Gadgetron exception occured:\n', err.value)
