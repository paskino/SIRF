/*
CCP PETMR Synergistic Image Reconstruction Framework (SIRF)
Copyright 2015 - 2017 Rutherford Appleton Laboratory STFC

This is software developed for the Collaborative Computational
Project in Positron Emission Tomography and Magnetic Resonance imaging
(http://www.ccppetmr.ac.uk/).

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/

/*!
\file
\ingroup Gadgetron Data Containers
\brief Implementation file for SIRF data container classes for Gadgetron data.

\author Evgueni Ovtchinnikov
\author CCP PETMR
*/

#include "gadgetron_data_containers.h"

void 
ImageWrap::get_cmplx_data(double* re, double* im) const
{
	int dim[4];
	size_t n = get_dim(dim);
	if (type_ == ISMRMRD::ISMRMRD_CXFLOAT) {
		const CFImage& img = *(const CFImage*)ptr_;
		const complex_float_t* ptr = img.getDataPtr();
		for (size_t i = 0; i < n; i++) {
			complex_float_t z = ptr[i];
			re[i] = std::real(z);
			im[i] = std::imag(z);
		}
	}
	else if (type_ == ISMRMRD::ISMRMRD_CXDOUBLE) {
		const CDImage& img = *(const CDImage*)ptr_;
		const complex_double_t* ptr = img.getDataPtr();
		for (size_t i = 0; i < n; i++) {
			complex_double_t z = ptr[i];
			re[i] = std::real(z);
			im[i] = std::imag(z);
		}
	}
	else {
		get_data(re);
		for (size_t i = 0; i < n; i++)
			im[i] = 0;
	}
}

void 
ImageWrap::set_cmplx_data(const double* re, const double* im) const
{
	int dim[4];
	size_t n = get_dim(dim);
	if (type_ == ISMRMRD::ISMRMRD_CXFLOAT) {
		CFImage& img = *(CFImage*)ptr_;
		complex_float_t* ptr = img.getDataPtr();
		for (size_t i = 0; i < n; i++)
			ptr[i] = std::complex<float>((float)re[i], (float)im[i]);
	}
	else if (type_ == ISMRMRD::ISMRMRD_CXDOUBLE) {
		CDImage& img = *(CDImage*)ptr_;
		complex_double_t* ptr = img.getDataPtr();
		for (size_t i = 0; i < n; i++)
			ptr[i] = std::complex<double>(re[i], im[i]);
	}
}

int 
AcquisitionsContainer::get_acquisitions_dimensions(size_t ptr_dim)
{
	ISMRMRD::Acquisition acq;
	int* dim = (int*)ptr_dim;

	int na = number();
	int ms, ns;
	int mc, nc;
	int my, ny;
	int slice = 0;
	int y = 0;
	// assume all dimensions (samples, coils [, acqs per slice]) regular
	int nrd = ordered() ? 3 : 2;
	// number of regular readouts
	int nrr = 0;
	//int not_reg = 0;
	for (; y < na;) {
		for (; y < na && ordered();) {
			get_acquisition(y, acq);
			if (acq.isFlagSet(ISMRMRD::ISMRMRD_ACQ_FIRST_IN_SLICE))
				break;
			y++;
		}
		if (y >= na)
			break;
		ny = 0;
		for (; y < na; y++) {
			get_acquisition(y, acq);
			if (TO_BE_IGNORED(acq)) // not a regular acquisition
				continue;
			ns = acq.number_of_samples();
			nc = acq.active_channels();
			nrr += ns*nc;
			if (slice == 0) {
				ms = ns;
				mc = nc;
			}
			else {
				if (ms != ns)
					nrd = 0;
				else if (mc != nc && nrd > 1)
					nrd = 1;
			}
			ny++;
			if (acq.isFlagSet(ISMRMRD::ISMRMRD_ACQ_LAST_IN_SLICE) && ordered())
				break;
		}
		if (slice == 0) {
			my = ny;
		}
		else {
			if (my != ny && nrd > 2) {
				nrd = 2;
			}
			//if (my != ny)
			//	not_reg = 1;
		}
		slice++;
	}

	int reg_size = 1;
	if (nrd > 0) {
		dim[0] = ms;
		reg_size *= ms;
	}
	if (nrd > 1) {
		dim[1] = mc;
		reg_size *= mc;
	}
	if (nrd > 2) {
		dim[2] = my;
		reg_size *= my;
	}
	dim[nrd] = nrr / reg_size;
	return nrd;
	//dim[0] = acq.number_of_samples();
	//dim[1] = acq.active_channels();
	//dim[2] = my; // e.reconSpace.matrixSize.y;
	//dim[3] = slice;
	//return not_reg;
}

