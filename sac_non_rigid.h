#ifndef sac_non_rigid_h_
#define sac_non_rigid_h_

#include <boost/unordered_map.hpp>
#include <Eigen/Core>
#include "pcl/sample_consensus/sac_model.h"
#include "pcl/sample_consensus/model_types.h"
#include <pcl/common/eigen.h>
#include <pcl/common/centroid.h>

namespace pcl
{
  /** \brief SampleConsensusModelNonRigid defines a model for Point-To-Point registration outlier rejection.
    * \author Radu Bogdan Rusu
    * \ingroup sample_consensus
    */
  template <typename PointT>
  class SampleConsensusModelNonRigid : public SampleConsensusModel<PointT>
  {
    using SampleConsensusModel<PointT>::input_;
    using SampleConsensusModel<PointT>::indices_;

    public:
      typedef typename SampleConsensusModel<PointT>::PointCloud PointCloud;
      typedef typename SampleConsensusModel<PointT>::PointCloudPtr PointCloudPtr;
      typedef typename SampleConsensusModel<PointT>::PointCloudConstPtr PointCloudConstPtr;

      typedef boost::shared_ptr<SampleConsensusModelNonRigid> Ptr;

      /** \brief Constructor for base SampleConsensusModelNonRigid.
        * \param[in] cloud the input point cloud dataset
        */
      SampleConsensusModelNonRigid (const PointCloudConstPtr &cloud) : SampleConsensusModel<PointT> (cloud)
      {
        // Call our own setInputCloud
        setInputCloud (cloud);
      }

      /** \brief Constructor for base SampleConsensusModelNonRigid.
        * \param[in] cloud the input point cloud dataset
        * \param[in] indices a vector of point indices to be used from \a cloud
        */
      SampleConsensusModelNonRigid (const PointCloudConstPtr &cloud, 
                                        const std::vector<int> &indices) : 
        SampleConsensusModel<PointT> (cloud, indices)
      {
        computeOriginalIndexMapping ();
        computeSampleDistanceThreshold (cloud, indices);
      }

      /** \brief Provide a pointer to the input dataset
        * \param[in] cloud the const boost shared pointer to a PointCloud message
        */
      inline virtual void
      setInputCloud (const PointCloudConstPtr &cloud)
      {
        SampleConsensusModel<PointT>::setInputCloud (cloud);
        computeOriginalIndexMapping ();
        computeSampleDistanceThreshold (cloud);
      }

      /** \brief Set the input point cloud target.
        * \param target the input point cloud target
        */
      inline void
      setInputTarget (const PointCloudConstPtr &target)
      {
        target_ = target;
        indices_tgt_.reset (new std::vector<int>);
        // Cache the size and fill the target indices
        unsigned int target_size = target->size ();
        indices_tgt_->resize (target_size);

        for (unsigned int i = 0; i < target_size; ++i)
          (*indices_tgt_)[i] = i;
        computeOriginalIndexMapping ();
      }

      /** \brief Set the input point cloud target.
        * \param[in] target the input point cloud target
        * \param[in] indices_tgt a vector of point indices to be used from \a target
        */
      inline void 
      setInputTarget (const PointCloudConstPtr &target, const std::vector<int> &indices_tgt)
      {
        target_ = target;
        indices_tgt_.reset (new std::vector<int> (indices_tgt));
        computeOriginalIndexMapping ();
      }

      /** \brief Compute a 4x4 rigid transformation matrix from the samples given
        * \param[in] samples the indices found as good candidates for creating a valid model
        * \param[out] model_coefficients the resultant model coefficients
        */
      bool 
      computeModelCoefficients (const std::vector<int> &samples, 
                                Eigen::VectorXf &model_coefficients);

      /** \brief Compute all distances from the transformed points to their correspondences
        * \param[in] model_coefficients the 4x4 transformation matrix
        * \param[out] distances the resultant estimated distances
        */
      void 
      getDistancesToModel (const Eigen::VectorXf &model_coefficients, 
                           std::vector<double> &distances);

      /** \brief Select all the points which respect the given model coefficients as inliers.
        * \param[in] model_coefficients the 4x4 transformation matrix
        * \param[in] threshold a maximum admissible distance threshold for determining the inliers from the outliers
        * \param[out] inliers the resultant model inliers
        */
      void 
      selectWithinDistance (const Eigen::VectorXf &model_coefficients, 
                            const double threshold, 
                            std::vector<int> &inliers);

      /** \brief Count all the points which respect the given model coefficients as inliers. 
        * 
        * \param[in] model_coefficients the coefficients of a model that we need to compute distances to
        * \param[in] threshold maximum admissible distance threshold for determining the inliers from the outliers
        * \return the resultant number of inliers
        */
      virtual int
      countWithinDistance (const Eigen::VectorXf &model_coefficients, 
                           const double threshold);

      /** \brief Recompute the 4x4 transformation using the given inlier set
        * \param[in] inliers the data inliers found as supporting the model
        * \param[in] model_coefficients the initial guess for the optimization
        * \param[out] optimized_coefficients the resultant recomputed transformation
        */
      void 
      optimizeModelCoefficients (const std::vector<int> &inliers, 
                                 const Eigen::VectorXf &model_coefficients, 
                                 Eigen::VectorXf &optimized_coefficients);

      void 
      projectPoints (const std::vector<int> &inliers, 
                     const Eigen::VectorXf &model_coefficients, 
                     PointCloud &projected_points, bool copy_data_fields = true) 
      {};

