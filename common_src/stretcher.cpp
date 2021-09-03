// based on https://pixinsight.com/doc/docs/XISF-1.0-spec/XISF-1.0-spec.html

#include "stretcher.h"

#include <math.h>
#include <QCoreApplication>

// Returns the median value of the vector.
// The vector is modified in an undefined way.
template <typename T>
T median(std::vector<T> &values) {
	const int middle = values.size() / 2;
	std::nth_element(values.begin(), values.begin() + middle, values.end());
	return values[middle];
}

// Returns the median of the sample values.
// The values are not modified.
template <typename T>
T median(T const *values, int size, int sampleBy) {
	const int downsampled_size = size / sampleBy;
	std::vector<T> samples(downsampled_size);
	for (int index = 0, i = 0; i < downsampled_size; ++i, index += sampleBy) {
		samples[i] = values[index];
	}
	return median(samples);
}

template <typename T>
void stretchOneChannel(
	T *input_buffer,
	QImage *output_image,
	const StretchParams &stretch_params,
	int input_range,
	int image_height,
	int image_width,
	int sampling
) {
	constexpr int maxOutput = 255;

	// Maximum possible input value (e.g. 1024*64 - 1 for a 16 bit unsigned int).
	const float maxInput = input_range > 1 ? input_range - 1 : input_range;

	const float midtones   = stretch_params.grey_red.midtones;
	const float highlights = stretch_params.grey_red.highlights;
	const float shadows    = stretch_params.grey_red.shadows;

	// Precomputed expressions moved out of the loop.
	// highlights - shadows, protecting for divide-by-0, in a 0->1.0 scale.
	const float hsRangeFactor = highlights == shadows ? 1.0f : 1.0f / (highlights - shadows);
	// Shadow and highlight values translated to the ADU scale.
	const T nativeShadows = shadows * maxInput;
	const T nativeHighlights = highlights * maxInput;
	// Constants based on above needed for the stretch calculations.
	const float k1 = (midtones - 1) * hsRangeFactor * maxOutput / maxInput;
	const float k2 = ((2 * midtones) - 1) * hsRangeFactor / maxInput;

	for (int j = 0, jout = 0; j < image_height; j += sampling, jout++) {
		T * inputLine  = input_buffer + j * image_width;
		auto * scanLine = reinterpret_cast<QRgb*>(output_image->scanLine(jout));
		QCoreApplication::processEvents();
		for (int i = 0, iout = 0; i < image_width; i += sampling, iout++) {
			const T input = inputLine[i];
			if (input < nativeShadows) output_image->setPixel(iout, jout, qRgb(0, 0, 0));
			else if (input >= nativeHighlights) output_image->setPixel(iout, jout, qRgb(maxOutput,maxOutput,maxOutput));
			else {
				const T inputFloored = (input - nativeShadows);
				int val = (inputFloored * k1) / (inputFloored * k2 - midtones);
				scanLine[iout] = qRgb(val, val, val);
			}
		}
	}
}

