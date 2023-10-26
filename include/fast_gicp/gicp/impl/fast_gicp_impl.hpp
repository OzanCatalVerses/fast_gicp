#ifndef FAST_GICP_FAST_GICP_IMPL_HPP
#define FAST_GICP_FAST_GICP_IMPL_HPP

#include <fast_gicp/so3/so3.hpp>

namespace fast_gicp {

template <typename PointSource, typename PointTarget, typename SearchMethodSource, typename SearchMethodTarget>
FastGICP<PointSource, PointTarget, SearchMethodSource, SearchMethodTarget>::FastGICP() {
#ifdef _OPENMP
  num_threads_ = omp_get_max_threads();
#else
  num_threads_ = 1;
#endif

  k_correspondences_ = 25;
  reg_name_ = "FastGICP";
  corr_dist_threshold_ = std::numeric_limits<float>::max();
  
  source_covs_.clear();  
  source_rotationsq_.clear();
  source_scales_.clear();
    
  target_covs_.clear();
  target_rotationsq_.clear();
  target_scales_.clear();

  regularization_method_ = RegularizationMethod::NORMALIZED_ELLIPSE;
  search_source_.reset(new SearchMethodSource);
  search_target_.reset(new SearchMethodTarget);
}

template <typename PointSource, typename PointTarget, typename SearchMethodSource, typename SearchMethodTarget>
FastGICP<PointSource, PointTarget, SearchMethodSource, SearchMethodTarget>::~FastGICP() {}

template <typename PointSource, typename PointTarget, typename SearchMethodSource, typename SearchMethodTarget>
void FastGICP<PointSource, PointTarget, SearchMethodSource, SearchMethodTarget>::setNumThreads(int n) {
  num_threads_ = n;

#ifdef _OPENMP
  if (n == 0) {
    num_threads_ = omp_get_max_threads();
  }
#endif
}

template <typename PointSource, typename PointTarget, typename SearchMethodSource, typename SearchMethodTarget>
void FastGICP<PointSource, PointTarget, SearchMethodSource, SearchMethodTarget>::setCorrespondenceRandomness(int k) {
  k_correspondences_ = k;
}

template <typename PointSource, typename PointTarget, typename SearchMethodSource, typename SearchMethodTarget>
void FastGICP<PointSource, PointTarget, SearchMethodSource, SearchMethodTarget>::setRegularizationMethod(RegularizationMethod method) {
  regularization_method_ = method;
}

template <typename PointSource, typename PointTarget, typename SearchMethodSource, typename SearchMethodTarget>
void FastGICP<PointSource, PointTarget, SearchMethodSource, SearchMethodTarget>::swapSourceAndTarget() {
  input_.swap(target_);
  search_source_.swap(search_target_);
  source_covs_.swap(target_covs_);
  source_rotationsq_.swap(target_rotationsq_);
  source_scales_.swap(target_scales_);

  correspondences_.clear();
  sq_distances_.clear();
}

template <typename PointSource, typename PointTarget, typename SearchMethodSource, typename SearchMethodTarget>
void FastGICP<PointSource, PointTarget, SearchMethodSource, SearchMethodTarget>::clearSource() {
  input_.reset();
  source_covs_.clear();
  source_rotationsq_.clear();
  source_scales_.clear();
}

template <typename PointSource, typename PointTarget, typename SearchMethodSource, typename SearchMethodTarget>
void FastGICP<PointSource, PointTarget, SearchMethodSource, SearchMethodTarget>::clearTarget() {
  target_.reset();
  target_covs_.clear();
  target_rotationsq_.clear();
  target_scales_.clear();
}

template <typename PointSource, typename PointTarget, typename SearchMethodSource, typename SearchMethodTarget>
void FastGICP<PointSource, PointTarget, SearchMethodSource, SearchMethodTarget>::setInputSource(const PointCloudSourceConstPtr& cloud) {
  if (input_ == cloud) {
    return;
  }

  pcl::Registration<PointSource, PointTarget, Scalar>::setInputSource(cloud);
  search_source_->setInputCloud(cloud);
  source_covs_.clear();
  source_rotationsq_.clear();
  source_scales_.clear();
  std::cout<<"set input source end"<<std::endl;
}

template <typename PointSource, typename PointTarget, typename SearchMethodSource, typename SearchMethodTarget>
void FastGICP<PointSource, PointTarget, SearchMethodSource, SearchMethodTarget>::calculateSourceCovariance() {
	if (input_->size() == 0){
		std::cerr<<"no point cloud"<<std::endl;
		return;
	}
	source_covs_.clear();
	source_rotationsq_.clear();
	source_scales_.clear();
	calculate_covariances(input_, *search_source_, source_covs_, source_rotationsq_, source_scales_);
}

template <typename PointSource, typename PointTarget, typename SearchMethodSource, typename SearchMethodTarget>
void FastGICP<PointSource, PointTarget, SearchMethodSource, SearchMethodTarget>::setInputTarget(const PointCloudTargetConstPtr& cloud) {
  if (target_ == cloud) {
    return;
  }
  pcl::Registration<PointSource, PointTarget, Scalar>::setInputTarget(cloud);
  search_target_->setInputCloud(cloud);
  target_covs_.clear();
  target_rotationsq_.clear();
  target_scales_.clear();
  std::cout<<"set input target end"<<std::endl;
}

template <typename PointSource, typename PointTarget, typename SearchMethodSource, typename SearchMethodTarget>
void FastGICP<PointSource, PointTarget, SearchMethodSource, SearchMethodTarget>::calculateTargetCovariance() {
	if (target_->size() == 0){
		std::cerr<<"no point cloud"<<std::endl;
		return;
	}
	target_covs_.clear();
	target_rotationsq_.clear();
	target_scales_.clear();
	calculate_covariances(target_, *search_target_, target_covs_, target_rotationsq_, target_scales_);
}

template <typename PointSource, typename PointTarget, typename SearchMethodSource, typename SearchMethodTarget>
void FastGICP<PointSource, PointTarget, SearchMethodSource, SearchMethodTarget>::setSourceCovariances(const std::vector<Eigen::Matrix4d, Eigen::aligned_allocator<Eigen::Matrix4d>>& covs) {
  source_covs_ = covs;
}

template <typename PointSource, typename PointTarget, typename SearchMethodSource, typename SearchMethodTarget>
void FastGICP<PointSource, PointTarget, SearchMethodSource, SearchMethodTarget>::setSourceCovariances(
	const std::vector<Eigen::Vector4d, Eigen::aligned_allocator<Eigen::Vector4d>>& input_rotationsq,
	const std::vector<Eigen::Vector3d, Eigen::aligned_allocator<Eigen::Vector3d>>& input_scales)
	{
		setCovariances(input_rotationsq, input_scales, source_covs_, source_rotationsq_, source_scales_);
}


template <typename PointSource, typename PointTarget, typename SearchMethodSource, typename SearchMethodTarget>
void FastGICP<PointSource, PointTarget, SearchMethodSource, SearchMethodTarget>::setTargetCovariances(const std::vector<Eigen::Matrix4d, Eigen::aligned_allocator<Eigen::Matrix4d>>& covs) {
  target_covs_ = covs;
}

template <typename PointSource, typename PointTarget, typename SearchMethodSource, typename SearchMethodTarget>
void FastGICP<PointSource, PointTarget, SearchMethodSource, SearchMethodTarget>::setTargetCovariances(
	const std::vector<Eigen::Vector4d, Eigen::aligned_allocator<Eigen::Vector4d>>& input_rotationsq,
	const std::vector<Eigen::Vector3d, Eigen::aligned_allocator<Eigen::Vector3d>>& input_scales)
	{
		setCovariances(input_rotationsq, input_scales, target_covs_, target_rotationsq_, target_scales_);
}

template <typename PointSource, typename PointTarget, typename SearchMethodSource, typename SearchMethodTarget>
void FastGICP<PointSource, PointTarget, SearchMethodSource, SearchMethodTarget>::computeTransformation(PointCloudSource& output, const Matrix4& guess) {
  if (output.points.data() == input_->points.data() || output.points.data() == target_->points.data()) {
    throw std::invalid_argument("FastGICP: destination cloud cannot be identical to source or target");
  }
  if (source_covs_.size() != input_->size()) {
//    std::cout<<"compute source cov"<<std::endl;
    calculate_covariances(input_, *search_source_, source_covs_, source_rotationsq_, source_scales_);
  }
  if (target_covs_.size() != target_->size()) {
//    std::cout<<"compute target cov"<<std::endl;
    calculate_covariances(target_, *search_target_, target_covs_, target_rotationsq_, target_scales_);
  }

  LsqRegistration<PointSource, PointTarget>::computeTransformation(output, guess);
}

template <typename PointSource, typename PointTarget, typename SearchMethodSource, typename SearchMethodTarget>
void FastGICP<PointSource, PointTarget, SearchMethodSource, SearchMethodTarget>::update_correspondences(const Eigen::Isometry3d& trans) {
  assert(source_covs_.size() == input_->size());
  assert(target_covs_.size() == target_->size());

  Eigen::Isometry3f trans_f = trans.cast<float>();

  correspondences_.resize(input_->size());
  sq_distances_.resize(input_->size());
  mahalanobis_.resize(input_->size());

  std::vector<int> k_indices(1);
  std::vector<float> k_sq_dists(1);

#pragma omp parallel for num_threads(num_threads_) firstprivate(k_indices, k_sq_dists) schedule(guided, 8)
  for (int i = 0; i < input_->size(); i++) {
    PointTarget pt;
    pt.getVector4fMap() = trans_f * input_->at(i).getVector4fMap();

    search_target_->nearestKSearch(pt, 1, k_indices, k_sq_dists);

    sq_distances_[i] = k_sq_dists[0];
    correspondences_[i] = k_sq_dists[0] < corr_dist_threshold_ * corr_dist_threshold_ ? k_indices[0] : -1;

    if (correspondences_[i] < 0) {
      continue;
    }

    const int target_index = correspondences_[i];
    const auto& cov_A = source_covs_[i];
    const auto& cov_B = target_covs_[target_index];

    Eigen::Matrix4d RCR = cov_B + trans.matrix() * cov_A * trans.matrix().transpose();
    RCR(3, 3) = 1.0;
    mahalanobis_[i] = RCR.inverse();
    mahalanobis_[i](3, 3) = 0.0f;
  }
}

template <typename PointSource, typename PointTarget, typename SearchMethodSource, typename SearchMethodTarget>
double FastGICP<PointSource, PointTarget, SearchMethodSource, SearchMethodTarget>::linearize(const Eigen::Isometry3d& trans, Eigen::Matrix<double, 6, 6>* H, Eigen::Matrix<double, 6, 1>* b) {
  update_correspondences(trans);

  double sum_errors = 0.0;
  std::vector<Eigen::Matrix<double, 6, 6>, Eigen::aligned_allocator<Eigen::Matrix<double, 6, 6>>> Hs(num_threads_);
  std::vector<Eigen::Matrix<double, 6, 1>, Eigen::aligned_allocator<Eigen::Matrix<double, 6, 1>>> bs(num_threads_);
  for (int i = 0; i < num_threads_; i++) {
    Hs[i].setZero();
    bs[i].setZero();
  }

#pragma omp parallel for num_threads(num_threads_) reduction(+ : sum_errors) schedule(guided, 8)
  for (int i = 0; i < input_->size(); i++) {
    int target_index = correspondences_[i];
    if (target_index < 0) {
      continue;
    }

    const Eigen::Vector4d mean_A = input_->at(i).getVector4fMap().template cast<double>();
    const auto& cov_A = source_covs_[i];

    const Eigen::Vector4d mean_B = target_->at(target_index).getVector4fMap().template cast<double>();
    const auto& cov_B = target_covs_[target_index];

    const Eigen::Vector4d transed_mean_A = trans * mean_A;
    const Eigen::Vector4d error = mean_B - transed_mean_A;

    sum_errors += error.transpose() * mahalanobis_[i] * error;

    if (H == nullptr || b == nullptr) {
      continue;
    }

    Eigen::Matrix<double, 4, 6> dtdx0 = Eigen::Matrix<double, 4, 6>::Zero();
    dtdx0.block<3, 3>(0, 0) = skewd(transed_mean_A.head<3>());
    dtdx0.block<3, 3>(0, 3) = -Eigen::Matrix3d::Identity();

    Eigen::Matrix<double, 4, 6> jlossexp = dtdx0;

    Eigen::Matrix<double, 6, 6> Hi = jlossexp.transpose() * mahalanobis_[i] * jlossexp;
    Eigen::Matrix<double, 6, 1> bi = jlossexp.transpose() * mahalanobis_[i] * error;

    Hs[omp_get_thread_num()] += Hi;
    bs[omp_get_thread_num()] += bi;
  }

  if (H && b) {
    H->setZero();
    b->setZero();
    for (int i = 0; i < num_threads_; i++) {
      (*H) += Hs[i];
      (*b) += bs[i];
    }
  }

  return sum_errors;
}

template <typename PointSource, typename PointTarget, typename SearchMethodSource, typename SearchMethodTarget>
double FastGICP<PointSource, PointTarget, SearchMethodSource, SearchMethodTarget>::compute_error(const Eigen::Isometry3d& trans) {
  double sum_errors = 0.0;

#pragma omp parallel for num_threads(num_threads_) reduction(+ : sum_errors) schedule(guided, 8)
  for (int i = 0; i < input_->size(); i++) {
    int target_index = correspondences_[i];
    if (target_index < 0) {
      continue;
    }

    const Eigen::Vector4d mean_A = input_->at(i).getVector4fMap().template cast<double>();
    const auto& cov_A = source_covs_[i];

    const Eigen::Vector4d mean_B = target_->at(target_index).getVector4fMap().template cast<double>();
    const auto& cov_B = target_covs_[target_index];

    const Eigen::Vector4d transed_mean_A = trans * mean_A;
    const Eigen::Vector4d error = mean_B - transed_mean_A;

    sum_errors += error.transpose() * mahalanobis_[i] * error;
  }

  return sum_errors;
}

template <typename PointSource, typename PointTarget, typename SearchMethodSource, typename SearchMethodTarget>
template <typename PointT>
bool FastGICP<PointSource, PointTarget, SearchMethodSource, SearchMethodTarget>::calculate_covariances(
  const typename pcl::PointCloud<PointT>::ConstPtr& cloud,
  pcl::search::Search<PointT>& kdtree,
  std::vector<Eigen::Matrix4d, Eigen::aligned_allocator<Eigen::Matrix4d>>& covariances,
  std::vector<Eigen::Vector4d, Eigen::aligned_allocator<Eigen::Vector4d>>& rotationsq,
  std::vector<Eigen::Vector3d, Eigen::aligned_allocator<Eigen::Vector3d>>& scales
  ) {
  if (kdtree.getInputCloud() != cloud) {
    kdtree.setInputCloud(cloud);
  }
  covariances.resize(cloud->size());
  rotationsq.resize(cloud->size());
  scales.resize(cloud->size());

#pragma omp parallel for num_threads(num_threads_) schedule(guided, 8)
  for (int i = 0; i < cloud->size(); i++) {
    std::vector<int> k_indices;
    std::vector<float> k_sq_distances;
    kdtree.nearestKSearch(cloud->at(i), k_correspondences_, k_indices, k_sq_distances);

    Eigen::Matrix<double, 4, -1> neighbors(4, k_correspondences_);
    for (int j = 0; j < k_indices.size(); j++) {
      neighbors.col(j) = cloud->at(k_indices[j]).getVector4fMap().template cast<double>();
    }

    neighbors.colwise() -= neighbors.rowwise().mean().eval();
    Eigen::Matrix4d cov = neighbors * neighbors.transpose() / k_correspondences_;
    
    //compute raw scale and quaternions using cov
    Eigen::JacobiSVD<Eigen::Matrix3d> svd(cov.block<3, 3>(0, 0), Eigen::ComputeFullU | Eigen::ComputeFullV);
    Eigen::Quaterniond qfrommat(svd.matrixU());
    qfrommat.normalize();
    Eigen::Vector4d q = {qfrommat.x(), qfrommat.y(), qfrommat.z(), qfrommat.w()};
    rotationsq[i] = q;
    scales[i] = svd.singularValues().cwiseSqrt();    

    // compute regularized covariance
    if (regularization_method_ == RegularizationMethod::NONE) {
      covariances[i] = cov;
    } else if (regularization_method_ == RegularizationMethod::FROBENIUS) {
      double lambda = 1e-3;
      Eigen::Matrix3d C = cov.block<3, 3>(0, 0).cast<double>() + lambda * Eigen::Matrix3d::Identity();
      Eigen::Matrix3d C_inv = C.inverse();
      covariances[i].setZero();
      covariances[i].template block<3, 3>(0, 0) = (C_inv / C_inv.norm()).inverse();
    } else {
      // Eigen::JacobiSVD<Eigen::Matrix3d> svd(cov.block<3, 3>(0, 0), Eigen::ComputeFullU | Eigen::ComputeFullV);
      Eigen::Vector3d values;
      switch (regularization_method_) {
        default:
          std::cerr << "here must not be reached" << std::endl;
          abort();
        case RegularizationMethod::PLANE:
          values = Eigen::Vector3d(1, 1, 1e-3);
          break;
        case RegularizationMethod::MIN_EIG:
          values = svd.singularValues().array().max(1e-3);
          break;
        case RegularizationMethod::NORMALIZED_MIN_EIG:
          values = svd.singularValues() / svd.singularValues().maxCoeff();
          values = values.array().max(1e-3);
          break;
        case RegularizationMethod::NORMALIZED_ELLIPSE:
          // std::cout<<svd.singularValues()(1)<<std::endl;
          if (svd.singularValues()(1) == 0){
          	values = Eigen::Vector3d(1e-9, 1e-9, 1e-9);
          }
          else{          
		  values = svd.singularValues() / svd.singularValues()(1);
		  values = values.array().max(1e-3);
	  }
      }
      // use regularized covariance
      covariances[i].setZero();
      covariances[i].template block<3, 3>(0, 0) = svd.matrixU() * values.asDiagonal() * svd.matrixV().transpose();
    }
  }

  return true;
}

template <typename PointSource, typename PointTarget, typename SearchMethodSource, typename SearchMethodTarget>
void FastGICP<PointSource, PointTarget, SearchMethodSource, SearchMethodTarget>::setCovariances(
	const std::vector<Eigen::Vector4d, Eigen::aligned_allocator<Eigen::Vector4d>>& input_rotationsq,
	const std::vector<Eigen::Vector3d, Eigen::aligned_allocator<Eigen::Vector3d>>& input_scales,
	std::vector<Eigen::Matrix4d, Eigen::aligned_allocator<Eigen::Matrix4d>>& covariances,
       std::vector<Eigen::Vector4d, Eigen::aligned_allocator<Eigen::Vector4d>>& rotationsq,
       std::vector<Eigen::Vector3d, Eigen::aligned_allocator<Eigen::Vector3d>>& scales) 
	{
	if(input_rotationsq.size() != input_scales.size()){
		std::cerr << "size not match" <<std::endl;
		abort();
	}
	rotationsq.clear();
	scales.clear();
	rotationsq = input_rotationsq;
	scales = input_scales;
	covariances.resize(input_scales.size());
	// rotationsq.resize(input_scales.size());
	// scales.resize(input_scales.size());
#pragma omp parallel for num_threads(num_threads_) schedule(guided, 8)
	for(int i=0; i<scales.size(); i++){
		Eigen::Vector3d singular_values = input_scales[i].cwiseProduct(input_scales[i]);
		switch (regularization_method_) {
		default:
		  std::cerr << "here must not be reached" << std::endl;
		  abort();
		case RegularizationMethod::PLANE:
		  singular_values = Eigen::Vector3d(1, 1, 1e-3);
		  break;
		case RegularizationMethod::MIN_EIG:
		  singular_values = singular_values.array().max(1e-3);
		  break;
		case RegularizationMethod::NORMALIZED_MIN_EIG:
		  singular_values = singular_values / singular_values.maxCoeff();
		  singular_values = singular_values.array().max(1e-3);
		  break;
		case RegularizationMethod::NORMALIZED_ELLIPSE:
		  // std::cout<<svd.singularValues()(1)<<std::endl;
		  if (singular_values(1) < 1e-3){
		  	singular_values = Eigen::Vector3d(1e-3, 1e-3, 1e-3);
		  }
		  else{          
			  singular_values = singular_values / singular_values(1);
			  singular_values = singular_values.array().max(1e-3);
		  }
		  break;
		case RegularizationMethod::NONE:
		  // do nothing
		  break;
		case RegularizationMethod::FROBENIUS:
		  std::cerr<< "should be implemented"<< std::endl;
		  abort();
	      }
	      // scales[i] = singular_values.cwiseSqrt();
	      // rotationsq[i] = input_rotationsq[i];
	      Eigen::Quaterniond q(input_rotationsq[i](0), input_rotationsq[i](1), input_rotationsq[i](2), input_rotationsq[i](3));
	      q = q.normalized();
	      covariances[i].setZero();
	      covariances[i].template block<3, 3>(0, 0) = q.toRotationMatrix() * singular_values.asDiagonal() * q.toRotationMatrix().transpose();
	}
}

}  // namespace fast_gicp

#endif