void 
AcquisitionsContainer::get_acquisitions_flags(unsigned int n, int* flags)
{
	ISMRMRD::Acquisition acq;
	unsigned int na = number();
	for (unsigned int a = 0, i = 0; a < na; a++) {
		get_acquisition(a, acq);
		if (TO_BE_IGNORED(acq) && n < na) {
			std::cout << "ignoring acquisition " << a << '\n';
			continue;
		}
		flags[i++] = (int)acq.flags();
	}
}

unsigned int 
AcquisitionsContainer::get_acquisitions_data(unsigned int slice, double* re, double* im)
{
	ISMRMRD::Acquisition acq;
	unsigned int na = number();
	unsigned int n = 0;
	if (slice >= na) {
		for (unsigned int a = 0, i = 0; a < na; a++) {
			get_acquisition(a, acq);
			if (TO_BE_IGNORED(acq) && slice > na) {
				std::cout << "ignoring acquisition " << a << '\n';
				continue;
			}
			n++;
			unsigned int nc = acq.active_channels();
			unsigned int ns = acq.number_of_samples();
			for (unsigned int c = 0; c < nc; c++) {
				for (unsigned int s = 0; s < ns; s++, i++) {
					complex_float_t z = acq.data(s, c);
					re[i] = std::real(z);
					im[i] = std::imag(z);
				}
			}
		}
		return n;
	}
	int* dim = new int[3];
	size_t ptr_dim = (size_t)dim;
	get_acquisitions_dimensions(ptr_dim);
	unsigned int ny = dim[2]; //e.reconSpace.matrixSize.y;
							  //unsigned int ny = dim[1]; //e.reconSpace.matrixSize.y;
	delete[] dim;
	unsigned int y = 0;
	for (; y + ny*slice < na;) {
		get_acquisition(y + ny*slice, acq);
		if (acq.isFlagSet(ISMRMRD::ISMRMRD_ACQ_FIRST_IN_SLICE))
			break;
		y++;
	}
	for (; y + ny*slice < na; n++) {
		get_acquisition(y + ny*slice, acq);
		unsigned int nc = acq.active_channels();
		unsigned int ns = acq.number_of_samples();
		for (unsigned int c = 0; c < nc; c++) {
			for (unsigned int s = 0; s < ns; s++) {
				complex_float_t z = acq.data(s, c);
				re[s + ns*(n + ny*c)] = std::real(z);
				im[s + ns*(n + ny*c)] = std::imag(z);
			}
		}
		y++;
		if (acq.isFlagSet(ISMRMRD::ISMRMRD_ACQ_LAST_IN_SLICE))
			break;
	}
	return n;
}

void 
AcquisitionsContainer::axpby
(complex_double_t a, const ISMRMRD::Acquisition& acq_x,
	complex_double_t b, ISMRMRD::Acquisition& acq_y)
{
	complex_float_t* px;
	complex_float_t* py;
	for (px = acq_x.data_begin(), py = acq_y.data_begin();
		px != acq_x.data_end() && py != acq_y.data_end(); px++, py++) {
		if (b == complex_double_t(0.0))
			*py = a*complex_double_t(*px);
		else
			*py = a*complex_double_t(*px) + b*complex_double_t(*py);
	}
}

complex_double_t 
AcquisitionsContainer::dot
(const ISMRMRD::Acquisition& acq_a, const ISMRMRD::Acquisition& acq_b)
{
	complex_float_t* pa;
	complex_float_t* pb;
	complex_double_t z = 0;
	for (pa = acq_a.data_begin(), pb = acq_b.data_begin();
		pa != acq_a.data_end() && pb != acq_b.data_end(); pa++, pb++) {
		z += std::conj(*pb) * (*pa);
	}
	return z;
}

double 
AcquisitionsContainer::norm(const ISMRMRD::Acquisition& acq_a)
{
	complex_float_t* pa;
	double r = 0;
	for (pa = acq_a.data_begin(); pa != acq_a.data_end(); pa++) {
		complex_float_t z = std::conj(*pa) * (*pa);
		r += z.real();
	}
	r = sqrt(r);
	return r;
}