template <typename T>
void stretchThreeChannels(
	T *inputBuffer, QImage *outputImage,
	const StretchParams &stretchParams,
	int inputRange,
	int imageHeight,
	int imageWidth,
	int sampling
) {
	constexpr int maxOutput = 255;

	// Maximum possible input value (e.g. 1024*64 - 1 for a 16 bit unsigned int).
	const float maxInput = inputRange > 1 ? inputRange - 1 : inputRange;

	float midtonesR   = stretchParams.grey_red.midtones;
	float highlightsR = stretchParams.grey_red.highlights;
	float shadowsR    = stretchParams.grey_red.shadows;
	float midtonesG   = stretchParams.green.midtones;
	float highlightsG = stretchParams.green.highlights;
	float shadowsG    = stretchParams.green.shadows;
	float midtonesB   = stretchParams.blue.midtones;
	float highlightsB = stretchParams.blue.highlights;
	float shadowsB    = stretchParams.blue.shadows;

	if (stretchParams.refChannel) {
		midtonesR   = stretchParams.refChannel->midtones;
		highlightsR = stretchParams.refChannel->highlights;
		shadowsR    = stretchParams.refChannel->shadows;
		midtonesG   = stretchParams.refChannel->midtones;
		highlightsG = stretchParams.refChannel->highlights;
		shadowsG    = stretchParams.refChannel->shadows;
		midtonesB   = stretchParams.refChannel->midtones;
		highlightsB = stretchParams.refChannel->highlights;
		shadowsB    = stretchParams.refChannel->shadows;
	}

	// Precomputed expressions moved out of the loop.
	// highlights - shadows, protecting for divide-by-0, in a 0->1.0 scale.
	const float hsRangeFactorR = highlightsR == shadowsR ? 1.0f : 1.0f / (highlightsR - shadowsR);
	const float hsRangeFactorG = highlightsG == shadowsG ? 1.0f : 1.0f / (highlightsG - shadowsG);
	const float hsRangeFactorB = highlightsB == shadowsB ? 1.0f : 1.0f / (highlightsB - shadowsB);
	// Shadow and highlight values translated to the ADU scale.
	const T nativeShadowsR = shadowsR * maxInput;
	const T nativeShadowsG = shadowsG * maxInput;
	const T nativeShadowsB = shadowsB * maxInput;
	const T nativeHighlightsR = highlightsR * maxInput;
	const T nativeHighlightsG = highlightsG * maxInput;
	const T nativeHighlightsB = highlightsB * maxInput;
	// Constants based on above needed for the stretch calculations.
	const float k1R = (midtonesR - 1) * hsRangeFactorR * maxOutput / maxInput;
	const float k1G = (midtonesG - 1) * hsRangeFactorG * maxOutput / maxInput;
	const float k1B = (midtonesB - 1) * hsRangeFactorB * maxOutput / maxInput;
	const float k2R = ((2 * midtonesR) - 1) * hsRangeFactorR / maxInput;
	const float k2G = ((2 * midtonesG) - 1) * hsRangeFactorG / maxInput;
	const float k2B = ((2 * midtonesB) - 1) * hsRangeFactorB / maxInput;

	const int size = imageWidth * imageHeight;

	const int skip = sampling * 3;
	const int imageWidth3 = imageWidth * 3;

	int index = 0;
	for (int j = 0, jout = 0; j < imageHeight; j += sampling, jout++) {
		QCoreApplication::processEvents();
		auto * scanLine = reinterpret_cast<QRgb*>(outputImage->scanLine(jout));
		for (int i = 0, iout = 0; i < imageWidth3; i += skip, iout++) {
			const T inputR = inputBuffer[index++];
			const T inputG = inputBuffer[index++];
			const T inputB = inputBuffer[index++];

			uint8_t red, green, blue;

			if (inputR < nativeShadowsR) red = 0;
			else if (inputR >= nativeHighlightsR) red = maxOutput;
			else {
				const T inputFloored = (inputR - nativeShadowsR);
				red = (inputFloored * k1R) / (inputFloored * k2R - midtonesR);
			}

			if (inputG < nativeShadowsG) green = 0;
			else if (inputG >= nativeHighlightsG) green = maxOutput;
			else {
				const T inputFloored = (inputG - nativeShadowsG);
				green = (inputFloored * k1G) / (inputFloored * k2G - midtonesG);
			}

			if (inputB < nativeShadowsB) blue = 0;
			else if (inputB >= nativeHighlightsB) blue = maxOutput;
			else {
				const T inputFloored = (inputB - nativeShadowsB);
				blue = (inputFloored * k1B) / (inputFloored * k2B - midtonesB);
			}
			scanLine[iout] = qRgb(red, green, blue);
		}
	}
}

template <typename T>
void stretchChannels(
	T *input_buffer,
	QImage *output_image,
	const StretchParams &stretch_params,
	int input_range,
	int image_height,
	int image_width,
	int num_channels,
	int sampling
) {
	if (num_channels == 1) {
		stretchOneChannel(input_buffer, output_image, stretch_params, input_range,
		                  image_height, image_width, sampling);
	} else if (num_channels == 3) {
		stretchThreeChannels(input_buffer, output_image, stretch_params, input_range,
		                  image_height, image_width, sampling);
	}
}


