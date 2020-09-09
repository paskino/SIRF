#include "sirf/Gadgetron/encoding.h"

#include <sstream>
#include <math.h>

#include <cassert>
#define assertm(exp, msg) assert(((void)msg, exp)

#include "sirf/iUtilities/LocalisedException.h"

using namespace sirf;
using namespace ISMRMRD;

#define SIRF_GOLDEN_ANGLE M_PI*(3-sqrt(5))

void sirf::aTrajectoryPreparation::update_acquisitions_info(MRAcquisitionData& mr_acq)
{

    IsmrmrdHeader hdr = mr_acq.acquisitions_info().get_IsmrmrdHeader();

    if(hdr.encoding.size() != 1)
        throw LocalisedException("Currrently only files with one encoding are supported", __FILE__, __LINE__);

    hdr.encoding[0].trajectory = this->traj_type_;

    this->kspace_encoding_ = hdr.encoding[0];

    std::stringstream hdr_stream;
    serialize(hdr, hdr_stream);

    AcquisitionsInfo ai(hdr_stream.str());
    mr_acq.set_acquisitions_info(ai);
}

void sirf::CartesianTrajectoryPrep::set_trajectory(MRAcquisitionData& mr_acq)
{
    update_acquisitions_info(mr_acq); // do nothing for cartesian trajectories
}


void sirf::GRPETrajectoryPrep::set_trajectory(MRAcquisitionData& mr_acq)
{
    update_acquisitions_info(mr_acq);

    for(size_t ia=0; ia<mr_acq.number(); ++ia)
    {
        Acquisition acq;
        mr_acq.get_acquisition(ia, acq);
        this->set_acquisition_trajectory(acq);
        mr_acq.set_acquisition(ia, acq);
    }
}

void sirf::GRPETrajectoryPrep::set_acquisition_trajectory(Acquisition& acq)
{
    acq.resize(acq.number_of_samples(),acq.active_channels(), this->traj_dim_);
    std::vector<float> acq_traj = this->calculate_trajectory(acq);
    acq.setTraj(&acq_traj[0]);
}

std::vector<float> sirf::GRPETrajectoryPrep::calculate_trajectory(Acquisition& acq)
{
    ISMRMRD::Limit rad_lims(0,0,0), ang_lims(0,0,0);
    if(this->kspace_encoding_.encodingLimits.kspace_encoding_step_1.is_present())
        rad_lims = this->kspace_encoding_.encodingLimits.kspace_encoding_step_1.get();
    if(this->kspace_encoding_.encodingLimits.kspace_encoding_step_2.is_present())
        ang_lims = this->kspace_encoding_.encodingLimits.kspace_encoding_step_2.get();

    const ISMRMRD::EncodingCounters idx = acq.idx();

    float const pe_angle = SIRF_GOLDEN_ANGLE * idx.kspace_encode_step_2;

    size_t const num_diff_shifts = this->rad_shift_.size();
    float rad_shift = float( this->rad_shift_.at(this->circ_mod(idx.kspace_encode_step_2 - ang_lims.center,num_diff_shifts))) / float(num_diff_shifts);

    float pe_radius = idx.kspace_encode_step_1 - rad_lims.center;
    pe_radius = (pe_radius==0) ? pe_radius : pe_radius+rad_shift;

    float const traj_norm = 2*std::max<float>(( rad_lims.center - rad_lims.minimum + 0), (rad_lims.maximum - rad_lims.center + (num_diff_shifts-1)/num_diff_shifts));
    pe_radius /= traj_norm;

    std::vector<float> traj;

    for(size_t i_sample=0; i_sample<acq.number_of_samples();++i_sample)
    {
        traj.push_back(0); //dummy for RPE as the readout is cartesian
        traj.push_back(pe_radius * cos( pe_angle ));
        traj.push_back(pe_radius * sin( pe_angle ));
    }

    return traj;
}

void sirf::FourierEncoding::match_img_header_to_acquisition(CFImage& img, const ISMRMRD::Acquisition& acq)
{

    auto acq_hdr = acq.getHead();
    auto idx = acq_hdr.idx;

    img.setAverage(idx.average);
    img.setSlice(idx.slice);
    img.setContrast(idx.contrast);
    img.setPhase(idx.phase);
    img.setRepetition(idx.repetition);
    img.setSet(idx.set);

}