float 
AcquisitionsContainer::diff
(const ISMRMRD::Acquisition& acq_a, const ISMRMRD::Acquisition& acq_b)
{
	complex_float_t* pa;
	complex_float_t* pb;
	complex_float_t z = 0;
	float s = 0.0;
	float sa = 0.0;
	float sb = 0.0;
	for (pa = acq_a.data_begin(), pb = acq_b.data_begin();
		pa != acq_a.data_end() && pb != acq_b.data_end(); pa++, pb++) {
		float ta = std::abs(*pa);
		float tb = std::abs(*pb);
		sa += pow(std::abs(*pa), 2);
		sb += pow(std::abs(*pb), 2);
		z += std::conj(*pb) * (*pa);
	}
	z /= sb;
	sa = std::sqrt(sa);
	sb = std::sqrt(sb);
	for (pa = acq_a.data_begin(), pb = acq_b.data_begin();
		pa != acq_a.data_end() && pb != acq_b.data_end(); pa++, pb++) {
		s += pow(std::abs(*pa - *pb * z), 2);
	}
	s = std::sqrt(s);
	s /= sa;
	return s;
}

void 
AcquisitionsContainer::axpby(
	complex_double_t a, const aDataContainer<complex_double_t>& a_x,
	complex_double_t b, const aDataContainer<complex_double_t>& a_y)
{
	AcquisitionsContainer& x = (AcquisitionsContainer&)a_x;
	AcquisitionsContainer& y = (AcquisitionsContainer&)a_y;
	int m = x.number();
	int n = y.number();
	ISMRMRD::Acquisition ax;
	ISMRMRD::Acquisition ay;
	for (int i = 0, j = 0; i < n && j < m;) {
		y.get_acquisition(i, ay);
		x.get_acquisition(j, ax);
		if (TO_BE_IGNORED(ay)) {
			std::cout << i << " ignored (ay)\n";
			i++;
			continue;
		}
		if (TO_BE_IGNORED(ax)) {
			std::cout << j << " ignored (ax)\n";
			j++;
			continue;
		}
		AcquisitionsContainer::axpby(a, ax, b, ay);
		append_acquisition(ay);
		i++;
		j++;
	}
}

complex_double_t 
AcquisitionsContainer::dot(aDataContainer<complex_double_t>& dc)
{
	AcquisitionsContainer& other = (AcquisitionsContainer&)dc;
	int n = number();
	int m = other.number();
	complex_double_t z = 0;
	ISMRMRD::Acquisition a;
	ISMRMRD::Acquisition b;
	for (int i = 0, j = 0; i < n && j < m;) {
		get_acquisition(i, a);
		if (TO_BE_IGNORED(a)) {
			i++;
			continue;
		}
		other.get_acquisition(j, b);
		if (TO_BE_IGNORED(b)) {
			j++;
			continue;
		}
		z += AcquisitionsContainer::dot(a, b);
		i++;
		j++;
	}
	return z;
}

double 
AcquisitionsContainer::norm()
{
	int n = number();
	double r = 0;
	ISMRMRD::Acquisition a;
	for (int i = 0; i < n; i++) {
		get_acquisition(i, a);
		if (TO_BE_IGNORED(a)) {
			continue;
		}
		double s = AcquisitionsContainer::norm(a);
		r += s*s;
	}
	return sqrt(r);
}

float 
AcquisitionsContainer::diff(AcquisitionsContainer& other)
{
	int n = number();
	int m = other.number();
	float smax = 0.0;
	float save = 0.0;
	ISMRMRD::Acquisition a;
	ISMRMRD::Acquisition b;
	for (int i = 0; i < n && i < m; i++) {
		get_acquisition(i, a);
		other.get_acquisition(i, b);
		float s = AcquisitionsContainer::diff(a, b);
		smax = std::max(smax, s);
		save += s*s;
	}
	save = sqrt(save / std::min(n, m));
	return save;
}

void 
AcquisitionsContainer::order()
{
	class triple {
	public:
		triple(int i, int f, int s, int t) :
			ind(i), first(f), second(s), third(t) {}
		triple(const triple& t) :
			ind(t.ind), first(t.first), second(t.second), third(t.third) {}
		int ind;
		int first;
		int second;
		int third;
	};

	int na = number();
	std::vector<triple> t;
	ISMRMRD::Acquisition acq;
	for (int i = 0; i < na; i++) {
		get_acquisition(i, acq);
		int rep = acq.idx().repetition;
		int slice = acq.idx().slice;
		int phase = acq.idx().kspace_encode_step_1;
		t.push_back(triple(i, rep, slice, phase));
	}
	std::stable_sort(t.begin(), t.end(),
		[](triple a, triple b) { return b.first > a.first; });

	int i = 0;
	int j = i;
	while (i < na) {
		std::vector<triple> ts;
		for (; j < na; j++) {
			if (t[j].first != t[i].first)
				break;
		}
		for (int k = i; k < j; k++)
			ts.push_back(triple(t[k]));
		std::stable_sort(ts.begin(), ts.end(),
			[](triple a, triple b) { return b.second > a.second; });
		for (int k = i; k < j; k++)
			t[k] = ts[k - i];
		i = j;
	}

	if (index_)
		delete[] index_;
	index_ = new int[na];

	i = 0;
	j = i;
	while (i < na) {
		std::vector<triple> ts;
		for (; j < na; j++) {
			if (t[j].first != t[i].first || t[j].second != t[i].second)
				break;
		}
		for (int k = i; k < j; k++)
			ts.push_back(triple(t[k]));
		std::sort(ts.begin(), ts.end(),
			[](triple a, triple b) { return b.third > a.third; });
		for (int k = i; k < j; k++)
			index_[k] = ts[k - i].ind;
		i = j;
	}
	ordered_ = true;
}