template <typename T>
void computeParamsOneChannel(
	T const *buffer,
	StretchParams1Channel *params,
	int inputRange,
	int height,
	int width
) {
	constexpr int maxSamples = 50000;
	const int sampleBy = width * height < maxSamples ? 1 : width * height / maxSamples;

	T medianSample = median(buffer, width * height, sampleBy);
	// Find the Median deviation: 1.4826 * median of abs(sample[i] - median).
	const int numSamples = width * height / sampleBy;
	std::vector<T> deviations(numSamples);
	for (int index = 0, i = 0; i < numSamples; ++i, index += sampleBy) {
		if (medianSample > buffer[index])
			deviations[i] = medianSample - buffer[index];
		else
			deviations[i] = buffer[index] - medianSample;
	}

	// Shift everything to 0 -> 1.0.
	const float medDev = median(deviations);
	const float normalizedMedian = medianSample / static_cast<float>(inputRange);
	const float MADN = 1.4826 * medDev / static_cast<float>(inputRange);

	const bool upperHalf = normalizedMedian > 0.5;

	const float shadows = (upperHalf || MADN == 0) ? 0.0 : fmin(1.0, fmax(0.0, (normalizedMedian + -2.8 * MADN)));

	const float highlights = (!upperHalf || MADN == 0) ? 1.0 : fmin(1.0, fmax(0.0, (normalizedMedian - -2.8 * MADN)));

	float X, M;
	constexpr float B = 0.25;
	if (!upperHalf) {
		X = normalizedMedian - shadows;
		M = B;
    } else {
		X = B;
		M = highlights - normalizedMedian;
	}
	float midtones;
	if (X == 0) midtones = 0.0f;
	else if (X == M) midtones = 0.5f;
	else if (X == 1) midtones = 1.0f;
	else midtones = ((M - 1) * X) / ((2 * M - 1) * X - M);

	// Store the params.
	params->shadows = shadows;
	params->highlights = highlights;
	params->midtones = midtones;
	params->shadows_expansion = 0.0;
	params->highlights_expansion = 1.0;
}