void sirf::CartesianFourierEncoding::forward(MRAcquisitionData& ac, const CFImage* ptr_img)
{

    std::string par;
    ISMRMRD::IsmrmrdHeader header;
    par = ac.acquisitions_info();
    ISMRMRD::deserialize(par.c_str(), header);

    if( header.encoding.size() > 1)
        LocalisedException("Currently only one encoding is supported per rawdata file.", __FUNCTION__, __LINE__);

    ISMRMRD::Encoding e = header.encoding[0];

    ISMRMRD::Acquisition acq;
    ac.get_acquisition(0, acq);


    unsigned int readout = acq.number_of_samples();
    unsigned int ny_k_space = e.encodedSpace.matrixSize.y;
    unsigned int nz_k_space = e.encodedSpace.matrixSize.z;

    ISMRMRD::Limit ky_lim, kz_lim(0,0,0);

    ky_lim = e.encodingLimits.kspace_encoding_step_1.get();
    if(e.encodingLimits.kspace_encoding_step_2.is_present())
        kz_lim = e.encodingLimits.kspace_encoding_step_2.get();


    CFImage img = *ptr_img;

    unsigned int nx = img.getMatrixSizeX();
    unsigned int ny = img.getMatrixSizeY();
    unsigned int nz = img.getMatrixSizeZ();
    unsigned int nc = img.getNumberOfChannels();

    if(nx != readout || ny_k_space!= ny || nz_k_space != nz || nc != acq.active_channels())
        throw LocalisedException("K-space dimensions and image dimensions don't match.",   __FILE__, __LINE__);


    std::vector<size_t> dims;
    dims.push_back(nx);
    dims.push_back(ny);
    dims.push_back(nz);
    dims.push_back(nc);

    ISMRMRD::NDArray<complex_float_t> ci(dims);
    memset(ci.getDataPtr(), 0, ci.getDataSize());

    for (unsigned int c = 0; c < nc; c++) {
        for (unsigned int z = 0; z < nz; z++) {
            for (unsigned int y = 0; y < ny; y++) {
                for (unsigned int x = 0; x < nx; x++) {
                    ci(x, y, z, c) = (complex_float_t)img(x, y, z, c);
                }
            }
        }
    }

    fft3c(ci);

    for(size_t i =0; i<ac.items(); ++i)
    {
        ac.get_acquisition(i, acq);
        acq.resize(nx, nc); // no trajectory information is set

        int ky = ny/2 - ky_lim.center + acq.idx().kspace_encode_step_1;
        int kz = nz/2 - kz_lim.center + acq.idx().kspace_encode_step_2;

        for (unsigned int c = 0; c < nc; c++) {
            for (unsigned int s = 0; s < nx; s++) {
                acq.data(s, c) = ci(s, ky, kz, c);
            }
        }
        ac.set_acquisition(i, acq);
    }
}

void sirf::CartesianFourierEncoding::backward(CFImage* ptr_img, const MRAcquisitionData& ac)
{

    if(ac.items()<1)
        LocalisedException("No acquisitions in vector. Trying to backward transform empty vector.", __FUNCTION__, __LINE__);

    ISMRMRD::IsmrmrdHeader header = ac.acquisitions_info().get_IsmrmrdHeader();

    if( header.encoding.size() > 1)
        LocalisedException("Currently only one encoding is supported per rawdata file.", __FUNCTION__, __LINE__);

    ISMRMRD::Encoding e = header.encoding[0];

    ISMRMRD::Acquisition acq;

    ac.get_acquisition(0, acq);

    unsigned int readout = acq.number_of_samples();
    unsigned int nc = acq.active_channels();

    unsigned int ny = e.encodedSpace.matrixSize.y;
    unsigned int nz = e.encodedSpace.matrixSize.z;

    unsigned int nx_img = e.reconSpace.matrixSize.x;

    if(nx_img != readout)
        throw LocalisedException("Number of readout points and reconstructed image dimension in readout direction are assumed the same.",   __FILE__, __LINE__);

    std::vector<size_t> dims, dims_dcf;

    dims.push_back(readout);
    dims.push_back(ny); dims_dcf.push_back(ny);
    dims.push_back(nz); dims_dcf.push_back(nz);
    dims.push_back(nc);

    ISMRMRD::Limit ky_lim, kz_lim(0,0,0);

    ky_lim = e.encodingLimits.kspace_encoding_step_1.get();
    if(e.encodingLimits.kspace_encoding_step_2.is_present())
        kz_lim = e.encodingLimits.kspace_encoding_step_2.get();

    ISMRMRD::NDArray<complex_float_t> ci(dims);
    ISMRMRD::NDArray<float> dcf(dims_dcf);

    memset(ci.getDataPtr(), 0, ci.getDataSize());
    memset(dcf.getDataPtr(), 0, dcf.getDataSize());

    for (int a=0; a < ac.number(); a++) {
        ac.get_acquisition(a, acq);
        int y = ny/2 - ky_lim.center + acq.idx().kspace_encode_step_1 ;
        int z = nz/2 - kz_lim.center + acq.idx().kspace_encode_step_2;
        dcf(y, z) += (float)1;
        for (unsigned int c = 0; c < nc; c++) {
            for (unsigned int s = 0; s < readout; s++) {
                ci(s, y, z, c) += acq.data(s, c);
            }
        }
    }

    // correct for multi-acquired PE points
    for(unsigned int c=0; c<nc; ++c)
    for(unsigned int z=0; z<nz; ++z)
    for(unsigned int y=0; y<ny; ++y)
    for(unsigned int x=0; x<readout; ++x)
        ci(x,y,z,c) /= (complex_float_t)std::max(1.f, dcf(y,z));


    // now if image and kspace have different dimension then you need to interpolate or pad with zeros here
    ifft3c(ci);

    unsigned int ny_img = e.reconSpace.matrixSize.y;
    unsigned int nz_img = e.reconSpace.matrixSize.z;

    if( ny!=ny_img || nz!=nz_img)
        throw LocalisedException("Phase and slice encoding are not consistent between reconstructed image and k-space.", __FILE__, __LINE__);

    ptr_img->resize(nx_img, ny_img, nz_img, nc);
    memcpy(ptr_img->begin(), ci.begin(), ci.getDataSize());

    // set the header correctly of the image
    this->match_img_header_to_acquisition(*ptr_img, acq);

}