int
AcquisitionsFile::set_acquisition_data
(int na, int nc, int ns, const double* re, const double* im)
{
	boost::shared_ptr<AcquisitionsContainer> sptr_ac =
		this->new_acquisitions_container();
	sptr_ac->set_parameters(par_);
	sptr_ac->write_parameters();
	sptr_ac->set_ordered(true);
	ISMRMRD::Acquisition acq;
	int ma = number();
	for (int a = 0, i = 0; a < ma; a++) {
		get_acquisition(a, acq);
		if (TO_BE_IGNORED(acq) && ma > na) {
			std::cout << "ignoring acquisition " << a << '\n';
			continue;
		}
		unsigned int mc = acq.active_channels();
		unsigned int ms = acq.number_of_samples();
		if (mc != nc || ms != ns)
			return -1;
		for (int c = 0; c < nc; c++)
			for (int s = 0; s < ns; s++, i++)
				acq.data(s, c) = complex_float_t((float)re[i], (float)im[i]);
		sptr_ac->append_acquisition(acq);
	}
	take_over(*sptr_ac);
	return 0;
}

void
ImagesContainer::axpby(
	complex_double_t a, const aDataContainer<complex_double_t>& a_x,
	complex_double_t b, const aDataContainer<complex_double_t>& a_y)
{
	ImagesContainer& x = (ImagesContainer&)a_x;
	ImagesContainer& y = (ImagesContainer&)a_y;
	ImageWrap w(x.image_wrap(0));
	complex_double_t zero(0.0, 0.0);
	complex_double_t one(1.0, 0.0);
	for (unsigned int i = 0; i < x.number() && i < y.number(); i++) {
		const ImageWrap& u = x.image_wrap(i);
		const ImageWrap& v = y.image_wrap(i);
		w.axpby(a, u, zero);
		w.axpby(b, v, one);
		append(w);
	}
}

complex_double_t 
ImagesContainer::dot(aDataContainer<complex_double_t>& dc)
{
	ImagesContainer& ic = (ImagesContainer&)dc;
	complex_double_t z = 0;
	for (unsigned int i = 0; i < number() && i < ic.number(); i++) {
		const ImageWrap& u = image_wrap(i);
		const ImageWrap& v = ic.image_wrap(i);
		z += u.dot(v);
	}
	return z;
}

double 
ImagesContainer::norm()
{
	double r = 0;
	for (unsigned int i = 0; i < number(); i++) {
		const ImageWrap& u = image_wrap(i);
		double s = u.norm();
		r += s*s;
	}
	r = sqrt(r);
	return r;
}

ImagesList::ImagesList(const ImagesList& list, const char* attr, const char* target)
{
#ifdef _MSC_VER
	std::list<boost::shared_ptr<ImageWrap> >::const_iterator i;
#else
	typename std::list<boost::shared_ptr<ImageWrap> >::const_iterator i;
#endif
	for (i = list.images_.begin(); i != list.images_.end(); i++) {
		const boost::shared_ptr<ImageWrap>& sptr_iw = *i;
		std::string atts = sptr_iw->attributes();
		ISMRMRD::MetaContainer mc;
		ISMRMRD::deserialize(atts.c_str(), mc);
		std::string value = mc.as_str(attr);
		if (boost::iequals(value, target))
			append(*sptr_iw);
	}
}

ImagesList::ImagesList(const ImagesList& list, unsigned int inc, unsigned int off)
{
	int n = 0;
	unsigned int j = 0;
#ifdef _MSC_VER
	std::list<boost::shared_ptr<ImageWrap> >::const_iterator i;
#else
	typename std::list<boost::shared_ptr<ImageWrap> >::const_iterator i;
#endif
	for (i = list.images_.begin(); i != list.images_.end() && j < off; i++, j++);

	for (; i != list.images_.end(); i++, j++) {
		if ((j - off) % inc)
			continue;
		const boost::shared_ptr<ImageWrap>& sptr_iw = *i;
		append(*sptr_iw);
		n++;
	}
	nimages_ = n;
}

