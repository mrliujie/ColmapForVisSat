// Copyright (c) 2018, ETH Zurich and UNC Chapel Hill.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//
//     * Neither the name of ETH Zurich and UNC Chapel Hill nor the names of
//       its contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Author: Johannes L. Schoenberger (jsch-at-demuc-dot-de)

#ifndef COLMAP_SRC_BASE_DATABASE_CACHE_H_
#define COLMAP_SRC_BASE_DATABASE_CACHE_H_

#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <Eigen/Core>

#include "base/camera.h"
#include "base/camera_models.h"
#include "base/correspondence_graph.h"
#include "base/database.h"
#include "base/image.h"
#include "util/alignment.h"
#include "util/types.h"

namespace colmap {

// A class that caches the contents of the database in memory, used to quickly
// create new reconstruction instances when multiple models are reconstructed.
class DatabaseCache {
 public:
  DatabaseCache();

  // Get number of objects.
  inline size_t NumCameras() const;
  inline size_t NumImages() const;

  // Get specific objects.
  inline class Camera& Camera(const camera_t camera_id);
  inline const class Camera& Camera(const camera_t camera_id) const;
  inline class Image& Image(const image_t image_id);
  inline const class Image& Image(const image_t image_id) const;

  // Get all objects.
  inline const EIGEN_STL_UMAP(camera_t, class Camera) & Cameras() const;
  inline const EIGEN_STL_UMAP(image_t, class Image) & Images() const;

  // Check whether specific object exists.
  inline bool ExistsCamera(const camera_t camera_id) const;
  inline bool ExistsImage(const image_t image_id) const;

  // Get reference to correspondence graph.
  inline const class CorrespondenceGraph& CorrespondenceGraph() const;

  // Manually add data to cache.
  void AddCamera(const class Camera& camera);
  void AddImage(const class Image& image);

  // Load cameras, images, features, and matches from database.
  //
  // @param database              Source database from which to load data.
  // @param min_num_matches       Only load image pairs with a minimum number
  //                              of matches.
  // @param ignore_watermarks     Whether to ignore watermark image pairs.
  // @param image_names           Whether to use only load the data for a subset
  //                              of the images. All images are used if empty.
  void Load(const Database& database, const size_t min_num_matches,
            const bool ignore_watermarks,
            const std::set<std::string>& image_names);

   // @kai print statistics
  std::string GetStatsString() const {
    std::stringstream buffer;

    std::vector<int> num_observations;
    std::vector<int> image_ids;
    // per-view observations
    for (const auto& image: images_) {
      image_ids.push_back(image.first);
      int cnt = image.second.NumObservations();
      std::cout << image.second.Name() << ", " << cnt << std::endl;
      num_observations.push_back(cnt);
    }
    std::sort(num_observations.begin(), num_observations.end()); 
    int min_val = num_observations[0];
    int max_val = num_observations.back();
    
    double avg_val = 0.0;
    for(size_t i=0; i<num_observations.size(); ++i) {
      avg_val += num_observations[i];
    }
    avg_val /= num_observations.size();
    buffer << "\nAvg. Per-view Observations: " << avg_val;
    buffer << "\nMin. Per-view Observations: " << min_val;
    buffer << "\nMax. Per-view Observations: " << max_val;

    // pair-wise matches
    std::vector<int> num_pairwise_matches;
    for (size_t i=0; i<image_ids.size(); ++i) {
      for (size_t j=i+1; j<image_ids.size(); ++j) {
        int cnt = correspondence_graph_.NumCorrespondencesBetweenImages(image_ids[i], image_ids[j]);
        num_pairwise_matches.push_back(cnt);
      }
    }

    std::sort(num_pairwise_matches.begin(), num_pairwise_matches.end()); 
    min_val = num_pairwise_matches[0];
    max_val = num_pairwise_matches.back();
    
    avg_val = 0.0;
    for(size_t i=0; i<num_pairwise_matches.size(); ++i) {
      avg_val += num_pairwise_matches[i];
    }
    avg_val /= num_pairwise_matches.size();
    buffer << "\nAvg. Pair-wise Matches: " << avg_val;
    buffer << "\nMin. Pair-wise Matches: " << min_val;
    buffer << "\nMax. Pair-wise Matches: " << max_val << std::endl;

    return buffer.str();
  }


 private:
  class CorrespondenceGraph correspondence_graph_;

  EIGEN_STL_UMAP(camera_t, class Camera) cameras_;
  EIGEN_STL_UMAP(image_t, class Image) images_;
};

////////////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////////////

size_t DatabaseCache::NumCameras() const { return cameras_.size(); }
size_t DatabaseCache::NumImages() const { return images_.size(); }

class Camera& DatabaseCache::Camera(const camera_t camera_id) {
  return cameras_.at(camera_id);
}

const class Camera& DatabaseCache::Camera(const camera_t camera_id) const {
  return cameras_.at(camera_id);
}

class Image& DatabaseCache::Image(const image_t image_id) {
  return images_.at(image_id);
}

const class Image& DatabaseCache::Image(const image_t image_id) const {
  return images_.at(image_id);
}

const EIGEN_STL_UMAP(camera_t, class Camera) & DatabaseCache::Cameras() const {
  return cameras_;
}

const EIGEN_STL_UMAP(image_t, class Image) & DatabaseCache::Images() const {
  return images_;
}

bool DatabaseCache::ExistsCamera(const camera_t camera_id) const {
  return cameras_.find(camera_id) != cameras_.end();
}

bool DatabaseCache::ExistsImage(const image_t image_id) const {
  return images_.find(image_id) != images_.end();
}

inline const class CorrespondenceGraph& DatabaseCache::CorrespondenceGraph()
    const {
  return correspondence_graph_;
}

}  // namespace colmap

#endif  // COLMAP_SRC_BASE_DATABASE_CACHE_H_