Gadgetron::hoNDArray<SirfTrajectoryType2D> RPEFourierEncoding::get_trajectory(const MRAcquisitionData& ac) const
{
    if(ac.get_trajectory_type() != ISMRMRD::TrajectoryType::OTHER)
        throw std::runtime_error("Please only ask to get the trajectory for acquisition data with an RPE trajectory pre-computed in the acquisitions.");

    if(ac.number() <= 0)
        throw std::runtime_error("Please pass a non-empty container.");

    ISMRMRD::Acquisition acq;
    ac.get_acquisition(0, acq);

    if( acq.trajectory_dimensions() != 3)
        throw std::runtime_error("Please give Acquisition with a 3D RPE trajectory if you want to use it here.");

    std::vector<int> kspace_dims;
    ac.get_acquisition_dimensions(kspace_dims);

    std::vector<size_t> traj_dims{kspace_dims[1], kspace_dims[2]};

    Gadgetron::hoNDArray<SirfTrajectoryType2D> traj(traj_dims);

    for(int ia=0; ia<ac.number(); ++ia)
    {
        ac.get_acquisition(ia, acq);

        size_t ky = acq.idx().kspace_encode_step_1;
        size_t kz = acq.idx().kspace_encode_step_2;

        traj(ky,kz)[0] = acq.traj(1, 0);
        traj(ky,kz)[1] = acq.traj(2, 0);
    }

    return traj;
}

void RPEFourierEncoding::backward(CFImage* ptr_img, const MRAcquisitionData& ac)
{
    ASSERT( ac.get_trajectory_type() != ISMRMRD::TrajectoryType::OTHER, "Give a MRAcquisitionData reference with the trajectory type OTHER.");

    ISMRMRD::IsmrmrdHeader hdr = ac.acquisitions_info().get_IsmrmrdHeader();
    ISMRMRD::Encoding e = hdr.encoding[0];

    std::vector<int> dims;
    ac.get_acquisition_dimensions(dims);

    // set meta data for reconstructed image

    EncodingSpace rec_space = e.reconSpace;

    ptr_img->resize(rec_space.matrixSize.x, rec_space.matrixSize.y, rec_space.matrixSize.z, dims[3]);
    ptr_img->setFieldOfView( rec_space.fieldOfView_mm.x, rec_space.fieldOfView_mm.y ,rec_space.fieldOfView_mm.z );

    ISMRMRD::Acquisition acq;
    ac.get_acquisition(0,acq);

    this->match_img_header_to_acquisition(*ptr_img, acq);

    throw std::runtime_error("The backward Model Is not implemented yet for RPE.");

}

void RPEFourierEncoding::forward(MRAcquisitionData& ac, const CFImage* ptr_img)
{
    throw std::runtime_error("The Forward Model Is not implemented yet for RPE.");
}