boost::shared_ptr<ImageWrap> 
ImagesList::sptr_image_wrap(unsigned int im_num)
{
#ifdef _MSC_VER
	std::list<boost::shared_ptr<ImageWrap> >::iterator i;
#else
	typename std::list<boost::shared_ptr<ImageWrap> >::iterator i;
#endif
	unsigned int count = 0;
	for (i = images_.begin();
		i != images_.end() && count < im_num && count < images_.size() - 1; i++)
		count++;
	return *i;
}

boost::shared_ptr<const ImageWrap> 
ImagesList::sptr_image_wrap(unsigned int im_num) const
{
#ifdef _MSC_VER
	std::list<boost::shared_ptr<ImageWrap> >::const_iterator i;
#else
	typename std::list<boost::shared_ptr<ImageWrap> >::const_iterator i;
#endif
	unsigned int count = 0;
	for (i = images_.begin(); i != images_.end() && count < im_num; i++)
		count++;
	return *i;
}

void 
ImagesList::write(std::string filename, std::string groupname)
{
	if (images_.size() < 1)
		return;
	Mutex mtx;
	mtx.lock();
	ISMRMRD::Dataset dataset(filename.c_str(), groupname.c_str());
	mtx.unlock();
#ifdef _MSC_VER
	std::list<boost::shared_ptr<ImageWrap> >::iterator i;
#else
	typename std::list<boost::shared_ptr<ImageWrap> >::iterator i;
#endif
	for (i = images_.begin(); i != images_.end(); i++) {
		boost::shared_ptr<ImageWrap>& sptr_iw = *i;
		ImageWrap& iw = *sptr_iw;
		iw.write(dataset);
	}
}

void 
ImagesList::get_images_data_as_double_array(double* data) const
{
#ifdef _MSC_VER
	std::list<boost::shared_ptr<ImageWrap> >::const_iterator i;
#else
	typename std::list<boost::shared_ptr<ImageWrap> >::const_iterator i;
#endif
	int dim[4];
	for (i = images_.begin(); i != images_.end(); i++) {
		const boost::shared_ptr<ImageWrap>& sptr_iw = *i;
		ImageWrap& iw = *sptr_iw;
		iw.get_data(data);
		iw.get_dim(dim);
		size_t size = dim[0];
		size *= dim[1];
		size *= dim[2];
		size *= dim[3];
		data += size;
	}
}

void 
ImagesList::get_images_data_as_complex_array(double* re, double* im) const
{
#ifdef _MSC_VER
	std::list<boost::shared_ptr<ImageWrap> >::const_iterator i;
#else
	typename std::list<boost::shared_ptr<ImageWrap> >::const_iterator i;
#endif
	int dim[4];
	for (i = images_.begin(); i != images_.end(); i++) {
		const boost::shared_ptr<ImageWrap>& sptr_iw = *i;
		ImageWrap& iw = *sptr_iw;
		iw.get_dim(dim);
		size_t size = dim[0];
		size *= dim[1];
		size *= dim[2];
		size *= dim[3];
		int type = iw.type();
		if (type == ISMRMRD::ISMRMRD_CXFLOAT || type == ISMRMRD::ISMRMRD_CXDOUBLE)
			iw.get_cmplx_data(re, im);
		else {
			iw.get_data(re);
			for (int i = 0; i < size; i++)
				im[i] = 0;
		}
		re += size;
		im += size;
	}
}

void 
ImagesList::set_complex_images_data(const double* re, const double* im)
{
#ifdef _MSC_VER
	std::list<boost::shared_ptr<ImageWrap> >::iterator i;
#else
	typename std::list<boost::shared_ptr<ImageWrap> >::iterator i;
#endif
	int dim[4];
	for (i = images_.begin(); i != images_.end(); i++) {
		boost::shared_ptr<ImageWrap>& sptr_iw = *i;
		ImageWrap& iw = *sptr_iw;
		size_t n = iw.get_dim(dim);
		iw.set_cmplx_data(re, im);
		re += n;
		im += n;
	}
}

void 
CoilDataAsCFImage::get_data(double* re, double* im) const
{
	size_t n = img_.getNumberOfDataElements();
	const complex_float_t* ptr = img_.getDataPtr();
	for (size_t i = 0; i < n; i++) {
		complex_float_t z = ptr[i];
		re[i] = std::real(z);
		im[i] = std::imag(z);
	}
}