      bool 
      doSamplesVerifyModel (const std::set<int> &indices, 
                            const Eigen::VectorXf &model_coefficients, 
                            const double threshold)
      {
        //PCL_ERROR ("[pcl::SampleConsensusModelNonRigid::doSamplesVerifyModel] called!\n");
        return (false);
      }

      /** \brief Return an unique id for this model (SACMODEL_REGISTRATION). */
      inline pcl::SacModel 
      getModelType () const { return (SACMODEL_REGISTRATION); }

    protected:
      /** \brief Check whether a model is valid given the user constraints.
        * \param[in] model_coefficients the set of model coefficients
        */
      inline bool 
      isModelValid (const Eigen::VectorXf &model_coefficients)
      {
        // Needs a valid model coefficients
        if (model_coefficients.size () != 16)
          return (false);

        return (true);
      }

      /** \brief Check if a sample of indices results in a good sample of points
        * indices.
        * \param[in] samples the resultant index samples
        */
      bool
      isSampleGood (const std::vector<int> &samples) const;

      /** \brief Computes an "optimal" sample distance threshold based on the
        * principal directions of the input cloud.
        * \param[in] cloud the const boost shared pointer to a PointCloud message
        */
      inline void 
      computeSampleDistanceThreshold (const PointCloudConstPtr &cloud)
      {
        // Compute the principal directions via PCA
        Eigen::Vector4f xyz_centroid;
        compute3DCentroid (*cloud, xyz_centroid);
        EIGEN_ALIGN16 Eigen::Matrix3f covariance_matrix;
        computeCovarianceMatrixNormalized (*cloud, xyz_centroid, covariance_matrix);
        EIGEN_ALIGN16 Eigen::Vector3f eigen_values;
        EIGEN_ALIGN16 Eigen::Matrix3f eigen_vectors;
        pcl::eigen33 (covariance_matrix, eigen_vectors, eigen_values);

        // Compute the distance threshold for sample selection
        sample_dist_thresh_ = eigen_values.array ().sqrt ().sum () / 3.0;
        sample_dist_thresh_ *= sample_dist_thresh_;
        cout<<"SAC Sample distance threshold="<<sample_dist_thresh_<<endl;
        PCL_DEBUG ("[pcl::SampleConsensusModelNonRigid::setInputCloud] Estimated a sample selection distance threshold of: %f\n", sample_dist_thresh_);
      }

      /** \brief Computes an "optimal" sample distance threshold based on the
        * principal directions of the input cloud.
        * \param[in] cloud the const boost shared pointer to a PointCloud message
        */
      inline void 
      computeSampleDistanceThreshold (const PointCloudConstPtr &cloud,
                                      const std::vector<int> &indices) 
      {
        // Compute the principal directions via PCA
        Eigen::Vector4f xyz_centroid;
        compute3DCentroid (*cloud, indices, xyz_centroid);
        EIGEN_ALIGN16 Eigen::Matrix3f covariance_matrix;
        computeCovarianceMatrixNormalized (*cloud, indices, xyz_centroid, covariance_matrix);
        EIGEN_ALIGN16 Eigen::Vector3f eigen_values;
        EIGEN_ALIGN16 Eigen::Matrix3f eigen_vectors;
        pcl::eigen33 (covariance_matrix, eigen_vectors, eigen_values);

        // Compute the distance threshold for sample selection
        sample_dist_thresh_ = eigen_values.array ().sqrt ().sum () / 3.0;
        sample_dist_thresh_ *= sample_dist_thresh_;
        cout<<"SAC Sample distance threshold="<<sample_dist_thresh_<<endl;
        PCL_DEBUG ("[pcl::SampleConsensusModelNonRigid::setInputCloud] Estimated a sample selection distance threshold of: %f\n", sample_dist_thresh_);
      }

    private:

    /** \brief Estimate a nonrigid transformation between a source and a target point cloud using an SVD closed-form 
      * solution of absolute orientation using unit quaternions
      * \param[in] cloud_src the source point cloud dataset
      * \param[in] indices_src the vector of indices describing the points of interest in cloud_src
      * \param[in] cloud_tgt the target point cloud dataset
      * \param[in] indices_tgt the vector of indices describing the correspondences of the interest points from
      * indices_src
      * \param[out] transform the resultant transformation matrix (as model coefficients)
      *
      * Uses procrustes to estimate scale, rotation, and translation.  
      */
      void 
      estimateNonRigidTransformationSVD ( const pcl::PointCloud<PointT> &cloud_src, 
                                          const std::vector<int> &indices_src, 
                                          const pcl::PointCloud<PointT> &cloud_tgt, 
                                          const std::vector<int> &indices_tgt, 
                                          Eigen::VectorXf &transform);                                      

      /** \brief Compute mappings between original indices of the input_/target_ clouds. */
      void
      computeOriginalIndexMapping () 
      {
        if (!indices_tgt_ || !indices_ || indices_->empty () || indices_->size () != indices_tgt_->size ())
          return;
        for (size_t i = 0; i < indices_->size (); ++i)
          correspondences_[(*indices_)[i]] = (*indices_tgt_)[i];
      }

      /** \brief A boost shared pointer to the target point cloud data array. */
      PointCloudConstPtr target_;

      /** \brief A pointer to the vector of target point indices to use. */
      boost::shared_ptr <std::vector<int> > indices_tgt_;

      /** \brief Given the index in the original point cloud, give the matching original index in the target cloud */
      boost::unordered_map<int, int> correspondences_;

      /** \brief Internal distance threshold used for the sample selection step. */
      double sample_dist_thresh_;
    public:
      EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  };
}

#include "sac_non_rigid.hpp"

#endif  //#ifndef sac_non_rigid_h_

