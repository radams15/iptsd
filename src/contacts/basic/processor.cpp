// SPDX-License-Identifier: GPL-2.0-or-later

#include "processor.hpp"

#include "cluster.hpp"

#include <common/types.hpp>
#include <contacts/processor.hpp>
#include <container/image.hpp>
#include <math/mat2.hpp>
#include <math/vec2.hpp>

#include <gsl/gsl_util>
#include <vector>

namespace iptsd::contacts::basic {

TouchProcessor::TouchProcessor(Config cfg)
	: heatmap {cfg.size, cfg.touch_thresh}, touchpoints {}, perfreg {}
{
	this->touchpoints.reserve(32);
}

const std::vector<TouchPoint> &TouchProcessor::process()
{
	this->heatmap.reset();
	this->touchpoints.clear();

	for (index_t x = 0; x < this->heatmap.size.x; x++) {
		for (index_t y = 0; y < this->heatmap.size.y; y++) {
			index2_t pos {x, y};

			if (!this->heatmap.is_touch(pos))
				continue;

			if (this->heatmap.get_visited(pos))
				continue;

			Cluster cluster(this->heatmap, pos);

			math::Mat2s<f32> cov = cluster.cov();
			math::Vec2<f32> mean = cluster.mean();

			mean.x /= gsl::narrow_cast<f32>(this->heatmap.size.x) - 1.0f;
			mean.y /= gsl::narrow_cast<f32>(this->heatmap.size.y) - 1.0f;

			math::Eigen2<f32> eigen = cov.eigen();
			if (std::min(eigen.w[0], eigen.w[1]) <= 0)
				continue;

			TouchPoint point {};
			point.cov = cov;
			point.mean = mean;
			point.confidence = 0; // TODO: Whats this?
			point.scale = 0;      // see above

			this->touchpoints.push_back(point);
		}
	}

	return this->touchpoints;
}

} // namespace iptsd::contacts::basic