void 
CoilDataAsCFImage::set_data(const double* re, const double* im)
{
	size_t n = img_.getNumberOfDataElements();
	complex_float_t* ptr = img_.getDataPtr();
	for (size_t i = 0; i < n; i++)
		ptr[i] = complex_float_t((float)re[i], (float)im[i]);
}

void 
CoilDataAsCFImage::get_data_abs(double* v) const
{
	size_t n = img_.getNumberOfDataElements();
	const complex_float_t* ptr = img_.getDataPtr();
	for (size_t i = 0; i < n; i++) {
		complex_float_t z = ptr[i];
		v[i] = std::abs(z);
	}
}

CoilData& 
CoilDataList::data(int slice)
{
#ifdef _MSC_VER
	std::list<boost::shared_ptr<CoilData> >::const_iterator i;
#else
	typename std::list<boost::shared_ptr<CoilData> >::const_iterator i;
#endif
	int count = 0;
	for (i = list_.begin();
		i != list_.end() && count < slice && count < (int)list_.size() - 1; i++)
		count++;
	return **i;
}

void 
CoilImagesContainer::compute(AcquisitionsContainer& ac)
{
	std::string par;
	ISMRMRD::IsmrmrdHeader header;
	ISMRMRD::Acquisition acq;
	par = ac.parameters();
	ISMRMRD::deserialize(par.c_str(), header);
	ac.get_acquisition(0, acq);
	encoding_ = header.encoding[0];

	ISMRMRD::Encoding e = header.encoding[0];
	bool parallel = e.parallelImaging.is_present() &&
		e.parallelImaging().accelerationFactor.kspace_encoding_step_1 > 1;
	unsigned int nx = e.reconSpace.matrixSize.x;
	unsigned int ny = e.reconSpace.matrixSize.y;
	unsigned int nc = acq.active_channels();
	unsigned int readout = acq.number_of_samples();

	//std::cout << nx << ' ' << ny << ' ' << nc << ' ' << readout << '\n';
	//if (e.parallelImaging.is_present()) {
	//	std::cout << "parallel imaging present\n";
	//	std::cout << "acceleration factors: " 
	//		<< e.parallelImaging().accelerationFactor.kspace_encoding_step_1 << ' '
	//		<< e.parallelImaging().accelerationFactor.kspace_encoding_step_2 << '\n';
	//}
	//else
	//	std::cout << "parallel imaging not present\n";

	int nmap = 0;
	std::cout << "map ";

	for (unsigned int na = 0; na < ac.number();) {

		std::cout << ++nmap << ' ' << std::flush;

		std::vector<size_t> ci_dims;
		ci_dims.push_back(readout);
		ci_dims.push_back(ny);
		ci_dims.push_back(nc);
		ISMRMRD::NDArray<complex_float_t> ci(ci_dims);
		memset(ci.getDataPtr(), 0, ci.getDataSize());

		int y = 0;
		for (;;) {
			ac.get_acquisition(na + y, acq);
			if (acq.isFlagSet(ISMRMRD::ISMRMRD_ACQ_FIRST_IN_SLICE))
				break;
			y++;
		}
		for (;;) {
			ac.get_acquisition(na + y, acq);
			int yy = acq.idx().kspace_encode_step_1;
			//if (!e.parallelImaging.is_present() ||
			if (!parallel ||
				acq.isFlagSet(ISMRMRD::ISMRMRD_ACQ_IS_PARALLEL_CALIBRATION) ||
				acq.isFlagSet(ISMRMRD::ISMRMRD_ACQ_IS_PARALLEL_CALIBRATION_AND_IMAGING)) {
				for (unsigned int c = 0; c < nc; c++) {
					for (unsigned int s = 0; s < readout; s++) {
						ci(s, yy, c) = acq.data(s, c);
					}
				}
			}
			y++;
			if (acq.isFlagSet(ISMRMRD::ISMRMRD_ACQ_LAST_IN_SLICE))
				break;
		}
		na += y;

		ifft2c(ci);

		boost::shared_ptr<CoilData>
			sptr_ci(new CoilDataAsCFImage(readout, ny, 1, nc));
		CFImage& coil_im = (*(CoilDataAsCFImage*)sptr_ci.get()).image();
		memcpy(coil_im.getDataPtr(), ci.getDataPtr(), ci.getDataSize());
		append(sptr_ci);
	}
	std::cout << '\n';
}