template <typename T>
void computeParamsThreeChannels(
	T const *buffer,
	StretchParams *params,
	int inputRange,
	int height,
	int width
) {
	constexpr int maxSamples = 50000;
	const int sampleBy = width * height < maxSamples ? 1 : width * height / maxSamples;
	const int sampleBy3 = sampleBy * 3;

	T medianSampleR = median(buffer, width * height * 3, sampleBy3);
	T medianSampleG = median(buffer+1, width * height * 3, sampleBy3);
	T medianSampleB = median(buffer+2, width * height * 3, sampleBy3);

	// Find the Median deviation: 1.4826 * median of abs(sample[i] - median).
	const int numSamples = width * height / sampleBy;
	std::vector<T> deviationsR(numSamples);
	std::vector<T> deviationsG(numSamples);
	std::vector<T> deviationsB(numSamples);

	for (int index = 0, i = 0; i < numSamples; ++i, index += sampleBy3) {
		T value = buffer[index];
		if (medianSampleR > value)
			deviationsR[i] = medianSampleR - value;
		else
			deviationsR[i] = value - medianSampleR;

		value = buffer[index+1];
		if (medianSampleG > value)
			deviationsG[i] = medianSampleG - value;
		else
			deviationsG[i] = value - medianSampleG;

		value = buffer[index+2];
		if (medianSampleB > value)
			deviationsB[i] = medianSampleB - value;
		else
			deviationsB[i] = value - medianSampleB;
	}

	// Shift everything to 0 -> 1.0.
	float medDev = median(deviationsR);
	float normalizedMedian = medianSampleR / static_cast<float>(inputRange);
	float MADN = 1.4826 * medDev / static_cast<float>(inputRange);

	bool upperHalf = normalizedMedian > 0.5;

	float shadowsR = (upperHalf || MADN == 0) ? 0.0 : fmin(1.0, fmax(0.0, (normalizedMedian + -2.8 * MADN)));

	float highlightsR = (!upperHalf || MADN == 0) ? 1.0 : fmin(1.0, fmax(0.0, (normalizedMedian - -2.8 * MADN)));

	float X, M;
	constexpr float B = 0.25;
	if (!upperHalf) {
		X = normalizedMedian - shadowsR;
		M = B;
	} else {
		X = B;
		M = highlightsR - normalizedMedian;
	}
	float midtonesR;
	if (X == 0) midtonesR = 0.0f;
	else if (X == M) midtonesR = 0.5f;
	else if (X == 1) midtonesR = 1.0f;
	else midtonesR = ((M - 1) * X) / ((2 * M - 1) * X - M);

	// Green
	medDev = median(deviationsG);
	normalizedMedian = medianSampleG / static_cast<float>(inputRange);
	MADN = 1.4826 * medDev / static_cast<float>(inputRange);

	upperHalf = normalizedMedian > 0.5;

	const float shadowsG = (upperHalf || MADN == 0) ? 0.0 : fmin(1.0, fmax(0.0, (normalizedMedian + -2.8 * MADN)));

	const float highlightsG = (!upperHalf || MADN == 0) ? 1.0 : fmin(1.0, fmax(0.0, (normalizedMedian - -2.8 * MADN)));

	if (!upperHalf) {
		X = normalizedMedian - shadowsG;
		M = B;
	} else {
		X = B;
		M = highlightsG - normalizedMedian;
	}
	float midtonesG;
	if (X == 0) midtonesG = 0.0f;
	else if (X == M) midtonesG = 0.5f;
	else if (X == 1) midtonesG = 1.0f;
	else midtonesG = ((M - 1) * X) / ((2 * M - 1) * X - M);

	// Blue
	medDev = median(deviationsB);
	normalizedMedian = medianSampleB / static_cast<float>(inputRange);
	MADN = 1.4826 * medDev / static_cast<float>(inputRange);

	upperHalf = normalizedMedian > 0.5;

	const float shadowsB = (upperHalf || MADN == 0) ? 0.0 : fmin(1.0, fmax(0.0, (normalizedMedian + -2.8 * MADN)));

	const float highlightsB = (!upperHalf || MADN == 0) ? 1.0 : fmin(1.0, fmax(0.0, (normalizedMedian - -2.8 * MADN)));

	if (!upperHalf) {
		X = normalizedMedian - shadowsB;
		M = B;
	} else {
		X = B;
		M = highlightsB - normalizedMedian;
	}
	float midtonesB;
	if (X == 0) midtonesB = 0.0f;
	else if (X == M) midtonesB = 0.5f;
	else if (X == 1) midtonesB = 1.0f;
	else midtonesB = ((M - 1) * X) / ((2 * M - 1) * X - M);

	// Store the params.
	params->grey_red.shadows = shadowsR;
	params->grey_red.highlights = highlightsR;
	params->grey_red.midtones = midtonesR;
	params->grey_red.shadows_expansion = 0.0;
	params->grey_red.highlights_expansion = 1.0;
	params->green.shadows = shadowsG;
	params->green.highlights = highlightsG;
	params->green.midtones = midtonesG;
	params->green.shadows_expansion = 0.0;
	params->green.highlights_expansion = 1.0;
	params->blue.shadows = shadowsB;
	params->blue.highlights = highlightsB;
	params->blue.midtones = midtonesB;
	params->blue.shadows_expansion = 0.0;
	params->blue.highlights_expansion = 1.0;
}

