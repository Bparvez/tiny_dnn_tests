/*
    Copyright (c) 2013, Taiga Nomi and the respective contributors
    All rights reserved.

    Use of this source code is governed by a BSD-style license that can be found
    in the LICENSE file.

    Additions made by:
    @Author Bilal Parvez <bilalp@kth.se>
*/
#include <iostream>

#include "tiny_dnn/tiny_dnn.h"

/*
 * @brief rescale output to 0-100
 *
 * @note TODO: Look into what this does, and why it does it
 */
template <typename Activation> double rescale(double x) {
  Activation a(1);
  return 100.0 * (x - a.scale().first) / (a.scale().second - a.scale().first);
}


/*
 * @brief Convert the input image resolution
 *
 * @note Convert the resolution to the resolution supported by the network.
 * @note TODO: See what resolution I can get away with, change it so that the
 * camera provides the most optimal resolution making this function obselete
 */
void convert_image(const std::string &imagefilename,
		double minv, double maxv, int w, int h, tiny_dnn::vec_t &data) {
	tiny_dnn::image<> img(imagefilename, tiny_dnn::image_type::rgb);
	tiny_dnn::image<> resized = resize_image(img, w, h);
	data.resize(resized.width() * resized.height() * resized.depth());
	for (size_t c = 0; c < resized.depth(); ++c) {
		for (size_t y = 0; y < resized.height(); ++y) {
			for (size_t x = 0; x < resized.width(); ++x) {
				data[c * resized.width() * resized.height() + y * resized.width() + x] =
					(maxv - minv) * (resized[y * resized.width() + x + c]) / 255.0 + minv;
			}
		}
	}
}


/*
 * @brief To Construct a Network.
 *
 * @note Function template is used to define the type.
 * @note TODO: Look into why the function template isnt used when calling the
 * function.
 */
template <typename N> void construct_net(N &nn) {
  using conv    = tiny_dnn::convolutional_layer;
  using pool    = tiny_dnn::max_pooling_layer;
  using fc      = tiny_dnn::fully_connected_layer;
  using relu    = tiny_dnn::relu_layer;
  using softmax = tiny_dnn::softmax_layer;

  const size_t n_fmaps  = 32;  ///< number of feature maps for upper layer
  const size_t n_fmaps2 = 64;  ///< number of feature maps for lower layer
  const size_t n_fc = 64;  ///< number of hidden units in fully-connected layer

  nn << conv(32, 32, 5, 3, n_fmaps, tiny_dnn::padding::same)  // C1
     << pool(32, 32, n_fmaps, 2)                              // P2
     << relu(16, 16, n_fmaps)                                 // activation
     << conv(16, 16, 5, n_fmaps, n_fmaps, tiny_dnn::padding::same)  // C3
     << pool(16, 16, n_fmaps, 2)                                    // P4
     << relu(8, 8, n_fmaps)                                        // activation
     << conv(8, 8, 5, n_fmaps, n_fmaps2, tiny_dnn::padding::same)  // C5
     << pool(8, 8, n_fmaps2, 2)                                    // P6
     << relu(4, 4, n_fmaps2)                                       // activation
     << fc(4 * 4 * n_fmaps2, n_fc)                                 // FC7
     << fc(n_fc, 10) << softmax(10);                               // FC10
}


/*
 * @brief Do the object recognition.
 *
 * @note: TODO: Would this need to be changed if we change the network
 */
void recognize(const std::string &dictionary, const std::string &src_filename) {
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	//TODO: Hardcoded should read the batches.meta.txt file.
	const std::array<const std::string, 10> names = {
		"airplane", "automobile", "bird", "cat", "deer", "dog", "frog", "horse",
		"ship", "truck",
	};

	tiny_dnn::network<tiny_dnn::sequential> nn;

	construct_net(nn);

	// load nets
	std::ifstream ifs(dictionary.c_str());
	ifs >> nn;

	// convert imagefile to vec_t
	tiny_dnn::vec_t data;
	convert_image(src_filename, -1.0, 1.0, 32, 32, data);

	// recognize
	auto res = nn.predict(data);
	std::vector<std::pair<double, int>> scores;

	std::chrono::steady_clock::time_point end= std::chrono::steady_clock::now();

	// sort & print top-3
	for (int i = 0; i < 10; i++)
		scores.emplace_back(rescale<tiny_dnn::tanh_layer>(res[i]), i);

	sort(scores.begin(), scores.end(), std::greater<std::pair<double, int>>());

	for (int i = 0; i < 3; i++)
		std::cout << names[scores[i].second] << "," << scores[i].first << std::endl;

	std::cout << "Time elapsed = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() <<std::endl;

}


int main(int argc, char **argv) {
	/* Command line error handling */
	if (argc != 2) {
		std::cout << "please specify image file";
		return 0;
	}

	recognize("cifar-weights", argv[1]);
}