void 
CoilSensitivitiesContainer::compute(CoilImagesContainer& cis)
{

	ISMRMRD::Encoding e = cis.encoding();
	unsigned int nx = e.reconSpace.matrixSize.x;
	unsigned int ny = e.reconSpace.matrixSize.y;
	int dim[4];
	cis(0).get_dim(dim);
	unsigned int readout = dim[0];
	unsigned int nc = dim[3];

	std::vector<size_t> cm_dims;
	cm_dims.push_back(readout);
	cm_dims.push_back(ny);
	cm_dims.push_back(nc);
	ISMRMRD::NDArray<complex_float_t> cm(cm_dims);

	std::vector<size_t> csm_dims;
	csm_dims.push_back(nx);
	csm_dims.push_back(ny);
	csm_dims.push_back(1);
	csm_dims.push_back(nc);
	ISMRMRD::NDArray<complex_float_t> csm(csm_dims);

	std::vector<size_t> img_dims;
	img_dims.push_back(nx);
	img_dims.push_back(ny);
	ISMRMRD::NDArray<float> img(img_dims);

	unsigned int nmap = 0;

	std::cout << "map ";
	for (nmap = 1; nmap <= cis.items(); nmap++) {
		std::cout << nmap << ' ' << std::flush;
		cis(nmap - 1).get_data(cm.getDataPtr());
		//CoilData* ptr_img = new CoilDataType(nx, ny, 1, nc);
		CoilData* ptr_img = new CoilDataAsCFImage(nx, ny, 1, nc);
		boost::shared_ptr<CoilData> sptr_img(ptr_img);
		compute_csm_(cm, img, csm);
		ptr_img->set_data(csm.getDataPtr());
		append(sptr_img);
	}
	std::cout << '\n';
}

float 
CoilSensitivitiesContainer::max_(int nx, int ny, float* u)
{
	float r = 0.0;
	int i = 0;
	for (int iy = 0; iy < ny; iy++)
		for (int ix = 0; ix < nx; ix++, i++) {
			float t = fabs(u[i]);
			if (t > r)
				r = t;
		}
	return r;
}

void 
CoilSensitivitiesContainer::mask_noise_
(int nx, int ny, float* u, float noise, int* mask)
{
	int i = 0;
	for (int iy = 0; iy < ny; iy++)
		for (int ix = 0; ix < nx; ix++, i++) {
			float t = fabs(u[i]);
			mask[i] = (t > noise);
		}
}

void 
CoilSensitivitiesContainer::cleanup_mask_(int nx, int ny, int* mask, int bg, int minsz, int ex)
{
	int ll, il;
	int* listx = new int[nx*ny];
	int* listy = new int[nx*ny];
	int* inlist = new int[nx*ny];
	std::memset(inlist, 0, nx*ny * sizeof(int));
	for (int iy = 0, i = 0; iy < ny; iy++) {
		for (int ix = 0; ix < nx; ix++, i++) {
			if (mask[i] == bg)
				continue;
			bool skip = false;
			ll = 1;
			listx[0] = ix;
			listy[0] = iy;
			inlist[i] = 1;
			il = 0;
			while (il < ll && ll < minsz) {
				int lx = listx[il];
				int ly = listy[il];
				int l = ll + ex;
				for (int jy = -l; jy <= l; jy++) {
					for (int jx = -l; jx <= l; jx++) {
						int kx = lx + jx;
						int ky = ly + jy;
						if (kx < 0 || kx >= nx)
							continue;
						if (ky < 0 || ky >= ny)
							continue;
						int j = kx + ky*nx;
						if (inlist[j])
							continue;
						if (mask[j] != bg) {
							listx[ll] = kx;
							listy[ll] = ky;
							inlist[j] = 1;
							ll++;
						}
					}
				}
				il++;
			}
			if (il == ll)
				mask[i] = bg;
			for (il = 0; il < ll; il++) {
				int lx = listx[il];
				int ly = listy[il];
				int j = lx + ly*nx;
				inlist[j] = 0;
			}
		}
	}
	delete[] listx;
	delete[] listy;
	delete[] inlist;
}