template <typename T>
void computeParamsThreeChannelsUnbalanced(
	T const *buffer,
	StretchParams *params,
	int inputRange,
	int height,
	int width,
	int refChannel = 1  // Use green as default reference
) {
	constexpr int maxSamples = 50000;
	const int sampleBy = width * height < maxSamples ? 1 : width * height / maxSamples;
	const int sampleBy3 = sampleBy * 3;

	T medianSample = median(buffer + refChannel, width * height * 3, sampleBy3);

	// Find the Median deviation: 1.4826 * median of abs(sample[i] - median).
	const int numSamples = width * height / sampleBy;
	std::vector<T> deviations(numSamples);

	for (int index = 0, i = 0; i < numSamples; ++i, index += sampleBy3) {
		T value = buffer[index + refChannel];
		if (medianSample > value)
			deviations[i] = medianSample - value;
		else
			deviations[i] = value - medianSample;
	}

	// Shift everything to 0 -> 1.0.
	float medDev = median(deviations);
	float normalizedMedian = medianSample / static_cast<float>(inputRange);
	float MADN = 1.4826 * medDev / static_cast<float>(inputRange);

	bool upperHalf = normalizedMedian > 0.5;

	float shadows = (upperHalf || MADN == 0) ? 0.0 : fmin(1.0, fmax(0.0, (normalizedMedian + -2.8 * MADN)));

	float highlights = (!upperHalf || MADN == 0) ? 1.0 : fmin(1.0, fmax(0.0, (normalizedMedian - -2.8 * MADN)));

	float X, M;
	constexpr float B = 0.25;
	if (!upperHalf) {
		X = normalizedMedian - shadows;
		M = B;
	} else {
		X = B;
		M = highlights - normalizedMedian;
	}
	float midtones;
	if (X == 0) midtones = 0.0f;
	else if (X == M) midtones = 0.5f;
	else if (X == 1) midtones = 1.0f;
	else midtones = ((M - 1) * X) / ((2 * M - 1) * X - M);

	// Store the params.
	params->grey_red.shadows = params->green.shadows = params->blue.shadows = shadows;
	params->grey_red.highlights = params->green.highlights = params->blue.highlights = highlights;
	params->grey_red.midtones = params->green.midtones = params->blue.midtones = midtones;
	params->grey_red.shadows_expansion = params->green.shadows_expansion = params->blue.shadows_expansion = 0.0;
	params->grey_red.highlights_expansion = params->green.highlights_expansion = params->blue.highlights_expansion = 1.0;
}

// Need to know the possible range of input values.
// Using the type of the sample and guessing.
// Perhaps we should examine the contents for the file
// (e.g. look at maximum value and extrapolate from that).
int getRange(int data_type) {
	switch (data_type) {
		case PIX_FMT_Y8:
		case PIX_FMT_RGB24:
			return 256;
		default:
			return 64 * 1024;
	}
}

Stretcher::Stretcher(int width, int height, int data_type) {
	image_width = width;
	image_height = height;
	dataType = data_type;
	input_range = getRange(dataType);
}

void Stretcher::stretch(uint8_t const *input, QImage *outputImage, int sampling) {
	Q_ASSERT(outputImage->width() == (image_width + sampling - 1) / sampling);
	Q_ASSERT(outputImage->height() == (image_height + sampling - 1) / sampling);

	switch (dataType) {
		case PIX_FMT_Y8:
			stretchChannels(reinterpret_cast<uint8_t const*>(input), outputImage, params,
			                input_range, image_height, image_width, 1, sampling);
		break;
		case PIX_FMT_Y16:
			stretchChannels(reinterpret_cast<uint16_t const*>(input), outputImage, params,
			                input_range, image_height, image_width, 1, sampling);
			break;
		case PIX_FMT_RGB24:
			stretchChannels(reinterpret_cast<uint8_t const*>(input), outputImage, params,
			                input_range, image_height, image_width, 3, sampling);
			break;
		case PIX_FMT_RGB48:
			stretchChannels(reinterpret_cast<uint16_t const*>(input), outputImage, params,
			                input_range, image_height, image_width, 3, sampling);
			break;
		default:
			break;
	}
}

StretchParams Stretcher::computeParams(uint8_t const *input) {
	StretchParams result;

	StretchParams1Channel *params = &result.grey_red;
	switch (dataType) {
		case PIX_FMT_Y8: {
			auto buffer = reinterpret_cast<uint8_t const*>(input);
			computeParamsOneChannel(buffer, params, input_range, image_height, image_width);
			break;
		}
		case PIX_FMT_Y16: {
			auto buffer = reinterpret_cast<uint16_t const*>(input);
			computeParamsOneChannel(buffer, params, input_range, image_height, image_width);
			break;
		}
		case PIX_FMT_RGB24:{
			auto buffer = reinterpret_cast<uint8_t const*>(input);
			computeParamsThreeChannels(buffer, &result, input_range, image_height, image_width);
			break;
		}
		case PIX_FMT_RGB48: {
			auto buffer = reinterpret_cast<uint16_t const*>(input);
			computeParamsThreeChannels(buffer, &result, input_range, image_height, image_width);
			break;
		}
		default:
			break;
	}
	return result;
}