void 
CoilSensitivitiesContainer::smoothen_
(int nx, int ny, int nz,
	complex_float_t* u, complex_float_t* v,
	int* obj_mask)
{
	const complex_float_t ONE(1.0, 0.0);
	const complex_float_t TWO(2.0, 0.0);
	for (int iz = 0, i = 0; iz < nz; iz++)
		for (int iy = 0, k = 0; iy < ny; iy++)
			for (int ix = 0; ix < nx; ix++, i++, k++) {
				//if (edge_mask[k]) {
				//	v[i] = u[i];
				//	continue;
				//}
				int n = 0;
				complex_float_t r(0.0, 0.0);
				complex_float_t s(0.0, 0.0);
				for (int jy = -1; jy <= 1; jy++)
					for (int jx = -1; jx <= 1; jx++) {
						if (ix + jx < 0 || ix + jx >= nx)
							continue;
						if (iy + jy < 0 || iy + jy >= ny)
							continue;
						int j = i + jx + jy*nx;
						int l = k + jx + jy*nx;
						if (i != j && obj_mask[l]) { // && !edge_mask[l]) {
							n++;
							r += ONE;
							s += u[j];
						}
					}
				if (n > 0)
					v[i] = (u[i] + s / r) / TWO;
				else
					v[i] = u[i];
			}
	memcpy(u, v, nx*ny*nz * sizeof(complex_float_t));
}

void 
CoilSensitivitiesContainer::compute_csm_(
	ISMRMRD::NDArray<complex_float_t>& cm,
	ISMRMRD::NDArray<float>& img,
	ISMRMRD::NDArray<complex_float_t>& csm
)
{
	int ndims = cm.getNDim();
	const size_t* dims = cm.getDims();
	unsigned int readout = (unsigned int)dims[0];
	unsigned int ny = (unsigned int)dims[1];
	unsigned int nc = (unsigned int)dims[2];
	unsigned int nx = (unsigned int)img.getDims()[0];

	std::vector<size_t> cm0_dims;
	cm0_dims.push_back(nx);
	cm0_dims.push_back(ny);
	cm0_dims.push_back(nc);

	ISMRMRD::NDArray<complex_float_t> cm0(cm0_dims);
	for (unsigned int c = 0; c < nc; c++) {
		for (unsigned int y = 0; y < ny; y++) {
			for (unsigned int x = 0; x < nx; x++) {
				uint16_t xout = x + (readout - nx) / 2;
				cm0(x, y, c) = cm(xout, y, c);
			}
		}
	}

	int* object_mask = new int[nx*ny*nc];
	memset(object_mask, 0, nx*ny*nc * sizeof(int));

	ISMRMRD::NDArray<complex_float_t> w(cm0);

	float* ptr_img = img.getDataPtr();
	for (unsigned int y = 0; y < ny; y++) {
		for (unsigned int x = 0; x < nx; x++) {
			double r = 0.0;
			for (unsigned int c = 0; c < nc; c++) {
				float s = std::abs(cm0(x, y, c));
				r += s*s;
			}
			img(x, y) = (float)std::sqrt(r);
		}
	}

	float noise = max_(5, 5, ptr_img) + (float)1e-6*max_(nx, ny, ptr_img);
	mask_noise_(nx, ny, ptr_img, noise, object_mask);
	cleanup_mask_(nx, ny, object_mask, 0, 2, 0);
	cleanup_mask_(nx, ny, object_mask, 0, 3, 0);
	cleanup_mask_(nx, ny, object_mask, 0, 4, 0);

	for (int i = 0; i < csm_smoothness_; i++)
		smoothen_(nx, ny, nc, cm0.getDataPtr(), w.getDataPtr(), object_mask);

	for (unsigned int y = 0; y < ny; y++) {
		for (unsigned int x = 0; x < nx; x++) {
			double r = 0.0;
			for (unsigned int c = 0; c < nc; c++) {
				float s = std::abs(cm0(x, y, c));
				r += s*s;
			}
			img(x, y) = (float)std::sqrt(r);
		}
	}

	for (unsigned int y = 0, i = 0; y < ny; y++) {
		for (unsigned int x = 0; x < nx; x++, i++) {
			double r = img(x, y);
			float s;
			if (r != 0.0)
				s = (float)(1.0 / r);
			else
				s = 0.0;
			complex_float_t z(s, 0.0);
			for (unsigned int c = 0; c < nc; c++) {
				csm(x, y, 0, c) = cm0(x, y, c) * z;
			}
		}
	}

	delete[] object_mask;

}

CoilSensitivitiesAsImages::CoilSensitivitiesAsImages(const char* file)
{
	Mutex mtx;
	mtx.lock();
	ISMRMRD::Dataset csm_file(file, "dataset");
	int nm = csm_file.getNumberOfImages("csm");
	mtx.unlock();
	for (int i = 0; i < nm; i++) {
		boost::shared_ptr<CoilData> sptr_img(new CoilDataAsCFImage);
		mtx.lock();
		CFImage& csm = (*(CoilDataAsCFImage*)sptr_img.get()).image();
		csm_file.readImage("csm", i, csm);
		mtx.unlock();
		append(sptr_img);
	}
	csm_smoothness_ = 0;
}